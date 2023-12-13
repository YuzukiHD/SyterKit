/* SPDX-License-Identifier: Apache-2.0 */

#include <arm32.h>
#include <config.h>
#include <io.h>
#include <log.h>

#include <sys-timer.h>

#include <reg-ccu.h>
#include <reg-ncat.h>

void clean_syterboot_data(void) {
    /* Disable MMU, data cache, instruction cache, interrupts */
    arm32_mmu_disable();
    printk(LOG_LEVEL_INFO, "disable mmu ok...\n");
    arm32_dcache_disable();
    printk(LOG_LEVEL_INFO, "disable dcache ok...\n");
    arm32_icache_disable();
    printk(LOG_LEVEL_INFO, "disable icache ok...\n");
    arm32_interrupt_disable();
    printk(LOG_LEVEL_INFO, "free interrupt ok...\n");
}

void rtc_set_vccio_det_spare(void) {
    u32 val = 0;
    val = readl(SUNXI_RTC_BASE + 0x1f4);
    val &= ~(0xff << 4);
    val |= (VCCIO_THRESHOLD_VOLTAGE_2_9 | FORCE_DETECTER_OUTPUT);
    val &= ~VCCIO_DET_BYPASS_EN;
    writel(val, SUNXI_RTC_BASE + 0x1f4);
}

void sys_ldo_check(void) {
    uint32_t reg_val = 0;
    uint32_t roughtrim_val = 0, finetrim_val = 0;

    /* reset */
    reg_val = readl(CCU_AUDIO_CODEC_BGR_REG);
    reg_val &= ~(1 << 16);
    writel(reg_val, CCU_AUDIO_CODEC_BGR_REG);

    sdelay(2);

    reg_val |= (1 << 16);
    writel(reg_val, CCU_AUDIO_CODEC_BGR_REG);

    /* enable AUDIO gating */
    reg_val = readl(CCU_AUDIO_CODEC_BGR_REG);
    reg_val |= (1 << 0);
    writel(reg_val, CCU_AUDIO_CODEC_BGR_REG);

    /* enable pcrm CTRL */
    reg_val = readl(ANA_PWR_RST_REG);
    reg_val &= ~(1 << 0);
    writel(reg_val, ANA_PWR_RST_REG);

    /* read efuse */
    printk(LOG_LEVEL_DEBUG, "Audio: avcc calibration\n");
    reg_val = readl(SUNXI_SID_SRAM_BASE + 0x28);
    roughtrim_val = (reg_val >> 0) & 0xF;
    reg_val = readl(SUNXI_SID_SRAM_BASE + 0x24);
    finetrim_val = (reg_val >> 16) & 0xFF;

    if (roughtrim_val == 0 && finetrim_val == 0) {
        reg_val = readl(SUNXI_VER_REG);
        reg_val = (reg_val >> 0) & 0x7;
        if (reg_val) {
            printk(LOG_LEVEL_DEBUG, "Audio: chip not version A\n");
        } else {
            roughtrim_val = 0x5;
            finetrim_val = 0x19;
            printk(LOG_LEVEL_DEBUG, "Audio: chip version A\n");
        }
    }
    reg_val = readl(AUDIO_POWER_REG);
    reg_val &= ~(0xF << 8 | 0xFF);
    reg_val |= roughtrim_val << 8 | finetrim_val;
    writel(reg_val, AUDIO_POWER_REG);
}

void show_banner(void) {
    uint32_t id[4];

    printk(LOG_LEVEL_MUTE, "\n");
    printk(LOG_LEVEL_INFO, " _____     _           _____ _ _   \n");
    printk(LOG_LEVEL_INFO, "|   __|_ _| |_ ___ ___|  |  |_| |_ \n");
    printk(LOG_LEVEL_INFO, "|__   | | |  _| -_|  _|    -| | _| \n");
    printk(LOG_LEVEL_INFO, "|_____|_  |_| |___|_| |__|__|_|_|  \n");
    printk(LOG_LEVEL_INFO, "      |___|                        \n");
    printk(LOG_LEVEL_INFO, "***********************************\n");
    printk(LOG_LEVEL_INFO, " %s V0.1.2 Commit: %s\n", PROJECT_NAME, PROJECT_GIT_HASH);
    printk(LOG_LEVEL_INFO, " Built by: %s\n", PROJECT_C_COMPILER);
    printk(LOG_LEVEL_INFO, "***********************************\n");
    printk(LOG_LEVEL_INFO, "\n");

    id[0] = read32(0x03006200 + 0x0);
    id[1] = read32(0x03006200 + 0x4);
    id[2] = read32(0x03006200 + 0x8);
    id[3] = read32(0x03006200 + 0xc);

    printk(LOG_LEVEL_INFO, "Chip ID is: %08x%08x%08x%08x\n", id[0], id[1], id[2], id[3]);
}
