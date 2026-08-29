/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <malloc.h>

#include <driver.h>

#include <drivers/mtd/spif-nor.h>
#include <drivers/spif/spif.h>
#include <dt2c/driver.h>
#include <string.h>

#define SPIF_NOR_MAX_TRANSFER		     65536U
#define SPIF_NOR_TRAINING_PAGE_SIZE	     256U
#define SPIF_NOR_TRAINING_MAX_ERASE_SIZE     (64U * 1024U)
#define SPIF_NOR_TRAINING_MAX_PATTERN_SIZE   (16U * 1024U)
#define SPIF_NOR_TRAINING_MODES		     3U
#define SPIF_NOR_TRAINING_DELAYS	     64U
#define SPIF_NOR_TRAINING_FALLBACK_FREQUENCY 25000000U
#define SPIF_NOR_SFDP_DUMMY_CYCLES	     8U
#define SPIF_NOR_DTR_DUMMY_CYCLES	     7U

static const spi_nor_info_t spif_nor_info_table[] = {
	{ "W25X40", 0xef3013, 512 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG, NOR_OPCODE_WREN,
		NOR_OPCODE_E4K, 0, NOR_OPCODE_E64K, 0, SNOR_PROTO_1_1_1, 0, 0 },
	{ "W25Q128JVEIQ", 0xefc018, 16 * 1024 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG,
		NOR_OPCODE_WREN, NOR_OPCODE_E4K, NOR_OPCODE_E32K, NOR_OPCODE_E64K, 0, SNOR_PROTO_1_1_1, 0, 0 },
	{ "GD25D10B", 0xc84011, 128 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG, NOR_OPCODE_WREN,
		NOR_OPCODE_E4K, NOR_OPCODE_E32K, NOR_OPCODE_E64K, 0, SNOR_PROTO_1_1_1, 0, 0 },
};

static int spif_nor_exec_op(spif_nor_t *nor, const struct spi_mem_op *op)
{
	if (nor == NULL || nor->spif == NULL || op == NULL || !nor->spif->initialized)
		return DRIVER_ERROR_INVALID;
	return sunxi_spif_exec_op(nor->spif, op);
}

/* Encode the legacy command-plus-buffer calls as one memory operation. */
static int spif_nor_transfer(spif_nor_t *nor, void *txbuf, uint32_t txlen, void *rxbuf, uint32_t rxlen)
{
	const uint8_t *tx = (const uint8_t *)txbuf;
	struct spi_mem_op op = { 0 };
	uint32_t address;
	uint32_t i;
	uint8_t address_length;

	if (nor == NULL || tx == NULL || txlen == 0U)
		return DRIVER_ERROR_INVALID;
	if (rxlen != 0U && rxbuf == NULL)
		return DRIVER_ERROR_INVALID;

	op.cmd.nbytes = 1U;
	op.cmd.buswidth = SPI_MEM_BUSWIDTH_1;
	op.cmd.opcode = tx[0];
	if (txlen == 1U) {
		if (rxlen != 0U) {
			op.data.dir = SPI_MEM_DATA_IN;
			op.data.nbytes = rxlen;
			op.data.buswidth = SPI_MEM_BUSWIDTH_1;
			op.data.buf.in = rxbuf;
		}
	} else if (txlen >= 2U && rxlen == 0U) {
		op.data.dir = SPI_MEM_DATA_OUT;
		op.data.nbytes = txlen - 1U;
		op.data.buswidth = SPI_MEM_BUSWIDTH_1;
		op.data.buf.out = tx + 1;
	} else if (rxlen != 0U && (txlen == 4U || txlen == 5U)) {
		address_length = (uint8_t)(txlen - 1U);
		if (tx[0] == NOR_OPCODE_SFDP && txlen == 5U) {
			address_length = 3U;
			/* SPIF's dummy field is counted in clock cycles. */
			op.dummy.nbytes = SPIF_NOR_SFDP_DUMMY_CYCLES;
			op.dummy.buswidth = SPI_MEM_BUSWIDTH_1;
		}
		address = 0U;
		for (i = 0U; i < address_length; ++i)
			address = (address << 8) | tx[1U + i];
		op.addr.nbytes = address_length;
		op.addr.buswidth = SPI_MEM_BUSWIDTH_1;
		op.addr.val = address;
		op.data.dir = SPI_MEM_DATA_IN;
		op.data.nbytes = rxlen;
		op.data.buswidth = SPI_MEM_BUSWIDTH_1;
		op.data.buf.in = rxbuf;
	} else {
		return DRIVER_ERROR_INVALID;
	}
	return spif_nor_exec_op(nor, &op);
}

static int spif_nor_read_reg(spif_nor_t *nor, uint8_t opcode, uint8_t *buf, uint32_t len)
{
	return spif_nor_transfer(nor, &opcode, 1U, buf, len);
}

static int spif_nor_write_reg(spif_nor_t *nor, uint8_t opcode, const uint8_t *buf, uint32_t len)
{
	uint8_t tx[1U + 2U];

	if (len > 2U || (len != 0U && buf == NULL))
		return DRIVER_ERROR_INVALID;
	tx[0] = opcode;
	if (len != 0U)
		memcpy(&tx[1], buf, len);
	return spif_nor_transfer(nor, tx, 1U + len, NULL, 0U);
}

/**
 * @brief Dump the contents of the SFDP (Serial Flash Discoverable Parameters) data structure.
 * 
 * This function prints the SFDP header, parameter headers, and the basic table information to the log.
 * It will provide detailed output about the SFDP format, including version information, the number of parameter headers,
 * and the contents of the basic table in the SFDP structure.
 * 
 * @param sfdp Pointer to an `sfdp_t` structure containing the SFDP data to be dumped.
 * 
 * @note This function is marked as `__attribute__((unused))` to avoid unused function warnings if not used.
 * 
 * @warning If the provided `sfdp` pointer is NULL, the function will print a trace log indicating the issue.
 */
__attribute__((unused)) static inline void spif_nor_dump_sfdp(const sfdp_t *sfdp)
{
	if (sfdp == NULL) {
		pr_trace("SFDP data is NULL.\n");
		return;
	}

	pr_trace("SFDP Header:\n");
	pr_trace("  Signature: %c%c%c%c\n", sfdp->header.sign[0], sfdp->header.sign[1], sfdp->header.sign[2],
		sfdp->header.sign[3]);
	pr_trace("  Minor version: %u\n", sfdp->header.minor);
	pr_trace("  Major version: %u\n", sfdp->header.major);
	pr_trace(
		"  Number of Parameter Headers: %u (wire NPH=%u)\n", sfdp->parameter_header_count, sfdp->header.nph);
	pr_trace("  Unused: 0x%02X\n", sfdp->header.unused);

	pr_trace("SFDP Parameter Headers:\n");
	for (int i = 0; i < sfdp->parameter_header_count; i++) {
		const sfdp_parameter_header_t *header = &sfdp->parameter_header[i];
		bool unused = header->idlsb == 0xff && header->minor == 0xff && header->major == 0xff &&
			      header->length == 0xff && header->ptp[0] == 0xff && header->ptp[1] == 0xff &&
			      header->ptp[2] == 0xff && header->idmsb == 0xff;

		pr_trace("  Parameter Header #%d:\n", i + 1);
		if (unused) {
			pr_trace("    unused\n");
			continue;
		}
		pr_trace("    IDLSB: 0x%02X\n", sfdp->parameter_header[i].idlsb);
		pr_trace("    Minor version: %u\n", sfdp->parameter_header[i].minor);
		pr_trace("    Major version: %u\n", sfdp->parameter_header[i].major);
		pr_trace("    Length: %u\n", sfdp->parameter_header[i].length);
		pr_trace("    PTP: 0x%02X 0x%02X 0x%02X\n", sfdp->parameter_header[i].ptp[0],
			sfdp->parameter_header[i].ptp[1], sfdp->parameter_header[i].ptp[2]);
		pr_trace("    IDMSB: 0x%02X\n", sfdp->parameter_header[i].idmsb);
	}

	pr_trace("SFDP Basic Table:\n");
	pr_trace("  Minor version: %u\n", sfdp->basic_table.minor);
	pr_trace("  Major version: %u\n", sfdp->basic_table.major);
	pr_trace("  Table (%u x 4 bytes):\n", sfdp->basic_table.length);
	for (int i = 0; i < sfdp->basic_table.length; i++) {
		pr_trace("    ");
		for (int j = 0; j < 4; j++) {
			printk(LOG_LEVEL_MUTE, "0x%02X ", sfdp->basic_table.table[i * 4 + j]);
		}
		printk(LOG_LEVEL_MUTE, "\n");
	}
}

/**
 * @brief Read the SFDP (Serial Flash Discoverable Parameters) data from a SPI NOR Flash chip.
 * 
 * This function sends the appropriate commands to the SPI NOR Flash to retrieve the SFDP information.
 * The SFDP is a standardized way for flash devices to expose their capabilities and characteristics.
 * It reads the SFDP header and parameter headers, then fetches the basic table if the parameter header conditions are met.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 * @param sfdp Pointer to a `sfdp_t` structure where the SFDP data will be stored.
 * 
 * @return 1 if the SFDP data was successfully read, 0 if there was an error or the data was invalid.
 */
static inline int spif_nor_read_sfdp(spif_nor_t *nor, sfdp_t *sfdp)
{
	uint32_t addr;
	uint8_t tx[5];
	uint32_t i;

	memset(sfdp, 0, sizeof(sfdp_t));
	tx[0] = NOR_OPCODE_SFDP;
	tx[1] = 0x0;
	tx[2] = 0x0;
	tx[3] = 0x0;
	tx[4] = 0x0;
	if (spif_nor_transfer(nor, tx, 5, &sfdp->header, sizeof(sfdp_header_t)) != 0) {
		pr_trace("SFDP: read header failed.\n");
		return 0;
	}

	if ((sfdp->header.sign[0] != 'S') || (sfdp->header.sign[1] != 'F') || (sfdp->header.sign[2] != 'D') ||
		(sfdp->header.sign[3] != 'P')) {
		pr_trace("SFDP: invalid signature 0x%02X 0x%02X 0x%02X 0x%02X.\n", sfdp->header.sign[0],
			     sfdp->header.sign[1], sfdp->header.sign[2], sfdp->header.sign[3]);
		return 0;
	}

	/* NPH is zero-based on the wire: NPH=0 means one parameter header. */
	uint32_t header_count = (uint32_t)sfdp->header.nph + 1U;
	if (header_count > SFDP_MAX_NPH)
		header_count = SFDP_MAX_NPH;
	sfdp->parameter_header_count = (uint8_t)header_count;

	for (i = 0; i < header_count; i++) {
		addr = i * sizeof(sfdp_parameter_header_t) + sizeof(sfdp_header_t);
		tx[0] = NOR_OPCODE_SFDP;
		tx[1] = (addr >> 16) & 0xff;
		tx[2] = (addr >> 8) & 0xff;
		tx[3] = (addr >> 0) & 0xff;
		tx[4] = 0x0;
		if (spif_nor_transfer(nor, tx, 5, &sfdp->parameter_header[i], sizeof(sfdp_parameter_header_t)) != 0) {
			pr_trace("SFDP: read parameter header #%u failed.\n", i);
			return 0;
		}
	}
	for (i = 0; i < header_count; i++) {
		uint32_t table_length;
		uint32_t table_bytes;

		if ((sfdp->parameter_header[i].idlsb == 0x00) && (sfdp->parameter_header[i].idmsb == 0xff) &&
			sfdp->parameter_header[i].length != 0U) {
			addr = (sfdp->parameter_header[i].ptp[0] << 0) | (sfdp->parameter_header[i].ptp[1] << 8) |
			       (sfdp->parameter_header[i].ptp[2] << 16);
			table_length = sfdp->parameter_header[i].length;
			if (table_length > sizeof(sfdp->basic_table.table) / 4U)
				table_length = sizeof(sfdp->basic_table.table) / 4U;
			table_bytes = table_length * 4U;
			if (table_bytes > 0x01000000U - addr)
				continue;
			tx[0] = NOR_OPCODE_SFDP;
			tx[1] = (addr >> 16) & 0xff;
			tx[2] = (addr >> 8) & 0xff;
			tx[3] = (addr >> 0) & 0xff;
			tx[4] = 0x0;
			if (spif_nor_transfer(nor, tx, 5, &sfdp->basic_table.table[0], table_bytes) == 0) {
				sfdp->basic_table.length = table_length;
				sfdp->basic_table.major = sfdp->parameter_header[i].major;
				sfdp->basic_table.minor = sfdp->parameter_header[i].minor;
				pr_trace("SFDP: basic table read OK (header #%u, length=%u, addr=0x%X).\n", i, table_length, addr);
				return 1;
			}
		}
	}
	pr_trace("SFDP: no valid JEDEC basic table found.\n");
	return 0;
}

/**
 * @brief Read the identification (ID) of the SPI NOR Flash chip.
 * 
 * This function sends the "Read ID" command (RDID) to the SPI NOR Flash chip and reads back its 3-byte identification.
 * The identification is stored in the provided `id` variable as a 24-bit value, combining the 3 received bytes.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 * @param id Pointer to a `uint32_t` variable where the 24-bit chip ID will be stored.
 * 
 * @return 1 if the ID was successfully read, 0 if the transfer failed.
 */
static inline int spinor_read_id(spif_nor_t *nor, uint32_t *id)
{
	uint8_t tx[1];
	uint8_t rx[3];

	tx[0] = NOR_OPCODE_RDID;
	if (spif_nor_transfer(nor, tx, 1, rx, 3) != 0)
		return 0;
	*id = (rx[0] << 16) | (rx[1] << 8) | (rx[2] << 0);
	return 1;
}

/**
 * @brief Read the status register of the SPI NOR Flash chip.
 * 
 * This function sends the "Read Status Register" command (RDSR) to the SPI NOR Flash chip and reads back a single byte
 * representing the current status register value.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 * 
 * @return The 1-byte status register value returned by the NOR Flash chip.
 */
static int spif_nor_read_status_register(spif_nor_t *nor, uint8_t *status)
{
	uint8_t tx = NOR_OPCODE_RDSR;

	if (status == NULL)
		return DRIVER_ERROR_INVALID;
	return spif_nor_read_reg(nor, tx, status, 1U);
}

/**
 * @brief Write to the status register of the SPI NOR Flash chip.
 * 
 * This function sends the "Write Status Register" command (WRSR) to the SPI NOR Flash chip, followed by a byte
 * containing the new status register value to be written.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 * @param sr The new status register value to write to the NOR Flash chip.
 */
static inline void spif_nor_write_status_register(spif_nor_t *nor, uint8_t sr)
{
	spif_nor_write_reg(nor, NOR_OPCODE_WRSR, &sr, 1U);
}

/**
 * @brief Wait for SPI NOR Flash to finish operation by checking its "busy" status.
 * 
 * This function continuously checks the NOR Flash status register until the "busy" bit is cleared (i.e., operation is complete).
 * It reads the status register to determine whether the NOR Flash is still in a busy state.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 */
static int spif_nor_wait_for_busy(spif_nor_t *nor)
{
	uint32_t timeout = 0xffff;
	uint8_t status;
	int ret;

	for (;;) {
		ret = spif_nor_read_status_register(nor, &status);
		if (ret != 0)
			return ret;
		if ((status & BIT(0)) == 0U)
			return DRIVER_OK;
		timeout--;
		if (!timeout) {
			pr_warn("SPIF NOR wait busy timeout\n");
			return DRIVER_ERROR_INVALID;
		}
	}
}

/**
 * @brief Reset the SPI NOR Flash chip.
 * 
 * This function sends a specific command sequence (0x66 and 0x99) to reset the NOR Flash chip.
 * These two bytes are typically used to return the NOR Flash to its initial state. 
 * It is often used during chip initialization or recovery processes.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 */
