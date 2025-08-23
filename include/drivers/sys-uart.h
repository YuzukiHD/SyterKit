#ifndef __SYS_UART_H__
#define __SYS_UART_H__

#include <types.h>

#include <sys-clk.h>
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

/*
 * Define a structure sunxi_serial_reg_t for accessing serial registers.
 * This structure provides a convenient way to access various registers
 * associated with a serial interface.
 */
typedef struct {
	union {
		volatile uint32_t rbr; /* Receiver Buffer Register (offset 0) */
		volatile uint32_t thr; /* Transmitter Holding Register (offset 0) */
		volatile uint32_t dll; /* Divisor Latch LSB (offset 0) */
	};
	union {
		volatile uint32_t ier; /* Interrupt Enable Register (offset 1) */
		volatile uint32_t dlh; /* Divisor Latch MSB (offset 1) */
	};
	union {
		volatile uint32_t fcr; /* FIFO Control Register (offset 2) */
		volatile uint32_t iir; /* Interrupt Identification Register (offset 2) */
	};
	volatile uint32_t lcr; /* Line Control Register (offset 3) */
	volatile uint32_t mcr; /* Modem Control Register (offset 4) */
	volatile uint32_t lsr; /* Line Status Register (offset 5) */
	volatile uint32_t msr; /* Modem Status Register (offset 6) */
	volatile uint32_t sch; /* Scratch Register (offset 7) */
} sunxi_serial_reg_t;

typedef struct {
	gpio_mux_t gpio_tx; /* GPIO pin for data transmission */
	gpio_mux_t gpio_rx; /* GPIO pin for data reception */
} sunxi_serial_pin_t;

/* Define a structure sunxi_serial_t for serial configuration */
typedef struct {
	uint32_t base; /* Base address of the serial device */
	uint8_t id;	   /* ID of the serial device */
	sunxi_clk_t uart_clk;
	sunxi_serial_pin_t gpio_pin;
	sunxi_serial_baudrate_t baud_rate; /* Baud rate configuration */
	sunxi_serial_parity_t parity;	   /* Parity configuration */
	sunxi_serial_stop_bit_t stop;	   /* Stop bit configuration */
	sunxi_serial_dlen_t dlen;		   /* Data length configuration */
} sunxi_serial_t;

#define SERIAL_DEFAULT_CLK_RST_OFFSET(x) (x + 16)
#define SERIAL_DEFAULT_CLK_GATE_OFFSET(x) (x)

#define SERIAL_DEFAULT_PARENT_CLK (24000000)

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