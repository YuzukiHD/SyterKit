/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYS_PWM_H__
#define __SYS_PWM_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <sys-clk.h>
#include <sys-gpio.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

typedef enum {
	PWM_PIER = 0x00, /* PWM IRQ enable register */
	PWM_PISR = 0x04, /* PWM IRQ status register */
	PWM_CIER = 0x10, /* PWM capture IRQ enable register */
	PWM_CISR = 0x14, /* PWM capture IRQ status register */

	PWM_PCCR_BASE = 0x20, /* Base address for PWM clock configuration registers */
	PWM_PCCR01 = 0x20,	  /* PWM01 clock configuration register */
	PWM_PCCR23 = 0x24,	  /* PWM23 clock configuration register */
	PWM_PCCR45 = 0x28,	  /* PWM45 clock configuration register */
	PWM_PCCR67 = 0x2C,	  /* PWM67 clock configuration register */
	PWM_PCCR89 = 0x30,	  /* PWM89 clock configuration register */
	PWM_PCCRab = 0x34,	  /* PWMab clock configuration register */
	PWM_PCCRcd = 0x38,	  /* PWMcd clock configuration register */
	PWM_PCCRef = 0x3C,	  /* PWMef clock configuration register */

	PWM_PCGR = 0x40, /* PWM Clock Gating Register */

	PWM_PDZCR_BASE = 0x60, /* Base address for PWM Dead Zone Control registers */
	PWM_PDZCR01 = 0x60,	   /* PWM01 Dead Zone Control Register */
	PWM_PDZCR23 = 0x64,	   /* PWM23 Dead Zone Control Register */
	PWM_PDZCR45 = 0x68,	   /* PWM45 Dead Zone Control Register */
	PWM_PDZCR67 = 0x6C,	   /* PWM67 Dead Zone Control Register */
	PWM_PDZCR89 = 0x70,	   /* PWM89 Dead Zone Control Register */
	PWM_PDZCRab = 0x74,	   /* PWMad Dead Zone Control Register */
	PWM_PDZCRcd = 0x78,	   /* PWMcd Dead Zone Control Register */
	PWM_PDZCRef = 0x7C,	   /* PWMef Dead Zone Control Register */

	PWM_PER = 0x80, /* PWM Enable Register */

	PWM_PGR0 = 0x90, /* PWM Group0 Register */
	PWM_PGR1 = 0x94, /* PWM Group1 Register */
	PWM_PGR2 = 0x98, /* PWM Group2 Register */
	PWM_PGR3 = 0x9C, /* PWM Group3 Register */

	PWM_CER = 0xC0, /* PWM Capture Enable Register */

	PWM_PCR = 0x0100,	 /* PWM Control Register */
	PWM_PPR = 0x0104,	 /* PWM Period Register */
	PWM_PCNTR = 0x0108,	 /* PWM Counter Register */
	PWM_PPCNTR = 0x010C, /* PWM Pulse Counter Register */
	PWM_CCR = 0x0110,	 /* Capture Control Register */
	PWM_CRLR = 0x0114,	 /* Capture Rise Lock Register */
	PWM_CFLR = 0x0118,	 /* Capture Fall Lock Register */

	PWM_VR = 0x03F0, /* PWM Version Register */
} sunxi_pwm_reg_offset_t;

#define PWM_REG_CHN_OFFSET 0x20

#define PWM_CLK_SRC_SHIFT 0x7
#define PWM_CLK_SRC_WIDTH 0x2

#define PWM_DIV_M_SHIFT 0x0
#define PWM_DIV_M_WIDTH 0x4

#define PWM_PRESCAL_SHIFT 0x0
#define PWM_PRESCAL_WIDTH 0x8

#define PWM_ACT_CYCLES_SHIFT 0x0
#define PWM_ACT_CYCLES_WIDTH 0x10

#define PWM_PERIOD_CYCLES_SHIFT 0x10
#define PWM_PERIOD_CYCLES_WIDTH 0x10