static inline void spif_nor_chip_reset(spif_nor_t *nor)
{
	uint8_t enable = 0x66;
	uint8_t reset = 0x99;

	/* Reset Enable and Reset are separate commands and need separate CS cycles. */
	spif_nor_transfer(nor, &enable, 1U, NULL, 0U);
	spif_nor_transfer(nor, &reset, 1U, NULL, 0U);
	udelay(30);
}

/**
 * @brief Enable write operations on the SPI NOR Flash chip.
 * 
 * This function sends the "Write Enable" command (usually 0x06) to enable write operations on the NOR Flash chip.
 * The Write Enable command must be issued before any write operation can take place.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 */
static int spif_nor_set_write_enable(spif_nor_t *nor)
{
	uint8_t opcode = nor->info.opcode_write_enable;
	uint8_t status;
	int ret;

	ret = spif_nor_transfer(nor, &opcode, sizeof(opcode), NULL, 0U);
	if (ret != 0)
		return ret;
	ret = spif_nor_read_status_register(nor, &status);
	if (ret != 0)
		return ret;
	return (status & BIT(1)) != 0U ? DRIVER_OK : DRIVER_ERROR_INVALID;
}

static uint32_t spif_nor_sfdp_dword(const sfdp_t *sfdp, uint32_t number)
{
	const uint8_t *table;
	uint32_t offset;

	if (sfdp == NULL || number == 0U || number > sfdp->basic_table.length)
		return 0U;
	offset = (number - 1U) * 4U;
	table = &sfdp->basic_table.table[offset];
	return ((uint32_t)table[3] << 24) | ((uint32_t)table[2] << 16) | ((uint32_t)table[1] << 8) | (uint32_t)table[0];
}

static bool spif_nor_rx_quad_capable(const spif_nor_t *nor)
{
	return nor != NULL && nor->spif != NULL && (nor->spif->mode & SPIF_RX_QUAD) != 0U;
}

static bool spif_nor_quad_capable(const spif_nor_t *nor)
{
	return spif_nor_rx_quad_capable(nor);
}

static bool spif_nor_tx_quad_capable(const spif_nor_t *nor)
{
	return nor != NULL && nor->spif != NULL && (nor->spif->mode & SPIF_TX_QUAD) != 0U;
}

static bool spif_nor_dtr_capable(const spif_nor_t *nor)
{
	return nor != NULL && nor->spif != NULL && (nor->spif->mode & SPIF_DTR_MODE) != 0U;
}

static int spif_nor_read_setting(const sfdp_t *sfdp, uint32_t dword, uint32_t shift, uint8_t *opcode, uint8_t *dummy)
{
	uint32_t setting;

	if (opcode == NULL || dummy == NULL || (shift != 0U && shift != 16U))
		return DRIVER_ERROR_INVALID;
	setting = spif_nor_sfdp_dword(sfdp, dword) >> shift;
	*opcode = (uint8_t)(setting >> 8);
	*dummy = (uint8_t)(((setting >> 5) & 0x7U) + (setting & 0x1fU));
	if (*opcode == 0U || *opcode == 0xffU)
		return DRIVER_ERROR_INVALID;
	return DRIVER_OK;
}

static uint8_t spif_nor_read_opcode_for_address(const spi_nor_info_t *info)
{
	if (info == NULL || info->address_length != 4U)
		return info != NULL ? info->opcode_read : NOR_OPCODE_READ;

	switch (info->read_proto) {
	case SNOR_PROTO_1_1_1:
		if (info->opcode_read == NOR_OPCODE_READ)
			return NOR_OPCODE_READ_4B;
		if (info->opcode_read == NOR_OPCODE_READ_FAST)
			return NOR_OPCODE_READ_FAST_4B;
		return info->opcode_read;
	case SNOR_PROTO_1_1_4:
		return info->opcode_read == NOR_OPCODE_READ_1_1_4 ? NOR_OPCODE_READ_1_1_4_4B : info->opcode_read;
	case SNOR_PROTO_1_4_4:
	case SNOR_PROTO_4_4_4:
		return info->opcode_read == NOR_OPCODE_READ_1_4_4 || info->opcode_read == NOR_OPCODE_READ_4_4_4 ?
			       NOR_OPCODE_READ_1_4_4_4B :
			       info->opcode_read;
	case SNOR_PROTO_1_4_4_DTR:
		return info->opcode_read == NOR_OPCODE_READ_DTR ? NOR_OPCODE_READ_DTR_4B : info->opcode_read;
	default:
		return info->opcode_read;
	}
}

static int spif_nor_enable_quad(spif_nor_t *nor)
{
	spi_nor_info_t *info;
	uint8_t status[2];
	int ret;

	if (nor == NULL)
		return DRIVER_ERROR_INVALID;
	info = &nor->info;
	switch (info->qe_method) {
	case 0U:
		return DRIVER_OK;
	case 2U:
		ret = spif_nor_read_reg(nor, NOR_OPCODE_RDSR, &status[0], 1U);
		if (ret != 0)
			return ret;
		if ((status[0] & BIT(6)) != 0U)
			return DRIVER_OK;
		ret = spif_nor_set_write_enable(nor);
		if (ret != 0)
			return ret;
		status[0] |= BIT(6);
		ret = spif_nor_write_reg(nor, NOR_OPCODE_WRSR, status, 1U);
		if (ret != 0)
			return ret;
		ret = spif_nor_wait_for_busy(nor);
		if (ret != 0)
			return ret;
		ret = spif_nor_read_reg(nor, NOR_OPCODE_RDSR, &status[0], 1U);
		return ret == 0 && (status[0] & BIT(6)) != 0U ? DRIVER_OK : DRIVER_ERROR_INVALID;
	case 3U:
		ret = spif_nor_read_reg(nor, NOR_OPCODE_RDSR2, &status[1], 1U);
		if (ret != 0)
			return ret;
		if ((status[1] & BIT(7)) != 0U)
			return DRIVER_OK;
		ret = spif_nor_set_write_enable(nor);
		if (ret != 0)
			return ret;
		status[1] |= BIT(7);
		ret = spif_nor_write_reg(nor, NOR_OPCODE_WRSR2, &status[1], 1U);
		if (ret != 0)
			return ret;
		ret = spif_nor_wait_for_busy(nor);
		if (ret != 0)
			return ret;
		ret = spif_nor_read_reg(nor, NOR_OPCODE_RDSR2, &status[1], 1U);
		return ret == 0 && (status[1] & BIT(7)) != 0U ? DRIVER_OK : DRIVER_ERROR_INVALID;
	case 1U:
	case 4U:
		ret = spif_nor_read_reg(nor, NOR_OPCODE_RDSR, &status[0], 1U);
		if (ret != 0)
			return ret;
		status[1] = BIT(1);
		ret = spif_nor_set_write_enable(nor);
		if (ret != 0)
			return ret;
		ret = spif_nor_write_reg(nor, NOR_OPCODE_WRSR, status, 2U);
		if (ret != 0)
			return ret;
		ret = spif_nor_wait_for_busy(nor);
		if (ret != 0)
			return ret;
		return DRIVER_OK;
	case 5U:
		ret = spif_nor_read_reg(nor, NOR_OPCODE_RDSR, &status[0], 1U);
		if (ret != 0)
			return ret;
		ret = spif_nor_read_reg(nor, NOR_OPCODE_RDSR2, &status[1], 1U);
		if (ret != 0)
			return ret;
		if ((status[1] & BIT(1)) != 0U)
			return DRIVER_OK;
		ret = spif_nor_set_write_enable(nor);
		if (ret != 0)
			return ret;
		status[1] |= BIT(1);
		ret = spif_nor_write_reg(nor, NOR_OPCODE_WRSR, status, 2U);
		if (ret != 0)
			return ret;
		ret = spif_nor_wait_for_busy(nor);
		if (ret != 0)
			return ret;
		ret = spif_nor_read_reg(nor, NOR_OPCODE_RDSR2, &status[1], 1U);
		return ret == 0 && (status[1] & BIT(1)) != 0U ? DRIVER_OK : DRIVER_ERROR_INVALID;
	default:
		return DRIVER_ERROR_INVALID;
	}
}

