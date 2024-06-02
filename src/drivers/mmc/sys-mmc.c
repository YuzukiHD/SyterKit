/* SPDX-License-Identifier:	GPL-2.0+ */

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

static inline int sunxi_mmc_host_is_spi(mmc_t *mmc) {
    return (mmc->host_caps & MMC_MODE_SPI);
}

static int sunxi_mmc_send_status(sdhci_t *sdhci, uint32_t timeout) {
    mmc_t *mmc = sdhci->mmc;
    int err = 0;

    mmc_cmd_t cmd;
    cmd.cmdidx = MMC_CMD_SEND_STATUS;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = mmc->rca << 16;
    cmd.flags = 0;

    do {
        err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);
        if (err) {
            printk_error("SMHC%u: Send status failed\n", sdhci->id);
            return err;
        } else if (cmd.response[0] & MMC_STATUS_RDY_FOR_DATA)
            break;
        mdelay(1);
        if (cmd.response[0] & MMC_STATUS_MASK) {
            printk_error("SMHC%u: Status Error: 0x%08X\n", sdhci->id, cmd.response[0]);
            return COMM_ERR;
        }
    } while (timeout--);

    if (!timeout) {
        printk_error("SMHC%u: Timeout waiting card ready\n", sdhci->id);
        return TIMEOUT;
    }

    return 0;
}

static int sunxi_mmc_set_block_len(sdhci_t *sdhci, uint32_t len) {
    mmc_t *mmc = sdhci->mmc;
    mmc_cmd_t cmd;
    /* don't set blocklen at ddr mode */
    if ((mmc->speed_mode == MMC_HSDDR52_DDR50) || (mmc->speed_mode == MMC_HS400)) {
        return 0;
    }
    cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = len;
    cmd.flags = 0;

    return sunxi_sdhci_xfer(sdhci, &cmd, NULL);
}

static uint64_t sunxi_mmc_read_blocks(sdhci_t *sdhci, void *dst, uint64_t start, uint64_t blkcnt) {
    mmc_t *mmc = sdhci->mmc;

    mmc_cmd_t cmd;
    mmc_data_t data;

    int timeout = 1000;

    if (blkcnt > 1)
        cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
    else
        cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

    if (mmc->high_capacity)
        cmd.cmdarg = start;
    else
        cmd.cmdarg = start * mmc->read_bl_len;

    cmd.resp_type = MMC_RSP_R1;
    cmd.flags = 0;

    data.b.dest = dst;
    data.blocks = blkcnt;
    data.blocksize = mmc->read_bl_len;
    data.flags = MMC_DATA_READ;

    if (sunxi_sdhci_xfer(sdhci, &cmd, &data)) {
        printk_error("SMHC: read blcok failed\n");
        return 0;
    }

    if (blkcnt > 1) {
        cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
        cmd.cmdarg = 0;
        cmd.resp_type = MMC_RSP_R1b;
        cmd.flags = 0;
        if (sunxi_sdhci_xfer(sdhci, &cmd, NULL)) {
            printk_error("SMHC: fail to send stop cmd\n");
            return 0;
        }

        /* Waiting for the ready status */
        sunxi_mmc_send_status(sdhci, timeout);
    }

    return blkcnt;
}

static int sunxi_mmc_go_idle(sdhci_t *sdhci) {
    mmc_t *mmc = sdhci->mmc;
    mmc_cmd_t cmd;

    int err = 0;

    cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
    cmd.cmdarg = 0;
    cmd.resp_type = MMC_RSP_NONE;
    cmd.flags = 0;

    err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

    if (err) {
        printk_error("SMHC: idle failed\n");
        return err;
    }
    mdelay(2);
    return 0;
}

