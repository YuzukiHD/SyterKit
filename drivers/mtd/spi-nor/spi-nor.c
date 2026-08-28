/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <driver.h>

#include <drivers/mtd/spi-nor.h>
#include <drivers/spif/spif.h>
#include <dt2c/driver.h>
#include <string.h>

#define SPI_NOR_MAX_TRANSFER 65536U
#define SPI_NOR_MAX_HEADER   261U

static const spi_nor_info_t spi_nor_info_table[] = {
	{ "W25X40", 0xef3013, 512 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG, NOR_OPCODE_WREN,
		NOR_OPCODE_E4K, 0, NOR_OPCODE_E64K, 0, SNOR_PROTO_1_1_1, 0, 0 },
	{ "W25Q128JVEIQ", 0xefc018, 16 * 1024 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG,
		NOR_OPCODE_WREN, NOR_OPCODE_E4K, NOR_OPCODE_E32K, NOR_OPCODE_E64K, 0, SNOR_PROTO_1_1_1, 0, 0 },
	{ "GD25D10B", 0xc84011, 128 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG, NOR_OPCODE_WREN,
		NOR_OPCODE_E4K, NOR_OPCODE_E32K, NOR_OPCODE_E64K, 0, SNOR_PROTO_1_1_1, 0, 0 },
};

static int spi_nor_exec_op(spi_nor_t *nor, const struct spi_mem_op *op)
{
	uint8_t tx[SPI_NOR_MAX_HEADER];
	uint32_t txlen = 0U;
	uint32_t rxlen;
	uint32_t address;
	uint32_t dummy_bytes;
	uint8_t index;
	spi_io_mode_t io_mode;
	int ret;

	if (nor == NULL || nor->spi == NULL || op == NULL || nor->spi->base == 0U)
		return DRIVER_ERROR_INVALID;

	io_mode = SPI_IO_SINGLE;
	if (op->data.nbytes != 0U && op->data.buswidth == SPI_MEM_BUSWIDTH_4) {
		if (op->addr.buswidth == SPI_MEM_BUSWIDTH_1 && op->mode.val == NULL &&
			op->dummy.buswidth == SPI_MEM_BUSWIDTH_1)
			io_mode = SPI_IO_QUAD_RX;
		else if (op->addr.buswidth == SPI_MEM_BUSWIDTH_4 && op->mode.val != NULL)
			io_mode = SPI_IO_QUAD_IO;
	}

	if (op->cmd.nbytes != 0U)
		tx[txlen++] = (uint8_t)op->cmd.opcode;

	address = (uint32_t)op->addr.val;
	for (index = 0U; index < op->addr.nbytes; ++index)
		tx[txlen++] = (uint8_t)(address >> (8U * (op->addr.nbytes - index - 1U)));

	if (op->mode.val != NULL)
		tx[txlen++] = *(const uint8_t *)op->mode.val;

	dummy_bytes = ((uint32_t)op->dummy.nbytes * op->dummy.buswidth) / 8U;
	if (dummy_bytes != 0U) {
		memset(tx + txlen, 0, dummy_bytes);
		txlen += dummy_bytes;
	}

	if (op->data.dir == SPI_MEM_DATA_OUT) {
		memcpy(tx + txlen, op->data.buf.out, op->data.nbytes);
		txlen += op->data.nbytes;
	}

	rxlen = op->data.dir == SPI_MEM_DATA_IN ? op->data.nbytes : 0U;
	ret = sunxi_spi_transfer(
		nor->spi, io_mode, tx, txlen, op->data.dir == SPI_MEM_DATA_IN ? op->data.buf.in : NULL, rxlen);
	return ret < 0 ? ret : DRIVER_OK;
}

/* Encode the legacy command-plus-buffer calls as one memory operation. */
static int spi_nor_transfer(spi_nor_t *nor, void *txbuf, uint32_t txlen, void *rxbuf, uint32_t rxlen)
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
			op.dummy.nbytes = 8U;
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
	return spi_nor_exec_op(nor, &op);
}

static int spi_nor_read_reg(spi_nor_t *nor, uint8_t opcode, uint8_t *buf, uint32_t len)
{
	return spi_nor_transfer(nor, &opcode, 1U, buf, len);
}

