/* SPDX-License-Identifier: GPL-2.0+ */
#include "../../dsi/dsi-variant.h"

/* sun252iw2 DSI */
const struct sunxi_dsi_variant sunxi_dsi_variant_sun252iw2 = {
	.name = "sun252iw2",
	.modclk = {
		.div = SUNXI_DISP_DIV_LINEAR,
		.div_width = 4,
		.parents = { SUNXI_DISP_SRC_HOSC, SUNXI_DISP_SRC_PLL_PERI_1X },
	},
	.max_lanes = 4,
	.max_tx_bytes = 256,
	.has_second_bank = 1,
	.inst_loop_count = 50,
	.trans_start = 10,
	.burst_sync_point = 40,
	.slave_tri_delay = 48,
	.video_start_delay = 1,
	.ecc_crc_en = 1,
};
