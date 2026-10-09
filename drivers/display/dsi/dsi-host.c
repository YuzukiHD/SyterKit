/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * MIPI DSI host controller.
 *
 * Sequence driven by the display: sunxi_dsi_prepare() powers the host and the
 * D-PHY and parks the clock lane in HS, the panel init commands are sent with
 * the dcs/generic write calls (LP), sunxi_dsi_start_video() then streams the
 * pixels the TCON delivers.
 */
#include <io.h>
#include <log.h>
#include <string.h>
#include <timer.h>

#include <drivers/display/display.h>

#include <dt2c/driver.h>

#include "dsi-regs.h"
#include "dsi-variant.h"

#define DSI_ERR_TIMEOUT (-3)
#define DSI_ERR_IO	(-4)

/* data types of the DSI packets */
#define DSI_DT_V_SYNC_START	0x01
#define DSI_DT_V_SYNC_END	0x11
#define DSI_DT_H_SYNC_START	0x21
#define DSI_DT_H_SYNC_END	0x31
#define DSI_DT_BLANKING_PACKET	0x19
#define DSI_DT_GENERIC_SHORT_0	0x03
#define DSI_DT_GENERIC_SHORT_1	0x13
#define DSI_DT_GENERIC_SHORT_2	0x23
#define DSI_DT_GENERIC_LONG	0x29
#define DSI_DT_DCS_SHORT	0x05
#define DSI_DT_DCS_SHORT_PARAM	0x15
#define DSI_DT_DCS_LONG		0x39
#define DSI_DT_PIXEL_16		0x0e
#define DSI_DT_PIXEL_18_PACKED	0x1e
#define DSI_DT_PIXEL_18_LOOSE	0x2e
#define DSI_DT_PIXEL_24		0x3e
#define DSI_DCS_WRITE_MEM_START 0x2c
#define DSI_DCS_WRITE_MEM_CONT	0x3c

#define DSI_PH_DT  DISP_GENMASK(5, 0)
#define DSI_PH_VC  DISP_GENMASK(7, 6)
#define DSI_PH_WC  DISP_GENMASK(23, 8)
#define DSI_PH_ECC DISP_GENMASK(31, 24)

enum dsi_mode {
	DSI_VIDEO_SYNC_PULSE = 0,
	DSI_VIDEO_BURST,
	DSI_COMMAND,
};

/* lane sequences run by the instruction engine */
enum dsi_seq {
	DSI_SEQ_LP11 = 0,
	DSI_SEQ_HS_CLOCK, /* clock lane to HS */
	DSI_SEQ_HS_VIDEO, /* clock + data lanes to HS, stream */
	DSI_SEQ_HS_DATA, /* data lanes to HS, stream */
	DSI_SEQ_LP_TX, /* escape mode LP transmit */
};


static const struct sunxi_dsi_variant *const dsi_variants[] = {
	&sunxi_dsi_variant_sun252iw2,
};

const struct sunxi_dsi_variant *sunxi_dsi_variant_lookup(const char *name);

const struct sunxi_dsi_variant *sunxi_dsi_variant_lookup(const char *name)
{
	unsigned int i;

	if (name == NULL)
		return NULL;
	for (i = 0; i < sizeof(dsi_variants) / sizeof(dsi_variants[0]); i++)
		if (strcmp(name, dsi_variants[i]->name) == 0)
			return dsi_variants[i];
	return NULL;
}

/* Video running state, indexed by host id (the public struct has no room). */
static uint8_t dsi_video_running[4];

#define DSI_RUNNING(d) dsi_video_running[(d)->id & 3]

static inline void dsi_write(const sunxi_dsi_t *d, uint32_t reg, uint32_t val)
{
	writel(val, d->res.base + reg);
}

static inline uint32_t dsi_read(const sunxi_dsi_t *d, uint32_t reg)
{
	return readl(d->res.base + reg);
}

static inline void dsi_update(const sunxi_dsi_t *d, uint32_t reg, uint32_t mask, uint32_t val)
{
	clrsetbits_le32(d->res.base + reg, mask, val);
}

/* ------------------------------------------------------------------ */
/* Packet helpers                                                      */
/* ------------------------------------------------------------------ */
/* Packet header ECC: Hamming code of the DSI specification. */
static uint8_t dsi_ecc(uint32_t header24)
{
	static const uint32_t masks[6] = {
		0xf12cb7,
		0xf2555b,
		0x749a6d,
		0xb8e38e,
		0xdf03f0,
		0xeffc00,
	};
	uint8_t ecc = 0;
	unsigned int i;

	header24 &= 0xffffff;
	for (i = 0; i < 6; i++)
		if (__builtin_parity(header24 & masks[i]))
			ecc |= (uint8_t)(1U << i);
	return ecc;
}

