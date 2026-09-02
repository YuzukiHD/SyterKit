/* SPDX-License-Identifier: GPL-2.0+ */

/*
 * Application-owned adapters for the device-tree helpers.
 *
 * The parsers are intentionally static inline because normal C applications
 * include them directly. Rust needs linkable symbols, so this small C unit is
 * built only with app-rs and forwards to the existing parsers.
 */

#include <drivers/mtd/spif-nor.h>
#include <drivers/psram/psram.h>
#include <drivers/spif/spif.h>
#include <dt-compatible/psram-dt.h>
#include <dt-compatible/spif-dt.h>
#include <dt-compatible/spif-nor-dt.h>

int sunxi_psram_dt_read_alias_ffi(sunxi_psram_t *psram, const char *alias)
{
	return sunxi_psram_dt_read_alias(psram, alias);
}

int sunxi_spif_dt_read_alias_ffi(sunxi_spif_t *spif, const char *alias)
{
	return sunxi_spif_dt_read_alias(spif, alias);
}

int spif_nor_dt_read_alias_ffi(spif_nor_t *nor, const char *alias, sunxi_spif_t *spif)
{
	return spif_nor_dt_read_alias(nor, alias, spif);
}
