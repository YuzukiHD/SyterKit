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
        .reg = (sdhci_reg_t *) SUNXI_SMHC0_BASE,
        .voltage = MMC_VDD_27_36,
        .width = MMC_BUS_WIDTH_4,
        .clock = MMC_CLK_100M,
        .removable = 0,
        .isspi = FALSE,
        .gpio_clk = {GPIO_PIN(GPIO_PORTF, 2), GPIO_PERIPH_MUX2},
        .gpio_cmd = {GPIO_PIN(GPIO_PORTF, 3), GPIO_PERIPH_MUX2},
        .gpio_d0 = {GPIO_PIN(GPIO_PORTF, 1), GPIO_PERIPH_MUX2},
        .gpio_d1 = {GPIO_PIN(GPIO_PORTF, 0), GPIO_PERIPH_MUX2},
        .gpio_d2 = {GPIO_PIN(GPIO_PORTF, 5), GPIO_PERIPH_MUX2},
        .gpio_d3 = {GPIO_PIN(GPIO_PORTF, 4), GPIO_PERIPH_MUX2},
};

sunxi_i2c_t i2c_pmu = {
        .base = SUNXI_R_TWI0_BASE,
        .id = SUNXI_R_I2C0,
        .speed = 4000000,
        .gpio_scl = {GPIO_PIN(GPIO_PORTL, 0), GPIO_PERIPH_MUX2},
        .gpio_sda = {GPIO_PIN(GPIO_PORTL, 1), GPIO_PERIPH_MUX2},
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
    uint32_t val = 0;

    /* set detection threshold to 2.9V */
    val = readl(SUNXI_RTC_BASE + VDD_OFF_GATING_CTRL_REG);
    val &= ~(VCCIO_THRESHOLD_MASK << 4);
    val |= (VCCIO_THRESHOLD_VOLTAGE_2_9);
    writel(val, SUNXI_RTC_BASE + VDD_OFF_GATING_CTRL_REG);

    /* enable vccio debonce */
    val = readl(SUNXI_RTC_BASE + VDD_OFF_GATING_CTRL_REG);
    val |= DEBOUNCE_NO_BYPASS;
    writel(val, SUNXI_RTC_BASE + VDD_OFF_GATING_CTRL_REG);

    /* enable vccio detecter output */
    val = readl(SUNXI_RTC_BASE + VDD_OFF_GATING_CTRL_REG);
    val |= FORCE_DETECTER_OUTPUT;
    writel(val, SUNXI_RTC_BASE + VDD_OFF_GATING_CTRL_REG);

    /* enbale vccio detect */
    val = readl(SUNXI_RTC_BASE + VDD_OFF_GATING_CTRL_REG);
    val &= ~VCCIO_DET_BYPASS_EN;
    writel(val, SUNXI_RTC_BASE + VDD_OFF_GATING_CTRL_REG);
}

void set_rpio_power_mode(void) {
    uint32_t reg_val = read32(SUNXI_R_GPIO_BASE + 0x348);
    if (reg_val & 0x1) {
        printk(LOG_LEVEL_DEBUG, "PL gpio voltage : 1.8V \n");
        write32(SUNXI_R_GPIO_BASE + 0x340, 0x1);
    } else {
        printk(LOG_LEVEL_DEBUG, "PL gpio voltage : 3.3V \n");
    }
}

int sunxi_nsi_init(void) {
    /* IOMMU prio 3 */
    writel(0x1, 0x02021418);
    writel(0xf, 0x02021414);
    /* DE prio 2 */
    writel(0x1, 0x02021a18);
    writel(0xa, 0x02021a14);
    /* VE R prio 2 */
    writel(0x1, 0x02021618);
    writel(0xa, 0x02021614);
    /* VE RW prio 2 */
    writel(0x1, 0x02021818);
    writel(0xa, 0x02021814);
    /* ISP prio 2 */
    writel(0x1, 0x02020c18);
    writel(0xa, 0x02020c14);
    /* CSI prio 2 */
    writel(0x1, 0x02021c18);
    writel(0xa, 0x02021c14);
    /* NPU prio 2 */
    writel(0x1, 0x02020a18);
    writel(0xa, 0x02020a14);

    /* close ra0 autogating */
    writel(0x0, 0x02023c00);
    /* close ta autogating */
    writel(0x0, 0x02023e00);
    /* close pcie autogating */
    writel(0x0, 0x02020600);
    return 0;
}

void enable_sram_a3() {
    uint32_t reg_val;

    /* De-assert PUBSRAM Clock and Gating */
    reg_val = readl(RISCV_PUBSRAM_CFG_REG);
    reg_val |= RISCV_PUBSRAM_RST;
    reg_val |= RISCV_PUBSRAM_GATING;
    writel(reg_val, RISCV_PUBSRAM_CFG_REG);

    /* assert */
    writel(0, RISCV_CFG_BGR_REG);
}

void show_chip() {
    uint32_t chip_sid[4];
    chip_sid[0] = read32(SUNXI_SID_SRAM_BASE + 0x0);
    chip_sid[1] = read32(SUNXI_SID_SRAM_BASE + 0x4);
    chip_sid[2] = read32(SUNXI_SID_SRAM_BASE + 0x8);
    chip_sid[3] = read32(SUNXI_SID_SRAM_BASE + 0xc);

    printk(LOG_LEVEL_INFO, "Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);

    uint32_t chip_markid_sid = chip_sid[0] & 0xffff;

    switch (chip_markid_sid) {
        case 0x5f30:
            printk(LOG_LEVEL_INFO, "Chip type = T527M00X0DCH");
            break;
        case 0x5500:
            printk(LOG_LEVEL_INFO, "Chip type = MR527M02X0D00");
            break;
        default:
            printk(LOG_LEVEL_INFO, "Chip type = UNKNOW");
            break;
    }

    uint32_t version = read32(SUNXI_SYSCTRL_BASE + 0x24) & 0x7;
    printk(LOG_LEVEL_MUTE, "\tChip Version = %x \n", version);
}