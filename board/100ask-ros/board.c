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
        .base = SUNXI_UART0_BASE,
        .id = 0,
        .gpio_tx = {GPIO_PIN(GPIO_PORTB, 9), GPIO_PERIPH_MUX2},
        .gpio_rx = {GPIO_PIN(GPIO_PORTB, 10), GPIO_PERIPH_MUX2},
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
        .reg = (sdhci_reg_t *) SUNXI_SMHC0_BASE,
        .voltage = MMC_VDD_27_36,
        .width = MMC_BUS_WIDTH_4,
        .clock = MMC_CLK_50M,
        .removable = 0,
        .isspi = FALSE,
        .skew_auto_mode = FALSE,
        .sdhci_pll = CCU_MMC_CTRL_PLL_PERIPH1X,
        .gpio_clk = {GPIO_PIN(GPIO_PORTF, 2), GPIO_PERIPH_MUX2},
        .gpio_cmd = {GPIO_PIN(GPIO_PORTF, 3), GPIO_PERIPH_MUX2},
        .gpio_d0 = {GPIO_PIN(GPIO_PORTF, 1), GPIO_PERIPH_MUX2},
        .gpio_d1 = {GPIO_PIN(GPIO_PORTF, 0), GPIO_PERIPH_MUX2},
        .gpio_d2 = {GPIO_PIN(GPIO_PORTF, 5), GPIO_PERIPH_MUX2},
        .gpio_d3 = {GPIO_PIN(GPIO_PORTF, 4), GPIO_PERIPH_MUX2},
};

sunxi_i2c_t i2c_pmu = {
        .base = SUNXI_RTWI_BASE,
        .id = SUNXI_R_I2C0,
        .speed = 4000000,
        .gpio_scl = {GPIO_PIN(GPIO_PORTL, 0), GPIO_PERIPH_MUX2},
        .gpio_sda = {GPIO_PIN(GPIO_PORTL, 1), GPIO_PERIPH_MUX2},
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

#define RTC_DATA_COLD_START (7)
#define CPUS_CODE_LENGTH (0x1000)
#define CPUS_VECTOR_LENGTH (0x4000)

extern uint8_t ar100code_bin[];
extern uint32_t ar100code_bin_len;

int ar100s_gpu_fix(void) {
    uint32_t value;
    uint32_t id = (readl(SUNXI_SYSCRL_BASE + 0x24)) & 0x07;
    printk(LOG_LEVEL_DEBUG, "SUNXI_SYSCRL_BASE + 0x24 = 0x%08x, id = %d, RTC_DATA_COLD_START = %d\n",
           readl(SUNXI_SYSCRL_BASE + 0x24), id, rtc_read_data(RTC_DATA_COLD_START));
    if (((id == 0) || (id == 3) || (id == 4) || (id == 5))) {
        if (rtc_read_data(RTC_DATA_COLD_START) == 0) {
            rtc_write_data(RTC_DATA_COLD_START, 0x1);

            value = readl(SUNXI_RCPUCFG_BASE + 0x0);
            value &= ~1;
            writel(value, SUNXI_RCPUCFG_BASE + 0x0);

            memcpy((void *) SCP_SRAM_BASE, (void *) ar100code_bin, CPUS_CODE_LENGTH);
            memcpy((void *) (SCP_SRAM_BASE + CPUS_VECTOR_LENGTH), (char *) ar100code_bin + CPUS_CODE_LENGTH, ar100code_bin_len - CPUS_CODE_LENGTH);
            asm volatile("dsb");

            value = readl(SUNXI_RCPUCFG_BASE + 0x0);
            value &= ~1;
            writel(value, SUNXI_RCPUCFG_BASE + 0x0);
            value = readl(SUNXI_RCPUCFG_BASE + 0x0);
            value |= 1;
            writel(value, SUNXI_RCPUCFG_BASE + 0x0);
            while (1)
                asm volatile("WFI");
        } else {
            rtc_write_data(RTC_DATA_COLD_START, 0x0);
        }
    }

    return 0;
}