/* CRC-16/CCITT, reflected polynomial 0x8408, seed 0xffff */
static uint16_t dsi_crc_byte(uint16_t crc, uint8_t byte)
{
	unsigned int bit;

	for (bit = 0; bit < 8; bit++) {
		if ((crc ^ byte) & 1)
			crc = (uint16_t)((crc >> 1) ^ 0x8408);
		else
			crc >>= 1;
		byte >>= 1;
	}
	return crc;
}

static uint16_t dsi_crc(const uint8_t *data, size_t len)
{
	uint16_t crc = 0xffff;

	while (len--)
		crc = dsi_crc_byte(crc, *data++);
	return crc;
}

static uint16_t dsi_crc_repeat(uint8_t byte, size_t len)
{
	uint16_t crc = 0xffff;

	while (len--)
		crc = dsi_crc_byte(crc, byte);
	return crc;
}

static uint32_t dsi_header(uint8_t dt, uint8_t vc, uint16_t wc)
{
	uint32_t ph = DISP_FIELD_PREP(DSI_PH_DT, dt) | DISP_FIELD_PREP(DSI_PH_VC, vc) | DISP_FIELD_PREP(DSI_PH_WC, wc);

	return ph | DISP_FIELD_PREP(DSI_PH_ECC, dsi_ecc(ph));
}

static uint32_t dsi_bpp(sunxi_dsi_format_t format)
{
	switch (format) {
	case SUNXI_DSI_FMT_RGB666_PACKED:
		return 18;
	case SUNXI_DSI_FMT_RGB565:
		return 16;
	default:
		return 24;
	}
}

/* ------------------------------------------------------------------ */
/* Mode derived from the panel                                         */
/* ------------------------------------------------------------------ */
struct dsi_mode_info {
	uint32_t hdisplay, hsync_start, hsync_end, htotal;
	uint32_t vdisplay, vsync_start, vsync_end, vtotal;
	uint32_t pixclk_hz;
	uint32_t lanes;
	sunxi_dsi_format_t format;
	uint32_t bpp;
	enum dsi_mode mode;
	uint8_t channel;
};

static void dsi_mode_from_panel(struct dsi_mode_info *m, const sunxi_panel_t *p)
{
	const sunxi_disp_timing_t *t = &p->timing;

	m->hdisplay = t->hactive;
	m->hsync_start = t->hactive + t->hfront_porch;
	m->hsync_end = m->hsync_start + t->hsync_len;
	m->htotal = m->hsync_end + t->hback_porch;
	m->vdisplay = t->vactive;
	m->vsync_start = t->vactive + t->vfront_porch;
	m->vsync_end = m->vsync_start + t->vsync_len;
	m->vtotal = m->vsync_end + t->vback_porch;
	m->pixclk_hz = t->pixel_clock_hz;
	m->lanes = p->dsi_lanes;
	m->format = p->dsi_format;
	m->bpp = dsi_bpp(p->dsi_format);
	m->channel = p->dsi_channel;
	if (!(p->dsi_mode_flags & SUNXI_DSI_MODE_VIDEO))
		m->mode = DSI_COMMAND;
	else if (p->dsi_mode_flags & SUNXI_DSI_MODE_VIDEO_BURST)
		m->mode = DSI_VIDEO_BURST;
	else
		m->mode = DSI_VIDEO_SYNC_PULSE;
}

/*
 * In the non-burst video modes every horizontal blanking period is sent as a
 * packet; it has to be long enough to carry the packet overheads.
 */
static int dsi_check_mode(const struct dsi_mode_info *m)
{
	if (m->mode == DSI_COMMAND || m->mode == DSI_VIDEO_BURST)
		return 0;
	if ((m->hsync_end - m->hsync_start) * m->bpp / 8 <= 4 + 4 + 2 ||
		(m->htotal - m->hsync_end) * m->bpp / 8 <= 10 || (m->hsync_start - m->hdisplay) * m->bpp / 8 <= 6 + 6)
		return DRIVER_ERROR_INVALID;
	return 0;
}

/* ------------------------------------------------------------------ */
/* Instruction engine                                                  */
/* ------------------------------------------------------------------ */
static void dsi_set_inst(
	const sunxi_dsi_t *d, uint32_t slot, uint32_t mode, uint32_t packet, bool clock, uint32_t data_lanes)
{
	uint32_t reg = slot < 8 ? DSI_INST_FUNC(slot) : DSI_INST_FUNC1(slot);
	uint32_t val = DISP_FIELD_PREP(DSI_INST_MODE, mode) | DISP_FIELD_PREP(DSI_INST_PACKET, packet) |
		       DISP_FIELD_PREP(DSI_INST_LANE_DEN, data_lanes) | (clock ? DSI_INST_LANE_CEN : 0);

	if (mode == DSI_MODE_ESCAPE)
		val |= DISP_FIELD_PREP(DSI_INST_ESCAPE_ENTRY, DSI_ESCAPE_LPDT);
	dsi_write(d, reg, val);
}