static int spi_nor_write_reg(spi_nor_t *nor, uint8_t opcode, const uint8_t *buf, uint32_t len)
{
	uint8_t tx[1U + 2U];

	if (len > 2U || (len != 0U && buf == NULL))
		return DRIVER_ERROR_INVALID;
	tx[0] = opcode;
	if (len != 0U)
		memcpy(&tx[1], buf, len);
	return spi_nor_transfer(nor, tx, 1U + len, NULL, 0U);
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
__attribute__((unused)) static inline void spi_nor_dump_sfdp(const sfdp_t *sfdp)
{
	if (sfdp == NULL) {
		printk_trace("SFDP data is NULL.\n");
		return;
	}

	printk_trace("SFDP Header:\n");
	printk_trace("  Signature: %c%c%c%c\n", sfdp->header.sign[0], sfdp->header.sign[1], sfdp->header.sign[2],
		sfdp->header.sign[3]);
	printk_trace("  Minor version: %u\n", sfdp->header.minor);
	printk_trace("  Major version: %u\n", sfdp->header.major);
	printk_trace(
		"  Number of Parameter Headers: %u (wire NPH=%u)\n", sfdp->parameter_header_count, sfdp->header.nph);
	printk_trace("  Unused: 0x%02X\n", sfdp->header.unused);

	printk_trace("SFDP Parameter Headers:\n");
	for (int i = 0; i < sfdp->parameter_header_count; i++) {
		const sfdp_parameter_header_t *header = &sfdp->parameter_header[i];
		bool unused = header->idlsb == 0xff && header->minor == 0xff && header->major == 0xff &&
			      header->length == 0xff && header->ptp[0] == 0xff && header->ptp[1] == 0xff &&
			      header->ptp[2] == 0xff && header->idmsb == 0xff;

		printk_trace("  Parameter Header #%d:\n", i + 1);
		if (unused) {
			printk_trace("    unused\n");
			continue;
		}
		printk_trace("    IDLSB: 0x%02X\n", sfdp->parameter_header[i].idlsb);
		printk_trace("    Minor version: %u\n", sfdp->parameter_header[i].minor);
		printk_trace("    Major version: %u\n", sfdp->parameter_header[i].major);
		printk_trace("    Length: %u\n", sfdp->parameter_header[i].length);
		printk_trace("    PTP: 0x%02X 0x%02X 0x%02X\n", sfdp->parameter_header[i].ptp[0],
			sfdp->parameter_header[i].ptp[1], sfdp->parameter_header[i].ptp[2]);
		printk_trace("    IDMSB: 0x%02X\n", sfdp->parameter_header[i].idmsb);
	}

	printk_trace("SFDP Basic Table:\n");
	printk_trace("  Minor version: %u\n", sfdp->basic_table.minor);
	printk_trace("  Major version: %u\n", sfdp->basic_table.major);
	printk_trace("  Table (%u x 4 bytes):\n", sfdp->basic_table.length);
	for (int i = 0; i < sfdp->basic_table.length; i++) {
		printk_trace("    ");
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
static inline int spi_nor_read_sfdp(spi_nor_t *nor, sfdp_t *sfdp)
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
	if (spi_nor_transfer(nor, tx, 5, &sfdp->header, sizeof(sfdp_header_t)) != 0)
		return 0;

	if ((sfdp->header.sign[0] != 'S') || (sfdp->header.sign[1] != 'F') || (sfdp->header.sign[2] != 'D') ||
		(sfdp->header.sign[3] != 'P'))
		return 0;

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
		if (spi_nor_transfer(nor, tx, 5, &sfdp->parameter_header[i], sizeof(sfdp_parameter_header_t)) != 0)
			return 0;
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
			if (spi_nor_transfer(nor, tx, 5, &sfdp->basic_table.table[0], table_bytes) == 0) {
				sfdp->basic_table.length = table_length;
				sfdp->basic_table.major = sfdp->parameter_header[i].major;
				sfdp->basic_table.minor = sfdp->parameter_header[i].minor;
				return 1;
			}
		}
	}
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
static inline int spinor_read_id(spi_nor_t *nor, uint32_t *id)
{
	uint8_t tx[1];
	uint8_t rx[3];

	tx[0] = NOR_OPCODE_RDID;
	if (spi_nor_transfer(nor, tx, 1, rx, 3) != 0)
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
static int spi_nor_read_status_register(spi_nor_t *nor, uint8_t *status)
{
	uint8_t tx = NOR_OPCODE_RDSR;

	if (status == NULL)
		return DRIVER_ERROR_INVALID;
	return spi_nor_read_reg(nor, tx, status, 1U);
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
static inline void spi_nor_write_status_register(spi_nor_t *nor, uint8_t sr)
{
	spi_nor_write_reg(nor, NOR_OPCODE_WRSR, &sr, 1U);
}

/**
 * @brief Wait for SPI NOR Flash to finish operation by checking its "busy" status.
 * 
 * This function continuously checks the NOR Flash status register until the "busy" bit is cleared (i.e., operation is complete).
 * It reads the status register to determine whether the NOR Flash is still in a busy state.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 */
static int spi_nor_wait_for_busy(spi_nor_t *nor)
{
	uint32_t timeout = 0xffff;
	uint8_t status;
	int ret;

	for (;;) {
		ret = spi_nor_read_status_register(nor, &status);
		if (ret != 0)
			return ret;
		if ((status & BIT(0)) == 0U)
			return DRIVER_OK;
		timeout--;
		if (!timeout) {
			printk_warning("SPI NOR: wait busy timeout\n");
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
static inline void spi_nor_chip_reset(spi_nor_t *nor)
{
	uint8_t enable = 0x66;
	uint8_t reset = 0x99;

	/* Reset Enable and Reset are separate commands and need separate CS cycles. */
	spi_nor_transfer(nor, &enable, 1U, NULL, 0U);
	spi_nor_transfer(nor, &reset, 1U, NULL, 0U);
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
static int spi_nor_set_write_enable(spi_nor_t *nor)
{
	uint8_t opcode = nor->info.opcode_write_enable;
	uint8_t status;
	int ret;

	ret = spi_nor_transfer(nor, &opcode, sizeof(opcode), NULL, 0U);
	if (ret != 0)
		return ret;
	ret = spi_nor_read_status_register(nor, &status);
	if (ret != 0)
		return ret;
	return (status & BIT(1)) != 0U ? DRIVER_OK : DRIVER_ERROR_INVALID;
}

static uint32_t spi_nor_sfdp_dword(const sfdp_t *sfdp, uint32_t number)
{
	const uint8_t *table;
	uint32_t offset;

	if (sfdp == NULL || number == 0U || number > sfdp->basic_table.length)
		return 0U;
	offset = (number - 1U) * 4U;
	table = &sfdp->basic_table.table[offset];
	return ((uint32_t)table[3] << 24) | ((uint32_t)table[2] << 16) | ((uint32_t)table[1] << 8) | (uint32_t)table[0];
}

static bool spi_nor_quad_capable(const spi_nor_t *nor)
{
	const sunxi_spi_t *spi;

	if (nor == NULL || nor->spi == NULL)
		return false;
	spi = nor->spi;
	return spi->gpio.gpio_wp.base != 0U && spi->gpio.gpio_hold.base != 0U &&
	       spi->gpio.gpio_wp.mux >= GPIO_PERIPH_MUX2 && spi->gpio.gpio_wp.mux < GPIO_DISABLED &&
	       spi->gpio.gpio_hold.mux >= GPIO_PERIPH_MUX2 && spi->gpio.gpio_hold.mux < GPIO_DISABLED;
}

static bool spi_nor_dtr_capable(const spi_nor_t *nor)
{
	(void)nor;
	return false;
}

static int spi_nor_read_setting(const sfdp_t *sfdp, uint32_t dword, uint32_t shift, uint8_t *opcode, uint8_t *dummy)
{
	uint32_t setting;

	if (opcode == NULL || dummy == NULL || (shift != 0U && shift != 16U))
		return DRIVER_ERROR_INVALID;
	setting = spi_nor_sfdp_dword(sfdp, dword) >> shift;
	*opcode = (uint8_t)(setting >> 8);
	*dummy = (uint8_t)(((setting >> 5) & 0x7U) + (setting & 0x1fU));
	if (*opcode == 0U || *opcode == 0xffU)
		return DRIVER_ERROR_INVALID;
	return DRIVER_OK;
}

static uint8_t spi_nor_read_opcode_for_address(const spi_nor_info_t *info)
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

static int spi_nor_enable_quad(spi_nor_t *nor)
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
		ret = spi_nor_read_reg(nor, NOR_OPCODE_RDSR, &status[0], 1U);
		if (ret != 0)
			return ret;
		if ((status[0] & BIT(6)) != 0U)
			return DRIVER_OK;
		ret = spi_nor_set_write_enable(nor);
		if (ret != 0)
			return ret;
		status[0] |= BIT(6);
		ret = spi_nor_write_reg(nor, NOR_OPCODE_WRSR, status, 1U);
		if (ret != 0)
			return ret;
		ret = spi_nor_wait_for_busy(nor);
		if (ret != 0)
			return ret;
		ret = spi_nor_read_reg(nor, NOR_OPCODE_RDSR, &status[0], 1U);
		return ret == 0 && (status[0] & BIT(6)) != 0U ? DRIVER_OK : DRIVER_ERROR_INVALID;
	case 3U:
		ret = spi_nor_read_reg(nor, NOR_OPCODE_RDSR2, &status[1], 1U);
		if (ret != 0)
			return ret;
		if ((status[1] & BIT(7)) != 0U)
			return DRIVER_OK;
		ret = spi_nor_set_write_enable(nor);
		if (ret != 0)
			return ret;
		status[1] |= BIT(7);
		ret = spi_nor_write_reg(nor, NOR_OPCODE_WRSR2, &status[1], 1U);
		if (ret != 0)
			return ret;
		ret = spi_nor_wait_for_busy(nor);
		if (ret != 0)
			return ret;
		ret = spi_nor_read_reg(nor, NOR_OPCODE_RDSR2, &status[1], 1U);
		return ret == 0 && (status[1] & BIT(7)) != 0U ? DRIVER_OK : DRIVER_ERROR_INVALID;
	case 1U:
	case 4U:
		ret = spi_nor_read_reg(nor, NOR_OPCODE_RDSR, &status[0], 1U);
		if (ret != 0)
			return ret;
		status[1] = BIT(1);
		ret = spi_nor_set_write_enable(nor);
		if (ret != 0)
			return ret;
		ret = spi_nor_write_reg(nor, NOR_OPCODE_WRSR, status, 2U);
		if (ret != 0)
			return ret;
		ret = spi_nor_wait_for_busy(nor);
		if (ret != 0)
			return ret;
		return DRIVER_OK;
	case 5U:
		ret = spi_nor_read_reg(nor, NOR_OPCODE_RDSR, &status[0], 1U);
		if (ret != 0)
			return ret;
		ret = spi_nor_read_reg(nor, NOR_OPCODE_RDSR2, &status[1], 1U);
		if (ret != 0)
			return ret;
		if ((status[1] & BIT(1)) != 0U)
			return DRIVER_OK;
		ret = spi_nor_set_write_enable(nor);
		if (ret != 0)
			return ret;
		status[1] |= BIT(1);
		ret = spi_nor_write_reg(nor, NOR_OPCODE_WRSR, status, 2U);
		if (ret != 0)
			return ret;
		ret = spi_nor_wait_for_busy(nor);
		if (ret != 0)
			return ret;
		ret = spi_nor_read_reg(nor, NOR_OPCODE_RDSR2, &status[1], 1U);
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
 * @see spinor_read_id(), spi_nor_read_sfdp(), spi_nor_dump_sfdp(), NOR_OPCODE_WREN, NOR_OPCODE_READ, NOR_OPCODE_PROG
 */
static inline int spi_nor_get_info(spi_nor_t *nor)
{
	sfdp_t sfdp;
	const spi_nor_info_t *tmp_info;
	spi_nor_info_t *info = &nor->info;
	uint32_t v, i, id = 0x0;
	uint64_t capacity;
	uint8_t opcode;
	uint8_t dummy;
	bool quad;
	bool dtr;

	if (!spinor_read_id(nor, &id))
		return 0;
	info->id = id;
	info->read_proto = SNOR_PROTO_1_1_1;
	info->read_dummy = 0U;
	info->qe_method = 0U;

	if (spi_nor_read_sfdp(nor, &sfdp) && sfdp.basic_table.length >= 9U) {
		info->name = "SFDP";
#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_TRACE
		spi_nor_dump_sfdp(&sfdp);
#endif
		v = spi_nor_sfdp_dword(&sfdp, 2U);
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
		v = spi_nor_sfdp_dword(&sfdp, 1U);
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
			sfdp.basic_table.length >= 15U ? (uint8_t)((spi_nor_sfdp_dword(&sfdp, 15U) >> 20) & 0x7U) : 0U;

		/* Prefer 1-4-4, then 1-1-4, when the SPI wiring exposes IO2/IO3. */
		quad = spi_nor_quad_capable(nor);
		dtr = spi_nor_dtr_capable(nor) && (spi_nor_sfdp_dword(&sfdp, 1U) & BIT(19)) != 0U;
		if (quad && (spi_nor_sfdp_dword(&sfdp, 1U) & BIT(21)) != 0U &&
			spi_nor_read_setting(&sfdp, 3U, 0U, &opcode, &dummy) == 0) {
			info->read_proto = dtr ? SNOR_PROTO_1_4_4_DTR : SNOR_PROTO_1_4_4;
			info->opcode_read = opcode;
			info->read_dummy = dummy;
		} else if (quad && (spi_nor_sfdp_dword(&sfdp, 1U) & BIT(22)) != 0U &&
			   spi_nor_read_setting(&sfdp, 3U, 16U, &opcode, &dummy) == 0) {
			info->read_proto = SNOR_PROTO_1_1_4;
			info->opcode_read = opcode;
			info->read_dummy = dummy;
		}
		info->opcode_read = spi_nor_read_opcode_for_address(info);
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
		for (i = 0; i < ARRAY_SIZE(spi_nor_info_table); i++) {
			tmp_info = &spi_nor_info_table[i];
			if (id == tmp_info->id) {
				memcpy(info, tmp_info, sizeof(spi_nor_info_t));
				info->read_proto = SNOR_PROTO_1_1_1;
				info->read_dummy = 0U;
				info->qe_method = 0U;
				return 1;
			}
		}
		printk_error("The spi nor flash '0x%x' is not yet supported\r\n", id);
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
 * @note This function uses the `sunxi_spi_transfer` function to perform
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
static int spi_nor_read_bytes(spi_nor_t *nor, uint32_t addr, uint8_t *buf, uint32_t count)
{
	const spi_nor_info_t *info;
	struct spi_mem_op op;
	uint8_t cmd_width;
	uint8_t addr_width;
	uint8_t data_width;
	bool dtr;

	if (nor == NULL || buf == NULL || count == 0U)
		return DRIVER_ERROR_INVALID;
	info = &nor->info;
	if (info->address_length != 3U && info->address_length != 4U)
		return DRIVER_ERROR_INVALID;

	cmd_width = spi_nor_get_protocol_inst_nbits(info->read_proto);
	addr_width = spi_nor_get_protocol_addr_nbits(info->read_proto);
	data_width = spi_nor_get_protocol_data_nbits(info->read_proto);
	dtr = spi_nor_protocol_is_dtr(info->read_proto);
	if (cmd_width == 0U || addr_width == 0U || data_width == 0U)
		return DRIVER_ERROR_INVALID;
	op = (struct spi_mem_op){ 0 };
	op.cmd.nbytes = 1U;
	op.cmd.opcode = info->opcode_read;
	op.cmd.buswidth = cmd_width;
	op.addr.nbytes = info->address_length;
	op.addr.val = addr;
	op.addr.buswidth = addr_width;
	op.dummy.nbytes = info->read_dummy;
	op.dummy.buswidth = addr_width == SPI_MEM_BUSWIDTH_4 ? SPI_MEM_BUSWIDTH_4 : SPI_MEM_BUSWIDTH_1;
	if (addr_width == SPI_MEM_BUSWIDTH_4) {
		static const uint8_t mode = 0U;

		op.mode.val = &mode;
		op.mode.buswidth = SPI_MEM_BUSWIDTH_4;
	}
	op.data.dir = SPI_MEM_DATA_IN;
	op.data.nbytes = count;
	op.data.buf.in = buf;
	op.data.buswidth = data_width;
	if (dtr) {
		op.cmd.dtr = 1U;
		op.addr.dtr = 1U;
		op.dummy.dtr = 1U;
		op.data.dtr = 1U;
	}
	return spi_nor_exec_op(nor, &op);
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
static int spi_nor_select(spi_nor_t *nor)
{
	uint32_t frequency;

	if (nor == NULL || nor->max_frequency == 0U)
		return DRIVER_ERROR_INVALID;
	frequency = nor->max_frequency;
	if (nor->spi == NULL || sunxi_spi_select(nor->spi, nor->chip_select) != 0)
		return DRIVER_ERROR_INVALID;
	if (nor->spi->clk_rate != frequency && sunxi_spi_update_clk(nor->spi, frequency) != 0)
		return DRIVER_ERROR_INVALID;
	return DRIVER_OK;
}

int spi_nor_detect(spi_nor_t *nor)
{
	spi_nor_info_t *info;

	if (spi_nor_select(nor) != 0)
		return -1;
	info = &nor->info;
	memset(info, 0, sizeof(*info));
	spi_nor_chip_reset(nor);
	if (spi_nor_wait_for_busy(nor) != 0)
		return -1;

	if (!spi_nor_get_info(nor)) {
		printk_warning("SPI NOR: Can not find any supported SPI NOR\n");
		return -1;
	}

	info = &nor->info;
	if (spi_nor_get_protocol_data_nbits(info->read_proto) == 4U) {
		if (spi_nor_enable_quad(nor) != 0) {
			printk_warning("SPI NOR: quad enable failed, using single-bit reads\n");
			info->read_proto = SNOR_PROTO_1_1_1;
			info->read_dummy = 0U;
			info->opcode_read = info->address_length == 4U ? NOR_OPCODE_READ_4B : NOR_OPCODE_READ;
		}
	}

	printk_info("SPI NOR: detect spi nor id=0x%06x capacity=%dMB\n", info->id, info->capacity / 1024 / 1024);
	printk_info("SPI NOR: read_proto=%d read_dummy=%d opcode_read=0x%02x\n", info->read_proto, info->read_dummy,
		    info->opcode_read);

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
 * @param[in] spi Pointer to the SPI interface structure.
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
uint32_t spi_nor_read_block(spi_nor_t *nor, uint8_t *buf, uint32_t blk_no, uint32_t blk_cnt)
{
	const spi_nor_info_t *info;
	uint64_t address;
	uint64_t count;
	uint32_t max_transfer;
	uint32_t length;
	uint8_t *current = buf;

	if (nor == NULL || buf == NULL || blk_cnt == 0U || nor->info.blksz == 0U || spi_nor_select(nor) != 0)
		return 0U;
	info = &nor->info;
	address = (uint64_t)blk_no * info->blksz;
	count = (uint64_t)blk_cnt * info->blksz;
	if (address > info->capacity || count > (uint64_t)info->capacity - address || address > 0xffffffffULL ||
		count > 0xffffffffULL)
		return 0U;
	max_transfer = SPI_NOR_MAX_TRANSFER;
	if (max_transfer == 0U)
		return 0U;
	while (count != 0U) {
		length = count > max_transfer ? max_transfer : (uint32_t)count;
		if (spi_nor_wait_for_busy(nor) != 0)
			return 0U;
		if (spi_nor_read_bytes(nor, (uint32_t)address, current, length) != 0)
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
 * @param[in] spi Pointer to the SPI interface structure.
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
uint32_t spi_nor_read(spi_nor_t *nor, uint8_t *buf, uint32_t addr, uint32_t rxlen)
{
	const spi_nor_info_t *info;
	uint32_t max_transfer;
	uint32_t length;
	uint32_t done = 0U;

	if (nor == NULL || buf == NULL || rxlen == 0U || nor->info.capacity == 0U || spi_nor_select(nor) != 0)
		return 0U;
	info = &nor->info;
	if ((uint64_t)addr >= info->capacity)
		return 0U;
	if ((uint64_t)rxlen > (uint64_t)info->capacity - addr)
		rxlen = info->capacity - addr;
	max_transfer = SPI_NOR_MAX_TRANSFER;
	if (max_transfer == 0U)
		return 0U;
	while (done < rxlen) {
		length = rxlen - done;
		if (length > max_transfer)
			length = max_transfer;
		if (spi_nor_wait_for_busy(nor) != 0)
			break;
		if (spi_nor_read_bytes(nor, addr + done, buf + done, length) != 0)
			break;
		done += length;
	}
	return done;
}

DT2C_DRIVER_COMPAT("jedec,spi-nor");
