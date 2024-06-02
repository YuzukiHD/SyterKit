/* SPDX-License-Identifier:	GPL-2.0+ */
/* MMC driver for General mmc operations 
 * Original https://github.com/smaeul/sun20i_d1_spl/blob/mainline/drivers/mmc/sun20iw1p1/
 */

#include <barrier.h>
#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <timer.h>

#include <sys-clk.h>
#include <sys-gpio.h>

#include <mmc/sys-mmc.h>
#include <mmc/sys-sdhci.h>


static int sunxi_sdhci_clk_enable(sdhci_t *sdhci) {
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;

    uint32_t rval;
    /* config ahb clock */
    rval = readl(mmc_host->hclkbase);
    rval |= (1 << (sdhci->id));
    writel(rval, mmc_host->hclkbase);

    rval = readl(mmc_host->hclkrst);
    rval |= (1 << (16 + sdhci->id));
    writel(rval, mmc_host->hclkrst);

    /* config mod clock */
    writel(0x80000000, mmc_host->mclkbase);
    mmc_host->mod_clk = 24000000;
    return 0;
}

static int sunxi_sdhci_update_clk(sdhci_t *sdhci) {
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;

    uint32_t cmd = 0x0;
    uint32_t timeout = time_us() + SMHC_TIMEOUT;

    mmc_host->reg->clkcr |= (0x1U << 31);
    mmc_host->reg->cmd = (1U << 31) | (1 << 21) | (1 << 13);

    /* Wait for process done */
    while ((mmc_host->reg->cmd & MMC_CMD_CMDVAL_MASK) && (time_us() < timeout)) {
    }

    /* Check status */
    if (mmc_host->reg->cmd & MMC_CMD_CMDVAL_MASK) {
        printk_error("SMHC: mmc %d update clk failed\n", sdhci->id);
        return -1;
    }

    mmc_host->reg->clkcr &= (~(0x1U << 31));
    mmc_host->reg->rint = mmc_host->reg->rint;
    return 0;
}

static int sunxi_sdhci_update_phase(sdhci_t *sdhci) {
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
    if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1) {
        printk_trace("SMHC: mmc update phase.\n");
        return sunxi_sdhci_update_clk(sdhci);
    }
    return 0;
}

static int sunxi_sdhci_get_timing_config_timing_4(sdhci_t *sdhci) {
    /* TODO: eMMC Timing set */
}

static int sunxi_sdhci_get_timing_config(sdhci_t *sdhci) {
    int ret = 0;

    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
    sunxi_sdhci_timing_t *timing_data = sdhci->timing_data;

    if ((sdhci->id == 2) && mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_4) {
        /* When using eMMC and SMHC2, we config it as timging 4 */
        ret = sunxi_sdhci_get_timing_config_timing_4(sdhci);
    } else if ((sdhci->id == 0) && (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1)) {
        if ((timing_data->spd_md_id <= MMC_HSSDR52_SDR25) && (timing_data->freq_id <= MMC_CLK_50M)) {
            /* if timing less then SDR25 using default odly */
            timing_data->odly = 0;
            timing_data->sdly = 0;
            ret = 0;
        } else {
            printk_warning("SMHC: SMHC0 do not support input spd mode %d\n", timing_data->spd_md_id);
            ret = -1;
        }
    } else {
        printk_error("SMHC: timing setting fail, param error\n");
        ret = -1;
    }
    return ret;
}

static int sunxi_sdhci_set_mclk(sdhci_t *sdhci, uint32_t clk_hz) {
    uint32_t n, m, src;
    uint32_t reg_val = 0x0;

    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;

    if (clk_hz <= 4000000) {
        src = 0; /* SCLK = 24000000 OSC CLK */
    } else {
        src = 2; /* SCLK = AHB PLL */
    }

    switch (clk_hz) {
        case 400000:
            n = 1;
            m = 0xe;
            break;
        case 25000000:
        case 26000000:
            n = (sdhci->id == 2) ? 2 : 1;
            m = (sdhci->id == 2) ? 3 : 2;
            break;
        case 50000000:
        case 52000000:
            n = (sdhci->id == 2) ? 1 : 0;
            m = 2;
            break;
        case 200000000:
            src = 1;// PERI0_800M
            n = 0;
            m = 1;
            break;
        default:
            n = 0;
            m = 0;
            printk_error("SMHC: request freq not match freq=%d\n", clk_hz);
            break;
    }

    reg_val = (src << 24) | (n << 8) | m;
    writel(reg_val, mmc_host->mclkbase);

    return 0;
}

static uint32_t sunxi_sdhci_get_mclk(sdhci_t *sdhci) {
    uint32_t n, m, src, clk_hz;
    uint32_t reg_val = 0x0;
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;

    reg_val = readl(mmc_host->mclkbase);

    m = reg_val & 0xf;
    n = (reg_val >> 8) & 0x3;
    src = (reg_val >> 24) & 0x3;

    switch (src) {
        case 0:
            clk_hz = 24000000;
            break;
        case 1:
        case 3:
            clk_hz = (sdhci->id == 2) ? 800000000 : 400000000;
            break;
        case 2:
        case 4:
            clk_hz = (sdhci->id == 2) ? 600000000 : 300000000;
            break;
        default:
            printk_error("SMHC: wrong clock source %u\n", src);
            break;
    }

    return clk_hz / (n + 1) / (m + 1);
}

