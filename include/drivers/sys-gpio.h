/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SYS_GPIO_H__
#define __SYS_GPIO_H__

#include <inttypes.h>
#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#ifdef __cplusplus
extern "C" { 
#endif // __cplusplus

enum {
    GPIO_INPUT = 0,
    GPIO_OUTPUT = 1,
    GPIO_PERIPH_MUX2 = 2,
    GPIO_PERIPH_MUX3 = 3,
    GPIO_PERIPH_MUX4 = 4,
    GPIO_PERIPH_MUX5 = 5,
    GPIO_PERIPH_MUX6 = 6,
    GPIO_PERIPH_MUX7 = 7,
    GPIO_PERIPH_MUX8 = 8,
    GPIO_PERIPH_MUX14 = 14,
    GPIO_DISABLED = 0xf,
};

enum {
    GPIO_PORTA = 0,
    GPIO_PORTB = 1,
    GPIO_PORTC = 2,
    GPIO_PORTD = 3,
    GPIO_PORTE = 4,
    GPIO_PORTF = 5,
    GPIO_PORTG = 6,
    GPIO_PORTH = 7,
    GPIO_PORTI = 8,
    GPIO_PORTJ = 9,
    GPIO_PORTK = 10,
    GPIO_PORTL = 11,
    GPIO_PORTM = 12,
    GPIO_PORTN = 13,
};


enum gpio_pull_t {
    GPIO_PULL_UP = 0,
    GPIO_PULL_DOWN = 1,
    GPIO_PULL_NONE = 2,
};

#ifndef CONFIG_CHIP_GPIO_V1
struct sunxi_gpio {
    uint32_t cfg[4];
    uint32_t dat;
    uint32_t drv[4];
    uint32_t pull[3];
};
#else
struct sunxi_gpio {
    uint32_t cfg[4];
    uint32_t dat;
    uint32_t drv[2];
    uint32_t pull[2];
};
#endif

/* gpio interrupt control */
struct sunxi_gpio_int {
    uint32_t cfg[3];
    uint32_t ctl;
    uint32_t sta;
    uint32_t deb; /* interrupt debounce */
};

#define SUNXI_GPIO_BANKS 10
#define SUNXI_GPIO_BANK_SIZE 32
#define PIO_NUM_IO_BITS 5

struct sunxi_gpio_reg {
    struct sunxi_gpio gpio_bank[SUNXI_GPIO_BANKS];
    uint8_t res[0xbc];
    struct sunxi_gpio_int gpio_int;
};

#define BANK_TO_GPIO(bank) (((bank) < GPIO_PORTL) ? &((struct sunxi_gpio_reg *) SUNXI_PIO_BASE)->gpio_bank[bank] : &((struct sunxi_gpio_reg *) SUNXI_R_PIO_BASE)->gpio_bank[(bank) -GPIO_PORTL])

#define GPIO_BANK(pin) ((pin) >> PIO_NUM_IO_BITS)

#define GPIO_CFG_INDEX(pin) (((pin) &0x1f) >> 3)
#define GPIO_CFG_OFFSET(pin) ((((pin) &0x1f) & 0x7) << 2)

#define GPIO_DRV_INDEX(pin) (((pin) &0x1f) >> 4)
#define GPIO_DRV_OFFSET(pin) ((((pin) &0x1f) & 0xf) << 1)

#define GPIO_PULL_INDEX(pin) (((pin) &0x1f) >> 4)
#define GPIO_PULL_OFFSET(pin) ((((pin) &0x1f) & 0xf) << 1)

typedef uint32_t gpio_t;

typedef struct {
    gpio_t pin;
    uint8_t mux;
} gpio_mux_t;

#define GPIO_PIN(x, y) ((x) * SUNXI_GPIO_BANK_SIZE + (y))

extern void sunxi_gpio_init(gpio_t pin, int cfg);

extern void sunxi_gpio_set_value(gpio_t pin, int value);

extern int sunxi_gpio_read(gpio_t pin);

extern void sunxi_gpio_set_pull(gpio_t pin, enum gpio_pull_t pull);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif