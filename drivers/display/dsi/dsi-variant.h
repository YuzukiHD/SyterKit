/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __DSI_VARIANT_H__
#define __DSI_VARIANT_H__

#include <stdint.h>

#include <drivers/display/display.h>

/*
 * Values that differ between DSI hosts of different SoCs, selected by the
 * "allwinner,variant" property of the node.
 */
struct sunxi_dsi_variant {
	const char *name;
	sunxi_disp_modclk_t modclk; /* module clock layout */
	uint8_t max_lanes;
	uint16_t max_tx_bytes; /* one LP packet (header + payload + crc) */
	uint8_t has_second_bank; /* instruction slots 8..14 */
	uint8_t inst_loop_count; /* stop/delay slot loop count */
	uint8_t trans_start;
	uint8_t burst_sync_point;
	uint8_t slave_tri_delay;
	uint8_t video_start_delay; /* lines of early data request */
	uint8_t ecc_crc_en; /* enable packet ECC / CRC generation */
};

/* chip descriptions live in drivers/display/platform/ */
extern const struct sunxi_dsi_variant sunxi_dsi_variant_sun252iw2;

#endif