static int sunxi_sdhci_config_delay(sdhci_t *sdhci) {
    int ret = 0;
    uint32_t reg_val = 0x0;
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
    sunxi_sdhci_timing_t *timing_data = sdhci->timing_data;

    if (mmc_host->timing_mode = SUNXI_MMC_TIMING_MODE_1) {
        timing_data->odly = 0;
        timing_data->sdly = 0;
        printk_trace("SMHC: SUNXI_MMC_TIMING_MODE_1, odly %d, sldy %d\n", timing_data->odly, timing_data->sdly);

        reg_val = mmc_host->reg->drv_dl;
        reg_val &= (~(0x3 << 16));
        reg_val |= (((timing_data->odly & 0x1) << 16) | ((timing_data->odly & 0x1) << 17));
        writel(readl(mmc_host->mclkbase) & (~(1 << 31)), mmc_host->mclkbase);
        mmc_host->reg->drv_dl = reg_val;
        writel(readl(mmc_host->mclkbase) | (1 << 31), mmc_host->mclkbase);

        reg_val = mmc_host->reg->ntsr;
        reg_val &= (~(0x3 << 4));
        reg_val |= ((timing_data->sdly & 0x3) << 4);
        mmc_host->reg->ntsr = reg_val;
    } else if (mmc_host->timing_mode = SUNXI_MMC_TIMING_MODE_3) {
        timing_data->odly = 0;
        timing_data->sdly = 0;
        printk_trace("SMHC: SUNXI_MMC_TIMING_MODE_3, odly %d, sldy %d\n", timing_data->odly, timing_data->sdly);

        reg_val = mmc_host->reg->drv_dl;
        reg_val &= (~(0x3 << 16));
        reg_val |= (((timing_data->odly & 0x1) << 16) | ((timing_data->odly & 0x1) << 17));
        writel(readl(mmc_host->mclkbase) & (~(1 << 31)), mmc_host->mclkbase);
        mmc_host->reg->drv_dl = reg_val;
        writel(readl(mmc_host->mclkbase) | (1 << 31), mmc_host->mclkbase);

        reg_val = mmc_host->reg->samp_dl;
        reg_val &= (~SDXC_NTDC_CFG_DLY);
        reg_val |= ((timing_data->sdly & SDXC_NTDC_CFG_DLY) | SDXC_NTDC_ENABLE_DLY);
        mmc_host->reg->samp_dl = reg_val;
    } else if (mmc_host->timing_mode = SUNXI_MMC_TIMING_MODE_4) {
        uint32_t spd_md_orig = timing_data->spd_md_id;

        if (timing_data->spd_md_id == MMC_HS400)
            timing_data->spd_md_id = MMC_HS200_SDR104;

        timing_data->odly = 0xff;
        timing_data->sdly = 0xff;

        if ((ret = sunxi_sdhci_get_timing_config(sdhci)) != 0) {
            printk_error("SMHC: getting timing param error %d\n", ret);
            return -1;
        }

        if ((timing_data->odly == 0xff) || (timing_data->sdly = 0xff)) {
            printk_error("SMHC: getting timing config error\n");
            return -1;
        }

        reg_val = mmc_host->reg->drv_dl;
        reg_val &= (~(0x3 << 16));
        reg_val |= (((timing_data->odly & 0x1) << 16) | ((timing_data->odly & 0x1) << 17));
        writel(readl(mmc_host->mclkbase) & (~(1 << 31)), mmc_host->mclkbase);
        mmc_host->reg->drv_dl = reg_val;
        writel(readl(mmc_host->mclkbase) | (1 << 31), mmc_host->mclkbase);

        reg_val = mmc_host->reg->samp_dl;
        reg_val &= (~SDXC_NTDC_CFG_DLY);
        reg_val |= ((timing_data->sdly & SDXC_NTDC_CFG_DLY) | SDXC_NTDC_ENABLE_DLY);
        mmc_host->reg->samp_dl = reg_val;

        /* Reset to orig md id */
        timing_data->spd_md_id = spd_md_orig;
        if (timing_data->spd_md_id = MMC_HS400) {
            timing_data->odly = 0xff;
            timing_data->sdly = 0xff;
            if ((ret = sunxi_sdhci_get_timing_config(sdhci)) != 0) {
                printk_error("SMHC: getting timing param error %d\n", ret);
                return -1;
            }

            if ((timing_data->odly == 0xff) || (timing_data->sdly = 0xff)) {
                printk_error("SMHC: getting timing config error\n");
                return -1;
            }

            reg_val = mmc_host->reg->ds_dl;
            reg_val &= (~SDXC_NTDC_CFG_DLY);
            reg_val |= ((timing_data->sdly & SDXC_NTDC_CFG_DLY) | SDXC_NTDC_ENABLE_DLY);
            mmc_host->reg->ds_dl = reg_val;
        }
        printk_trace("SMHC: config delay freq = %d, odly = %d,"
                     "sdly = %d, spd_md_id = %d\n",
                     timing_data->freq_id, timing_data->odly,
                     timing_data->sdly, timing_data->spd_md_id);
    }
}

