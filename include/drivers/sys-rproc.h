/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYS_RPROC_H__
#define __SYS_RPROC_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "reg/reg-rproc.h"

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

/**
 * @brief Initialize the HiFi4 clock.
 * 
 * This function sets up the clock for the HiFi4 module using the 
 * specified address for clock configuration.
 * 
 * @param addr The address used for clock initialization.
 */
void sunxi_hifi4_clock_init(uint32_t addr);

/**
 * @brief Start the HiFi4 module.
 * 
 * This function starts the HiFi4 module, enabling its operations 
 * and functionality.
 */
void sunxi_hifi4_start(void);

/**
 * @brief Reset the HiFi4 clock.
 * 
 * This function resets the HiFi4 clock to its default state, 
 * halting its operation temporarily.
 */
void sunxi_hifi4_clock_reset(void);

/**
 * @brief Initialize the E906 clock.
 * 
 * This function sets up the clock for the E906 module using the 
 * specified address for clock configuration.
 * 
 * @param addr The address used for clock initialization.
 */
void sunxi_e906_clock_init(uint32_t addr);

/**
 * @brief Reset the E906 clock.
 * 
 * This function resets the E906 clock to its default state, 
 * halting its operation temporarily.
 */
void sunxi_e906_clock_reset(void);

/**
 * @brief Initialize the E907 clock.
 * 
 * This function sets up the clock for the E907 module using the 
 * specified address for clock configuration.
 * 
 * @param addr The address used for clock initialization.
 */
void sunxi_e907_clock_init(uint32_t addr);

/**
 * @brief Reset the E907 clock.
 * 
 * This function resets the E907 clock to its default state, 
 * halting its operation temporarily.
 */
void sunxi_e907_clock_reset(void);

/**
 * @brief Initialize the C906 clock.
 * 
 * This function sets up the clock for the C906 module using the 
 * specified address for clock configuration.
 * 
 * @param addr The address used for clock initialization.
 */
void sunxi_c906_clock_init(uint32_t addr);

/**
 * @brief Reset the C906 clock.
 * 
 * This function resets the C906 clock to its default state, 
 * halting its operation temporarily.
 */
void sunxi_c906_clock_reset(void);

/**
 * @brief Initialize the E902 clock.
 * 
 * This function sets up the clock for the E902 module using the 
 * specified address for clock configuration.
 * 
 * @param addr The address used for clock initialization.
 */
void sunxi_e902_clock_init(uint32_t addr);

/**
 * @brief Reset the E902 clock.
 * 
 * This function resets the E902 clock to its default state, 
 * halting its operation temporarily.
 */
void sunxi_e902_clock_reset(void);

/**
 * @brief Initialize the A27L2 clock.
 * 
 * This function sets up the clock for the A27L2 module using the 
 * specified address for clock configuration.
 * 
 * @param addr The address used for clock initialization.
 */
void sunxi_a27l2_clock_init(uint32_t addr);

/**
 * @brief Reset the A27L2 clock.
 * 
 * This function resets the A27L2 clock to its default state, 
 * halting its operation temporarily.
 */
void sunxi_a27l2_clock_reset(void);

/**
 * @brief Initialize the A55 clock.
 * 
 * This function sets up the clock for the A55 module using the 
 * specified address for clock configuration.
 * 
 * @param addr The address used for clock initialization.
 */
void sunxi_a55_clock_init(uint32_t addr);

/**
 * @brief Reset the A55 clock.
 * 
 * This function resets the A55 clock to its default state, 
 * halting its operation temporarily.
 */
void sunxi_a55_clock_reset(void);

/**
 * @brief Initialize the A53 clock.
 * 
 * This function sets up the clock for the A53 module using the 
 * specified address for clock configuration.
 * 
 * @param addr The address used for clock initialization.
 */
void sunxi_a53_clock_init(uint32_t addr);

/**
 * @brief Reset the A53 clock.
 * 
 * This function resets the A53 clock to its default state, 
 * halting its operation temporarily.
 */
void sunxi_a53_clock_reset(void);


#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __SYS_RPROC_H__