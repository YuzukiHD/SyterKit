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

#if defined(CONFIG_CHIP_GPIO_V1)
enum {
    GPIO_CFG0 = 0x00,
    GPIO_CFG1 = 0x04,
    GPIO_CFG2 = 0x08,
    GPIO_CFG3 = 0x0c,
    GPIO_DAT = 0x10,
    GPIO_DRV0 = 0x14,
    GPIO_DRV1 = 0x18,
    GPIO_PUL0 = 0x1c,
    GPIO_PUL1 = 0x20,
    GPIO_OFFSET = 0x24,
    GPIO_CFG_MASK = 0x7,
    GPIO_DRV_MASK = 0x3,
};
#else
enum {
    GPIO_CFG0 = 0x00,
    GPIO_CFG1 = 0x04,
    GPIO_CFG2 = 0x08,
    GPIO_CFG3 = 0x0c,
    GPIO_DAT = 0x10,
    GPIO_DRV0 = 0x14,
    GPIO_DRV1 = 0x18,
    GPIO_DRV2 = 0x1c,
    GPIO_DRV3 = 0x20,
    GPIO_PUL0 = 0x24,
    GPIO_PUL1 = 0x28,
    GPIO_OFFSET = 0x30,
    GPIO_CFG_MASK = 0xf,
    GPIO_DRV_MASK = 0x3,
};
#endif

static inline uint32_t _port_num(gpio_t pin) {
    return pin >> PIO_NUM_IO_BITS;
}

static uint32_t _port_base_get(gpio_t pin) {
    uint32_t port = pin >> PIO_NUM_IO_BITS;

    /* PL PM PN in R_PIO */
    if (port > 10) {
        return SUNXI_RPIO_BASE + (port - GPIO_PORTL) * GPIO_OFFSET;
    }
    /* PA PB PC PD PE PF PG PH PI PJ PK in PIO */
    return SUNXI_PIO_BASE + port * GPIO_OFFSET;
}

static inline uint32_t _pin_num(gpio_t pin) {
    return (pin & ((1 << PIO_NUM_IO_BITS) - 1));
}

void sunxi_gpio_init(gpio_t pin, int cfg) {
    uint32_t port_addr = _port_base_get(pin);
    uint32_t pin_num = _pin_num(pin);
    uint32_t addr;
    uint32_t val;

    addr = port_addr + GPIO_CFG0 + ((pin_num >> 3) << 2);
    val = read32(addr);
    val &= ~(0xf << ((pin_num & 0x7) << 2));
    val |= ((cfg & GPIO_CFG_MASK) << ((pin_num & 0x7) << 2));
    write32(addr, val);

    printk(LOG_LEVEL_TRACE, "GPIO: MUX pin = %d, num in bank = %d, addr = 0x%08x, val = 0x%08x, set cfg = %d\n",
           pin, pin_num, addr, read32(addr), cfg);
}

void sunxi_gpio_set_value(gpio_t pin, int value) {
    uint32_t port_addr = _port_base_get(pin);
    uint32_t pin_num = _pin_num(pin);
    uint32_t val;

    val = read32(port_addr + GPIO_DAT);
    val &= ~(1 << pin_num);
    val |= (!!value) << pin_num;
    write32(port_addr + GPIO_DAT, val);
}

int sunxi_gpio_read(gpio_t pin) {
    uint32_t port_addr = _port_base_get(pin);
    uint32_t pin_num = _pin_num(pin);
    uint32_t val;

    val = read32(port_addr + GPIO_DAT);
    return !!(val & (1 << pin_num));
}

void sunxi_gpio_set_pull(gpio_t pin, enum gpio_pull_t pull) {
    uint32_t port_addr = _port_base_get(pin);
    uint32_t pin_num = _pin_num(pin);
    uint32_t addr;
    uint32_t val, v;

    switch (pull) {
        case GPIO_PULL_UP:
            v = 0x1;
            break;

        case GPIO_PULL_DOWN:
            v = 0x2;
            break;

        case GPIO_PULL_NONE:
            v = 0x0;
            break;

        default:
            v = 0x0;
            break;
    }

    addr = port_addr + GPIO_PUL0 + ((pin_num >> 4) << 2);
    val = read32(addr);
    val &= ~(v << ((pin_num & 0xf) << 1));
    val |= (v << ((pin_num & 0xf) << 1));
    write32(addr, val);

    printk(LOG_LEVEL_TRACE, "GPIO: PULL pin = %d, addr = 0x%08x, val = 0x%08x, set pull = %d\n",
           pin_num, addr, read32(addr), v);
}