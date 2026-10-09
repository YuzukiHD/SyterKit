/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "tcon-top: " fmt

/*
 * TCON top: glue registers shared by the timing controllers (which TCON is
 * fed by which display engine port, who drives the LCD pads, where the TCON
 * pixel clock comes from, the DSI source and clock gate).
 */

#include <io.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include <common.h>
#include <log.h>
#include <dt2c/driver.h>

#include "tcon-priv.h"
#include "tcon-regs.h"

extern const struct sunxi_tcon_top_variant sunxi_tcon_top_variant_sun252iw2;

static const struct sunxi_tcon_top_variant *const top_variants[] = {
	&sunxi_tcon_top_variant_sun252iw2,
};

const struct sunxi_tcon_top_variant *sunxi_tcon_top_find_variant(const char *name)
{
	unsigned int i;

	for (i = 0; name != NULL && i < sizeof(top_variants) / sizeof(top_variants[0]); i++)
		if (strcmp(top_variants[i]->name, name) == 0)
			return top_variants[i];
	return NULL;
}

static uintptr_t tcon_top_reg_address(const sunxi_tcon_top_t *top, uint16_t offset)
{
	return top->res.base + offset;
}

int sunxi_tcon_top_get(sunxi_tcon_top_t *top)
{
	if (top->users++)
		return 0;
	sunxi_disp_res_bus_enable(&top->res, true);
	return 0;
}

void sunxi_tcon_top_put(sunxi_tcon_top_t *top)
{
	if (!top->users || --top->users)
		return;
	sunxi_disp_res_bus_enable(&top->res, false);
}

void sunxi_tcon_top_route_de(sunxi_tcon_top_t *top, uint32_t de_port, uint32_t tcon_id)
{
	uintptr_t reg = tcon_top_reg_address(top, top->var->reg.de_perh);
	uint32_t shift = de_port ? 4 : 0;
	uint32_t other = de_port ? 0 : 4;
	uint32_t val = readl(reg);
	uint32_t old = (val >> shift) & 0x3;

	if (de_port >= top->var->n_de_ports || tcon_id >= top->var->n_tcons) {
		pr_warn("no route from DE port %u to TCON %u\n", de_port, tcon_id);
		return;
	}
	/* two DE ports must never feed the same TCON: swap */
	if (((val >> other) & 0x3) == tcon_id)
		val = (val & ~(0x3U << other)) | (old << other);
	val = (val & ~(0x3U << shift)) | ((tcon_id & 0x3) << shift);
	writel(val, reg);
}

void sunxi_tcon_top_lcd_to_pads(sunxi_tcon_top_t *top, uint32_t tcon_id)
{
	if (tcon_id >= top->var->n_tcons)
		return;
	clrbits_le32(tcon_top_reg_address(top, top->var->reg.tv_setup), tcon_id ? TCON_TOP_TV1_OUT : TCON_TOP_TV0_OUT);
}

void sunxi_tcon_top_lcd_clk_from_phy(sunxi_tcon_top_t *top, uint32_t tcon_id, uint32_t phy_id, bool from_phy)
{
	uint32_t mask = TCON_TOP_LCD_CLK_SRC(tcon_id) | TCON_TOP_PHY_CLK_SRC(phy_id);

	clrsetbits_le32(tcon_top_reg_address(top, top->var->reg.clk_src), mask, from_phy ? mask : 0);
}

void sunxi_tcon_top_dsi_route(sunxi_tcon_top_t *top, uint32_t dsi_id, uint32_t tcon_id, bool enable)
{
	clrsetbits_le32(tcon_top_reg_address(top, top->var->reg.dsi_src), TCON_TOP_DSI_SRC_SEL(dsi_id),
		tcon_id != dsi_id ? TCON_TOP_DSI_SRC_SEL(dsi_id) : 0);
	if (dsi_id < top->var->n_dsi_clk_gates)
		clrsetbits_le32(tcon_top_reg_address(top, top->var->reg.clk_gate), TCON_TOP_DSI_CLK_GATE,
			enable ? TCON_TOP_DSI_CLK_GATE : 0);
}

void sunxi_tcon_top_dump(sunxi_tcon_top_t *top)
{
	pr_debug("tcon_top: tv_setup %08x dsi_src %08x clk_src %08x de_perh %08x clk_gate %08x users %u\n",
		(unsigned int)readl(tcon_top_reg_address(top, top->var->reg.tv_setup)),
		(unsigned int)readl(tcon_top_reg_address(top, top->var->reg.dsi_src)),
		(unsigned int)readl(tcon_top_reg_address(top, top->var->reg.clk_src)),
		(unsigned int)readl(tcon_top_reg_address(top, top->var->reg.de_perh)),
		(unsigned int)readl(tcon_top_reg_address(top, top->var->reg.clk_gate)), (unsigned int)top->users);
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-tcon-top");