/**
 * @brief Retrieves the information of the SPI NOR flash.
 * 
 * This function reads the identification (ID) of the SPI NOR flash and attempts
 * to fetch its detailed parameters using the Serial Flash Discoverable Parameters (SFDP)
 
 * table. Based on the flash type and capacity, it configures the address length, 
 * erase opcodes, write granularity, and other flash-specific parameters. If the SFDP
 * table is unavailable, the function will fall back on a predefined lookup table using 
 * the flash's ID to find a match. It then populates the `info` structure with the gathered 
 * information.
 * 
 * @param spi The SPI interface structure representing the SPI controller to interact with the NOR flash.
 * 
 * @return 1 if the flash information was retrieved, or 0 if the flash is not
 *         recognized or cannot be detected.
 * 
 * @note If the SPI NOR flash is recognized using its ID, this function will populate the `info`
 *       structure with its capabilities such as capacity, address length, erase block size,
 *       and supported opcodes. If the SFDP is valid, it also provides more granular details
 *       such as the opcode for 4K, 32K, 64K, and 256K erases, as well as write granularity.
 * 
 * @see spinor_read_id(), spif_nor_read_sfdp(), spif_nor_dump_sfdp(), NOR_OPCODE_WREN, NOR_OPCODE_READ, NOR_OPCODE_PROG
 */
static inline int spif_nor_get_info(spif_nor_t *nor)
{
	sfdp_t sfdp;
	const spi_nor_info_t *tmp_info;
	spi_nor_info_t *info = &nor->info;
	uint32_t v, i, id = 0x0;
	uint64_t capacity;
	uint8_t opcode;
	uint8_t dummy;
	bool quad;
	bool tx_quad;
	bool io_mode;
	bool dtr;

	if (!spinor_read_id(nor, &id))
		return 0;
	info->id = id;
	info->read_proto = SNOR_PROTO_1_1_1;
	info->read_dummy = 0U;
	info->qe_method = 0U;

	if (spif_nor_read_sfdp(nor, &sfdp) && sfdp.basic_table.length >= 9U) {
		info->name = "SFDP";
#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_TRACE
		spif_nor_dump_sfdp(&sfdp);
#endif
		v = spif_nor_sfdp_dword(&sfdp, 2U);
		if (v & BIT(31)) {
			v &= ~BIT(31);
			if (v < 3U || v > 34U)
				return 0;
			capacity = 1ULL << (v - 3U);
		} else {
			capacity = ((uint64_t)v + 1ULL) >> 3;
		}
		if (capacity == 0U || capacity > 0xffffffffULL)
			return 0;
		info->capacity = (uint32_t)capacity;
		/* BFPT DWORD 1 address byte encoding. */
		v = spif_nor_sfdp_dword(&sfdp, 1U);
		switch ((v >> 17) & 0x3U) {
		case 0U:
		case 1U:
			info->address_length = info->capacity > (16U * 1024U * 1024U) ? 4U : 3U;
			break;
		case 2U:
			info->address_length = 4U;
			break;
		default:
			return 0;
		}
		if (((v >> 0) & 0x3) == 0x1)
			info->opcode_erase_4k = (v >> 8) & 0xff;
		else
			info->opcode_erase_4k = 0x00;
		info->opcode_erase_32k = 0x00;
		info->opcode_erase_64k = 0x00;
		info->opcode_erase_256k = 0x00;

		/* Basic flash parameter table 8th dword */
		v = (sfdp.basic_table.table[31] << 24) | (sfdp.basic_table.table[30] << 16) |
		    (sfdp.basic_table.table[29] << 8) | (sfdp.basic_table.table[28] << 0);

		switch ((v >> 0) & 0xff) {
		case 12:
			info->opcode_erase_4k = (v >> 8) & 0xff;
			break;
		case 15:
			info->opcode_erase_32k = (v >> 8) & 0xff;
			break;
		case 16:
			info->opcode_erase_64k = (v >> 8) & 0xff;
			break;
		case 18:
			info->opcode_erase_256k = (v >> 8) & 0xff;
			break;
		default:
			break;
		}
		switch ((v >> 16) & 0xff) {
		case 12:
			info->opcode_erase_4k = (v >> 24) & 0xff;
			break;
		case 15:
			info->opcode_erase_32k = (v >> 24) & 0xff;
			break;
		case 16:
			info->opcode_erase_64k = (v >> 24) & 0xff;
			break;
		case 18:
			info->opcode_erase_256k = (v >> 24) & 0xff;
			break;
		default:
			break;
		}

		/* Basic flash parameter table 9th dword */
		v = (sfdp.basic_table.table[35] << 24) | (sfdp.basic_table.table[34] << 16) |
		    (sfdp.basic_table.table[33] << 8) | (sfdp.basic_table.table[32] << 0);
		switch ((v >> 0) & 0xff) {
		case 12:
			info->opcode_erase_4k = (v >> 8) & 0xff;
			break;
		case 15:
			info->opcode_erase_32k = (v >> 8) & 0xff;
			break;
		case 16:
			info->opcode_erase_64k = (v >> 8) & 0xff;
			break;
		case 18:
			info->opcode_erase_256k = (v >> 8) & 0xff;
			break;
		default:
			break;
		}
		switch ((v >> 16) & 0xff) {
		case 12:
			info->opcode_erase_4k = (v >> 24) & 0xff;
			break;
		case 15:
			info->opcode_erase_32k = (v >> 24) & 0xff;
			break;
		case 16:
			info->opcode_erase_64k = (v >> 24) & 0xff;
			break;
		case 18:
			info->opcode_erase_256k = (v >> 24) & 0xff;
			break;
		default:
			break;
		}
		if (info->opcode_erase_4k != 0x00)
			info->blksz = 4096;
		else if (info->opcode_erase_32k != 0x00)
			info->blksz = 32768;
		else if (info->opcode_erase_64k != 0x00)
			info->blksz = 65536;
		else if (info->opcode_erase_256k != 0x00)
			info->blksz = 262144;

		info->opcode_write_enable = NOR_OPCODE_WREN;
		info->read_granularity = 1;
		info->opcode_read = NOR_OPCODE_READ;
		info->read_proto = SNOR_PROTO_1_1_1;
		info->read_dummy = 0U;
		info->qe_method =
			sfdp.basic_table.length >= 15U ? (uint8_t)((spif_nor_sfdp_dword(&sfdp, 15U) >> 20) & 0x7U) : 0U;

		/* Prefer 1-4-4, then 1-1-4, when the controller exposes the required lines. */
		quad = spif_nor_quad_capable(nor);
		tx_quad = spif_nor_tx_quad_capable(nor);
		io_mode = nor->spif != NULL && (nor->spif->mode & SPIF_IO_MODE) != 0U;
		dtr = io_mode && spif_nor_dtr_capable(nor) && (spif_nor_sfdp_dword(&sfdp, 1U) & BIT(19)) != 0U;
		if (quad && tx_quad && io_mode && (spif_nor_sfdp_dword(&sfdp, 1U) & BIT(21)) != 0U &&
			spif_nor_read_setting(&sfdp, 3U, 0U, &opcode, &dummy) == 0) {
			info->read_proto = dtr ? SNOR_PROTO_1_4_4_DTR : SNOR_PROTO_1_4_4;
			info->opcode_read = opcode;
			info->read_dummy = dtr ? SPIF_NOR_DTR_DUMMY_CYCLES : dummy;
		} else if (quad && (spif_nor_sfdp_dword(&sfdp, 1U) & BIT(22)) != 0U &&
			   spif_nor_read_setting(&sfdp, 3U, 16U, &opcode, &dummy) == 0) {
			info->read_proto = SNOR_PROTO_1_1_4;
			info->opcode_read = opcode;
			info->read_dummy = dummy;
		}
		info->opcode_read = spif_nor_read_opcode_for_address(info);
		if (info->read_proto == SNOR_PROTO_1_4_4_DTR)
			info->opcode_read = info->address_length == 4U ? NOR_OPCODE_READ_DTR_4B : NOR_OPCODE_READ_DTR;

		if ((sfdp.basic_table.major == 1) && (sfdp.basic_table.minor < 5)) {
			/* Basic flash parameter table 1th dword */
			v = (sfdp.basic_table.table[3] << 24) | (sfdp.basic_table.table[2] << 16) |
			    (sfdp.basic_table.table[1] << 8) | (sfdp.basic_table.table[0] << 0);
			if ((v >> 2) & 0x1)
				info->write_granularity = 64;
			else
				info->write_granularity = 1;
		} else if ((sfdp.basic_table.major == 1) && (sfdp.basic_table.minor >= 5) &&
			   sfdp.basic_table.length >= 11U) {
			/* Basic flash parameter table 11th dword */
			v = (sfdp.basic_table.table[43] << 24) | (sfdp.basic_table.table[42] << 16) |
			    (sfdp.basic_table.table[41] << 8) | (sfdp.basic_table.table[40] << 0);
			info->write_granularity = 1 << ((v >> 4) & 0xf);
		}
		info->opcode_write = NOR_OPCODE_PROG;
		return 1;
	} else if ((id != 0xffffff) && (id != 0)) {
		for (i = 0; i < ARRAY_SIZE(spif_nor_info_table); i++) {
			tmp_info = &spif_nor_info_table[i];
			if (id == tmp_info->id) {
				memcpy(info, tmp_info, sizeof(spi_nor_info_t));
				info->read_proto = SNOR_PROTO_1_1_1;
				info->read_dummy = 0U;
				info->qe_method = 0U;
				return 1;
			}
		}
		pr_err("The spi nor flash '0x%x' is not yet supported\r\n", id);
	}
	return 0;
}

/**
 * @brief Reads a specified number of bytes from the SPI NOR flash memory.
 *
 * This function sends the read command and address to the SPI NOR flash
 * memory and retrieves the requested data. It supports different address
 * lengths (3 or 4 bytes) based on the configuration in the `info` structure.
 *
 * @param[in] spi Pointer to the SPI interface structure.
 * @param[in] addr The starting address to read data from in the SPI NOR.
 * @param[out] buf Pointer to the buffer where the read data will be stored.
 * @param[in] count The number of bytes to read from the SPI NOR.
 *
 * @note This function uses the `sunxi_spif_exec_op` function to perform
 *       the SPI data transfer. The number of bytes transferred depends 
 *       on the address length configuration, which is either 3 or 4 bytes.
 * 
 * @details 
 * This function first checks the `address_length` configuration (from 
 * `info`) to determine if the address is 3 bytes or 4 bytes long. Based 
 * on this configuration, it sends the appropriate number of address bytes
 * and the read opcode to the SPI NOR. The data is then transferred to 
 * the provided buffer. The function supports 3-byte or 4-byte address
 * modes, but any other address length is not supported.
 */
static int spif_nor_read_operation(spif_nor_t *nor, enum spi_nor_protocol proto, uint8_t opcode, uint8_t read_dummy,
	uint32_t addr, uint8_t *buf, uint32_t count)
{
	struct spi_mem_op op;
	uint8_t mode = 0U;
	uint8_t cmd_width;
	uint8_t addr_width;
	uint8_t data_width;
	bool dtr;
	bool io_mode;

	if (nor == NULL || buf == NULL || count == 0U)
		return DRIVER_ERROR_INVALID;
	if (nor->info.address_length != 3U && nor->info.address_length != 4U)
		return DRIVER_ERROR_INVALID;

	cmd_width = spi_nor_get_protocol_inst_nbits(proto);
	addr_width = spi_nor_get_protocol_addr_nbits(proto);
	data_width = spi_nor_get_protocol_data_nbits(proto);
	dtr = spi_nor_protocol_is_dtr(proto);
	io_mode = nor->spif != NULL && (nor->spif->mode & SPIF_IO_MODE) != 0U;
	if (cmd_width == 0U || addr_width == 0U || data_width == 0U)
		return DRIVER_ERROR_INVALID;
	op = (struct spi_mem_op){ 0 };
	op.cmd.nbytes = 1U;
	op.cmd.opcode = opcode;
	op.cmd.buswidth = cmd_width;
	op.addr.nbytes = nor->info.address_length;
	op.addr.val = addr;
	op.addr.buswidth = addr_width;
	/* SPIF's TNM dummy field is expressed in clock cycles. */
	op.dummy.nbytes = read_dummy;
	op.dummy.buswidth = addr_width;
	op.data.dir = SPI_MEM_DATA_IN;
	op.data.nbytes = count;
	op.data.buf.in = buf;
	op.data.buswidth = data_width;
	if (io_mode && addr_width == SPI_MEM_BUSWIDTH_4) {
		op.mode.val = &mode;
		op.mode.buswidth = addr_width;
	}
	if (dtr) {
		op.cmd.dtr = 1U;
		op.addr.dtr = 1U;
		op.dummy.dtr = 1U;
		op.data.dtr = 1U;
	}
	return spif_nor_exec_op(nor, &op);
}

static int spif_nor_read_single(spif_nor_t *nor, uint32_t addr, uint8_t *buf, uint32_t count)
{
	uint8_t opcode;

	if (nor == NULL)
		return DRIVER_ERROR_INVALID;
	opcode = nor->info.address_length == 4U ? NOR_OPCODE_READ_4B : NOR_OPCODE_READ;
	return spif_nor_read_operation(nor, SNOR_PROTO_1_1_1, opcode, 0U, addr, buf, count);
}

static int spif_nor_read_bytes(spif_nor_t *nor, uint32_t addr, uint8_t *buf, uint32_t count)
{
	const spi_nor_info_t *info;

	if (nor == NULL)
		return DRIVER_ERROR_INVALID;
	info = &nor->info;
	return spif_nor_read_operation(nor, info->read_proto, info->opcode_read, info->read_dummy, addr, buf, count);
}

static int spif_nor_set_controller_config(spif_nor_t *nor, uint32_t valid, uint32_t speed_hz, bool rx_dtr, bool tx_dtr,
	uint32_t sample_mode, uint32_t sample_delay)
{
	struct spif_cfg cfg = {
		.valid = valid,
		.speed_hz = speed_hz,
		.rx_dtr_en = rx_dtr ? 1U : 0U,
		.tx_dtr_en = tx_dtr ? 1U : 0U,
		.sample_mode = sample_mode,
		.sample_delay = sample_delay,
	};

	if (nor == NULL || nor->spif == NULL)
		return DRIVER_ERROR_INVALID;
	return sunxi_spif_set_config(nor->spif, &cfg);
}

static uint8_t spif_nor_program_opcode(const spi_nor_info_t *info)
{
	if (info != NULL && info->address_length == 4U && info->opcode_write == NOR_OPCODE_PROG)
		return NOR_OPCODE_PROG_4B;
	return info != NULL ? info->opcode_write : NOR_OPCODE_PROG;
}

static uint8_t spif_nor_erase_opcode(const spi_nor_info_t *info, uint8_t opcode)
{
	if (info == NULL || info->address_length != 4U)
		return opcode;
	switch (opcode) {
	case NOR_OPCODE_E4K:
		return NOR_OPCODE_E4K_4B;
	case NOR_OPCODE_E32K:
		return NOR_OPCODE_E32K_4B;
	case NOR_OPCODE_E64K:
		return NOR_OPCODE_E64K_4B;
	default:
		return opcode;
	}
}

static int spif_nor_write_page(spif_nor_t *nor, uint32_t addr, const uint8_t *buf, uint32_t count)
{
	struct spi_mem_op op = { 0 };
	int ret;

	if (nor == NULL || buf == NULL || count == 0U || count > SPIF_NOR_TRAINING_PAGE_SIZE ||
		(addr & (SPIF_NOR_TRAINING_PAGE_SIZE - 1U)) + count > SPIF_NOR_TRAINING_PAGE_SIZE)
		return DRIVER_ERROR_INVALID;
	ret = spif_nor_set_write_enable(nor);
	if (ret != 0)
		return ret;
	op.cmd.nbytes = 1U;
	op.cmd.buswidth = SPI_MEM_BUSWIDTH_1;
	op.cmd.opcode = spif_nor_program_opcode(&nor->info);
	op.addr.nbytes = nor->info.address_length;
	op.addr.buswidth = SPI_MEM_BUSWIDTH_1;
	op.addr.val = addr;
	op.data.dir = SPI_MEM_DATA_OUT;
	op.data.nbytes = count;
	op.data.buswidth = SPI_MEM_BUSWIDTH_1;
	op.data.buf.out = buf;
	ret = spif_nor_exec_op(nor, &op);
	if (ret != 0)
		return ret;
	return spif_nor_wait_for_busy(nor);
}

