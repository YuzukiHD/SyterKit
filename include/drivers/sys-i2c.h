/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SYS_I2C_H__
#define __SYS_I2C_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

typedef struct {
    uint32_t base;
    uint8_t id;
    uint32_t speed;
    gpio_mux_t gpio_scl;
    gpio_mux_t gpio_sda;
} sunxi_i2c_t;

enum {
	SUNXI_I2C0 = 0,
	SUNXI_I2C1,
	SUNXI_I2C2,
	SUNXI_I2C3,
	SUNXI_I2C4,
	SUNXI_I2C5,
	SUNXI_R_I2C0,
	SUNXI_R_I2C1,
	SUNXI_I2C_BUS_MAX,
};

struct sunxi_twi_reg {
    volatile uint32_t addr;   /* slave address     */
    volatile uint32_t xaddr;  /* extend address    */
    volatile uint32_t data;   /* data              */
    volatile uint32_t ctl;    /* control           */
    volatile uint32_t status; /* status            */
    volatile uint32_t clk;    /* clock             */
    volatile uint32_t srst;   /* soft reset        */
    volatile uint32_t eft;    /* enhanced future   */
    volatile uint32_t lcr;    /* line control      */
    volatile uint32_t dvfs;   /* dvfs control      */
};

#define TWI_CTL_ACK (0x1 << 2)
#define TWI_CTL_INTFLG (0x1 << 3)
#define TWI_CTL_STP (0x1 << 4)
#define TWI_CTL_STA (0x1 << 5)
#define TWI_CTL_BUSEN (0x1 << 6)
#define TWI_CTL_INTEN (0x1 << 7)
#define TWI_LCR_WMASK (TWI_CTL_STA | TWI_CTL_STP | TWI_CTL_INTFLG)

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

#endif// __SYS_I2C_H__