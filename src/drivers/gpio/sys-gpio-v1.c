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

static void sunxi_gpio_bank_init(struct sunxi_gpio *pio, int pin, uint32_t val) {
    uint32_t index = GPIO_CFG_INDEX(pin);
    uint32_t offset = GPIO_CFG_OFFSET(pin);
    uint32_t addr = (uint32_t) &pio->cfg[0];
    clrsetbits_le32(addr + index, 0xf << offset, val << offset);
    printk(LOG_LEVEL_TRACE, "GPIO: MUX pin = %d, index = 0x%08x, offset = 0x%08x, addr = 0x%08x, val = 0x%08x, set cfg = %d\n",
           pin, index, offset, addr, read32(addr), val);
}

void sunxi_gpio_init(gpio_t pin, int cfg) {
    uint32_t bank = GPIO_BANK(pin);
    struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

    sunxi_gpio_bank_init(pio, pin, cfg);
}

void sunxi_gpio_set_value(gpio_t pin, int value) {
    uint32_t bank = GPIO_BANK(pin);
    uint32_t index = GPIO_DRV_INDEX(pin);
    uint32_t offset = GPIO_DRV_OFFSET(pin);
    struct sunxi_gpio *pio = BANK_TO_GPIO(bank);
    
    clrsetbits_le32((uint32_t) &pio->drv[0] + index, 0x3 << offset, value << offset);
}

int sunxi_gpio_read(gpio_t pin) {
    uint32_t bank = GPIO_BANK(pin);
    uint32_t index = GPIO_PULL_INDEX(pin);
    struct sunxi_gpio *pio = BANK_TO_GPIO(bank);
    uint32_t addr = (uint32_t) &pio->dat;

    return 0;
}

void sunxi_gpio_set_pull(gpio_t pin, enum gpio_pull_t pull) {
    uint32_t bank = GPIO_BANK(pin);
    uint32_t index = GPIO_PULL_INDEX(pin);
    uint32_t offset = GPIO_PULL_OFFSET(pin);
    struct sunxi_gpio *pio = BANK_TO_GPIO(bank);
    uint32_t addr = (uint32_t) &pio->pull[0];
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
    clrsetbits_le32(addr + index, 0x3 << offset, val << offset);
    printk(LOG_LEVEL_TRACE, "GPIO: PULL pin = %d, index = 0x%08x, offset = 0x%08x, addr = 0x%08x, val = 0x%08x, set cfg = %d\n",
           pin, index, offset, addr, read32(addr), val);
}