static int sunxi_mmc_sd_send_op_cond(sdhci_t *sdhci) {
    int timeout = 1000;
    int err;

    mmc_t *mmc = sdhci->mmc;
    mmc_cmd_t cmd;

    do {
        cmd.cmdidx = MMC_CMD_APP_CMD;
        cmd.resp_type = MMC_RSP_R1;
        cmd.cmdarg = 0;
        cmd.flags = 0;

        err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

        if (err) {
            printk_error("SMHC: send app cmd failed\n");
            return err;
        }

        cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
        cmd.resp_type = MMC_RSP_R3;

        /*
		 * Most cards do not answer if some reserved bits
		 * in the ocr are set. However, Some controller
		 * can set bit 7 (reserved for low voltages), but
		 * how to manage low voltages SD card is not yet
		 * specified.
		 */
        cmd.cmdarg = sunxi_mmc_host_is_spi(mmc) ? 0 : (mmc->voltages & 0xff8000);

        if (mmc->version == SD_VERSION_2)
            cmd.cmdarg |= OCR_HCS;

        err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

        if (err) {
            printk_error("SMHC: send cmd41 failed\n");
            return err;
        }

        mdelay(1);
    } while ((!(cmd.response[0] & OCR_BUSY)) && timeout--);

    if (timeout <= 0) {
        printk_error("SMHC: wait card init failed\n");
        return UNUSABLE_ERR;
    }

    if (mmc->version != SD_VERSION_2)
        mmc->version = SD_VERSION_1_0;

    if (mmc_host_is_spi(mmc)) { /* read OCR for spi */
        cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
        cmd.resp_type = MMC_RSP_R3;
        cmd.cmdarg = 0;
        cmd.flags = 0;

        err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

        if (err) {
            printk_error("SMHC: spi read ocr failed\n");
            return err;
        }
    }

    mmc->ocr = cmd.response[0];

    mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
    mmc->rca = 0;

    return 0;
}

static int sunxi_mmc_mmc_send_op_cond(sdhci_t *sdhci) {
    int timeout = 1000;
    int err;

    mmc_t *mmc = sdhci->mmc;
    mmc_cmd_t cmd;

    /* Some cards seem to need this */
    sunxi_mmc_go_idle(sdhci);

    /* Asking to the card its capabilities */
    cmd.cmdidx = MMC_CMD_SEND_OP_COND;
    cmd.resp_type = MMC_RSP_R3;
    cmd.cmdarg = 0; /*0x40ff8000;*/ /*foresee */
    cmd.flags = 0;

    err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

    if (err) {
        printk_error("SMHC: send op cond failed\n");
        return err;
    }

    mdelay(1);

    do {
        cmd.cmdidx = MMC_CMD_SEND_OP_COND;
        cmd.resp_type = MMC_RSP_R3;
        cmd.cmdarg = (sunxi_mmc_host_is_spi(mmc) ? 0 : (mmc->voltages & (cmd.response[0] & OCR_VOLTAGE_MASK)) | (cmd.response[0] & OCR_ACCESS_MODE));

        if (mmc->host_caps & MMC_MODE_HC)
            cmd.cmdarg |= OCR_HCS;

        cmd.flags = 0;

        err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

        if (err) {
            printk_error("SMHC: send op cond failed\n");
            return err;
        }

        mdelay(1);
    } while (!(cmd.response[0] & OCR_BUSY) && timeout--);

    if (timeout <= 0) {
        printk_error("SMHC: wait for mmc init failed\n");
        return UNUSABLE_ERR;
    }

    if (sunxi_mmc_host_is_spi(mmc)) { /* read OCR for spi */
        cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
        cmd.resp_type = MMC_RSP_R3;
        cmd.cmdarg = 0;
        cmd.flags = 0;

        err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);
        if (err)
            return err;
    }

    mmc->version = MMC_VERSION_UNKNOWN;
    mmc->ocr = cmd.response[0];

    mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
    mmc->rca = 1;

    return 0;
}

static int sunxi_mmc_send_ext_csd(sdhci_t *sdhci, char *ext_csd) {
    mmc_t *mmc = sdhci->mmc;

    mmc_cmd_t cmd;
    mmc_data_t data;
    int err;

    /* Get the Card Status Register */
    cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = 0;
    cmd.flags = 0;

    data.b.dest = ext_csd;
    data.blocks = 1;
    data.blocksize = 512;
    data.flags = MMC_DATA_READ;

    err = sunxi_sdhci_xfer(sdhci, &cmd, &data);
    if (err)
        printk_error("SMHC: send ext csd failed\n");

    return err;
}

static int sunxi_mmc_switch(sdhci_t *sdhci, uint8_t set, uint8_t index, uint8_t value) {
    mmc_t *mmc = sdhci->mmc;

    mmc_cmd_t cmd;
    int timeout = 1000;
    int ret;

    cmd.cmdidx = MMC_CMD_SWITCH;
    cmd.resp_type = MMC_RSP_R1b;
    cmd.cmdarg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) | (index << 16) | (value << 8);
    cmd.flags = 0;

    ret = sunxi_sdhci_xfer(sdhci, &cmd, NULL);
    if (ret) {
        printk_error("SMHC: switch failed\n");
    }

    /* for re-update sample phase */
    ret = sunxi_sdhci_update_phase(sdhci);
    if (ret) {
        printk_error("SMHC: update clock failed after send cmd6\n");
        return ret;
    }

    /* Waiting for the ready status */
    sunxi_mmc_send_status(sdhci, timeout);

    return ret;
}