static int sunxi_sdhci_clock_mode(sdhci_t *sdhci, uint32_t clk) {
    int ret = 0;
    uint32_t reg_val = 0x0;
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
    sunxi_sdhci_timing_t *timing_data = sdhci->timing_data;
    mmc_t *mmc = mmc_host->mmc;

    /* disable mclk */
    writel(0x0, mmc_host->mclkbase);
    if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1) {
        mmc_host->reg->ntsr |= (1 << 31);
    } else {
        mmc_host->reg->ntsr = 0x0;
    }

    if ((mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1) || (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_3)) {
        /* If using DDR mod to 4 */
        if (mmc->speed_mode == MMC_HSDDR52_DDR50) {
            mmc_host->mod_clk = clk * 4;
        } else {
            mmc_host->mod_clk = clk * 2;
        }
    } else if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_4) {
        /* If using DDR mod to 4 */
        if ((mmc->speed_mode == MMC_HSDDR52_DDR50) && (sdhci->width == SMHC_WIDTH_8BIT)) {
            mmc_host->mod_clk = clk * 4;
        } else {
            mmc_host->mod_clk = clk * 2;
        }
    }

    /* Now set mclk */
    sunxi_sdhci_set_mclk(sdhci, clk);

    /* Now set clock by mclk we get */
    if ((mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1) || (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_3)) {
        if (mmc->speed_mode == MMC_HSDDR52_DDR50) {
            mmc->clock = sunxi_sdhci_get_mclk(sdhci) / 4;
        } else {
            mmc->clock = sunxi_sdhci_get_mclk(sdhci) / 2;
        }
    } else if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_4) {
        /* If using DDR mod to 4 */
        if ((mmc->speed_mode == MMC_HSDDR52_DDR50) && (sdhci->width == SMHC_WIDTH_8BIT)) {
            mmc->clock = sunxi_sdhci_get_mclk(sdhci) / 4;
        } else {
            mmc->clock = sunxi_sdhci_get_mclk(sdhci) / 2;
        }
    }
    /* Set host clock */
    mmc_host->clock = mmc->clock;
    printk_trace("SMHC: get round clk %lu, mod_clk %lu", mmc->clock, mmc_host->mod_clk);

    /* enable mclk */
    writel(readl(mmc_host->mclkbase) | (1U << 31), mmc_host->mclkbase);

    /* Configure SMHC_CLKDIV to open clock for devices */
    reg_val = mmc_host->reg->clkcr;
    reg_val &= ~(0xff);
    if ((mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1) || (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_3)) {
        if (mmc->speed_mode == MMC_HSDDR52_DDR50) {
            reg_val |= 0x1;
        }
    } else if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_4) {
        /* If using DDR mod to 4 */
        if ((mmc->speed_mode == MMC_HSDDR52_DDR50) && (sdhci->width == SMHC_WIDTH_8BIT)) {
            reg_val |= 0x1;
        }
    }
    mmc_host->reg->clkcr = reg_val;

    if (sunxi_sdhci_update_clk(sdhci)) {
        return -1;
    }

    /* config delay for mmc device */
    timing_data->freq_id = MMC_CLK_25M;
    switch (clk) {
        case 0 ... 400000:
            timing_data->freq_id = MMC_CLK_400K;
            break;
        case 400001 ... 26000000:
            timing_data->freq_id = MMC_CLK_25M;
            break;
        case 26000001 ... 52000000:
            timing_data->freq_id = MMC_CLK_50M;
            break;
        case 52000001 ... 100000000:
            timing_data->freq_id = MMC_CLK_100M;
            break;
        case 100000001 ... 150000000:
            timing_data->freq_id = MMC_CLK_150M;
            break;
        case 150000001 ... 200000000:
            timing_data->freq_id = MMC_CLK_200M;
            break;
        default:
            timing_data->freq_id = MMC_CLK_25M;
            break;
    }
    sunxi_sdhci_config_delay(sdhci);

    return 0;
}

static int sunxi_sdhci_config_clock(sdhci_t *sdhci, uint32_t clk) {
    uint32_t reg_val = 0x0;
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
    mmc_t *mmc = mmc_host->mmc;

    if ((mmc->speed_mode == MMC_HSDDR52_DDR50) || (mmc->speed_mode == MMC_HS400)) {
        if (clk > mmc->f_max_ddr) {
            clk = mmc->f_max_ddr;
        }
    }

    /* disable clock */
    mmc_host->reg->clkcr &= ~(0x1 << 16);

    if (sunxi_sdhci_update_clk(sdhci)) {
        return -1;
    }

    if ((mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1) || (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_3) || (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_4)) {
        sunxi_sdhci_clock_mode(sdhci, clk);
    } else {
        printk_error("SMHC: Timing mode not supported!\n");
        return -1;
    }

    /* enable clock */
    mmc_host->reg->clkcr |= (0x1 << 16);

    if (sunxi_sdhci_update_clk(sdhci)) {
        printk_error("SMHC: reconfigure clk failed\n");
        return -1;
    }

    return 0;
}