#define DSI_SLOT(slot, next) ((uint32_t)(next) << (4 * (slot)))

static void dsi_config_instructions(const sunxi_dsi_t *d, const struct dsi_mode_info *m)
{
	const struct sunxi_dsi_variant *v = d->var;
	uint32_t lanes = (1U << m->lanes) - 1;
	uint32_t n0 = v->inst_loop_count, n1;

	/* bank 0: normal operation */
	dsi_set_inst(d, DSI_INST_LP11, DSI_MODE_STOP, 0, true, lanes);
	dsi_set_inst(d, DSI_INST_TBA, DSI_MODE_TBA, 0, false, 0x1);
	dsi_set_inst(d, DSI_INST_HSC, DSI_MODE_HS, DSI_PACKET_PIXEL, true, 0);
	dsi_set_inst(d, DSI_INST_HSD, DSI_MODE_HS, DSI_PACKET_PIXEL, false, lanes);
	dsi_set_inst(d, DSI_INST_LPDT, DSI_MODE_ESCAPE, DSI_PACKET_COMMAND, false, 0x1);
	dsi_set_inst(d, DSI_INST_HSCEXIT, DSI_MODE_HSCEXIT, 0, true, 0);
	dsi_set_inst(d, DSI_INST_NOP, DSI_MODE_STOP, 0, false, lanes);
	dsi_set_inst(d, DSI_INST_DLY, DSI_MODE_NOP, 0, true, lanes);

	if (v->has_second_bank) {
		/* bank 1: same with an initial skew calibration step */
		dsi_set_inst(d, DSI_INST_LP11_1, DSI_MODE_STOP, 0, true, lanes);
		dsi_set_inst(d, DSI_INST_HSC_1, DSI_MODE_HS, DSI_PACKET_PIXEL, true, 0);
		dsi_set_inst(d, DSI_INST_DS_1, DSI_MODE_SCINIT, DSI_PACKET_PIXEL, false, lanes);
		dsi_set_inst(d, DSI_INST_LPDT_1, DSI_MODE_ESCAPE, DSI_PACKET_COMMAND, false, 0x1);
		dsi_set_inst(d, DSI_INST_HSCEXIT_1, DSI_MODE_HSCEXIT, 0, true, 0);
		dsi_set_inst(d, DSI_INST_NOP_1, DSI_MODE_STOP, 0, false, lanes);
		dsi_set_inst(d, DSI_INST_DLY_1, DSI_MODE_NOP, 0, true, lanes);
		dsi_write(d, DSI_INST_LOOP_SEL1, DSI_SLOT(DSI_INST_LP11_1 - 8, 2) | DSI_SLOT(DSI_INST_DLY_1 - 8, 3));
	}

	/* the stop and delay slots loop (select loop counters 2 and 3) */
	dsi_write(d, DSI_INST_LOOP_SEL, DSI_SLOT(DSI_INST_LP11, 2) | DSI_SLOT(DSI_INST_DLY, 3));

	switch (m->mode) {
	case DSI_COMMAND:
		dsi_write(d, DSI_INST_LOOP_NUM,
			DISP_FIELD_PREP(DSI_INST_LOOP_N0, n0) | DISP_FIELD_PREP(DSI_INST_LOOP_N1, n0));
		break;
	case DSI_VIDEO_BURST:
		/* stay in LP for the horizontal blank, counted in module clock (MHz) ticks */
		n1 = (m->htotal - m->hdisplay) * ((d->mod_hz + 500000U) / 1000000U) / (m->pixclk_hz / 1000 * 8);
		n1 = n1 > n0 ? n1 - n0 : 1;
		dsi_write(d, DSI_INST_LOOP_NUM,
			DISP_FIELD_PREP(DSI_INST_LOOP_N0, n0 - 1) | DISP_FIELD_PREP(DSI_INST_LOOP_N1, n1));
		break;
	default:
		dsi_write(d, DSI_INST_LOOP_NUM,
			DISP_FIELD_PREP(DSI_INST_LOOP_N0, n0 - 1) | DISP_FIELD_PREP(DSI_INST_LOOP_N1, n0 - 1));
		break;
	}
	dsi_write(d, DSI_INST_LOOP_NUM2, dsi_read(d, DSI_INST_LOOP_NUM));

	/*
	 * In command mode the NOP slot jumps to HS clock exit after one frame of
	 * lines; in video mode the jump is armed only to pause.
	 */
	dsi_write(d, DSI_INST_JUMP_CFG(0),
		DISP_FIELD_PREP(DSI_JUMP_CFG_POINT, DSI_INST_NOP) | DISP_FIELD_PREP(DSI_JUMP_CFG_TO, DSI_INST_HSCEXIT) |
			(m->mode == DSI_COMMAND ? DSI_JUMP_CFG_EN | DISP_FIELD_PREP(DSI_JUMP_CFG_NUM, m->vdisplay) :
						  DISP_FIELD_PREP(DSI_JUMP_CFG_NUM, 1)));
}