static int spif_nor_erase_block(spif_nor_t *nor, uint32_t addr, uint32_t size, uint8_t opcode)
{
	struct spi_mem_op op = { 0 };
	int ret;

	if (nor == NULL || size == 0U || (uint64_t)addr + size > nor->info.capacity)
		return DRIVER_ERROR_INVALID;
	ret = spif_nor_set_write_enable(nor);
	if (ret != 0)
		return ret;
	op.cmd.nbytes = 1U;
	op.cmd.buswidth = SPI_MEM_BUSWIDTH_1;
	op.cmd.opcode = spif_nor_erase_opcode(&nor->info, opcode);
	op.addr.nbytes = nor->info.address_length;
	op.addr.buswidth = SPI_MEM_BUSWIDTH_1;
	op.addr.val = addr;
	ret = spif_nor_exec_op(nor, &op);
	if (ret != 0)
		return ret;
	return spif_nor_wait_for_busy(nor);
}

static int spif_nor_get_training_erase(const spif_nor_t *nor, uint32_t *size, uint8_t *opcode)
{
	const spi_nor_info_t *info;

	if (nor == NULL || size == NULL || opcode == NULL)
		return DRIVER_ERROR_INVALID;
	info = &nor->info;
	if (info->opcode_erase_64k != 0U && info->capacity >= 64U * 1024U) {
		*size = 64U * 1024U;
		*opcode = info->opcode_erase_64k;
		return DRIVER_OK;
	}
	if (info->opcode_erase_32k != 0U && info->capacity >= 32U * 1024U) {
		*size = 32U * 1024U;
		*opcode = info->opcode_erase_32k;
		return DRIVER_OK;
	}
	if (info->opcode_erase_4k != 0U && info->capacity >= 4U * 1024U) {
		*size = 4U * 1024U;
		*opcode = info->opcode_erase_4k;
		return DRIVER_OK;
	}
	return DRIVER_ERROR_INVALID;
}

static int spif_nor_restore_training(spif_nor_t *nor, uint32_t erase_size, uint32_t training_size, uint8_t erase_opcode,
	const uint8_t *backup, uint8_t *result)
{
	uint32_t erase_addr;
	uint32_t offset;
	uint32_t count;
	int ret;

	for (erase_addr = 0U; erase_addr < training_size; erase_addr += erase_size) {
		ret = spif_nor_erase_block(nor, erase_addr, erase_size, erase_opcode);
		if (ret != 0)
			return ret;
	}
	for (offset = 0U; offset < training_size; offset += SPIF_NOR_TRAINING_PAGE_SIZE) {
		count = training_size - offset;
		if (count > SPIF_NOR_TRAINING_PAGE_SIZE)
			count = SPIF_NOR_TRAINING_PAGE_SIZE;
		ret = spif_nor_write_page(nor, offset, &backup[offset], count);
		if (ret != 0)
			return ret;
	}
	for (offset = 0U; offset < training_size; offset += SPIF_NOR_TRAINING_PAGE_SIZE) {
		count = training_size - offset;
		if (count > SPIF_NOR_TRAINING_PAGE_SIZE)
			count = SPIF_NOR_TRAINING_PAGE_SIZE;
		ret = spif_nor_read_single(nor, offset, result, count);
		if (ret != 0 || memcmp(result, &backup[offset], count) != 0)
			return DRIVER_ERROR_INVALID;
	}
	return DRIVER_OK;
}

struct spif_nor_training {
	uint32_t erase_size;
	uint32_t test_size;
	uint32_t size;
	uint8_t erase_opcode;
	uint8_t *backup;
	uint8_t *pattern;
	uint8_t *result;
};

static uint32_t spif_nor_training_frequency(const spif_nor_t *nor)
{
	uint32_t frequency = nor->max_frequency;

	if (frequency > SPIF_NOR_TRAINING_FALLBACK_FREQUENCY)
		frequency = SPIF_NOR_TRAINING_FALLBACK_FREQUENCY;
	if (frequency < nor->spif->min_speed_hz)
		frequency = nor->spif->min_speed_hz;
	return frequency;
}