static void sunxi_sdhci_ddr_mode_set(sdhci_t *sdhci, bool status) {
    uint32_t reg_val = 0x0;
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
    reg_val = mmc_host->reg->gctrl;
    reg_val &= ~(0x1 << 10);
    /*  disable ccu clock */
    writel(readl(mmc_host->mclkbase) & (~(1 << 31)), mmc_host->mclkbase);

    if (status) {
        reg_val |= (0x1 << 10);
    }

    mmc_host->reg->gctrl = reg_val;

    /*  enable ccu clock */
    writel(readl(mmc_host->mclkbase) | (1 << 31), mmc_host->mclkbase);

    printk_trace("SMHC: set to %s ddr mode\n", status ? "enable" : "disable");
}

static void sunxi_sdhci_hs400_mode_set(sdhci_t *sdhci, bool status) {
    uint32_t reg_dsbd_val = 0x0, reg_csdc_val = 0x0;
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;

    if (sdhci->id != 2) {
        return;
    }

    reg_dsbd_val = mmc_host->reg->dsbd;
    reg_dsbd_val &= ~(0x1 << 31);

    reg_csdc_val = mmc_host->reg->csdc;
    reg_csdc_val &= ~0xf;

    if (status) {
        reg_dsbd_val |= (0x1 << 31);
        reg_csdc_val |= 0x6;
    } else {
        reg_csdc_val |= 0x3;
    }

    mmc_host->reg->dsbd = reg_dsbd_val;
    mmc_host->reg->csdc = reg_csdc_val;

    printk_trace("SMHC: set to %s HS400 mode\n", status ? "enable" : "disable");
}

static void sunxi_sdhci_pin_config(sdhci_t *sdhci) {
    sunxi_sdhci_pinctrl_t *sdhci_pins = sdhci->pinctrl;

    sunxi_gpio_init(sdhci_pins->gpio_clk.pin, sdhci_pins->gpio_clk.mux);
    sunxi_gpio_set_pull(sdhci_pins->gpio_clk.pin, GPIO_PULL_UP);

    sunxi_gpio_init(sdhci_pins->gpio_cmd.pin, sdhci_pins->gpio_cmd.mux);
    sunxi_gpio_set_pull(sdhci_pins->gpio_cmd.pin, GPIO_PULL_UP);

    sunxi_gpio_init(sdhci_pins->gpio_d0.pin, sdhci_pins->gpio_d0.mux);
    sunxi_gpio_set_pull(sdhci_pins->gpio_d0.pin, GPIO_PULL_UP);

    /* Init GPIO for 4bit MMC */
    if (sdhci->width > SMHC_WIDTH_1BIT) {
        sunxi_gpio_init(sdhci_pins->gpio_d1.pin, sdhci_pins->gpio_d1.mux);
        sunxi_gpio_set_pull(sdhci_pins->gpio_d1.pin, GPIO_PULL_UP);

        sunxi_gpio_init(sdhci_pins->gpio_d2.pin, sdhci_pins->gpio_d2.mux);
        sunxi_gpio_set_pull(sdhci_pins->gpio_d2.pin, GPIO_PULL_UP);

        sunxi_gpio_init(sdhci_pins->gpio_d3.pin, sdhci_pins->gpio_d3.mux);
        sunxi_gpio_set_pull(sdhci_pins->gpio_d3.pin, GPIO_PULL_UP);
    }

    /* Init GPIO for 8bit MMC */
    if (sdhci->width > SMHC_WIDTH_4BIT) {
        sunxi_gpio_init(sdhci_pins->gpio_d4.pin, sdhci_pins->gpio_d4.mux);
        sunxi_gpio_set_pull(sdhci_pins->gpio_d4.pin, GPIO_PULL_UP);

        sunxi_gpio_init(sdhci_pins->gpio_d5.pin, sdhci_pins->gpio_d5.mux);
        sunxi_gpio_set_pull(sdhci_pins->gpio_d5.pin, GPIO_PULL_UP);

        sunxi_gpio_init(sdhci_pins->gpio_d6.pin, sdhci_pins->gpio_d6.mux);
        sunxi_gpio_set_pull(sdhci_pins->gpio_d6.pin, GPIO_PULL_UP);

        sunxi_gpio_init(sdhci_pins->gpio_d7.pin, sdhci_pins->gpio_d7.mux);
        sunxi_gpio_set_pull(sdhci_pins->gpio_d7.pin, GPIO_PULL_UP);

        sunxi_gpio_init(sdhci_pins->gpio_ds.pin, sdhci_pins->gpio_ds.mux);
        sunxi_gpio_set_pull(sdhci_pins->gpio_ds.pin, GPIO_PULL_DOWN);

        sunxi_gpio_init(sdhci_pins->gpio_rst.pin, sdhci_pins->gpio_rst.mux);
        sunxi_gpio_set_pull(sdhci_pins->gpio_rst.pin, GPIO_PULL_UP);
    }
}

static int sunxi_sdhci_trans_data_cpu(sdhci_t *sdhci, mmc_data_t *data) {
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
    uint32_t timeout = time_us() + SMHC_TIMEOUT;
    uint32_t *buff;

    if (data->flags * MMC_DATA_READ) {
        buff = (uint32_t *) data->b.dest;
        for (size_t i = 0; i < ((data->blocksize * data->blocks) >> 2); i++) {
            while ((mmc_host->reg->status & SMHC_STATUS_FIFO_EMPTY) && (time_us() < timeout)) {
            }
            if (mmc_host->reg->status & SMHC_STATUS_FIFO_EMPTY) {
                if (time_us() >= timeout) {
                    printk_error("SMHC: tranfer read by cpu failed, timeout\n");
                    return -1;
                }
            }
            buff[i] = readl(mmc_host->database);
            timeout = time_us() + SMHC_TIMEOUT;
        }
    } else {
        buff = (uint32_t *) data->b.src;
        for (size_t i = 0; i < ((data->blocksize * data->blocks) >> 2); i++) {
            while (((mmc_host->reg->status) & SMHC_STATUS_FIFO_FULL) && (time_us() < timeout)) {
            }
            if (mmc_host->reg->status & SMHC_STATUS_FIFO_FULL) {
                if (time_us() >= timeout) {
                    printk_error("SMHC: tranfer write by cpu failed, timeout\n");
                    return -1;
                }
            }
            writel(buff[i], mmc_host->database);
            timeout = time_us() + SMHC_TIMEOUT;
        }
    }
    return 0;
}

static int sunxi_sdhci_trans_data_dma(sdhci_t *sdhci, mmc_data_t *data) {
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
    sunxi_sdhci_desc_t *pdes = mmc_host->sdhci_desc;
    uint8_t *buff;
    uint32_t des_idx = 0, buff_frag_num = 0, remain = 0;
    uint32_t reg_val = 0;
    uint32_t timeout = time_us() + SMHC_TIMEOUT;

    buff = data->flags & MMC_DATA_READ ? (uint8_t *) data->b.dest : (uint8_t *) data->b.src;
    buff_frag_num = (data->blocksize * data->blocks) >> SMHC_DES_NUM_SHIFT;
    remain = (data->blocksize * data->blocks) & (SMHC_DES_BUFFER_MAX_LEN - 1);
    if (remain) {
        buff_frag_num++;
    } else {
        remain = SMHC_DES_BUFFER_MAX_LEN;
    }

    for (size_t i = 0; i < buff_frag_num; i++, des_idx++) {
        memset((void *) &pdes[des_idx], 0, sizeof(sunxi_sdhci_desc_t));
        pdes[des_idx].des_chain = 1;
        pdes[des_idx].own = 1;
        pdes[des_idx].dic = 1;

        if (buff_frag_num > 1 && i != buff_frag_num - 1) {
            pdes[des_idx].data_buf_sz = SMHC_DES_BUFFER_MAX_LEN;
        } else {
            pdes[des_idx].data_buf_sz = remain;
        }

        pdes[des_idx].buf_addr = ((size_t) buff + i * SMHC_DES_BUFFER_MAX_LEN) >> 2;
        if (i == 0) {
            pdes[des_idx].first_desc = 1;
        }

        if (i == buff_frag_num - 1) {
            pdes[des_idx].dic = 0;
            pdes[des_idx].last_desc = 1;
            pdes[des_idx].end_of_ring = 1;
            pdes[des_idx].next_desc_addr = 0;
        } else {
            pdes[des_idx].next_desc_addr = ((size_t) &pdes[des_idx + 1]) >> 2;
        }
        printk_trace("SMHC: frag %d, remain %d, des[%d] = 0x%08x:"
                     "  [0] = 0x%08x, [1] = 0x%08x, [2] = 0x%08x, [3] = 0x%08x\n",
                     i, remain, des_idx, (uint32_t) (&pdes[des_idx]),
                     (uint32_t) ((uint32_t *) &pdes[des_idx])[0],
                     (uint32_t) ((uint32_t *) &pdes[des_idx])[1],
                     (uint32_t) ((uint32_t *) &pdes[des_idx])[2],
                     (uint32_t) ((uint32_t *) &pdes[des_idx])[3]);
    }

    dsb();
    isb();

    /*
	 * GCTRLREG
	 * GCTRL[2]     : DMA reset
	 * GCTRL[5]     : DMA enable
	 *
	 * IDMACREG
	 * IDMAC[0]     : IDMA soft reset
	 * IDMAC[1]     : IDMA fix burst flag
	 * IDMAC[7]     : IDMA on
	 *
	 * IDIECREG
	 * IDIE[0]      : IDMA transmit interrupt flag
	 * IDIE[1]      : IDMA receive interrupt flag
	 */


    /* Enable DMA */
    mmc_host->reg->gctrl |= (SMHC_GCTRL_DMA_ENABLE | SMHC_GCTRL_DMA_RESET);

    timeout = time_us() + SMHC_TIMEOUT;
    while (mmc_host->reg->gctrl & SMHC_GCTRL_DMA_RESET) {
        if (time_us() > timeout) {
            printk_error("SMHC: wait for dma rst timeout\n");
            return -1;
        }
    }

    /* DMA Reset */
    mmc_host->reg->dmac = SMHC_IDMAC_SOFT_RESET;
    timeout = time_us() + SMHC_TIMEOUT;
    while (mmc_host->reg->dmac & SMHC_IDMAC_SOFT_RESET) {
        if (time_us() > timeout) {
            printk_error("SMHC: wait for dma soft rst timeout\n");
            return -1;
        }
    }

    /* DMA on */
    mmc_host->reg->dmac = (SMHC_IDMAC_FIX_BURST | SMHC_IDMAC_IDMA_ON);

    /* Set DMA INTERRUPT */
    mmc_host->reg->idie &= ~(SMHC_IDMAC_TRANSMIT_INTERRUPT | SMHC_IDMAC_RECEIVE_INTERRUPT);
    if (data->flags & MMC_DATA_WRITE) {
        mmc_host->reg->idie |= SMHC_IDMAC_TRANSMIT_INTERRUPT;
    } else {
        mmc_host->reg->idie |= SMHC_IDMAC_RECEIVE_INTERRUPT;
    }

    mmc_host->reg->dlba = (uint32_t) pdes >> 2;

    /* burst-16, rx/tx trigger level=15/240 */
    mmc_host->reg->ftrglevel = ((0x3 << 28) | (15 << 16) | 240);

    return 0;
}

