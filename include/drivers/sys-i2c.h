/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYS_I2C_H__
#define __SYS_I2C_H__

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

/**
 * @brief Structure representing the GPIO configuration for I2C.
 *
 * This structure holds the GPIO multiplexing configurations 
 * for the SCL (Serial Clock Line) and SDA (Serial Data Line) 
 * used in the I2C interface.
 */
typedef struct {
	gpio_mux_t gpio_scl; /**< GPIO configuration for the SCL line. */
	gpio_mux_t gpio_sda; /**< GPIO configuration for the SDA line. */
} sunxi_i2c_gpio_t;

/**
 * @brief Structure representing an I2C device configuration.
 *
 * This structure contains the configuration details for an I2C 
 * device, including its base address, ID, speed, GPIO settings, 
 * clock settings, and operational status.
 */
typedef struct {
	uint32_t base;		   /**< Base address of the I2C hardware registers. */
	uint8_t id;			   /**< ID of the I2C device. */
	uint32_t speed;		   /**< Desired I2C speed (in Hz). */
	sunxi_i2c_gpio_t gpio; /**< GPIO configuration for the I2C lines. */
	sunxi_clk_t i2c_clk;   /**< Clock configuration for the I2C device. */
	bool status;		   /**< Operational status of the I2C device. */
} sunxi_i2c_t;

/**
 * @brief Enumeration of I2C speeds.
 *
 * This enumeration defines the supported I2C speeds for the device.
 */
enum {
	SUNXI_I2C_SPEED_100K = 100000, /**< 100 kHz I2C speed. */
	SUNXI_I2C_SPEED_400K = 400000, /**< 400 kHz I2C speed. */
};

/**
 * @brief Enumeration of I2C device IDs.
 *
 * This enumeration defines the IDs for available I2C devices on the 
 * Sunxi platform, including regular and reserved IDs.
 */
enum {
	SUNXI_I2C0 = 0,	   /**< I2C device 0. */
	SUNXI_I2C1,		   /**< I2C device 1. */
	SUNXI_I2C2,		   /**< I2C device 2. */
	SUNXI_I2C3,		   /**< I2C device 3. */
	SUNXI_I2C4,		   /**< I2C device 4. */
	SUNXI_I2C5,		   /**< I2C device 5. */
	SUNXI_R_I2C0,	   /**< Reserved I2C device 0. */
	SUNXI_R_I2C1,	   /**< Reserved I2C device 1. */
	SUNXI_I2C_BUS_MAX, /**< Maximum number of I2C buses. */
};

/**
 * @brief Structure representing the registers of the Sunxi TWI (Two Wire Interface).
 *
 * This structure defines the various control and status registers 
 * for the I2C (Inter-Integrated Circuit) interface on the Sunxi platform.
 */
struct sunxi_twi_reg {
	volatile uint32_t addr;	  /**< Slave address register. */
	volatile uint32_t xaddr;  /**< Extended address register. */
	volatile uint32_t data;	  /**< Data register for sending and receiving data. */
	volatile uint32_t ctl;	  /**< Control register for managing I2C operations. */
	volatile uint32_t status; /**< Status register for monitoring the I2C state. */
	volatile uint32_t clk;	  /**< Clock configuration register. */
	volatile uint32_t srst;	  /**< Soft reset register for resetting the I2C controller. */
	volatile uint32_t eft;	  /**< Enhanced future technology control register. */
	volatile uint32_t lcr;	  /**< Line control register for managing line states. */
	volatile uint32_t dvfs;	  /**< Dynamic Voltage and Frequency Scaling control register. */
};


/* TWI extend address register */
/* 7:0bits for extend slave address */
#define TWI_XADDR_MASK (0xff)
/* 31:8bits reserved */

/* TWI Data register default is 0x0000_0000 */
/* 7:0bits for send or received */
#define TWI_DATA_MASK (0xff)

/* TWI Control Register Bit Fields & Masks, default value: 0x0000_0000*/
/* 1:0 bits reserved */
/* set 1 to send A_ACK,then low level on SDA */
#define TWI_CTL_ACK (0x1 << 2)
/* INT_FLAG,interrupt status flag: set '1' when interrupt coming */
#define TWI_CTL_INTFLG (0x1 << 3)
#define TWI_CTL_STP (0x1 << 4) /* M_STP,Automatic clear 0 */
#define TWI_CTL_STA (0x1 << 5) /* M_STA,atutomatic clear 0 */
/* BUS_EN, master mode should be set 1.*/
#define TWI_CTL_BUSEN (0x1 << 6)
#define TWI_CTL_INTEN (0x1 << 7) /* INT_EN */
/* 31:8 bit reserved */

/* TWI Clock Register Bit Fields & Masks,default value:0x0000_0000 */
/*
 * Fin is APB CLOCK INPUT;
 * Fsample = F0 = Fin/2^CLK_N;
 *  F1 = F0/(CLK_M+1);
 *
 * Foscl = F1/10 = Fin/(2^CLK_N * (CLK_M+1)*10);
 * Foscl is clock SCL;standard mode:100KHz or fast mode:400KHz
 */
#define TWI_CLK_DUTY_30_EN (0x1 << 8) /* 8bit  */
#define TWI_CLK_DUTY (0x1 << 7)		  /* 7bit  */
#define TWI_CLK_DIV_M (0xf << 3)	  /* 6:3bit  */
#define TWI_CLK_DIV_N (0x7 << 0)	  /* 2:0bit */
#define TWI_LCR_WMASK (TWI_CTL_STA | TWI_CTL_STP | TWI_CTL_INTFLG)

/* CCU */
#define TWI_DEFAULT_CLK_RST_OFFSET(x) (x + 16)
#define TWI_DEFAULT_CLK_GATE_OFFSET(x) (x)

void sunxi_i2c_init(sunxi_i2c_t *i2c_dev);

/**
 * @brief sunxi_i2c write function
 *
 * @param i2c_dev Pointer to the sunxi_i2c controller device structure
 * @param addr Device address
 * @param reg Register to be read/written in the device
 * @param buffer Data to be written/read
 * @return int Number of status
 */
int sunxi_i2c_write(sunxi_i2c_t *i2c_dev, uint8_t addr, uint32_t reg, uint8_t data);

/**
 * @brief sunxi_i2c read function
 *
 * @param i2c_dev Pointer to the sunxi_i2c controller device structure
 * @param addr Device address
 * @param reg Register to be read from in the device
 * @param buffer Buffer to store the read data
 * @return int Number of status
 */
int sunxi_i2c_read(sunxi_i2c_t *i2c_dev, uint8_t addr, uint32_t reg, uint8_t *data);

/* status or interrupt source */
/*------------------------------------------------------------------------------
 * Code   Status
 * 00h	  Bus error
 * 08h	  START condition transmitted
 * 10h	  Repeated START condition transmitted
 * 18h	  Address + Write bit transmitted, ACK received
 * 20h	  Address + Write bit transmitted, ACK not received
 * 28h	  Data byte transmitted in master mode, ACK received
 * 30h	  Data byte transmitted in master mode, ACK not received
 * 38h	  Arbitration lost in address or data byte
 * 40h	  Address + Read bit transmitted, ACK received
 * 48h	  Address + Read bit transmitted, ACK not received
 * 50h	  Data byte received in master mode, ACK transmitted
 * 58h	  Data byte received in master mode, not ACK transmitted
 * 60h	  Slave address + Write bit received, ACK transmitted
 * 68h	  Arbitration lost in address as master, slave address + Write bit received, ACK transmitted
 * 70h	  General Call address received, ACK transmitted
 * 78h	  Arbitration lost in address as master, General Call address received, ACK transmitted
 * 80h	  Data byte received after slave address received, ACK transmitted
 * 88h	  Data byte received after slave address received, not ACK transmitted
 * 90h	  Data byte received after General Call received, ACK transmitted
 * 98h	  Data byte received after General Call received, not ACK transmitted
 * A0h	  STOP or repeated START condition received in slave mode
 * A8h	  Slave address + Read bit received, ACK transmitted
 * B0h	  Arbitration lost in address as master, slave address + Read bit received, ACK transmitted
 * B8h	  Data byte transmitted in slave mode, ACK received
 * C0h	  Data byte transmitted in slave mode, ACK not received
 * C8h	  Last byte transmitted in slave mode, ACK received
 * D0h	  Second Address byte + Write bit transmitted, ACK received
 * D8h	  Second Address byte + Write bit transmitted, ACK not received
 * F8h	  No relevant status information or no interrupt
 *-----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __SYS_I2C_H__