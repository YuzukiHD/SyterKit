#ifndef __SYS_UART_H__
#define __SYS_UART_H__

#include <types.h>

#include <sys-gpio.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

/* defines baud rate of the UART frame. */
typedef enum {
    UART_BAUDRATE_300 = 300,
    UART_BAUDRATE_600 = 600,
    UART_BAUDRATE_1200 = 1200,
    UART_BAUDRATE_2400 = 2400,
    UART_BAUDRATE_4800 = 4800,
    UART_BAUDRATE_9600 = 9600,
    UART_BAUDRATE_19200 = 19200,
    UART_BAUDRATE_38400 = 38400,
    UART_BAUDRATE_57600 = 57600,
    UART_BAUDRATE_115200 = 115200,
    UART_BAUDRATE_230400 = 230400,
    UART_BAUDRATE_460800 = 460800,
    UART_BAUDRATE_921600 = 921600,
    UART_BAUDRATE_1500000 = 1500000,
    UART_BAUDRATE_MAX,
} sunxi_serial_baudrate_t;


/* UART Line Control Parameter */
typedef enum {
    UART_PARITY_NO = 0,
    UART_PARITY_ODD,
    UART_PARITY_EVEN,
} sunxi_serial_parity_t;

/* UART Number of Stop Bit */
typedef enum {
    UART_STOP_BIT_0 = 0,
    UART_STOP_BIT_1,
} sunxi_serial_stop_bit_t;

/* UART Data Length */
typedef enum {
    UART_DLEN_5 = 0,
    UART_DLEN_6,
    UART_DLEN_7,
    UART_DLEN_8,
} sunxi_serial_dlen_t;

typedef struct {
    volatile uint32_t rbr; /* 0 */
    volatile uint32_t ier; /* 1 */
    volatile uint32_t fcr; /* 2 */
    volatile uint32_t lcr; /* 3 */
    volatile uint32_t mcr; /* 4 */
    volatile uint32_t lsr; /* 5 */
    volatile uint32_t msr; /* 6 */
    volatile uint32_t sch; /* 7 */
} sunxi_serial_reg_t;

#define thr rbr
#define dll rbr
#define dlh ier
#define iir fcr

typedef struct {
    uint32_t base;
    uint8_t id;
    sunxi_serial_baudrate_t baud_rate;
    sunxi_serial_parity_t parity;
    sunxi_serial_stop_bit_t stop;
    sunxi_serial_dlen_t dlen;
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

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __SYS_UART_H__