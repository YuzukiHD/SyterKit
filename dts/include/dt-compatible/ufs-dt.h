/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_UFS_DT_H__
#define __DT_COMPATIBLE_UFS_DT_H__

#include <driver.h>
#include <drivers/ufs/host.h>
#include <drivers/ufs/host/sunxi.h>
#include <drivers/ufs/ufshc.h>
#include <dt-compatible/dt-common.h>

#define SUNXI_UFS_COMPATIBLE "allwinner,sunxi-ufs"

/*
 * Read the transport-independent part of the UFS node and attach the host
 * selected by CONFIG_SOC_*.  Clock, reset and PHY sequencing remains in the
 * platform ops so the HCI layer does not acquire CCU/PHY dependencies.
 */
static inline __attribute__((always_inline)) int sunxi_ufs_dt_read_config(
	struct ufshc_config *config, int node,
	struct sunxi_ufs_platform_data *platform_data)
{
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *timeout;
	const dt2c_fdt32_t *axi_clock;
	const dt2c_fdt32_t *cfg_clock;
	const dt2c_fdt32_t *controller_clock;
	const dt2c_fdt32_t *phy_ref_clock;
	const dt2c_fdt32_t *rtc_clock;
	const dt2c_fdt32_t *reset;
	struct ufshc_config parsed = { 0 };
	uintptr_t reg_end;

