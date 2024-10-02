/* SPDX-License-Identifier: GPL-2.0+ */

/*
* Original from 
* - https://github.com/aodzip/u-boot-2022.10-Allwinner-A133/blob/main/arch/arm/mach-sunxi/dram_libdram_sun50iw10p1.c
*/

#include <barrier.h>
#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <jmp.h>
#include <log.h>

#include <sys-dram.h>
#include <sys-clk.h>
#include <sys-rtc.h>

static uint32_t ccu_set_pll_ddr_clk(uint32_t pll, uint32_t pll_reg, dram_para_v2_t *para) {
    uint32_t reg_val = 0, n, ret;
    n = pll / 24;
    reg_val = read32(pll_reg);
    clrbits_le32(pll_reg, (0xff << 8) | 0x3);
    setbits_le32(pll_reg, ((n - 1) << 8) | (1U << 31));

    /* set ddr sscg with para dram_tpr13 */
    ret = (para->dram_tpr13 >> 20) & 0x7;
    switch (ret) {
        case 0:
            break;
        case 1:
            write32((CCU_BASE + 0x110), (0xccccU | (0x3U << 17) | (0x48U << 20) | (0x3U << 29) | (0x1U << 31)));
            break;
        case 2:
            write32((CCU_BASE + 0x110), (0x9999U | (0x3U << 17) | (0x90U << 20) | (0x3U << 29) | (0x1U << 31)));
            break;
        case 3:
            write32((CCU_BASE + 0x110), (0x6666U | (0x3U << 17) | (0xD8U << 20) | (0x3U << 29) | (0x1U << 31)));
            break;
        case 4:
            write32((CCU_BASE + 0x110), (0x3333U | (0x3 << 17) | (0x120U << 20) | (0x3U << 29) | (0x1U << 31)));
            break;
        case 5:
            write32((CCU_BASE + 0x110), ((0x3U << 17) | (0x158U << 20) | (0x3U << 29) | (0x1U << 31)));
            break;
        default:
            write32((CCU_BASE + 0x110), (0x3333U | (0x3 << 17) | (0x120U << 20) | (0x3U << 29) | (0x1U << 31)));
            break;
    }
    reg_val = readl((CCU_BASE + 0x10));
    reg_val |= (0x1U << 24);
    write32((CCU_BASE + 0x10), reg_val);

    write32(pll_reg, reg_val | (1U << 29) | (1U << 30));
    write32(pll_reg, reg_val | (1U << 29));

    while (!(read32(pll_reg) & 0x1 << 28))
        ;

    return 24 * n;
}

static void mctl_com_set_controller_after_phy(dram_para_v2_t *para) {
}

static uint32_t mctl_phy_init(dram_para_v2_t *para) {
}

void mctl_com_set_controller_config(dram_para_v2_t *para) {
    uint32_t reg_val = 0;

    switch (((para->dram_type) >> 0) & 0xf) {
        case 3:
            reg_val |= (0x1 << 0);
            reg_val |= (0x4 << 16);
            break;
        case 4:
            reg_val |= (0x1 << 4);
            reg_val |= (0x4 << 16);
            break;
        case 7:
            reg_val |= (0x1 << 3);
            reg_val |= (0x4 << 16);
            break;
        case 8:
            reg_val |= (0x1 << 5);
            reg_val |= (0x8 << 16);
            break;
        default:
            reg_val |= (0x1 << 0);
            reg_val |= (0x4 << 16);
            break;
    }
    reg_val |= (((para->dram_para2 & 0x01) ? 0x1 : 0x0) << 12);
    reg_val |= (((((para->dram_para2 >> 12) & 0x03) << 1) + 1) << 24);
    reg_val |= (0x3U << 30);
    write32((MCTL_CTL_BASE + 0x0), reg_val);
}