static int spif_nor_training_init(const spif_nor_t *nor, struct spif_nor_training *training)
{
	if (spif_nor_get_training_erase(nor, &training->erase_size, &training->erase_opcode) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;
	training->test_size = SPIF_NOR_TRAINING_PAGE_SIZE * SPIF_NOR_TRAINING_DELAYS;
	if (training->test_size > SPIF_NOR_TRAINING_MAX_PATTERN_SIZE)
		return DRIVER_ERROR_INVALID;
	training->size = ((training->test_size + training->erase_size - 1U) / training->erase_size) * training->erase_size;
	if (training->size > SPIF_NOR_TRAINING_MAX_ERASE_SIZE || (uint64_t)training->size > nor->info.capacity)
		return DRIVER_ERROR_INVALID;
	training->backup = malloc(training->size);
	training->pattern = malloc(SPIF_NOR_TRAINING_PAGE_SIZE);
	training->result = malloc(SPIF_NOR_TRAINING_PAGE_SIZE);
	if (training->backup == NULL || training->pattern == NULL || training->result == NULL)
		return DRIVER_ERROR_INVALID;
	return DRIVER_OK;
}

static void spif_nor_training_cleanup(struct spif_nor_training *training)
{
	free(training->result);
	free(training->pattern);
	free(training->backup);
}

static int spif_nor_backup_training(spif_nor_t *nor, const struct spif_nor_training *training, uint32_t frequency)
{
	int ret;

	ret = spif_nor_set_controller_config(nor,
		SPIF_CFG_SPEED_HZ | SPIF_CFG_RX_DTR | SPIF_CFG_TX_DTR | SPIF_CFG_SAMPLE_MODE | SPIF_CFG_SAMPLE_DELAY,
		frequency, false, false, SUNXI_SPIF_SAMPLE_DEFAULT, SUNXI_SPIF_SAMPLE_DEFAULT);
	if (ret != 0)
		return ret;
	return spif_nor_read_single(nor, 0U, training->backup, training->size);
}

static int spif_nor_program_training_pattern(spif_nor_t *nor, struct spif_nor_training *training)
{
	uint32_t offset;
	uint32_t index;
	int ret;

	for (offset = 0U; offset < training->size; offset += training->erase_size) {
		ret = spif_nor_erase_block(nor, offset, training->erase_size, training->erase_opcode);
		if (ret != 0)
			return ret;
	}
	for (index = 0U; index < SPIF_NOR_TRAINING_PAGE_SIZE; ++index)
		training->pattern[index] = (uint8_t)(0x5aU + index);
	for (offset = 0U; offset < training->test_size; offset += SPIF_NOR_TRAINING_PAGE_SIZE) {
		ret = spif_nor_write_page(nor, offset, training->pattern, SPIF_NOR_TRAINING_PAGE_SIZE);
		if (ret != 0)
			return ret;
	}
	memset(training->result, 0xff, SPIF_NOR_TRAINING_PAGE_SIZE);
	if (spif_nor_read_single(nor, 0U, training->result, SPIF_NOR_TRAINING_PAGE_SIZE) != 0 ||
		memcmp(training->result, training->pattern, SPIF_NOR_TRAINING_PAGE_SIZE) != 0)
		return DRIVER_ERROR_INVALID;
	return DRIVER_OK;
}

#ifdef CONFIG_DRIVER_MTD_SPIF_NOR_SHOW_TRAINING
struct spif_nor_training_window {
	uint32_t start;
	uint32_t length;
};

static void spif_nor_print_training_chart(
	const char samples[SPIF_NOR_TRAINING_MODES][SPIF_NOR_TRAINING_DELAYS + 1U],
	const struct spif_nor_training_window windows[SPIF_NOR_TRAINING_MODES], uint32_t best_mode,
	uint32_t best_delay, uint32_t best_length)
{
	char selected[SPIF_NOR_TRAINING_DELAYS + 1U];
	uint32_t mode;

	pr_info("SPIF NOR: training diagram (O=OK, -=FAIL)\n");
	pr_info("SPIF NOR: vertical axis: sample mode; horizontal axis: delay\n");
	pr_info("SPIF NOR: delay    0         1         2         3         4         5         6\n");
	pr_info("SPIF NOR:          0123456789012345678901234567890123456789012345678901234567890123\n");
	pr_info("SPIF NOR:         +----------------------------------------------------------------+\n");
	for (mode = 0U; mode < SPIF_NOR_TRAINING_MODES; ++mode) {
		if (windows[mode].length == 0U)
			pr_info("SPIF NOR: mode=%u  |%s| window=none\n", mode, samples[mode]);
		else
			pr_info("SPIF NOR: mode=%u  |%s| window=%u-%u (%u)\n", mode, samples[mode],
				windows[mode].start, windows[mode].start + windows[mode].length - 1U,
				windows[mode].length);
	}
	pr_info("SPIF NOR:         +----------------------------------------------------------------+\n");
	if (best_length == 0U)
		return;
	memset(selected, ' ', SPIF_NOR_TRAINING_DELAYS);
	selected[best_delay] = '^';
	selected[SPIF_NOR_TRAINING_DELAYS] = '\0';
	pr_info("SPIF NOR: select  |%s| mode=%u delay=%u\n", selected, best_mode, best_delay);
}
#endif

static int spif_nor_find_training_window(spif_nor_t *nor, const struct spif_nor_training *training, bool dtr,
	uint32_t *best_mode, uint32_t *best_delay, uint32_t *best_length)
{
	const spi_nor_info_t *info = &nor->info;
#ifdef CONFIG_DRIVER_MTD_SPIF_NOR_SHOW_TRAINING
	char samples[SPIF_NOR_TRAINING_MODES][SPIF_NOR_TRAINING_DELAYS + 1U];
	struct spif_nor_training_window windows[SPIF_NOR_TRAINING_MODES] = { 0 };
#endif
	uint32_t mode;

	if (spif_nor_set_controller_config(nor, SPIF_CFG_SPEED_HZ | SPIF_CFG_RX_DTR | SPIF_CFG_TX_DTR,
		    nor->max_frequency, dtr, false, 0U, 0U) != 0)
		return DRIVER_ERROR_INVALID;
	for (mode = 0U; mode < SPIF_NOR_TRAINING_MODES; ++mode) {
		uint32_t delay;
		uint32_t run_start = 0U;
		uint32_t run_length = 0U;

		for (delay = 0U; delay < SPIF_NOR_TRAINING_DELAYS; ++delay) {
			bool passed;

			if (spif_nor_set_controller_config(nor, SPIF_CFG_SAMPLE_MODE | SPIF_CFG_SAMPLE_DELAY, 0U, false,
				    false, mode, delay) != 0)
				return DRIVER_ERROR_INVALID;
			passed = spif_nor_read_operation(nor, info->read_proto, info->opcode_read, info->read_dummy,
					 delay * SPIF_NOR_TRAINING_PAGE_SIZE, training->result,
					 SPIF_NOR_TRAINING_PAGE_SIZE) == 0 &&
				 memcmp(training->pattern, training->result, SPIF_NOR_TRAINING_PAGE_SIZE) == 0;
#ifdef CONFIG_DRIVER_MTD_SPIF_NOR_SHOW_TRAINING
			samples[mode][delay] = passed ? 'O' : '-';
#endif
			if (!passed) {
				run_length = 0U;
			} else {
				if (run_length == 0U)
					run_start = delay;
				++run_length;
				if (run_length > *best_length) {
					*best_length = run_length;
					*best_mode = mode;
					*best_delay = (run_start + delay) / 2U;
				}
#ifdef CONFIG_DRIVER_MTD_SPIF_NOR_SHOW_TRAINING
				if (run_length > windows[mode].length) {
					windows[mode].length = run_length;
					windows[mode].start = run_start;
				}
#endif
			}
		}
#ifdef CONFIG_DRIVER_MTD_SPIF_NOR_SHOW_TRAINING
		samples[mode][SPIF_NOR_TRAINING_DELAYS] = '\0';
#endif
	}
#ifdef CONFIG_DRIVER_MTD_SPIF_NOR_SHOW_TRAINING
	spif_nor_print_training_chart(samples, windows, *best_mode, *best_delay, *best_length);
#endif
	return *best_length == 0U ? DRIVER_ERROR_INVALID : DRIVER_OK;
}

static int spif_nor_restore_training_area(spif_nor_t *nor, const struct spif_nor_training *training,
	uint32_t frequency)
{
	if (spif_nor_set_controller_config(nor,
		    SPIF_CFG_SPEED_HZ | SPIF_CFG_RX_DTR | SPIF_CFG_TX_DTR | SPIF_CFG_SAMPLE_MODE |
			    SPIF_CFG_SAMPLE_DELAY,
		    frequency, false, false, SUNXI_SPIF_SAMPLE_DEFAULT, SUNXI_SPIF_SAMPLE_DEFAULT) != 0)
		return DRIVER_ERROR_INVALID;
	return spif_nor_restore_training(nor, training->erase_size, training->size, training->erase_opcode,
		training->backup, training->result);
}

static int spif_nor_apply_training_config(spif_nor_t *nor, uint32_t frequency, bool dtr, uint32_t mode,
	uint32_t delay)
{
	return spif_nor_set_controller_config(nor,
		SPIF_CFG_SPEED_HZ | SPIF_CFG_RX_DTR | SPIF_CFG_TX_DTR | SPIF_CFG_SAMPLE_MODE | SPIF_CFG_SAMPLE_DELAY,
		frequency, dtr, false, mode, delay);
}

static int spif_nor_train_sampling(spif_nor_t *nor)
{
	struct spif_nor_training training = { 0 };
	uint32_t safe_frequency;
	uint32_t best_mode = 0U;
	uint32_t best_delay = 0U;
	uint32_t best_length = 0U;
	int ret;
	bool dtr;
	bool modified = false;

	if (nor == NULL || nor->spif == NULL)
		return DRIVER_ERROR_INVALID;
	dtr = spi_nor_protocol_is_dtr(nor->info.read_proto);
	safe_frequency = spif_nor_training_frequency(nor);
	if (spif_nor_training_init(nor, &training) != DRIVER_OK)
		goto fallback;
	if (spif_nor_backup_training(nor, &training, safe_frequency) != DRIVER_OK)
		goto fallback;

	/* Program one page per delay point, then test each point at the target timing. */
	modified = true;
	if (spif_nor_program_training_pattern(nor, &training) != DRIVER_OK)
		goto restore;
	if (spif_nor_find_training_window(nor, &training, dtr, &best_mode, &best_delay, &best_length) != DRIVER_OK)
		goto restore;

restore:
	if (modified) {
		if (spif_nor_restore_training_area(nor, &training, safe_frequency) != DRIVER_OK)
			goto cleanup_error;
		modified = false;
	}
	if (best_length == 0U)
		goto fallback;
	if (spif_nor_apply_training_config(nor, nor->max_frequency, dtr, best_mode, best_delay) != 0)
		goto cleanup_error;
	pr_info("SPIF NOR: sample training mode=%u delay=%u window=%u\n", best_mode, best_delay, best_length);
	ret = DRIVER_OK;
	goto cleanup;

fallback:
	if (spif_nor_apply_training_config(nor, safe_frequency, dtr, SUNXI_SPIF_SAMPLE_DEFAULT,
		    SUNXI_SPIF_SAMPLE_DEFAULT) != 0)
		goto cleanup_error;
	pr_warn("SPIF NOR: sample training failed, using %uHz default timing\n", safe_frequency);
	ret = DRIVER_OK;

cleanup:
	spif_nor_training_cleanup(&training);
	return ret;

cleanup_error:
	ret = DRIVER_ERROR_INVALID;
	goto cleanup;
}

/**
 * @brief Detects the presence of an SPI NOR flash chip.
 *
 * This function attempts to identify and initialize the SPI NOR flash chip
 * connected to the specified SPI interface. It resets the chip, waits for it
 * to be ready, and retrieves its information. If successful, it logs the chip ID 
 * and its capacity.
 *
 * @param[in] spi Pointer to the SPI interface structure. This should be 
 *                initialized and configured before calling this function.
 *
 * @return 0 on successful detection and initialization, or -1 if no supported
 *         SPI NOR chip is found.
 *
 * @details The function performs the following steps:
 *          1. Resets the SPI NOR chip.
 *          2. Waits until the chip is not busy.
 *          3. Checks the chip information. If no supported chip is found, 
 *             a warning is logged and the function returns -1.
 *          4. If a chip is detected, its ID and capacity are logged to 
 *             inform the user.
 */
static int spif_nor_select_frequency(spif_nor_t *nor, uint32_t frequency)
{
	if (nor == NULL || nor->spif == NULL || frequency == 0U)
		return DRIVER_ERROR_INVALID;
	if (sunxi_spif_select(nor->spif, nor->chip_select) != 0)
		return DRIVER_ERROR_INVALID;
	if (nor->spif->speed_hz != frequency && sunxi_spif_update_clk(nor->spif, frequency) != 0)
		return DRIVER_ERROR_INVALID;
	return DRIVER_OK;
}

static int spif_nor_select(spif_nor_t *nor)
{
	if (nor == NULL || nor->max_frequency == 0U)
		return DRIVER_ERROR_INVALID;
	return spif_nor_select_frequency(nor, nor->max_frequency);
}

int spif_nor_detect(spif_nor_t *nor)
{
	spi_nor_info_t *info;
	uint32_t safe_frequency;

	if (nor == NULL || nor->spif == NULL || nor->max_frequency == 0U)
		return -1;
	safe_frequency = nor->max_frequency;
	if (safe_frequency > SPIF_NOR_TRAINING_FALLBACK_FREQUENCY)
		safe_frequency = SPIF_NOR_TRAINING_FALLBACK_FREQUENCY;
	if (safe_frequency < nor->spif->min_speed_hz)
		safe_frequency = nor->spif->min_speed_hz;
	if (spif_nor_select_frequency(nor, safe_frequency) != 0)
		return -1;
	info = &nor->info;
	memset(info, 0, sizeof(*info));
	spif_nor_chip_reset(nor);
	if (spif_nor_wait_for_busy(nor) != 0)
		return -1;

	if (!spif_nor_get_info(nor)) {
		pr_warn("SPIF NOR Can not find any supported SPI NOR\n");
		return -1;
	}

	info = &nor->info;
	if (spi_nor_get_protocol_data_nbits(info->read_proto) == 4U) {
		if (spif_nor_enable_quad(nor) != 0) {
			pr_warn("SPIF NOR quad enable failed, using single-bit reads\n");
			info->read_proto = SNOR_PROTO_1_1_1;
			info->read_dummy = 0U;
			info->opcode_read = info->address_length == 4U ? NOR_OPCODE_READ_4B : NOR_OPCODE_READ;
		}
	}
	if (spif_nor_train_sampling(nor) != DRIVER_OK)
		return -1;

	pr_info("SPIF NOR detect spi nor id=0x%06x capacity=%dMB\n", info->id, info->capacity / 1024 / 1024);
	pr_info("SPIF NOR read proto=%08x opcode=0x%02x dummy=%d\n", info->read_proto, info->opcode_read,
		info->read_dummy);

	return 0;
}

/**
 * @brief Reads a block or multiple blocks of data from the SPI NAND flash memory.
 *
 * This function reads one or more contiguous blocks from the SPI NAND flash memory
 * into a provided buffer. It handles reading the data in chunks defined by the 
 * read granularity and performs the necessary address calculations to handle 
 * multiple blocks.
 *
 * @param[in] nor Pointer to the SPI NOR flash instance.
 * @param[out] buf Pointer to the buffer where the read data will be stored.
 * @param[in] blk_no The starting block number from which to read data.
 * @param[in] blk_cnt The number of blocks to read from the SPI NAND.
 *
 * @return The number of blocks successfully read. In case of an error, it will 
 *         return the requested block count, indicating the operation was completed.
 *
 * @note This function assumes that the buffer provided by the caller is large 
 *       enough to accommodate the data being read. If the block count is large,
 *       ensure the buffer has sufficient space for all the blocks.
 *
 * @details 
 * The function reads data from the SPI NAND flash memory in chunks based on the 
 * configured read granularity (`info->read_granularity`). It waits for the SPI
 * bus to be ready before each read operation. The function will continue reading 
 * until the requested number of blocks has been fetched. If the data to be read 
 * exceeds the maximum read length (0x7FFFFFFF bytes), it adjusts the size of 
 * each read operation accordingly.
 */
uint32_t spif_nor_read_block(spif_nor_t *nor, uint8_t *buf, uint32_t blk_no, uint32_t blk_cnt)
{
	const spi_nor_info_t *info;
	uint64_t address;
	uint64_t count;
	uint32_t max_transfer;
	uint32_t length;
	uint8_t *current = buf;

	if (nor == NULL || buf == NULL || blk_cnt == 0U || nor->info.blksz == 0U || spif_nor_select(nor) != 0)
		return 0U;
	info = &nor->info;
	address = (uint64_t)blk_no * info->blksz;
	count = (uint64_t)blk_cnt * info->blksz;
	if (address > info->capacity || count > (uint64_t)info->capacity - address || address > 0xffffffffULL ||
		count > 0xffffffffULL)
		return 0U;
	max_transfer = SPIF_NOR_MAX_TRANSFER;
	if (spif_nor_wait_for_busy(nor) != 0)
		return 0U;
	while (count != 0U) {
		length = count > max_transfer ? max_transfer : (uint32_t)count;
		if (spif_nor_read_bytes(nor, (uint32_t)address, current, length) != 0)
			return 0U;
		address += length;
		current += length;
		count -= length;
	}
	return blk_cnt;
}

/**
 * @brief Reads data from the SPI NOR flash memory.
 *
 * This function reads a specified length of data from a given address in the 
 * SPI NOR flash memory into a provided buffer. The reading is performed in
 * blocks, and it handles cases where the read address is not aligned to
 * the block size.
 *
 * @param[in] nor Pointer to the SPI NOR flash instance.
 * @param[out] buf Pointer to the buffer where the read data will be stored.
 * @param[in] addr The starting address from which to read data in the SPI NOR.
 * @param[in] rxlen The number of bytes to read from the SPI NOR.
 *
 * @return The number of bytes successfully read from the SPI NOR. This can 
 *         be less than `rxlen` if an error occurs or if the end of the 
 *         memory space is reached.
 *
 * @note This function assumes that the buffer provided by the caller is 
 *       large enough to accommodate the data being read.
 * 
 * @details The function first checks if the read address is misaligned with
 *          the block size. If so, it reads a partial block. Then, it reads
 *          as many complete blocks as possible before potentially reading 
 *          another partial block at the end.
 */
uint32_t spif_nor_read(spif_nor_t *nor, uint8_t *buf, uint32_t addr, uint32_t rxlen)
{
	const spi_nor_info_t *info;
	uint32_t max_transfer;
	uint32_t length;
	uint32_t done = 0U;

	if (nor == NULL || buf == NULL || rxlen == 0U || nor->info.capacity == 0U || spif_nor_select(nor) != 0)
		return 0U;
	info = &nor->info;
	if ((uint64_t)addr >= info->capacity)
		return 0U;
	if ((uint64_t)rxlen > (uint64_t)info->capacity - addr)
		rxlen = info->capacity - addr;
	max_transfer = SPIF_NOR_MAX_TRANSFER;
	if (spif_nor_wait_for_busy(nor) != 0)
		return 0U;
	while (done < rxlen) {
		length = rxlen - done;
		if (length > max_transfer)
			length = max_transfer;
		if (spif_nor_read_bytes(nor, addr + done, buf + done, length) != 0)
			break;
		done += length;
	}
	return done;
}

DT2C_DRIVER_COMPAT("jedec,spi-nor");
