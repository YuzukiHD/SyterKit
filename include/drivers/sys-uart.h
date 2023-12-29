#ifndef __SYS_UART_H__
#define __SYS_UART_H__

#include <types.h>
#include <sys-gpio.h>

#ifdef __cplusplus
extern "C" { 
#endif // __cplusplus

typedef struct {
    uint32_t base;
    uint8_t id;
    gpio_mux_t gpio_tx;
    gpio_mux_t gpio_rx;
} sunxi_serial_t;

void sunxi_serial_init(sunxi_serial_t *uart);

void sunxi_serial_putc(void *arg, char c);

int sunxi_serial_tstc(void *arg);

char sunxi_serial_getc(void *arg);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __SYS_UART_H__