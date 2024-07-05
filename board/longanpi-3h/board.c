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
#include <sys-sid.h>
#include <sys-sdcard.h>
#include <sys-spi.h>
#include <sys-uart.h>

sunxi_serial_t uart_dbg = {
        .base = SUNXI_UART0_BASE,
        .id = 0,
        .gpio_tx = {GPIO_PIN(GPIO_PORTH, 0), GPIO_PERIPH_MUX2},
        .gpio_rx = {GPIO_PIN(GPIO_PORTH, 1), GPIO_PERIPH_MUX2},
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
        .gpio_scl = {GPIO_PIN(GPIO_PORTL, 0), GPIO_PERIPH_MUX3},
        .gpio_sda = {GPIO_PIN(GPIO_PORTL, 1), GPIO_PERIPH_MUX3},
};

void neon_enable(void) {
    /* set NSACR, both Secure and Non-secure access are allowed to NEON */
    asm volatile("MRC p15, 0, r0, c1, c1, 2");
    asm volatile("ORR r0, r0, #(0x3<<10) @ enable fpu/neon");
    asm volatile("MCR p15, 0, r0, c1, c1, 2");
    /* Set the CPACR for access to CP10 and CP11*/
    asm volatile("LDR r0, =0xF00000");
    asm volatile("MCR p15, 0, r0, c1, c0, 2");
    /* Set the FPEXC EN bit to enable the FPU */
    asm volatile("MOV r3, #0x40000000");
    /*@VMSR FPEXC, r3*/
    asm volatile("MCR p10, 7, r3, c8, c0, 0");
}

const uint32_t dram_para[32] = {
        0x2d0,     // dram_para[0]
        0x8,       // dram_para[1]
        0xc0c0c0c, // dram_para[2]
        0xe0e0e0e, // dram_para[3]
        0xa0e,     // dram_para[4]
        0x7887ffff,// dram_para[5]
        0x30fa,    // dram_para[6]
        0x4000000, // dram_para[7]
        0x0,       // dram_para[8]
        0x34,      // dram_para[9]
        0x1b,      // dram_para[10]
        0x33,      // dram_para[11]
        0x3,       // dram_para[12]
        0x0,       // dram_para[13]
        0x0,       // dram_para[14]
        0x4,       // dram_para[15]
        0x72,      // dram_para[16]
        0x0,       // dram_para[17]
        0x9,       // dram_para[18]
        0x0,       // dram_para[19]
        0x0,       // dram_para[20]
        0x24,      // dram_para[21]
        0x0,       // dram_para[22]
        0x0,       // dram_para[23]
        0x0,       // dram_para[24]
        0x0,       // dram_para[25]
        0x39808080,// dram_para[26]
        0x402f6603,// dram_para[27]
        0x20262620,// dram_para[28]
        0xe0e0f0f, // dram_para[29]
        0x1024,    // dram_para[30]
        0x0        // dram_para[31]
};

void set_cpu_down(unsigned int cpu) {
    clrbits_le32(SUNXI_CPUXCFG_BASE + SUNXI_DBG_REG1, 1 << cpu);
    udelay(10);

    setbits_le32(SUNXI_CPUXCFG_BASE + SUNXI_CLUSTER_PWROFF_GATING, 1 << cpu);
    udelay(20);

    clrbits_le32(SUNXI_CPUXCFG_BASE + SUNXI_CPU_RST_CTRL, 1 << cpu);
    udelay(10);

    printk_debug("CPU: Power-down cpu-%d ok.\n", cpu);
}

void set_cpu_poweroff(void) {
    if (((readl(SUNXI_SID_BASE + 0x248) >> 29) & 0x1) == 1) {
        set_cpu_down(2); /*power of cpu2*/
        set_cpu_down(3); /*power of cpu3*/
    }
}

void clean_syterkit_data(void) {
    /* Disable MMU, data cache, instruction cache, interrupts */
    arm32_mmu_disable();
    printk_info("disable mmu ok...\n");
    arm32_dcache_disable();
    printk_info("disable dcache ok...\n");
    arm32_icache_disable();
    printk_info("disable icache ok...\n");
    arm32_interrupt_disable();
    printk_info("free interrupt ok...\n");
}
