#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

void sunxi_uart_init(uint32_t base);

void sunxi_uart_putc(char c);

int uart_printf(const char *fmt, ...);

int printf(const char *fmt, ...);

uint32_t time_ms(void);

void mdelay(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* __STDLIB_H__ */
