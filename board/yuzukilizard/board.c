#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <sys-clk.h>
#include <reg-ncat.h>

#include <mmu.h>

#include <sys-gpio.h>
#include <sys-spi.h>
#include <sys-uart.h>
#include <sys-sdcard.h>

sunxi_serial_t uart_dbg = {
        .base = 0x02500000,
        .id = 0,
        .gpio_tx = {GPIO_PIN(GPIO_PORTH, 9), GPIO_PERIPH_MUX5},
        .gpio_rx = {GPIO_PIN(GPIO_PORTH, 10), GPIO_PERIPH_MUX5},
};

sunxi_spi_t sunxi_spi0 = {
        .base = 0x04025000,
        .id = 0,
        .clk_rate = 75 * 1000 * 1000,
        .gpio_cs = {GPIO_PIN(GPIO_PORTC, 1), GPIO_PERIPH_MUX4},
        .gpio_sck = {GPIO_PIN(GPIO_PORTC, 0), GPIO_PERIPH_MUX4},
        .gpio_mosi = {GPIO_PIN(GPIO_PORTC, 2), GPIO_PERIPH_MUX4},
        .gpio_miso = {GPIO_PIN(GPIO_PORTC, 3), GPIO_PERIPH_MUX4},
        .gpio_wp = {GPIO_PIN(GPIO_PORTC, 4), GPIO_PERIPH_MUX4},
        .gpio_hold = {GPIO_PIN(GPIO_PORTC, 5), GPIO_PERIPH_MUX4},
};

sdhci_t sdhci0 = {
        .name = "sdhci0",
        .reg = (sdhci_reg_t *) 0x04020000,
        .voltage = MMC_VDD_27_36,
        .width = MMC_BUS_WIDTH_4,
        .clock = MMC_CLK_50M,
        .removable = 0,
        .isspi = FALSE,
        .gpio_clk = {GPIO_PIN(GPIO_PORTF, 2), GPIO_PERIPH_MUX2},
        .gpio_cmd = {GPIO_PIN(GPIO_PORTF, 3), GPIO_PERIPH_MUX2},
        .gpio_d0 = {GPIO_PIN(GPIO_PORTF, 1), GPIO_PERIPH_MUX2},
        .gpio_d1 = {GPIO_PIN(GPIO_PORTF, 0), GPIO_PERIPH_MUX2},
        .gpio_d2 = {GPIO_PIN(GPIO_PORTF, 5), GPIO_PERIPH_MUX2},
        .gpio_d3 = {GPIO_PIN(GPIO_PORTF, 4), GPIO_PERIPH_MUX2},
};

void clean_syterkit_data(void) {
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