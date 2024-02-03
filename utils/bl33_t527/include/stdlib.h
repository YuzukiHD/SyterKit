#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

void sunxi_uart_init(uint32_t base);

void sunxi_uart_putc(char c);

void uart_printf(const char *fmt, ...);

void printf(const char *fmt, ...);

uint32_t time_ms(void);

uint64_t time_us(void);

void mdelay(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* __STDLIB_H__ */
