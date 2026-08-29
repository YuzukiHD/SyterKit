/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <backtrace.h>
#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>
#include <mmu.h>
#include <stdlib.h>

#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pmu/axp.h>
#include <dt-compatible/i2c-dt.h>
#if defined(CONFIG_DRIVER_UFS)
#include <drivers/ufs/ufs.h>
#include <dt-compatible/ufs-dt.h>
#endif

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>
#include <string.h>

#define UFS_IO_BUFFER_SIZE 0x00100000U

static sunxi_dram_t dram;

#if defined(CONFIG_DRIVER_UFS)
static struct ufs_device *ufs0;
static void *ufs_io_buffer;
static bool ufs_ready;

static bool ufs_parse_u64(const char *text, uint64_t *value)
{
	char *end;
	unsigned long long parsed;

	if (!text || !*text || !value)
		return false;
	parsed = simple_strtoull(text, &end, 0);
	if (end == text || *end != '\0')
		return false;
	*value = (uint64_t)parsed;
	return true;
}

msh_declare_command(ufs_info);
msh_define_help(ufs_info, "show UFS device information", "Usage: ufs_info\n");
int cmd_ufs_info(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	if (!ufs_ready) {
		pr_err("UFS: device is not initialized\n");
		return -1;
	}
	pr_info("UFS: LUN %u, %s, %llu blocks x %u bytes, manufacturer 0x%04x\n", ufs0->lun,
		ufs0->scsi.model[0] ? ufs0->scsi.model : "unknown", (unsigned long long)ufs_capacity(ufs0),
		ufs_block_size(ufs0), ufs_manufacturer_id(ufs0));
	return 0;
}

msh_declare_command(ufs_read);
msh_define_help(ufs_read, "read UFS blocks into the DRAM buffer", "Usage: ufs_read <lba> <blocks>\n");
int cmd_ufs_read(int argc, const char **argv)
{
	uint64_t lba;
	uint64_t blocks;
	uint32_t block_size;

	if (argc != 3 || !ufs_ready || !ufs_parse_u64(argv[1], &lba) || !ufs_parse_u64(argv[2], &blocks) ||
		blocks == 0U || blocks > 0xffffffffULL) {
		pr_err("Usage: ufs_read <lba> <blocks>\n");
		return -1;
	}
	block_size = ufs_block_size(ufs0);
	if (!block_size || blocks > UFS_IO_BUFFER_SIZE / block_size) {
		pr_err("UFS: transfer does not fit the DRAM buffer\n");
		return -1;
	}
	if (ufs_blk_read(ufs0, ufs_io_buffer, lba, (uint32_t)blocks) != blocks)
		return -1;
	dump_hex((uintptr_t)ufs_io_buffer, blocks * block_size > 0x100U ? 0x100U : blocks * block_size);
	return 0;
}

msh_declare_command(ufs_write);
msh_define_help(ufs_write, "write a pattern from the DRAM buffer to UFS", "Usage: ufs_write <lba> <blocks> <byte>\n");
int cmd_ufs_write(int argc, const char **argv)
{
	uint64_t lba;
	uint64_t blocks;
	uint64_t pattern;
	uint32_t block_size;

	if (argc != 4 || !ufs_ready || !ufs_parse_u64(argv[1], &lba) || !ufs_parse_u64(argv[2], &blocks) ||
		!ufs_parse_u64(argv[3], &pattern) || blocks == 0U || blocks > 0xffffffffULL || pattern > 0xffU) {
		pr_err("Usage: ufs_write <lba> <blocks> <byte>\n");
		return -1;
	}
	block_size = ufs_block_size(ufs0);
	if (!block_size || blocks > UFS_IO_BUFFER_SIZE / block_size) {
		pr_err("UFS: transfer does not fit the DRAM buffer\n");
		return -1;
	}
	memset(ufs_io_buffer, (int)pattern, (size_t)blocks * block_size);
	return ufs_blk_write(ufs0, ufs_io_buffer, lba, (uint32_t)blocks) == blocks ? 0 : -1;
}
#endif

extern sunxi_serial_t uart_dbg;

extern void board_common_init(void);

msh_declare_command(bt);
msh_define_help(bt, "backtrace test", "Usage: bt\n");
int cmd_bt(int argc, const char **argv)
{
	dump_stack();
	return 0;
}

msh_declare_command(ddr_test);
msh_define_help(ddr_test, "ddr w/r test", "Usage: ddr_test\n");
int cmd_ddr_test(int argc, const char **argv)
{
	dump_hex(dram.memory_base, 0x100);
	memset((void *)dram.memory_base, 0x5A, 0x2000);
	dump_hex(dram.memory_base, 0x100);
	return 0;
}

const msh_command_entry commands[] = {
	msh_define_command(bt),
	msh_define_command(ddr_test),
#if defined(CONFIG_DRIVER_UFS)
	msh_define_command(ufs_info),
	msh_define_command(ufs_read),
	msh_define_command(ufs_write),
#endif
	msh_command_end,
};

int main(void)
{
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK || pmu_axp8191_config(&pmu, &i2c) != DRIVER_OK) {
		pr_err("PMU: invalid devicetree configuration\n");
		return -1;
	}

	board_common_init();

	sunxi_i2c_init(&i2c);

	sunxi_clk_init();

	sunxi_clk_dump();

	pmu_axp8191_init(&pmu);

	pmu_axp8191_dump(&pmu);

	dram.pmu = &pmu;
	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		pr_err("DRAM: invalid devicetree configuration\n");
		return -1;
	}

	uint32_t dram_size_mb = sunxi_dram_init(&dram);

	if (!dram.memory_size && dram_size_mb)
		dram.memory_size = (size_t)dram_size_mb * 1024U * 1024U;

#if defined(CONFIG_DRIVER_UFS)
	struct ufshc_config ufs_config;

	if (!dram.memory_base || !dram.memory_size || malloc_init(dram.memory_base, dram.memory_size) != 0) {
		pr_err("UFS: unable to initialize the DRAM heap\n");
	} else if (sunxi_ufs_dt_read_alias(&ufs_config, "ufs0") != DRIVER_OK) {
		pr_err("UFS: invalid devicetree configuration\n");
	} else {
		ufs0 = malloc(sizeof(*ufs0));
		ufs_io_buffer = malloc(UFS_IO_BUFFER_SIZE);
		if (!ufs0 || !ufs_io_buffer) {
			pr_err("UFS: unable to allocate DRAM state or transfer buffer\n");
			free(ufs0);
			free(ufs_io_buffer);
			ufs0 = NULL;
			ufs_io_buffer = NULL;
		} else {
			memset(ufs0, 0, sizeof(*ufs0));
			ufs_debug("UFS: init base=%p timeout_us=%u state=%p buffer=%p\n", (void *)ufs_config.base,
				ufs_config.timeout_us, (void *)ufs0, ufs_io_buffer);
			int ufs_ret = ufs_init(ufs0, &ufs_config);
			if (ufs_ret != 0) {
				free(ufs0);
				free(ufs_io_buffer);
				ufs0 = NULL;
				ufs_io_buffer = NULL;
			} else {
				ufs_ready = true;
			}
		}
	}
#endif

	pr_info("Hello World!\n");

	syterkit_shell_attach(commands);

	abort();

	return 0;
}