static void dsi_config_basic(const sunxi_dsi_t *d, const struct dsi_mode_info *m, bool slave)
{
	const struct sunxi_dsi_variant *v = d->var;
	uint32_t hbp = m->htotal - m->hsync_start; /* includes sync */
	uint32_t basic = 0;
	uint32_t ctl0 = v->ecc_crc_en ? DSI_CTL0_ECC_EN | DSI_CTL0_CRC_EN : 0;

	dsi_write(d, DSI_TRANS_START, v->trans_start);
	dsi_write(d, DSI_TRANS_ZERO, 0);

	if (m->mode == DSI_COMMAND) {
		dsi_write(d, DSI_BASIC_CTL0, ctl0 | DSI_CTL0_HS_EOTP_EN);
		dsi_write(d, DSI_BASIC_CTL1, 0);
		dsi_write(d, DSI_BASIC_CTL, 0);
		return;
	}

	dsi_write(d, DSI_BASIC_CTL0, ctl0);
	/* the TCON holds the start delay; request data one line early */
	dsi_write(d, DSI_BASIC_CTL1,
		DSI_CTL1_VIDEO_MODE | DSI_CTL1_VIDEO_FRAME_START | DSI_CTL1_VIDEO_PREC_ALIGN |
			DISP_FIELD_PREP(DSI_CTL1_VIDEO_START_DELAY, v->video_start_delay) |
			(slave ? DISP_FIELD_PREP(DSI_CTL1_TRI_DELAY, v->slave_tri_delay) : 0));

	if (m->mode == DSI_VIDEO_BURST) {
		uint32_t sync_point = v->burst_sync_point;
		uint32_t line_num, edge0, edge1;

		line_num = m->htotal * m->bpp / (8 * m->lanes) * 10 / 9;
		edge1 = sync_point + (m->hdisplay + hbp + 20) * m->bpp / (8 * m->lanes);
		edge1 = edge1 < line_num ? edge1 : line_num;
		edge0 = edge1 + (m->hdisplay + 40) * 4 / 8;
		edge0 = edge0 > line_num ? edge0 - line_num : 1;
		dsi_write(d, DSI_BURST_DRQ,
			DISP_FIELD_PREP(DSI_BURST_DRQ_EDGE0, edge0) | DISP_FIELD_PREP(DSI_BURST_DRQ_EDGE1, edge1));
		dsi_write(d, DSI_TCON_DRQ, DSI_DRQ_MODE);
		dsi_write(d, DSI_BURST_LINE,
			DISP_FIELD_PREP(DSI_BURST_LINE_NUM, line_num) |
				DISP_FIELD_PREP(DSI_BURST_SYNC_POINT, sync_point));
		basic |= DSI_BASIC_VIDEO_BURST;
		if (m->lanes == 4)
			basic |= DISP_FIELD_PREP(DSI_BASIC_TRAIL_INV, 0xc) | DSI_BASIC_TRAIL_FILL;
	} else {
		uint32_t hfp = m->htotal - m->hdisplay - hbp;

		if (hfp < 21)
			dsi_write(d, DSI_TCON_DRQ, 0);
		else
			dsi_write(d, DSI_TCON_DRQ,
				DSI_DRQ_MODE | DISP_FIELD_PREP(DSI_DRQ_SET, (hfp - 20) * m->bpp / (8 * 4)));
	}
	if (slave)
		basic |= DSI_BASIC_START_MODE;
	dsi_write(d, DSI_BASIC_CTL, basic);
}

static void dsi_write_blank(const sunxi_dsi_t *d, uint32_t reg0, uint32_t reg1, uint8_t vc, uint32_t size)
{
	dsi_write(d, reg0, dsi_header(DSI_DT_BLANKING_PACKET, vc, (uint16_t)size));
	dsi_write(d, reg1, DISP_FIELD_PREP(DSI_BLK_PD, 0) | DISP_FIELD_PREP(DSI_BLK_PF, dsi_crc_repeat(0, size)));
}