static int sunxi_mmc_change_freq(sdhci_t *sdhci) {
    mmc_t *mmc = sdhci->mmc;

    char ext_csd[512];
    char cardtype;
    int err;
    int retry = 5;

    mmc->card_caps = 0;

    if (sunxi_mmc_host_is_spi(mmc))
        return 0;

    /* Only version 4 supports high-speed */
    if (mmc->version < MMC_VERSION_4)
        return 0;

    mmc->card_caps |= MMC_MODE_4BIT | MMC_MODE_8BIT;
    err = mmc_send_ext_csd(mmc, ext_csd);

    if (err) {
        printk_error("SMHC: get ext csd failed\n");
        return err;
    }

    cardtype = ext_csd[196] & 0xff;

    /*retry for Toshiba emmc,for the first time Toshiba emmc change to HS */
    /*it will return response crc err,so retry */
    do {
        err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);
        if (!err) {
            break;
        }
        printk_debug("SMHC: retry mmc switch(cmd6)\n");
    } while (retry--);

    if (err) {
        printk_error("SMHC: change to hs failed\n");
        return err;
    }

    /* Now check to see that it worked */
    err = sunxi_mmc_send_ext_csd(sdhci, ext_csd);

    if (err) {
        printk_error("SMHC: send ext csd faild\n");
        return err;
    }

    /* No high-speed support */
    if (!ext_csd[185])
        return 0;

    /* High Speed is set, there are two types: 52MHz and 26MHz */
    if (cardtype & EXT_CSD_CARD_TYPE_HS) {
        if (cardtype & EXT_CSD_CARD_TYPE_DDR_52) {
            printk_trace("SMHC: get ddr OK!\n");
            mmc->card_caps |= MMC_MODE_DDR_52MHz;
            mmc->speed_mode = MMC_HSDDR52_DDR50;
        } else
            mmc->speed_mode = MMC_HSSDR52_SDR25;
        mmc->card_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;

    } else {
        mmc->card_caps |= MMC_MODE_HS;
        mmc->speed_mode = MMC_DS26_SDR12;
    }

    return 0;
}

static int sunxi_mmc_sd_switch(sdhci_t *sdhci, int mode, int group, uint8_t value, uint8_t *resp) {
    mmc_cmd_t cmd;
    mmc_data_t data;

    /* Switch the frequency */
    cmd.cmdidx = SD_CMD_SWITCH_FUNC;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = ((uint32_t) mode << 31) | 0xffffff;
    cmd.cmdarg &= ~(0xf << (group * 4));
    cmd.cmdarg |= value << (group * 4);
    cmd.flags = 0;

    data.b.dest = (char *) resp;
    data.blocksize = 64;
    data.blocks = 1;
    data.flags = MMC_DATA_READ;

    return sunxi_sdhci_xfer(sdhci, &cmd, &data);
}

static int sunxi_mmc_sd_change_freq(sdhci_t *sdhci) {
    mmc_t *mmc = sdhci->mmc;

    mmc_cmd_t cmd;
    mmc_data_t data;
    uint32_t scr[2];
    uint32_t switch_status[16];
    int err;
    int timeout;

    mmc->card_caps = 0;

    if (sunxi_mmc_host_is_spi(mmc))
        return 0;

    /* Read the SCR to find out if this card supports higher speeds */
    cmd.cmdidx = MMC_CMD_APP_CMD;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = mmc->rca << 16;
    cmd.flags = 0;

    err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

    if (err) {
        printk_error("SMHC: Send app cmd failed\n");
        return err;
    }

    cmd.cmdidx = SD_CMD_APP_SEND_SCR;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = 0;
    cmd.flags = 0;

    timeout = 3;

retry_scr:
    data.b.dest = (char *) &scr;
    data.blocksize = 8;
    data.blocks = 1;
    data.flags = MMC_DATA_READ;

    err = sunxi_sdhci_xfer(sdhci, &cmd, &data);

    if (err) {
        if (timeout--)
            goto retry_scr;
        printk_error("SMHC: Send scr failed\n");
        return err;
    }

    mmc->scr[0] = be32_to_cpu(scr[0]);
    mmc->scr[1] = be32_to_cpu(scr[1]);

    switch ((mmc->scr[0] >> 24) & 0xf) {
        case 0:
            mmc->version = SD_VERSION_1_0;
            break;
        case 1:
            mmc->version = SD_VERSION_1_10;
            break;
        case 2:
            mmc->version = SD_VERSION_2;
            break;
        default:
            mmc->version = SD_VERSION_1_0;
            break;
    }

    if (mmc->scr[0] & SD_DATA_4BIT)
        mmc->card_caps |= MMC_MODE_4BIT;

    /* Version 1.0 doesn't support switching */
    if (mmc->version == SD_VERSION_1_0)
        return 0;

    timeout = 4;
    while (timeout--) {
        err = sunxi_mmc_sd_switch(sdhci, SD_SWITCH_CHECK, 0, 1, (uint8_t *) &switch_status);

        if (err) {
            printk_error("SMHC: Check high speed status faild\n");
            return err;
        }

        /* The high-speed function is busy.  Try again */
        if (!(be32_to_cpu(switch_status[7]) & SD_HIGHSPEED_BUSY))
            break;
    }

    /* If high-speed isn't supported, we return */
    if (!(be32_to_cpu(switch_status[3]) & SD_HIGHSPEED_SUPPORTED))
        return 0;

    err = sunxi_mmc_sd_switch(sdhci, SD_SWITCH_SWITCH, 0, 1, (uint8_t *) &switch_status);

    if (err) {
        printk_error("SMHC: switch to high speed failed\n");
        return err;
    }

    err = sunxi_sdhci_update_phase(sdhci);
    if (err) {
        printk_error("SMHC: update clock failed after send cmd6 to switch to sd high speed mode\n");
        return err;
    }

    if ((be32_to_cpu(switch_status[4]) & 0x0f000000) == 0x01000000) {
        mmc->card_caps |= MMC_MODE_HS;
        mmc->speed_mode = MMC_HSSDR52_SDR25;
    }

    return 0;
}

