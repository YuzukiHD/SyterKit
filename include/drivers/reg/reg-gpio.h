/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __REG_GPIO_H__
#define __REG_GPIO_H__

#if defined(CONFIG_CHIP_GPIO_V1)
enum {
    GPIO_CFG0 = 0x00,
    GPIO_CFG1 = 0x04,
    GPIO_CFG2 = 0x08,
    GPIO_CFG3 = 0x0c,
    GPIO_DAT = 0x10,
    GPIO_DRV0 = 0x14,
    GPIO_DRV1 = 0x18,
    GPIO_PUL0 = 0x1c,
    GPIO_PUL1 = 0x20,
    GPIO_OFFSET = 0x24,
    GPIO_CFG_MASK = 0x7,
    GPIO_DRV_MASK = 0x3,
};
#elif defined(CONFIG_CHIP_GPIO_V2)
enum {
    GPIO_CFG0 = 0x00,
    GPIO_CFG1 = 0x04,
    GPIO_CFG2 = 0x08,
    GPIO_CFG3 = 0x0c,
    GPIO_DAT = 0x10,
    GPIO_DRV0 = 0x14,
    GPIO_DRV1 = 0x18,
    GPIO_DRV2 = 0x1c,
    GPIO_DRV3 = 0x20,
    GPIO_PUL0 = 0x24,
    GPIO_PUL1 = 0x28,
    GPIO_OFFSET = 0x30,
    GPIO_CFG_MASK = 0xf,
    GPIO_DRV_MASK = 0x3,
};
#elif defined(CONFIG_CHIP_GPIO_V3)
enum {
    GPIO_CFG0 = 0x80,
    GPIO_CFG1 = 0x84,
    GPIO_CFG2 = 0x88,
    GPIO_CFG3 = 0x8c,
    GPIO_DAT = 0x90,
    GPIO_DAT_SET = 0x94,
    GPIO_DAT_CLR = 0x98,
    GPIO_DRV0 = 0xa0,
    GPIO_DRV1 = 0xa4,
    GPIO_DRV2 = 0xa8,
    GPIO_DRV3 = 0xac,
    GPIO_PUL0 = 0xb0,
    GPIO_PUL1 = 0xb4,
    GPIO_OFFSET = 0x80,
    GPIO_CFG_MASK = 0xf,
    GPIO_DRV_MASK = 0x3,
};
#else /* Dafault GPIO V2 */
enum {
    GPIO_CFG0 = 0x00,
    GPIO_CFG1 = 0x04,
    GPIO_CFG2 = 0x08,
    GPIO_CFG3 = 0x0c,
    GPIO_DAT = 0x10,
    GPIO_DRV0 = 0x14,
    GPIO_DRV1 = 0x18,
    GPIO_DRV2 = 0x1c,
    GPIO_DRV3 = 0x20,
    GPIO_PUL0 = 0x24,
    GPIO_PUL1 = 0x28,
    GPIO_OFFSET = 0x30,
    GPIO_CFG_MASK = 0xf,
    GPIO_DRV_MASK = 0x3,
};
#endif

#endif // __REG_GPIO_H__