static void dsi_config_packets(const sunxi_dsi_t *d, const struct dsi_mode_info *m)
{
	static const uint8_t pixel_dt[] = {
		[SUNXI_DSI_FMT_RGB888] = DSI_DT_PIXEL_24,
		[SUNXI_DSI_FMT_RGB666] = DSI_DT_PIXEL_18_LOOSE,
		[SUNXI_DSI_FMT_RGB666_PACKED] = DSI_DT_PIXEL_18_PACKED,
		[SUNXI_DSI_FMT_RGB565] = DSI_DT_PIXEL_16,
	};
	uint32_t hspw = m->hsync_end - m->hsync_start;
	uint32_t hbp = m->htotal - m->hsync_start; /* includes sync */
	uint32_t vspw = m->vsync_end - m->vsync_start;
	uint32_t vbp = m->vtotal - m->vsync_start;
	uint32_t hsa, hbp_b, hact, hfp, hblk, vblk;

	if (m->mode == DSI_COMMAND) {
		dsi_write(d, DSI_PIXEL_CTL0, DISP_FIELD_PREP(DSI_PIXEL_FORMAT, m->format));
		dsi_write(d, DSI_PIXEL_PH,
			dsi_header(DSI_DT_DCS_LONG, m->channel, (uint16_t)(1 + m->hdisplay * m->bpp / 8)));
		dsi_write(d, DSI_PIXEL_PD,
			DISP_FIELD_PREP(DSI_PIXEL_PD_TRAN0, DSI_DCS_WRITE_MEM_START) |
				DISP_FIELD_PREP(DSI_PIXEL_PD_TRANN, DSI_DCS_WRITE_MEM_CONT));
		dsi_write(d, DSI_PIXEL_PF0, 0xffff);
		/* CRC seeds of the command byte for first/next lines */
		dsi_write(d, DSI_PIXEL_PF1, 0xe4e9 | (0xf468U << 16));
		return;
	}

	dsi_write(d, DSI_PIXEL_CTL0, DSI_PIXEL_PD_PLUG_DIS | DISP_FIELD_PREP(DSI_PIXEL_FORMAT, 8 + m->format));
	dsi_write(d, DSI_PIXEL_PH, dsi_header(pixel_dt[m->format], 0, (uint16_t)(m->hdisplay * m->bpp / 8)));
	dsi_write(d, DSI_PIXEL_PF0, 0xffff);
	dsi_write(d, DSI_PIXEL_PF1, 0xffffffff);

	hact = m->hdisplay * m->bpp / 8;
	if (m->mode == DSI_VIDEO_BURST) {
		hsa = 0;
		hbp_b = 0;
		hfp = 0;
		hblk = hact;
		vblk = 0;
		dsi_update(d, DSI_BASIC_CTL, DSI_BASIC_HSA_HSE_DIS | DSI_BASIC_HBP_DIS,
			DSI_BASIC_HSA_HSE_DIS | DSI_BASIC_HBP_DIS);
	} else {
		/* blanking payload sizes minus the packet overheads */
		hsa = hspw * m->bpp / 8 - (4 + 4 + 2);
		hbp_b = (hbp - hspw) * m->bpp / 8 - 10;
		hblk = (m->htotal - hspw) * m->bpp / 8 - (4 + 4 + 2);
		hfp = (m->htotal - hbp - m->hdisplay) * m->bpp / 8 - 6 - 6;
		if (m->lanes == 4) {
			uint32_t t = (m->htotal * m->bpp / 8) * m->vtotal - (4 + hblk + 2);

			vblk = m->lanes - t % m->lanes;
		} else {
			vblk = 0;
		}
	}
	(void)hact;

	dsi_write(d, DSI_SYNC_HSS, dsi_header(DSI_DT_H_SYNC_START, 0, 0));
	dsi_write(d, DSI_SYNC_HSE, dsi_header(DSI_DT_H_SYNC_END, 0, 0));
	dsi_write(d, DSI_SYNC_VSS, dsi_header(DSI_DT_V_SYNC_START, 0, 0));
	dsi_write(d, DSI_SYNC_VSE, dsi_header(DSI_DT_V_SYNC_END, 0, 0));

	dsi_write(
		d, DSI_BASIC_SIZE0, DISP_FIELD_PREP(DSI_SIZE0_VSA, vspw) | DISP_FIELD_PREP(DSI_SIZE0_VBP, vbp - vspw));
	dsi_write(d, DSI_BASIC_SIZE1,
		DISP_FIELD_PREP(DSI_SIZE1_VACT, m->vdisplay) | DISP_FIELD_PREP(DSI_SIZE1_VT, m->vtotal));

	dsi_write_blank(d, DSI_BLK_HSA0, DSI_BLK_HSA1, 0, hsa);
	dsi_write_blank(d, DSI_BLK_HBP0, DSI_BLK_HBP1, 0, hbp_b);
	dsi_write_blank(d, DSI_BLK_HFP0, DSI_BLK_HFP1, 0, hfp);
	dsi_write_blank(d, DSI_BLK_HBLK0, DSI_BLK_HBLK1, 0, hblk);
	dsi_write_blank(d, DSI_BLK_VBLK0, DSI_BLK_VBLK1, 0, vblk);
}