#define PWM_CLK_GATING_SHIFT 0x0
#define PWM_CLK_GATING_WIDTH 0x1

#define PWM_EN_CONTROL_SHIFT 0x0
#define PWM_EN_CONTORL_WIDTH 0x1

#define PWM_ACT_STA_SHIFT 0x8
#define PWM_ACT_STA_WIDTH 0x1

#define PWM_DZ_EN_SHIFT 0x0
#define PWM_DZ_EN_WIDTH 0x1

#define PWM_PDZINTV_SHIFT 0x8
#define PWM_PDZINTV_WIDTH 0x8

#define PWM_PULSE_SHIFT 0x9
#define PWM_PULSE_WIDTH 0x1

#define PWM_PULSE_NUM_SHIFT 0x10
#define PWM_PULSE_NUM_WIDTH 0x10

#define PWM_PULSE_START_SHIFT 0xa
#define PWM_PULSE_START_WIDTH 0x1

#define PWM_CLK_BYPASS_SHIFT 0x10

#define TIME_1_SECOND 1000000000
#define PWM_BIND_NUM (2)

/**
 * @brief PWM operation modes.
 * 
 * This enum defines the modes of operation for the PWM signal.
 * - PWM_MODE_CYCLE: PWM signal operates in a continuous cycle.
 * - PWM_MODE_PLUSE: PWM signal generates a pulse waveform.
 */
typedef enum {
	PWM_MODE_CYCLE = 0, /**< PWM operates in cycle mode. */
	PWM_MODE_PLUSE = 1, /**< PWM operates in pulse mode. */
} sunxi_pwm_mode_t;

/**
 * @brief PWM signal polarity.
 * 
 * This enum defines the polarity of the PWM signal.
 * - PWM_POLARITY_INVERSED: Inverted polarity for the PWM signal.
 * - PWM_POLARITY_NORMAL: Normal polarity for the PWM signal.
 */
typedef enum {
	PWM_POLARITY_INVERSED = 0, /**< Inverted PWM signal polarity. */
	PWM_POLARITY_NORMAL = 1,   /**< Normal PWM signal polarity. */
} sunxi_pwm_polarity_t;

/**
 * @brief PWM clock source.
 * 
 * This enum defines the available clock sources for the PWM module.
 * - PWM_CLK_SRC_OSC: Clock sourced from the oscillator.
 * - PWM_CLK_SRC_APB: Clock sourced from the APB bus.
 */
typedef enum {
	PWM_CLK_SRC_OSC = 0, /**< Clock source is the oscillator. */
	PWM_CLK_SRC_APB = 1, /**< Clock source is the APB bus. */
} sunxi_pwm_source_t;

/**
 * @brief PWM channel modes.
 * 
 * This enum defines the operating modes for PWM channels.
 * - PWM_CHANNEL_SINGLE: Single-channel mode where each PWM channel operates independently.
 * - PWM_CHANNEL_BIND: Multi-channel bind mode where multiple channels can be synchronized.
 */
typedef enum {
	PWM_CHANNEL_SINGLE = 0, /**< Single-channel PWM mode. */
	PWM_CHANNEL_BIND = 1,	/**< Multi-channel bind mode. */
} sunxi_pwm_channel_mode_t;

/**
 * @brief PWM channel configuration.
 * 
 * This structure defines the configuration of a PWM channel.
 * It includes the GPIO pin, the bind channel for multi-channel operation,
 * the dead time between signal transitions, and the channel mode.
 */
typedef struct sunxi_pwm_channel {
	gpio_mux_t pin;						   /**< GPIO pin used for the PWM signal. */
	uint32_t bind_channel;				   /**< The bind channel ID for multi-channel synchronization. */
	uint32_t dead_time;					   /**< Dead time (in nanoseconds) between signal transitions. */
	sunxi_pwm_channel_mode_t channel_mode; /**< The mode of the PWM channel. */
} sunxi_pwm_channel_t;

