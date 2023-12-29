/* SPDX-License-Identifier: Apache-2.0 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <reg-ncat.h>
#include <sys-gpio.h>

static void sunxi_gpio_bank_init(struct sunxi_gpio *pio, int bank_offset, uint32_t val) {
    uint32_t index = GPIO_CFG_INDEX(bank_offset);
    uint32_t offset = GPIO_CFG_OFFSET(bank_offset);
    clrsetbits_le32((uint32_t) &pio->cfg[0] + index, 0xf << offset, val << offset);
    printk(LOG_LEVEL_DEBUG, "GPIO: index = 0x%08x, offset = 0x%08x, pio->cfg[0] = 0x%08x\n", index, offset, read32((uint32_t) &pio->cfg[0]));
}

void sunxi_gpio_init(gpio_t pin, int cfg) {
    uint32_t bank = GPIO_BANK(pin);
    struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

    printk(LOG_LEVEL_DEBUG, "GPIO: pin = %d, bank = %d, &pio->cfg[0] = 0x%08x\n", pin, bank, &pio->cfg[0]);

    sunxi_gpio_bank_init(pio, pin, cfg);
}

void sunxi_gpio_set_value(gpio_t pin, int value) {
    u32 bank = GPIO_BANK(pin);
    u32 index = GPIO_DRV_INDEX(pin);
    u32 offset = GPIO_DRV_OFFSET(pin);
    struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

    clrsetbits_le32((uint32_t) &pio->drv[0] + index, 0x3 << offset, value << offset);
}

static int sunxi_gpio_get_bank_cfg(struct sunxi_gpio *pio, int bank_offset) {
    uint32_t index = GPIO_CFG_INDEX(bank_offset);
    uint32_t offset = GPIO_CFG_OFFSET(bank_offset);
    uint32_t cfg;

    cfg = readl((uint32_t) &pio->cfg[0] + index);
    cfg >>= offset;

    return cfg & 0xf;
}

int sunxi_gpio_read(gpio_t pin) {
    uint32_t bank = GPIO_BANK(pin);
    struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

    return sunxi_gpio_get_bank_cfg(pio, pin);
}

void sunxi_gpio_set_pull(gpio_t pin, enum gpio_pull_t pull) {
    uint32_t bank = GPIO_BANK(pin);
    uint32_t index = GPIO_PULL_INDEX(pin);
    uint32_t offset = GPIO_PULL_OFFSET(pin);
    struct sunxi_gpio *pio = BANK_TO_GPIO(bank);
    uint32_t val;

    switch (pull) {
        case GPIO_PULL_UP:
            val = 0x1;
            break;

        case GPIO_PULL_DOWN:
            val = 0x2;
            break;

        case GPIO_PULL_NONE:
            val = 0x0;
            break;

        default:
            val = 0x0;
            break;
    }
    clrsetbits_le32((uint32_t) &pio->pull[0] + index, 0x3 << offset, val << offset);
}