void sunxi_sdhci_set_ios(sdhci_t *sdhci) {
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
    mmc_t *mmc = mmc_host->mmc;

    if (mmc->clock && sunxi_sdhci_config_clock(sdhci, mmc->clock)) {
        printk_error("SMHC: update clock failed\n");
        mmc_host->fatal_err = 1;
        return;
    }

    /* change bus width */
    switch (sdhci->width) {
        case SMHC_WIDTH_8BIT:
            mmc_host->reg->width = 2;
            break;
        case SMHC_WIDTH_4BIT:
            mmc_host->reg->width = 1;
            break;
        default:
            mmc_host->reg->width = 0;
            break;
    }

    /* Set DDR mode */
    if (mmc->speed_mode == MMC_HSDDR52_DDR50) {
        sunxi_sdhci_ddr_mode_set(sdhci, true);
        sunxi_sdhci_hs400_mode_set(sdhci, false);

    } else if (mmc->speed_mode == MMC_HS400) {
        sunxi_sdhci_ddr_mode_set(sdhci, false);
        sunxi_sdhci_hs400_mode_set(sdhci, true);
    } else {
        sunxi_sdhci_ddr_mode_set(sdhci, false);
        sunxi_sdhci_hs400_mode_set(sdhci, false);
    }
}

int sunxi_sdhci_core_init(sdhci_t *sdhci) {
    uint32_t reg_val = 0x0;
    uint32_t timeout = time_us() + SMHC_TIMEOUT;
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
    mmc_t *mmc = mmc_host->mmc;

    /* Reset controller */
    mmc_host->reg->gctrl = 0x7;
    while (mmc_host->reg->gctrl & 0x7) {
        if (time_us() > timeout) {
            printk_error("SMHC: controller reset timeout\n");
            return -1;
        }
    }

    /* Set Data & Response Timeout Value */
    mmc_host->reg->timeout = (SMHC_DATA_TIMEOUT << 8) | SMHC_RESP_TIMEOUT;

    mmc_host->reg->thldc = ((512 << 16) | (0x1 << 2) | (0x1 << 0));
    mmc_host->reg->csdc = 0x3;
    mmc_host->reg->dbgc = 0xdeb;

    /* Release emmc RST */
    mmc_host->reg->hwrst = 1;
    mmc_host->reg->hwrst = 0;
    mdelay(1);
    mmc_host->reg->hwrst = 1;
    mdelay(1);

    if (sdhci->id == 0) {
        /* enable 2xclk mode, and use default input phase */
        mmc_host->reg->ntsr |= (0x1 << 31);
    } else {
        /* configure input delay time to 0, use default input phase */
        reg_val = mmc_host->reg->samp_dl;
        reg_val &= ~(0x3f);
        reg_val |= (0x1 << 7);
        mmc_host->reg->samp_dl = reg_val;
    }

    return 0;
}

