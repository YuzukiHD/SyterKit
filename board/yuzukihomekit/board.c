#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <reg-ncat.h>
#include <sys-clk.h>

#include <mmu.h>

#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-sdcard.h>
#include <sys-sid.h>
#include <sys-spi.h>
#include <sys-uart.h>

sunxi_serial_t uart_dbg = {
        .base = SUNXI_UART3_BASE,
        .id = 3,
        .gpio_tx = {GPIO_PIN(GPIO_PORTB, 6), GPIO_PERIPH_MUX7},
        .gpio_rx = {GPIO_PIN(GPIO_PORTB, 7), GPIO_PERIPH_MUX7},
};

sunxi_spi_t sunxi_spi0 = {
        .base = SUNXI_SPI0_BASE,
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
        .id = 0,
        .reg = (sdhci_reg_t *) 0x04020000,
        .voltage = MMC_VDD_27_36,
        .width = MMC_BUS_WIDTH_4,
        .clock = MMC_CLK_200M,
        .removable = 0,
        .isspi = FALSE,
        .skew_auto_mode = TRUE,
        .sdhci_pll = CCU_MMC_CTRL_PLL_PERIPH1X,
        .gpio_clk = {GPIO_PIN(GPIO_PORTF, 2), GPIO_PERIPH_MUX2},
        .gpio_cmd = {GPIO_PIN(GPIO_PORTF, 3), GPIO_PERIPH_MUX2},
        .gpio_d0 = {GPIO_PIN(GPIO_PORTF, 1), GPIO_PERIPH_MUX2},
        .gpio_d1 = {GPIO_PIN(GPIO_PORTF, 0), GPIO_PERIPH_MUX2},
        .gpio_d2 = {GPIO_PIN(GPIO_PORTF, 5), GPIO_PERIPH_MUX2},
        .gpio_d3 = {GPIO_PIN(GPIO_PORTF, 4), GPIO_PERIPH_MUX2},
};

sunxi_i2c_t i2c_pmu = {
        .base = SUNXI_TWI0_BASE,
        .id = SUNXI_I2C0,
        .speed = 4000000,
        .gpio_scl = {GPIO_PIN(GPIO_PORTL, 0), GPIO_PERIPH_MUX3},
        .gpio_sda = {GPIO_PIN(GPIO_PORTL, 1), GPIO_PERIPH_MUX3},
};

dram_para_t dram_para = {
        .dram_clk = 792,
        .dram_type = 3,
        .dram_zq = 0x7b7bfb,
        .dram_odt_en = 0x00,
        .dram_para1 = 0x000010d2,
        .dram_para2 = 0,
        .dram_mr0 = 0x1c70,
        .dram_mr1 = 0x42,
        .dram_mr2 = 0x18,
        .dram_mr3 = 0,
        .dram_tpr0 = 0x004a2195,
        .dram_tpr1 = 0x02423190,
        .dram_tpr2 = 0x0008b061,
        .dram_tpr3 = 0xb4787896,// unused
        .dram_tpr4 = 0,
        .dram_tpr5 = 0x48484848,
        .dram_tpr6 = 0x00000048,
        .dram_tpr7 = 0x1620121e,// unused
        .dram_tpr8 = 0,
        .dram_tpr9 = 0,// clock?
        .dram_tpr10 = 0,
        .dram_tpr11 = 0x00340000,
        .dram_tpr12 = 0x00000046,
        .dram_tpr13 = 0x34000100,
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

void show_chip() {
    uint32_t chip_sid[4];
    chip_sid[0] = read32(SUNXI_SID_SRAM_BASE + 0x0);
    chip_sid[1] = read32(SUNXI_SID_SRAM_BASE + 0x4);
    chip_sid[2] = read32(SUNXI_SID_SRAM_BASE + 0x8);
    chip_sid[3] = read32(SUNXI_SID_SRAM_BASE + 0xc);

    printk(LOG_LEVEL_INFO, "Model: Yuzuki Home Kit\n");
    printk(LOG_LEVEL_INFO, "Host Core: Arm Dual-Core Cortex-A7 R2P0\n");
    printk(LOG_LEVEL_INFO, "AMP Core: Xuantie C906 RISC-V RV64IMAFDCVX R1S0P2 Vlen=128\n");
    printk(LOG_LEVEL_INFO, "Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);

    uint32_t chip_markid_sid = chip_sid[0] & 0xffff;

    switch (chip_markid_sid) {
        case 0x7200:
            printk(LOG_LEVEL_INFO, "Chip type = T113M4020DC0");
            break;
        default:
            printk(LOG_LEVEL_INFO, "Chip type = UNKNOW");
            break;
    }

    uint32_t version = read32(SUNXI_SYSCRL_BASE + 0x24) & 0x7;
    printk(LOG_LEVEL_MUTE, " Chip Version = %x \n", version);
}
