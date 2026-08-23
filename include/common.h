/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define ALIGN(size, align) (((size) + (align) -1) & (~((align) -1)))
#define OF_ALIGN(size) ALIGN(size, 4)

#ifndef NULL
#define NULL 0
#endif

#define FALSE 0
#define TRUE 1

typedef struct sunxi_rtc sunxi_rtc_t;

/**
 * @brief Reverse the byte order of a 32-bit value.
 * @param[in] data Value to convert.
 * @return Value with its four bytes reversed.
 */
static inline uint32_t swap_uint32(uint32_t data) {
	volatile uint32_t a, b, c, d;

	a = ((data) &0xff000000) >> 24;
	b = ((data) &0x00ff0000) >> 8;
	c = ((data) &0x0000ff00) << 8;
	d = ((data) &0x000000ff) << 24;

	return a | b | c | d;
}

/** @brief Stop execution after an unrecoverable runtime failure. */
void abort(void);

/** @brief Stop execution after a fatal firmware error. */
void panic(void);

/**
 * @brief Provide the freestanding C runtime signal stub.
 * @param[in] signum Signal number requested by the runtime.
 * @return Zero when the request has been consumed.
 */
int raise(int signum);

/** @brief Print the SyterKit build and version banner. */
void show_banner(void);

/** @brief Flush board state required before leaving SyterKit. */
void clean_syterkit_data(void);

/**
 * @brief Reset the current board through its platform reset register.
 *
 * This board-level function does not return after requesting the reset.
 */
void sys_reset(void);

/** @brief Enable the ARM NEON and floating-point execution units. */
void neon_enable(void);

/** @brief Configure the RTC spare register used for VCCIO detection. */
void rtc_set_vccio_det_spare(const sunxi_rtc_t *rtc);

/** @brief Configure the board's R_PIO power mode. */
void set_rpio_power_mode(void);

/**
 * @brief Initialize the SoC Network System Interconnect.
 * @return Zero on success, or a negative error code on failure.
 */
int sunxi_nsi_init(void);

/** @brief Configure GPIO power modes required by the board. */
void sunxi_gpio_power_mode_init(void);

/**
 * @brief Validate and configure the board's system LDO rails.
 */
void sys_ldo_check(void);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __COMMON_H__