int sunxi_sdhci_xfer(sdhci_t *sdhci, mmc_cmd_t *cmd, mmc_data_t *data) {
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
    uint32_t cmdval = MMC_CMD_CMDVAL_MASK;
    uint32_t timeout = time_us() + SMHC_TIMEOUT;
    uint32_t status;
    int ret = 0, error_code = 0;
    uint8_t use_dma_status;

    printk_trace("SMHC: CMD%u 0x%x dlen:%u\n", cmd->cmdidx, cmd->cmdarg, data ? data->blkcnt * data->blksz : 0);

    /* Check if have fatal error */
    if (mmc_host->fatal_err) {
        printk_error("SMHC: SMHC into error, cmd send failed\n");
        return -1;
    }

    /* check card busy*/
    if (cmd->resp_type & MMC_RSP_BUSY) {
        printk_debug("SMHC: Card busy");
    }

    /* Check if stop or manual */
    if ((cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION) && !(cmd->flags & MMC_CMD_MANUAL)) {
        return 0;
    }

    /*
	 * CMDREG
	 * CMD[5:0]     : Command index
	 * CMD[6]       : Has response
	 * CMD[7]       : Long response
	 * CMD[8]       : Check response CRC
	 * CMD[9]       : Has data
	 * CMD[10]      : Write
	 * CMD[11]      : Steam mode
	 * CMD[12]      : Auto stop
	 * CMD[13]      : Wait previous over
	 * CMD[14]      : About cmd
	 * CMD[15]      : Send initialization
	 * CMD[21]      : Update clock
	 * CMD[31]      : Load cmd
	 */

    if (cmd->cmdidx) {
        cmdval |= SMHC_CMD_SEND_INIT_SEQUENCE;
    }


    if (cmd->resp_type & MMC_RSP_PRESENT) {
        cmdval |= SMHC_CMD_RESP_EXPIRE;
        if (cmd->resp_type & MMC_RSP_136)
            cmdval |= SMHC_CMD_LONG_RESPONSE;
        if (cmd->resp_type & MMC_RSP_CRC)
            cmdval |= SMHC_CMD_CHECK_RESPONSE_CRC;
    }

    if (data) {
        /* Check data desc align */
        if ((uint32_t) data->b.dest & 0x3) {
            printk_error("SMHC: data dest is not 4 byte align\n");
            error_code = -1;
            goto out;
        }

        cmdval |= SMHC_CMD_DATA_EXPIRE | SMHC_CMD_WAIT_PRE_OVER;
        if (data->flags & MMC_DATA_WRITE) {
            cmdval |= SMHC_CMD_WRITE;
        }
        if (data->blocks > 1) {
            cmdval |= SMHC_CMD_SEND_AUTO_STOP;
        }
        mmc_host->reg->blksz = data->blocksize;
        mmc_host->reg->bytecnt = data->blocks * data->blocksize;
    } else {
        if ((cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION) && (cmd->flags & MMC_CMD_MANUAL)) {
            cmdval |= SMHC_CMD_STOP_ABORT_CMD;//stop current data transferin progress.
            cmdval &= ~SMHC_CMD_WAIT_PRE_OVER;//Send command at once, even if previous data transfer has notcompleted
        }
    }

    mmc_host->reg->arg = cmd->cmdarg;

    if (!data) {
        mmc_host->reg->cmd = (cmdval | cmd->cmdidx);
    }

    /*
	 * transfer data and check status
	 * STATREG[2] : FIFO empty
	 * STATREG[3] : FIFO full
	 */

    if (data) {
        printk_trace("SMHC: transfer data %lu bytes\n", data->blocksize * data->blocks);
        if (data->blocksize * data->blocks > 512) {
            use_dma_status = true;
            mmc_host->reg->gctrl &= (~MMC_CMD_CMDVAL_MASK);
            ret = sunxi_sdhci_trans_data_dma(sdhci, data);
            mmc_host->reg->cmd = (cmdval & cmd->cmdidx);
        } else {
            mmc_host->reg->gctrl |= MMC_CMD_CMDVAL_MASK;
            mmc_host->reg->cmd = (cmdval & cmd->cmdidx);
            ret = sunxi_sdhci_trans_data_cpu(sdhci, data);
        }

        if (ret) {
            error_code = mmc_host->reg->rint & SMHC_RINT_INTERRUPT_ERROR_BIT;
            printk_warning("SMHC: error 0x%x status 0x%x\n", error_code & SMHC_RINT_INTERRUPT_ERROR_BIT, error_code & ~SMHC_RINT_INTERRUPT_ERROR_BIT);
            if (!error_code) {
                error_code = 0xffffffff;
            }
            goto out;
        }
    }

    timeout = time_us() + SMHC_TIMEOUT;
    do {
        status = mmc_host->reg->rint;
        if ((time_us() > timeout) || (status & SMHC_RINT_INTERRUPT_ERROR_BIT)) {
            error_code = status & SMHC_RINT_INTERRUPT_ERROR_BIT;
            if (!error_code) {
                error_code = 0xffffffff;
            }
            printk_error("SMHC: cmd 0x%08x timeout, error %08x", cmd->cmdidx, error_code);
            goto out;
        }
    } while (!(status & SMHC_RINT_COMMAND_DONE));

    if (data) {
        uint32_t done = false;
        timeout = time_us() + (use_dma_status ? SMHC_DMA_TIMEOUT : SMHC_TIMEOUT);
        do {
            status = mmc_host->reg->rint;
            if ((time_us() > timeout) || (status & SMHC_RINT_INTERRUPT_ERROR_BIT)) {
                error_code = status & SMHC_RINT_INTERRUPT_ERROR_BIT;
                if (!error_code) {
                    error_code = 0xffffffff;
                }
                printk_error("SMHC: data timeout, error %08x", error_code);
                goto out;
            }

            if (data->blocks > 1) {
                done = status & SMHC_RINT_AUTO_COMMAND_DONE;
            } else {
                done = status & SMHC_RINT_DATA_OVER;
            }
        } while (!done);

        if ((data->flags & MMC_DATA_READ) && use_dma_status) {
            timeout = time_us() + SMHC_DMA_TIMEOUT;
            done = 0;
            status = 0;
            do {
                status = mmc_host->reg->idst;
                if ((time_us() > timeout) || (status & 0x234)) {
                    error_code = status & 0x1e34;
                    if (!error_code) {
                        error_code = 0xffffffff;
                    }
                    printk_error("SMHC: wait dma timeout, error %08x", error_code);
                    goto out;
                }
                done = status & BIT(1);
            } while (!done);
        }
    }

    if (cmd->resp_type & MMC_RSP_BUSY) {
        timeout = time_us() + SMHC_WAITBUSY_TIMEOUT;
        do {
            status = mmc_host->reg->status;
            if ((time_us() > timeout)) {
                error_code = -1;
                if (!error_code) {
                    error_code = 0xffffffff;
                }
                printk_error("SMHC: busy timeout, status %08x", status);
                goto out;
            }
        } while (status & SMHC_STATUS_CARD_DATA_BUSY);
    }

    if (cmd->resp_type & MMC_RSP_136) {
        cmd->response[0] = mmc_host->reg->resp3;
        cmd->response[1] = mmc_host->reg->resp2;
        cmd->response[2] = mmc_host->reg->resp1;
        cmd->response[3] = mmc_host->reg->resp0;
    } else {
        cmd->response[0] = mmc_host->reg->resp0;
    }

out:
    if (data && use_dma_status) {
        /* IDMASTAREG
		 * IDST[0] : idma tx int
		 * IDST[1] : idma rx int
		 * IDST[2] : idma fatal bus error
		 * IDST[4] : idma descriptor invalid
		 * IDST[5] : idma error summary
		 * IDST[8] : idma normal interrupt sumary
		 * IDST[9] : idma abnormal interrupt sumary
		 */
        status = mmc_host->reg->idst;
        mmc_host->reg->idst = status;
        mmc_host->reg->idie = 0;
        mmc_host->reg->dmac = 0;
        mmc_host->reg->gctrl &= ~SMHC_GCTRL_DMA_ENABLE;
    }

    if (error_code) {
        mmc_host->reg->gctrl = SMHC_GCTRL_HARDWARE_RESET;
        timeout = time_us() + SMHC_TIMEOUT;
        while (mmc_host->reg->gctrl & SMHC_GCTRL_HARDWARE_RESET) {
            if (time_us() > timeout) {
                printk_error("SMHC: controller error reset timeout\n");
                return -1;
            }
        }
        sunxi_sdhci_update_clk(sdhci);
        printk_error("SMHC: CMD 0x%08x, error 0x%08x", cmd->cmdidx, error_code);
    }

    mmc_host->reg->rint = 0xffffffff;

    if (error_code) {
        return -1;
    }

    return 0;
}