static void dsi_hw_run(const sunxi_dsi_t *d, enum dsi_seq seq)
{
	uint32_t jump;

	switch (seq) {
	case DSI_SEQ_HS_CLOCK:
		jump = DSI_SLOT(DSI_INST_LP11, DSI_INST_HSC) | DSI_SLOT(DSI_INST_HSC, DSI_INST_END);
		break;
	case DSI_SEQ_HS_VIDEO:
		jump = DSI_SLOT(DSI_INST_LP11, DSI_INST_HSC) | DSI_SLOT(DSI_INST_HSC, DSI_INST_NOP) |
		       DSI_SLOT(DSI_INST_NOP, DSI_INST_HSD) | DSI_SLOT(DSI_INST_HSD, DSI_INST_DLY) |
		       DSI_SLOT(DSI_INST_DLY, DSI_INST_NOP) | DSI_SLOT(DSI_INST_HSCEXIT, DSI_INST_END);
		break;
	case DSI_SEQ_HS_DATA:
		jump = DSI_SLOT(DSI_INST_LP11, DSI_INST_NOP) | DSI_SLOT(DSI_INST_NOP, DSI_INST_HSD) |
		       DSI_SLOT(DSI_INST_HSD, DSI_INST_DLY) | DSI_SLOT(DSI_INST_DLY, DSI_INST_NOP) |
		       DSI_SLOT(DSI_INST_HSCEXIT, DSI_INST_END);
		break;
	case DSI_SEQ_LP_TX:
		jump = DSI_SLOT(DSI_INST_LP11, DSI_INST_LPDT) | DSI_SLOT(DSI_INST_LPDT, DSI_INST_END);
		break;
	default:
		jump = DSI_SLOT(DSI_INST_LP11, DSI_INST_END);
		break;
	}
	dsi_write(d, DSI_INST_JUMP_SEL, jump);
	/* a rising edge of INST_ST starts the engine */
	dsi_update(d, DSI_BASIC_CTL0, DSI_CTL0_INST_ST, 0);
	dsi_update(d, DSI_BASIC_CTL0, DSI_CTL0_INST_ST, DSI_CTL0_INST_ST);

	if (seq == DSI_SEQ_HS_CLOCK) {
		/* keep the clock lane in HS unless pixel data is plugged */
		bool plug_dis = dsi_read(d, DSI_PIXEL_CTL0) & DSI_PIXEL_PD_PLUG_DIS;

		dsi_update(d, DSI_INST_FUNC(DSI_INST_LP11), DSI_INST_LANE_CEN, plug_dis ? 0 : DSI_INST_LANE_CEN);
	}
}

static int dsi_wait_idle(const sunxi_dsi_t *d, uint32_t timeout_us)
{
	while (dsi_read(d, DSI_BASIC_CTL0) & DSI_CTL0_INST_ST) {
		if (!timeout_us--)
			return DSI_ERR_TIMEOUT;
		udelay(1);
	}
	return 0;
}

/* pause/resume the video stream so LP commands can be interleaved */
static void dsi_video_hold(const sunxi_dsi_t *d, bool hold)
{
	dsi_update(d, DSI_INST_JUMP_CFG(0), DSI_JUMP_CFG_EN, hold ? DSI_JUMP_CFG_EN : 0);
	if (!hold)
		dsi_hw_run(d, DSI_SEQ_HS_VIDEO);
}

/* ------------------------------------------------------------------ */
/* Messages                                                            */
/* ------------------------------------------------------------------ */
/* Send one packet in LP escape mode. @tx: payload (long) or 1..2 bytes (short). */
static int dsi_send(sunxi_dsi_t *d, uint8_t type, uint8_t channel, const uint8_t *tx, size_t tx_len, bool is_long)
{
	uint8_t pkt[256 + 8];
	uint32_t header, word = 0;
	size_t len, i;
	bool hold;
	int ret;

	if (!d->prepared || d->var == NULL)
		return DRIVER_ERROR_INVALID;

	if (is_long) {
		uint16_t crc;

		/* the TX size field is 8 bits wide */
		if (tx_len + 6 > d->var->max_tx_bytes || tx_len + 6 > sizeof(pkt))
			return DRIVER_ERROR_INVALID;
		header = dsi_header(type, channel, (uint16_t)tx_len);
		memcpy(&pkt[4], tx, (int)tx_len);
		crc = dsi_crc(tx, tx_len);
		pkt[4 + tx_len] = (uint8_t)crc;
		pkt[5 + tx_len] = (uint8_t)(crc >> 8);
		len = 4 + tx_len + 2;
	} else {
		uint16_t data = 0;

		if (tx_len > 0)
			data = tx[0];
		if (tx_len > 1)
			data |= (uint16_t)tx[1] << 8;
		header = dsi_header(type, channel, data);
		len = 4;
	}
	pkt[0] = (uint8_t)header;
	pkt[1] = (uint8_t)(header >> 8);
	pkt[2] = (uint8_t)(header >> 16);
	pkt[3] = (uint8_t)(header >> 24);

	/* the engine loops over the video sequence while a stream runs */
	hold = DSI_RUNNING(d);
	if (hold) {
		dsi_video_hold(d, true);
		mdelay(20);
	}

	if (dsi_wait_idle(d, 5000))
		dsi_update(d, DSI_BASIC_CTL0, DSI_CTL0_INST_ST, 0);

	for (i = 0; i < len; i++) {
		word |= (uint32_t)pkt[i] << (8 * (i & 3));
		if ((i & 3) == 3 || i == len - 1) {
			dsi_write(d, DSI_CMD_TX(i / 4), word);
			word = 0;
		}
	}
	dsi_update(d, DSI_CMD_CTL, DSI_CMD_TX_SIZE, DISP_FIELD_PREP(DSI_CMD_TX_SIZE, len - 1));
	dsi_hw_run(d, DSI_SEQ_LP_TX);
	ret = dsi_wait_idle(d, 5000);

	if (hold)
		dsi_video_hold(d, false);
	return ret;
}

int sunxi_dsi_dcs_write(sunxi_dsi_t *dsi, uint8_t cmd, const uint8_t *data, size_t len)
{
	uint8_t buf[256];
	uint8_t type;

	if (dsi == NULL || len + 1 > sizeof(buf) - 6)
		return DRIVER_ERROR_INVALID;
	buf[0] = cmd;
	if (len)
		memcpy(&buf[1], data, (int)len);
	if (len == 0)
		type = DSI_DT_DCS_SHORT;
	else if (len == 1)
		type = DSI_DT_DCS_SHORT_PARAM;
	else
		type = DSI_DT_DCS_LONG;
	return dsi_send(dsi, type, 0, buf, len + 1, type == DSI_DT_DCS_LONG);
}

int sunxi_dsi_generic_write(sunxi_dsi_t *dsi, const uint8_t *data, size_t len)
{
	uint8_t type;

	if (dsi == NULL)
		return DRIVER_ERROR_INVALID;
	if (len == 0)
		type = DSI_DT_GENERIC_SHORT_0;
	else if (len == 1)
		type = DSI_DT_GENERIC_SHORT_1;
	else if (len == 2)
		type = DSI_DT_GENERIC_SHORT_2;
	else
		type = DSI_DT_GENERIC_LONG;
	return dsi_send(dsi, type, 0, data, len, type == DSI_DT_GENERIC_LONG);
}

/* ------------------------------------------------------------------ */
/* Power / stream control                                              */
/* ------------------------------------------------------------------ */
int sunxi_dsi_prepare(sunxi_dsi_t *dsi, const sunxi_panel_t *panel)
{
	sunxi_dphy_dsi_cfg_t phy_cfg;
	struct dsi_mode_info m;
	uint32_t rate;

	if (dsi == NULL || panel == NULL || dsi->var == NULL || dsi->clk == NULL || dsi->phy == NULL)
		return DRIVER_ERROR_INVALID;
	if (!panel->dsi_lanes || panel->dsi_lanes > dsi->var->max_lanes) {
		pr_err("dsi: invalid lane count %u\n", panel->dsi_lanes);
		return DRIVER_ERROR_INVALID;
	}
	dsi_mode_from_panel(&m, panel);
	if (dsi_check_mode(&m)) {
		pr_err("dsi: horizontal blanking too short for the sync packets\n");
		return DRIVER_ERROR_INVALID;
	}

	/* host power: reset, bus clock, module clock */
	sunxi_disp_res_bus_enable(&dsi->res, true);
	rate = sunxi_disp_modclk_set(dsi->clk, &dsi->res, &dsi->var->modclk, dsi->res.mod_rate);
	if (rate == 0 && dsi->res.mod_reg != 0) {
		pr_err("dsi: module clock failed\n");
		sunxi_disp_res_bus_enable(&dsi->res, false);
		return DSI_ERR_IO;
	}
	dsi->mod_hz = rate;

	if (sunxi_dphy_power_on(dsi->phy)) {
		sunxi_disp_modclk_gate(&dsi->res, false);
		sunxi_disp_res_bus_enable(&dsi->res, false);
		return DSI_ERR_IO;
	}

	/* host registers */
	dsi_config_basic(dsi, &m, false);
	dsi_config_instructions(dsi, &m);
	dsi_config_packets(dsi, &m);
	dsi_write(dsi, DSI_DEBUG_DATA, 0xff);
	dsi_write(dsi, DSI_CTL, DSI_CTL_EN);

	memset(&phy_cfg, 0, sizeof(phy_cfg));
	phy_cfg.lanes = m.lanes;
	phy_cfg.pixclk_hz = m.pixclk_hz;
	phy_cfg.bpp = m.bpp;
	if (sunxi_dphy_dsi_enable(dsi->phy, &phy_cfg, NULL)) {
		dsi_write(dsi, DSI_CTL, 0);
		sunxi_dphy_power_off(dsi->phy);
		sunxi_disp_modclk_gate(&dsi->res, false);
		sunxi_disp_res_bus_enable(&dsi->res, false);
		return DSI_ERR_IO;
	}

	/* clock lane in HS before talking to the panel */
	dsi->prepared = true;
	DSI_RUNNING(dsi) = 0;
	dsi_hw_run(dsi, DSI_SEQ_HS_CLOCK);
	dsi_wait_idle(dsi, 1000);
	return 0;
}