/**
 * @brief PWM clock source configuration.
 * 
 * This structure holds the configuration of the clock sources for the PWM module.
 * It contains fields for the oscillator and APB clock sources.
 */
typedef struct sunxi_pwm_clk_src {
	uint32_t clk_src_hosc; /**< The oscillator clock source. */
	uint32_t clk_src_apb;  /**< The APB clock source. */
} sunxi_pwm_clk_src_t;

/**
 * @brief Main PWM configuration structure.
 * 
 * This structure holds the complete configuration for the PWM module, including
 * the base address, channel settings, clock source settings, and the module's operational status.
 */
typedef struct sunxi_pwm {
	uint32_t base;				  /**< The base address of the PWM hardware module. */
	uint8_t id;					  /**< The PWM module ID. */
	sunxi_pwm_channel_t *channel; /**< Pointer to the array of PWM channels. */
	uint32_t channel_size;		  /**< The number of PWM channels. */
	sunxi_clk_t pwm_bus_clk;	  /**< Clock for the PWM bus. */
	sunxi_clk_t pwm_clk;		  /**< The main clock for the PWM module. */
	sunxi_pwm_clk_src_t clk_src;  /**< The clock sources for the PWM module. */
	bool status;				  /**< The operational status of the PWM module (enabled/disabled). */
} sunxi_pwm_t;

/**
 * @brief PWM configuration parameters.
 * 
 * This structure defines the parameters for configuring the PWM signal's behavior,
 * including the duty cycle, period, polarity, operating mode, and pulse count.
 */
typedef struct sunxi_pwm_config {
	uint32_t duty_ns;			   /**< The duty cycle duration in nanoseconds. */
	uint32_t period_ns;			   /**< The total period duration in nanoseconds. */
	sunxi_pwm_polarity_t polarity; /**< The polarity of the PWM signal. */
	sunxi_pwm_mode_t pwm_mode;	   /**< The mode of operation for the PWM signal. */
	uint32_t pluse_count;		   /**< The number of pulses in pulse mode operation. */
} sunxi_pwm_config_t;

/**
 * @brief Initialize the PWM instance.
 * 
 * This function initializes the PWM instance by setting up the necessary clocks and 
 * marking the PWM as initialized (status set to true).
 * 
 * @param pwm Pointer to the PWM instance structure.
 */
void sunxi_pwm_init(sunxi_pwm_t *pwm);

/**
 * @brief Deinitialize the PWM instance.
 * 
 * This function deinitializes the PWM instance by deactivating the clocks and
 * re-initializing the GPIOs for each channel. It also marks the PWM as uninitialized 
 * (status set to false).
 * 
 * @param pwm Pointer to the PWM instance structure.
 */
void sunxi_pwm_deinit(sunxi_pwm_t *pwm);

/**
 * @brief Set configuration for a PWM channel.
 * 
 * This function sets the configuration for the specified PWM channel. It checks if the PWM is initialized,
 * validates the channel index, and then applies the configuration either for a bound channel or a single channel
 * based on the current channel mode.
 *
 * @param pwm Pointer to the PWM instance structure.
 * @param channel The PWM channel to configure (0-based index).
 * @param config Pointer to the PWM configuration structure containing the settings.
 * 
 * @return 0 on success, -1 if an error occurs (e.g., PWM not initialized, invalid channel index).
 */
int sunxi_pwm_set_config(sunxi_pwm_t *pwm, int channel, sunxi_pwm_config_t *config);

/**
 * @brief Release the PWM channel.
 * 
 * This function releases the specified PWM channel. It checks if the PWM is initialized, 
 * validates the channel index, and then either releases a bound channel or a single channel 
 * based on the current channel mode.
 *
 * @param pwm Pointer to the PWM instance structure.
 * @param channel The PWM channel to release (0-based index).
 * 
 * @return 0 on success, -1 if an error occurs (e.g., PWM not initialized, invalid channel index).
 */
int sunxi_pwm_release(sunxi_pwm_t *pwm, int channel);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif//__SYS_PWM_H__