int sunxi_sdhci_init(sdhci_t *sdhci) {
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
    mmc_t *mmc = mmc_host->mmc;

    /* Check controller id correct */
    if (sdhci->id > MMC_CONTROLLER_2) {
        printk_error("SMHC: Unsupported MAX Controller reached\n");
        return -1;
    }

    memset(mmc_host, 0, sizeof(sunxi_sdhci_host_t));
    memset(mmc, 0, sizeof(mmc_t));
    if (sdhci->id == MMC_CONTROLLER_0) {
        mmc_host->timing_mode = SUNXI_MMC_TIMING_MODE_1;
    } else if (sdhci->id == MMC_CONTROLLER_2) {
        mmc_host->timing_mode = SUNXI_MMC_TIMING_MODE_4;
    }

    mmc->voltages = MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 |
                    MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_34_35 |
                    MMC_VDD_35_36;
    mmc->host_caps = MMC_MODE_HS_52MHz | MMC_MODE_HS | MMC_MODE_HC;

    if (sdhci->width >= SMHC_WIDTH_4BIT) {
        mmc->host_caps |= MMC_MODE_4BIT;
    }

    if ((sdhci->id == MMC_CONTROLLER_2) && (sdhci->width == SMHC_WIDTH_8BIT)) {
        mmc->host_caps |= MMC_MODE_8BIT | MMC_MODE_4BIT;
    }

    mmc->f_min = 400000;
    mmc->f_max = sdhci->max_clk;
    mmc->f_max_ddr = sdhci->max_clk;

    mmc_host->reg = (sdhci_reg_t *) sdhci->reg_base;
    mmc_host->database = (uint32_t) sdhci->reg_base + MMC_REG_FIFO_OS;
    mmc_host->hclkbase = sdhci->clk_ctrl_base;
    mmc_host->hclkrst = sdhci->clk_ctrl_base;
    mmc_host->mclkbase = sdhci->clk_base;

    sunxi_sdhci_pin_config(sdhci);
    sunxi_sdhci_clk_enable(sdhci);

    return 0;
}