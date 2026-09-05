/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file hifi4-reg.h
 * @brief HiFi4 DSP control register and bit definitions.
 *
 * Defines the default reset vector, DSP control register offsets and the
 * SRAM remap control bits shared by the Allwinner HiFi4 remoteproc drivers.
 */

#ifndef __SUNXI_HIFI4_REG_H__
#define __SUNXI_HIFI4_REG_H__

#define DSP_DEFAULT_RST_VEC 0x100000U

#define DSP_ALT_RESET_VEC_REG 0x0000U
#define DSP_CTRL_REG0 0x0004U

#define BIT_RUN_STALL 0U
#define BIT_START_VEC_SEL 1U
#define BIT_DSP_CLKEN 2U

#define BIT_SRAM_REMAP_ENABLE 0U
#define SRAMC_SRAM_REMAP_REG 0x8U

#endif /* __SUNXI_HIFI4_REG_H__ */