void mctl_com_set_controller_address_map(dram_para_v2_t *para) {
    uint32_t col, n, row, rank;

    col = ((para->dram_para1 >> 0) & 0xf);
    row = ((para->dram_para1 >> 4) & 0xff);
    rank = ((para->dram_para1 >> 12) & 0x3);
    n = ((para->dram_para1 >> 14) & 0x3);
    col = ((para->dram_para2 & 0xf) == 0) ? col : col - 1;

    write32((MCTL_CTL_BASE + 0x208), (0x0 << 0) | ((n) << 8) | ((n) << 16) | ((n) << 24));
    switch (col) {
        case 8: {
            write32((MCTL_CTL_BASE + 0x20C), ((n) << 0) | ((n) << 8) | ((0x1f) << 16) | ((0x1f) << 24));
            write32((MCTL_CTL_BASE + 0x210), ((0x1f) << 0) | ((0x1f) << 8));
            break;
        }
        case 9: {
            write32((MCTL_CTL_BASE + 0x20C), ((n) << 0) | ((n) << 8) | ((n) << 16) | ((0x1f) << 24));
            write32((MCTL_CTL_BASE + 0x210), ((0x1f) << 0) | ((0x1f) << 8));
            break;
        }
        case 10: {
            write32((MCTL_CTL_BASE + 0x20C), ((n) << 0) | ((n) << 8) | ((n) << 16) | ((n) << 24));
            write32((MCTL_CTL_BASE + 0x210), ((0x1f) << 0) | ((0x1f) << 8));
            break;
        }
        case 11: {
            write32((MCTL_CTL_BASE + 0x20C), ((n) << 0) | ((n) << 8) | ((n) << 16) | ((n) << 24));
            write32((MCTL_CTL_BASE + 0x210), ((n) << 0) | ((0x1f) << 8));
            break;
        }
        default: {
            write32((MCTL_CTL_BASE + 0x20C), ((n) << 0) | ((n) << 8) | ((n) << 16) | ((n) << 24));
            write32((MCTL_CTL_BASE + 0x210), ((n) << 0) | ((n) << 8));
            break;
        }
    }
    write32((MCTL_CTL_BASE + 0x220), (n == 2) ? 0x0101 : ((n == 1) ? 0x3f01 : 0x3f3f));
    if (rank == 3) {
        write32((MCTL_CTL_BASE + 0x204), ((n + col - 2) << 0) | ((n + col - 2) << 8) | ((n + col - 2) << 16));
    } else {
        write32((MCTL_CTL_BASE + 0x204), ((n + col - 2) << 0) | ((n + col - 2) << 8) | (0x3f << 16));
    }
    write32((MCTL_CTL_BASE + 0x214), (((n + col + rank - 6) << 0)) | (((n + col + rank - 6) << 8)) | (((n + col + rank - 6) << 16)) | (((n + col + rank - 6) << 24)));
    switch (row) {
        case 14: {
            write32((MCTL_CTL_BASE + 0x218), (((n + col + rank - 6) << 0)) | (((n + col + rank - 6) << 8)) | (((0xf) << 16)) | (((0xf) << 24)));
            write32((MCTL_CTL_BASE + 0x21C), (((0xf) << 0)) | (((0xf) << 8)));
            break;
        }
        case 15: {
            write32((MCTL_CTL_BASE + 0x218), (((n + col + rank - 6) << 0)) | (((n + col + rank - 6) << 8)) | (((n + col + rank - 6) << 16)) | (((0xf) << 24)));
            write32((MCTL_CTL_BASE + 0x21C), (((0xf) << 0)) | (((0xf) << 8)));
            break;
        }
        case 16: {
            write32((MCTL_CTL_BASE + 0x218), (((n + col + rank - 6) << 0)) | (((n + col + rank - 6) << 8)) | (((n + col + rank - 6) << 16)) | (((n + col + rank - 6) << 24)));
            write32((MCTL_CTL_BASE + 0x21C), (((0xf) << 0)) | (((0xf) << 8)));
            break;
        }
        case 17: {
            write32((MCTL_CTL_BASE + 0x218), (((n + col + rank - 6) << 0)) | (((n + col + rank - 6) << 8)) | (((n + col + rank - 6) << 16)) | (((n + col + rank - 6) << 24)));
            write32((MCTL_CTL_BASE + 0x21C), (((n + col + rank - 6) << 0)) | (((0xf) << 8)));
            break;
        }
        default: {
            write32((MCTL_CTL_BASE + 0x218), (((n + col + rank - 6) << 0)) | (((n + col + rank - 6) << 8)) | (((n + col + rank - 6) << 16)) | (((n + col + rank - 6) << 24)));
            write32((MCTL_CTL_BASE + 0x21C), (((n + col + rank - 6) << 0)) | (((n + col + rank - 6) << 8)));
            break;
        }
    }
    if ((para->dram_para2 >> 12) & 0x1) {
        write32((MCTL_CTL_BASE + 0x200), n + col + rank + row - 6);
    } else {
        write32((MCTL_CTL_BASE + 0x200), 0x1f);
    }
}