	if (config == NULL || node < 0 || !syterkit_dt_node_available(node) ||
	    dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
					    SUNXI_UFS_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	timeout = syterkit_dt_cells(node, "allwinner,timeout-us", 1);
	if (reg == NULL || dt2c_fdt32_to_cpu(reg[0]) == 0U ||
	    dt2c_fdt32_to_cpu(reg[1]) == 0U)
		return DRIVER_ERROR_INVALID;

	parsed.base = (uintptr_t)dt2c_fdt32_to_cpu(reg[0]);
	reg_end = parsed.base + (uintptr_t)dt2c_fdt32_to_cpu(reg[1]);
	if (reg_end < parsed.base)
		return DRIVER_ERROR_INVALID;
	parsed.timeout_us = timeout != NULL ? dt2c_fdt32_to_cpu(timeout[0]) :
		UFSHC_TIMEOUT_US;
	if (parsed.timeout_us == 0U)
		return DRIVER_ERROR_INVALID;
	parsed.platform = ufs_platform_default();
	if (parsed.platform == &sunxi_ufs_platform_ops) {
		if (!platform_data || sunxi_ufs_platform_data_init(platform_data) != 0)
			return DRIVER_ERROR_INVALID;
		axi_clock = syterkit_dt_cells(node, "allwinner,axi-clock", 6);
		cfg_clock = syterkit_dt_cells(node, "allwinner,cfg-clock", 6);
		controller_clock = syterkit_dt_cells(node, "allwinner,controller-clock", 15);
		phy_ref_clock = syterkit_dt_cells(node, "allwinner,phy-ref-clock", 1);
		rtc_clock = syterkit_dt_cells(node, "allwinner,rtc-clock", 9);
		reset = syterkit_dt_cells(node, "allwinner,reset", 6);
		if (!axi_clock || !cfg_clock || !controller_clock || !phy_ref_clock || !rtc_clock || !reset)
			return DRIVER_ERROR_INVALID;
		platform_data->variant.axi_clk_reg = (uintptr_t)dt2c_fdt32_to_cpu(axi_clock[0]);
		platform_data->variant.axi_clk_gate = dt2c_fdt32_to_cpu(axi_clock[1]);
		platform_data->variant.axi_clk_src_mask = dt2c_fdt32_to_cpu(axi_clock[2]);
		platform_data->variant.axi_clk_src_300 = dt2c_fdt32_to_cpu(axi_clock[3]);
		platform_data->variant.axi_clk_src_200 = dt2c_fdt32_to_cpu(axi_clock[4]);
		platform_data->variant.axi_clk_factor_mask = dt2c_fdt32_to_cpu(axi_clock[5]);
		platform_data->variant.cfg_clk_reg = (uintptr_t)dt2c_fdt32_to_cpu(cfg_clock[0]);
		platform_data->variant.cfg_clk_gate = dt2c_fdt32_to_cpu(cfg_clock[1]);
		platform_data->variant.cfg_clk_src_mask = dt2c_fdt32_to_cpu(cfg_clock[2]);
		platform_data->variant.cfg_clk_src_480 = dt2c_fdt32_to_cpu(cfg_clock[3]);
		platform_data->variant.cfg_clk_factor_mask = dt2c_fdt32_to_cpu(cfg_clock[4]);
		platform_data->variant.cfg_clk_factor = dt2c_fdt32_to_cpu(cfg_clock[5]);
		platform_data->variant.rtc_xo_ctrl = (uintptr_t)dt2c_fdt32_to_cpu(rtc_clock[0]);
		platform_data->variant.rtc_xo_ctrl1 = (uintptr_t)dt2c_fdt32_to_cpu(rtc_clock[1]);
		platform_data->variant.rtc_wp = (uintptr_t)dt2c_fdt32_to_cpu(rtc_clock[2]);
		platform_data->variant.rtc_ref_type_shift = dt2c_fdt32_to_cpu(rtc_clock[3]);
		platform_data->variant.rtc_ref_type_mask = dt2c_fdt32_to_cpu(rtc_clock[4]);
		platform_data->variant.rtc_external_ref_types = dt2c_fdt32_to_cpu(rtc_clock[5]);
		platform_data->variant.rtc_dcxo_gate = dt2c_fdt32_to_cpu(rtc_clock[6]);
		platform_data->variant.rtc_clk_req_disable = dt2c_fdt32_to_cpu(rtc_clock[7]);
		platform_data->variant.rtc_wp_key = dt2c_fdt32_to_cpu(rtc_clock[8]);
		platform_data->variant.ufs_cfg_reg = parsed.base +
			(uintptr_t)dt2c_fdt32_to_cpu(controller_clock[0]);
		platform_data->variant.ufs_clk_gate_reg = parsed.base +
			(uintptr_t)dt2c_fdt32_to_cpu(controller_clock[1]);
		platform_data->variant.ufs_cfg_clk_freq_mask = dt2c_fdt32_to_cpu(controller_clock[2]);
		platform_data->variant.ufs_cfg_clk_freq = dt2c_fdt32_to_cpu(controller_clock[3]);
		platform_data->variant.ufs_ref_clk_unipro_sel = dt2c_fdt32_to_cpu(controller_clock[4]);
		platform_data->variant.ufs_ref_clk_app_sel = dt2c_fdt32_to_cpu(controller_clock[5]);
		platform_data->variant.ufs_ref_clk_app_enable = dt2c_fdt32_to_cpu(controller_clock[6]);
		platform_data->variant.ufs_ref_clk_freq_mask = dt2c_fdt32_to_cpu(controller_clock[7]);
		for (unsigned int i = 0; i < 4U; ++i)
			platform_data->variant.ufs_ref_clk_freq[i] =
				dt2c_fdt32_to_cpu(controller_clock[8 + i]);
		platform_data->variant.ufs_mphy_cfgclk_gate = dt2c_fdt32_to_cpu(controller_clock[12]);
		platform_data->variant.ufs_clk24m_gate = dt2c_fdt32_to_cpu(controller_clock[13]);
		platform_data->variant.ufs_clk_gate_autogate_off =
			dt2c_fdt32_to_cpu(controller_clock[14]);
		platform_data->variant.phy_ref_clk_ctrl = dt2c_fdt32_to_cpu(phy_ref_clock[0]);
		platform_data->variant.reset_reg = (uintptr_t)dt2c_fdt32_to_cpu(reset[0]);
		platform_data->variant.core_rst = dt2c_fdt32_to_cpu(reset[1]);
		platform_data->variant.phy_rst = dt2c_fdt32_to_cpu(reset[2]);
		platform_data->variant.axi_rst = dt2c_fdt32_to_cpu(reset[3]);
		platform_data->variant.bus_rst = dt2c_fdt32_to_cpu(reset[4]);
		platform_data->variant.ahb_gate = dt2c_fdt32_to_cpu(reset[5]);
		if (!platform_data->variant.axi_clk_reg || !platform_data->variant.cfg_clk_reg ||
			!platform_data->variant.rtc_xo_ctrl || !platform_data->variant.rtc_xo_ctrl1 ||
			!platform_data->variant.rtc_wp || !platform_data->variant.reset_reg ||
			!platform_data->variant.axi_clk_gate || !platform_data->variant.cfg_clk_gate ||
			platform_data->variant.rtc_ref_type_shift >= 32U ||
			platform_data->variant.rtc_ref_type_mask == 0U ||
			platform_data->variant.phy_ref_clk_ctrl == 0U ||
			platform_data->variant.ufs_cfg_reg < parsed.base ||
			platform_data->variant.ufs_cfg_reg + sizeof(uint32_t) > reg_end ||
			platform_data->variant.ufs_clk_gate_reg < parsed.base ||
			platform_data->variant.ufs_clk_gate_reg + sizeof(uint32_t) > reg_end)
			return DRIVER_ERROR_INVALID;
		parsed.platform_priv = platform_data;
	}
	*config = parsed;
	SYTERKIT_DT_TRACE_NODE("ufs", node);
	SYTERKIT_DT_TRACE("ufs config base=%p timeout_us=%u\n",
			  (void *)config->base, config->timeout_us);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_ufs_dt_read_alias(
	struct ufshc_config *config, const char *alias,
	struct sunxi_ufs_platform_data *platform_data)
{
	if (config == NULL || alias == NULL)
		return DRIVER_ERROR_INVALID;
	return sunxi_ufs_dt_read_config(config,
			syterkit_dt_alias_node(alias, SUNXI_UFS_COMPATIBLE), platform_data);
}

#endif /* __DT_COMPATIBLE_UFS_DT_H__ */
