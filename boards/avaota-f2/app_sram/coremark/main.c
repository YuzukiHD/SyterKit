/* SPDX-License-Identifier: GPL-2.0+ */

/* Run CoreMark (components/coremark) on the C907 from SRAM; the image needs no DRAM. */

#include <drivers/serial/serial.h>

#include <common.h>
#include <csr.h>
#include <drivers/clk/clk.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pmu/axp.h>
#include <dt-compatible/i2c-dt.h>
#include <log.h>
#include <timer.h>

int coremark_main(void);

/* The CPU core is supplied by the AXP333 DCDC1; it has to be raised before the PLL. */
static void cpu_voltage_set(int mv)
{
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;

	if (sunxi_i2c_dt_read_alias(&i2c, "i2c2") != DRIVER_OK || pmu_axp333_config(&pmu, &i2c) != DRIVER_OK) {
		pr_err("PMU: invalid devicetree configuration\n");
		return;
	}
	sunxi_i2c_init(&i2c);
	pmu_axp333_init(&pmu);

	pr_info("PMU: dcdc1 = %d mV\n", pmu_axp333_get_vol(&pmu, "dcdc1"));
	pmu_axp333_set_vol(&pmu, "dcdc1", mv, 1);
	mdelay(5);
	pr_info("PMU: dcdc1 set to %d mV, now %d mV\n", mv, pmu_axp333_get_vol(&pmu, "dcdc1"));
}

int main(void)
{
	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();

	if (CONFIG_COREMARK_CPU_MV > 0)
		cpu_voltage_set(CONFIG_COREMARK_CPU_MV);

	/* From FEL the cores run on the 24 MHz oscillator; this switches them to the CPU PLL. */
	sunxi_clk_init();
	if (CONFIG_COREMARK_CPU_MHZ > 0) {
		int rc = sunxi_clk_cpu_set_freq(CONFIG_COREMARK_CPU_MHZ);

		if (rc != 0)
			pr_err("CoreMark: cannot set %d MHz (%d)\n", CONFIG_COREMARK_CPU_MHZ, rc);
	}
	{
		uint32_t c0 = (uint32_t)csr_read(mcycle);
		uint32_t t0 = (uint32_t)time_us();

		while ((uint32_t)time_us() - t0 < 10000U)
			;
		pr_info("CoreMark: hart 0 runs at %u MHz\n", ((uint32_t)csr_read(mcycle) - c0) / 10000U);
	}
	pr_info("CoreMark: %d hart(s), running on hart %u\n", CONFIG_COREMARK_THREADS, (unsigned int)csr_read(mhartid));

	coremark_main();

	pr_info("CoreMark: done\n");
	for (;;)
		__asm__ __volatile__("wfi");
}