static void mctl_com_init(dram_para_v2_t *para) {
    uint32_t reg_val = 0, window_length = 0, time_odt = 0;

    mctl_com_set_controller_config(para);

    /* if is DDR4, set controller geardown mode */
    if (para->dram_type == 4) {
        reg_val = readl(MCTL_CTL_BASE + 0x0);
        setbits_le32(MCTL_CTL_BASE + 0x0, ((para->dram_tpr13 >> 30) & 0x1));
    }
    /* if is DDR3 or DDR4, set controller 2T mode */
    if ((para->dram_type == 3) || (para->dram_type == 4)) {
        reg_val = readl(MCTL_CTL_BASE + 0x0);
        if (((reg_val >> 11) & 0x1) || ((para->dram_tpr13 >> 5) & 0x1)) {
            reg_val &= ~(0x1 << 10);
        } else {
            reg_val |= (0x1 << 10);
        }
        write32(MCTL_CTL_BASE + 0x0, reg_val);
    }


    if (((para->dram_para2 >> 12) & 0x1) == 0x1)
        reg_val = 0x303;
    else
        reg_val = 0x201;
    write32((MCTL_CTL_BASE + 0x244), reg_val);

    /* set odt */
    reg_val = 0x04000400;
    switch (((para->dram_type) >> 0) & 0x7) {
        case 3:
            reg_val &= (~(0xf << 24));
            reg_val |= (0x6 << 24);
            reg_val &= (~(0x1f << 16));
            reg_val |= (0x0 << 16);
            break;
        case 4:
            reg_val &= (~(0xf << 24));
            reg_val |= ((0x5 + (((para->dram_mr4 >> 12) & 0x1) + 1) + 0) << 24);
            reg_val &= (~(0x1f << 16));
            reg_val |= (((para->dram_mr4 >> 6) & 0x7) << 16);
            break;
        case 7:
            reg_val &= (~(0xf << 24));
            time_odt = (int) (7 * (para->dram_clk)) / 2000;
            reg_val |= ((0x7 + time_odt) << 24);
            if ((para->dram_clk) < 400)
                window_length = 3;
            else
                window_length = 4;
            reg_val &= (~(0x1f << 16));
            time_odt = (int) (7 * (para->dram_clk)) / 2000;
            reg_val |= ((window_length - time_odt) << 16);
            break;
        default:
            break;
    }
    write32((MCTL_CTL_BASE + 0x240), reg_val);
    write32((MCTL_CTL_BASE + 0x240) + 0x2000, reg_val);
    write32((MCTL_CTL_BASE + 0x240) + 0x3000, reg_val);
    write32((MCTL_CTL_BASE + 0x240) + 0x4000, reg_val);
}

static void mctl_com_set_bus_config(dram_para_v2_t *para) {
    uint32_t reg_val = 0;

    if (para->dram_type == 8) {
        reg_val = readl(0x03100000 + 0x200 * 23 + 0xA8);
        setbits_le32(0x03100000 + 0x200 * 23 + 0xA8, (0x1 << 0));
    }

    reg_val = readl(MCTL_CTL_BASE + 0x250);
    clrbits_le32(MCTL_CTL_BASE + 0x250, (0xff << 8));
    setbits_le32(MCTL_CTL_BASE + 0x250, (0x30 << 8));
}

static uint32_t dram_channel_init(dram_para_v2_t *para) {
    uint32_t ret_val = 0;
    clrbits_le32(MCTL_COM_BASE + 0x08, (0x1 << 24));
    setbits_le32(MCTL_COM_BASE + 0x08, (0x1 << 25) | (0x1 << 9));
    setbits_le32(MCTL_COM_BASE + 0x20, (0x1 << 15));
    mctl_com_set_bus_config(para);
    write32((MCTL_CTL_BASE + 0x38), 0x0);
    mctl_com_init(para);
    ret_val = mctl_phy_init(para);
    mctl_com_set_controller_after_phy(para);
    return ret_val;
}

static void mctl_sys_init(dram_para_v2_t *para) {
    clrbits_le32((CCU_BASE + 0x540), (1U << 31) | (1U << 30));
    clrbits_le32((CCU_BASE + 0x80C), (1U << 0) | (1U << 16));
    clrbits_le32((CCU_BASE + 0x10), (1U << 31));
    clrbits_le32((CCU_BASE + 0x800), (1U << 30));
    udelay(5);
    ccu_set_pll_ddr_clk((para->dram_clk << 1), (CCU_BASE + 0x10), para);
    clrbits_le32((CCU_BASE + 0x800), (0x3 << 24));
    setbits_le32((CCU_BASE + 0x800), (0x3 << 0));
    setbits_le32((CCU_BASE + 0x800), (1U << 27));
    setbits_le32((CCU_BASE + 0x80C), (1U << 16));
    setbits_le32((CCU_BASE + 0x80C), (1U << 0));
    setbits_le32((CCU_BASE + 0x540), (1U << 30));
    setbits_le32((CCU_BASE + 0x540), (1U << 31));
    clrbits_le32((MCTL_COM_BASE + 0x08), (0x1 << 25));
    setbits_le32((CCU_BASE + 0x800), (1U << 30));
    udelay(5);
    setbits_le32((0x07010000 + 0x250), (1U << 4));
}

static uint32_t dram_core_init(dram_para_v2_t *para) {
    uint32_t ret_val = 0;
    dram_sys_init(para);
    ret_val = dram_channel_init(para);
    return ret_val;
}

uint64_t sunxi_dram_init(void *param) {
    dram_para_v2_t *para = (dram_para_v2_t *) param;

    return 0;
}