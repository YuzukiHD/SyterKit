/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <mmc/sys-sdhci.h>

#include <sys-dram.h>
#include <sys-sdcard.h>
#include <sys-i2c.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

extern sunxi_serial_t uart_dbg;

extern sunxi_i2c_t i2c_pmu;

extern sunxi_sdhci_t sdhci0;

extern uint32_t dram_para[128];

msh_declare_command(bt);
msh_define_help(bt, "backtrace test", "Usage: bt\n");
int cmd_bt(int argc, const char **argv) {
	dump_stack();
	return 0;
}

msh_declare_command(ddr_test);
msh_define_help(ddr_test, "ddr w/r test", "Usage: ddr_test\n");
int cmd_ddr_test(int argc, const char **argv) {
	dump_hex(SDRAM_BASE, 0x100);
	memset((void *) SDRAM_BASE, 0x5A, 0x2000);
	dump_hex(SDRAM_BASE, 0x100);
	return 0;
}

const msh_command_entry commands[] = {
		msh_define_command(bt),
		msh_define_command(ddr_test),
		msh_command_end,
};

static void sunxi_res_ctrl_init(void) {
	uint8_t reg_val;

	reg_val = readl(SID_RES0_1_BASE) >> 24;
	if ((reg_val & 0xFF) == 0)
		return;

	uint8_t res0_val = (reg_val >> 0) & 0xF;
	writel((0x19190000 | (res0_val & 0xF)), INT_DSI_RES_CTRL_REG);
	writel((0x19190000 | res0_val), INT_CSI_RES_CTRL_REG);
	writel((0x19190000 | res0_val), INT_USB_RES_CTRL_REG);

	uint8_t res1_val = (reg_val >> 4) & 0xF;
	writel((0x19190000 | res1_val), INT_EDP_RES_CTRL_REG);
	writel((0x19190000 | res1_val), INT_HS_COMBO_RES_CTRL_REG);
	writel((0x19190000 | res1_val), INT_DDR_RES_CTRL_REG);

	return;
}

static void sunxi_board_power_init(void) {
	uint32_t sys_vol, gpu_vol;
	uint32_t efuse;
	uint32_t efuse_ext;

	efuse = (readl(SUNXI_SID_BASE + 0x200 + 0x14) & 0xff0000) >> 16;
	efuse_ext = (readl(SUNXI_SID_BASE + 0x200 + 0x14) & 0xff000000) >> 24;
	if (efuse_ext) {
		efuse = efuse_ext;
	}

	switch (efuse) {
		/* VF0/VF4-2/VF3: sys-0.9v gpu-0.94v */
		case 0x00:
		case 0x24:
		case 0x03:
			sys_vol = 900;
			gpu_vol = 940;
			break;
		/* VF1/VF4-4: sys-0.9v gpu-0.9v */
		case 0x01:
			sys_vol = 900;
			gpu_vol = 980;
			break;
		case 0x44:
			sys_vol = 900;
			gpu_vol = 900;
			break;
		/* VF4-3: sys-0.92v gpu-0.96v */
		case 0x34:
			sys_vol = 920;
			gpu_vol = 960;
			break;
		/* default: sys-0.9v gpu-0.94v */
		default:
			sys_vol = 900;
			gpu_vol = 940;
			break;
	}

	sunxi_i2c_init(&i2c_pmu);

	pmu_axp2202_init(&i2c_pmu);
	pmu_axp1530_init(&i2c_pmu);

	if (readl(SUNXI_SOC_VER_REG) & SUNXI_SOC_VER_MASK < 2)
		sys_vol = gpu_vol;

	pmu_axp2202_set_vol(&i2c_pmu, "dcdc1", 1050, 1);
	pmu_axp2202_set_vol(&i2c_pmu, "dcdc2", sys_vol, 1);

	pmu_axp1530_set_vol(&i2c_pmu, "dcdc1", 1000, 1);
	pmu_axp1530_set_vol(&i2c_pmu, "dcdc2", 1000, 1);
	pmu_axp1530_set_vol(&i2c_pmu, "dcdc3", gpu_vol, 1);

	/* dcdc4 for 3v3 */
	pmu_axp2202_set_vol(&i2c_pmu, "dcdc4", 3300, 1);

	/* bldo3 for 1v8 */
	pmu_axp2202_set_vol(&i2c_pmu, "bldo3", 1800, 1);
}

int main(void) {
	sunxi_serial_init(&uart_dbg);

	sunxi_res_ctrl_init();

	show_banner();

	sunxi_board_power_init();

	sunxi_dram_init(dram_para);

	syterkit_shell_attach(commands);

	abort();

	return 0;
}