int sunxi_dsi_start_video(sunxi_dsi_t *dsi)
{
	if (dsi == NULL || !dsi->prepared)
		return DRIVER_ERROR_INVALID;
	/* command mode frames are kicked by the TCON trigger, nothing to start */
	if (!(dsi_read(dsi, DSI_BASIC_CTL1) & DSI_CTL1_VIDEO_MODE))
		return 0;
	/* the stop slot must not take the clock lane down while the data lanes go to HS */
	if (dsi_read(dsi, DSI_PIXEL_CTL0) & DSI_PIXEL_PD_PLUG_DIS)
		dsi_update(dsi, DSI_INST_FUNC(DSI_INST_LP11), DSI_INST_LANE_CEN, 0);
	/* the LP command transfers drop the clock lane out of HS: bring clock and data lanes up together */
	dsi_hw_run(dsi, DSI_SEQ_HS_VIDEO);
	DSI_RUNNING(dsi) = 1;
	return 0;
}

void sunxi_dsi_stop_video(sunxi_dsi_t *dsi)
{
	if (dsi == NULL || !dsi->prepared)
		return;
	if (DSI_RUNNING(dsi)) {
		/* finish the current frame and fall back to LP */
		dsi_video_hold(dsi, true);
		mdelay(30);
		DSI_RUNNING(dsi) = 0;
	}
}

void sunxi_dsi_unprepare(sunxi_dsi_t *dsi)
{
	if (dsi == NULL || !dsi->prepared)
		return;
	sunxi_dsi_stop_video(dsi);
	sunxi_dphy_dsi_disable(dsi->phy);
	sunxi_dphy_power_off(dsi->phy);
	dsi_write(dsi, DSI_GINT0, 0);
	dsi_write(dsi, DSI_CTL, 0);
	sunxi_disp_modclk_gate(&dsi->res, false);
	sunxi_disp_res_bus_enable(&dsi->res, false);
	dsi->prepared = false;
}

bool sunxi_dsi_frame_start(sunxi_dsi_t *dsi)
{
	if (dsi == NULL || !dsi->prepared || dsi_wait_idle(dsi, 50))
		return false;
	dsi_hw_run(dsi, DSI_SEQ_HS_VIDEO);
	return true;
}

void sunxi_dsi_dump(sunxi_dsi_t *dsi)
{
	static const uint16_t regs[] = {
		DSI_CTL,
		DSI_GINT0,
		DSI_BASIC_CTL,
		DSI_BASIC_CTL0,
		DSI_BASIC_CTL1,
		DSI_BASIC_SIZE0,
		DSI_BASIC_SIZE1,
		DSI_INST_JUMP_SEL,
		DSI_TCON_DRQ,
		DSI_PIXEL_CTL0,
		DSI_PIXEL_PH,
		DSI_CMD_CTL,
		DSI_DEBUG_VIDEO0,
		DSI_DEBUG_INST,
	};
	unsigned int i;

	if (dsi == NULL)
		return;
	pr_debug("dsi%u: %s, module clock %u Hz, video %s\n", dsi->id, dsi->prepared ? "on" : "off", dsi->mod_hz,
		DSI_RUNNING(dsi) ? "running" : "stopped");
	if (!dsi->prepared)
		return;
	for (i = 0; i < sizeof(regs) / sizeof(regs[0]); i++)
		pr_debug("  +%03x: %08x\n", regs[i], dsi_read(dsi, regs[i]));
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-mipi-dsi");