/* frequency bases */
/* divided by 10 to be nice to platforms without floating point */
static const int fbase[] = {
        10000,
        100000,
        1000000,
        10000000,
};

/* Multiplier values for TRAN_SPEED.  Multiplied by 10 to be nice
 * to platforms without floating point.
 */
static const int multipliers[] = {
        0, /* reserved */
        10,
        12,
        13,
        15,
        20,
        25,
        30,
        35,
        40,
        45,
        50,
        55,
        60,
        70,
        80,
};

static void sunxi_mmc_set_clock(sdhci_t *sdhci, uint32_t clock) {
    mmc_t *mmc = sdhci->mmc;

    if (clock > mmc->f_max) {
        clock = mmc->f_max;
    }

    if (clock < mmc->f_min) {
        clock = mmc->f_min;
    }

    mmc->clock = clock;
    sunxi_sdhci_set_ios(sdhci);
}

static void sunxi_mmc_set_bus_width(sdhci_t *sdhci, uint32_t width) {
    sdhci->width = width;
    sunxi_sdhci_set_ios(sdhci);
}

static int sunxi_mmc_mmc_switch_ds(sdhci_t *sdhci) {
    mmc_t *mmc = sdhci->mmc;
    int err;

    if (mmc->speed_mode == MMC_DS26_SDR12) {
        printk_trace("SMHC: set in SDR12 mode\n");
    }

    if (!(mmc->card_caps && MMC_MODE_HS)) {
        printk_error("SMHC: Card not support ds mode\n");
        return -1;
    }

    err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_BC);

    if (err) {
        printk_error("SMHC: Change to ds failed\n");
        return err;
    }

    mmc->speed_mode = MMC_DS26_SDR12;

    return 0;
}

static int sunxi_mmc_mmc_switch_hs(sdhci_t *sdhci) {
    mmc_t *mmc = sdhci->mmc;
    int err;

    if (mmc->speed_mode == MMC_HSSDR52_SDR25) {
        printk_trace("SMHC: set in SDR25 mode\n");
    }

    if (!(mmc->card_caps && MMC_MODE_HS_52MHz)) {
        printk_error("SMHC: Card not support hs mode\n");
        return -1;
    }

    err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS);

    if (err) {
        printk_error("SMHC: Change to hs failed\n");
        return err;
    }

    mmc->speed_mode = MMC_HSSDR52_SDR25;

    return 0;
}

static int sunxi_mmc_mmc_switch_hs200(sdhci_t *sdhci) {
    mmc_t *mmc = sdhci->mmc;
    int err;

    if (mmc->speed_mode == MMC_HS200_SDR104) {
        printk_trace("SMHC: set in SDR104 mode\n");
    }

    if (!(mmc->card_caps && MMC_MODE_HS200)) {
        printk_error("SMHC: Card not support hs200 mode\n");
        return -1;
    }

    err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS200);

    if (err) {
        printk_error("SMHC: Change to hs200 failed\n");
        return err;
    }

    mmc->speed_mode = MMC_HS200_SDR104;

    return 0;
}

