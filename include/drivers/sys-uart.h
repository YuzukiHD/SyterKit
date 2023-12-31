#ifndef __SYS_UART_H__
#define __SYS_UART_H__

#include <types.h>
#include <sys-gpio.h>

typedef struct {
    uint32_t base;
    uint8_t id;
    gpio_mux_t gpio_tx;
    gpio_mux_t gpio_rx;
} sunxi_serial_t;

/**
 * Initialize the Sunxi serial interface with the specified configuration.
 *
 * @param uart Pointer to the Sunxi serial interface structure.
 */
void sunxi_serial_init(sunxi_serial_t *uart);

/**
 * Send a character via the Sunxi serial interface.
 *
 * @param arg Pointer to the Sunxi serial interface argument.
 * @param c The character to send.
 */
void sunxi_serial_putc(void *arg, char c);

/**
 * Check if there is any character available for reading from the Sunxi serial interface.
 *
 * @param arg Pointer to the Sunxi serial interface argument.
 * @return 1 if there is a character available, 0 otherwise.
 */
int sunxi_serial_tstc(void *arg);

/**
 * Read a character from the Sunxi serial interface.
 *
 * @param arg Pointer to the Sunxi serial interface argument.
 * @return The character read from the interface.
 */
char sunxi_serial_getc(void *arg);


#endif // __SYS_UART_H__