static int sunxi_mmc_mmc_switch_hs400(sdhci_t *sdhci) {
    mmc_t *mmc = sdhci->mmc;
    int err;

    if (mmc->speed_mode == MMC_HS400) {
        printk_trace("SMHC: set in HS400 mode\n");
    }

    if (!(mmc->card_caps && MMC_MODE_HS400)) {
        printk_error("SMHC: Card not support hs400 mode\n");
        return -1;
    }

    err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS400);

    if (err) {
        printk_error("SMHC: Change to hs400 failed\n");
        return err;
    }

    mmc->speed_mode = MMC_HS400;

    return 0;
}

static int sunxi_mmc_mmc_switch_mdoe(sdhci_t *sdhci, uint32_t spd_mode) {
    mmc_t *mmc = sdhci->mmc;
    int ret = 0;

    if (sunxi_mmc_host_is_spi(mmc)) {
        return 0;
    }

    switch (spd_mode) {
        case MMC_DS26_SDR12:
            ret = sunxi_mmc_mmc_switch_ds(sdhci);
            break;
        case MMC_HSSDR52_SDR25:
            ret = sunxi_mmc_mmc_switch_to_hs(sdhci);
            break;
        case MMC_HS200_SDR104:
            ret = sunxi_mmc_mmc_switch_to_hs200(sdhci);
            break;
        case MMC_HS400:
            ret = sunxi_mmc_mmc_switch_to_hs400(sdhci);
            break;
        default:
            ret = -1;
            printk_debug("SMHC: error speed mode %d\n", spd_mode);
            break;
    }
    return ret;
}

static int sunxi_mmc_check_bus_width(sdhci_t *sdhci, uint32_t emmc_hs_ddr, uint32_t bus_width) {
    mmc_t *mmc = sdhci->mmc;
    int ret = 0;

    if (bus_width == SMHC_WIDTH_1BIT) {
        /* don't consider SD3.0. tSD/fSD is SD2.0, 1-bit can be support */ {
            if ((emmc_hs_ddr && (!IS_SD(mmc)) && (mmc->speed_mode == MMC_HSSDR52_SDR25)) ||
                ((!IS_SD(mmc)) && (mmc->speed_mode == MMC_HSDDR52_DDR50)) ||
                ((!IS_SD(mmc)) && (mmc->speed_mode == MMC_HS200_SDR104)) ||
                ((!IS_SD(mmc)) && (mmc->speed_mode == MMC_HS400)))
                ret = -1;
        }
    } else if (bus_width == SMHC_WIDTH_1BIT) {
        if (!(mmc->card_caps & MMC_MODE_4BIT)) {
            ret = -1;
        }
    } else if (bus_width == SMHC_WIDTH_8BIT) {
        if (!(mmc->card_caps & MMC_MODE_8BIT))
            ret = -1;
        if (IS_SD(mmc))
            ret = -1;
    } else {
        printk_debug("SMHC: bus width error %d\n", bus_width);
        ret = -1;
    }

    return ret;
}

static int sunxi_mmc_mmc_switch_bus_width(sdhci_t *sdhci, uint32_t spd_mode, uint32_t width) {
    mmc_t *mmc = sdhci->mmc;
    int err = 0;
    uint32_t emmc_hs_ddr = 0;
    uint32_t val = 0;

    if (spd_mode == MMC_HS400) {
        return 0;
    }

    if (spd_mode == MMC_DS26_SDR12) {
        emmc_hs_ddr = 1;
    }

    err = sunxi_mmc_check_bus_width(sdhci, emmc_hs_ddr, width);

    if (err) {
        printk_error("SMHC: bus witdh param error.\n");
        return -1;
    }

    if (width == SMHC_WIDTH_1BIT)
        val = EXT_CSD_BUS_WIDTH_1;
    else if (spd_mode == MMC_HSDDR52_DDR50) {
        if (width == SMHC_WIDTH_4BIT)
            val = EXT_CSD_BUS_DDR_4;
        else if (width == SMHC_WIDTH_8BIT)
            val = EXT_CSD_BUS_DDR_8;
    } else if (width == SMHC_WIDTH_4BIT)
        val = EXT_CSD_BUS_WIDTH_4;
    else if (width == SMHC_WIDTH_8BIT)
        val = EXT_CSD_BUS_WIDTH_8;
    else
        val = EXT_CSD_BUS_WIDTH_1;

    err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH, val);

    if (err) {
        printk_error("SMHC: set bus witdh error.\n");
        return -1;
    }
    if (spd_mode == MMC_HSDDR52_DDR50) {
        mmc->speed_mode = MMC_HSDDR52_DDR50;
    }

    sunxi_mmc_set_bus_width(sdhci, width);

    return err;
}