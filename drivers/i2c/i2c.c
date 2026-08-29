/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "i2c: " fmt

/**
 * @file
 * @brief System I2C (Inter-Integrated Circuit) driver for Allwinner (sunxi) platforms
 * @details This file implements I2C communication functionality for Allwinner SoCs,
 *          supporting standard I2C master operations including initialization,
 *          read/write transactions, clock configuration, and bus reset. The driver
 *          handles both 100kHz (standard) and 400kHz (fast) I2C speeds.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <common.h>
#include <log.h>

#include <drivers/clk/clk.h>

#include <drivers/i2c/i2c.h>
#include <driver.h>
#include <dt2c/driver.h>

/**
 * @def I2C_WRITE
 * @brief I2C write transaction type
 */
#define I2C_WRITE 0

/**
 * @def I2C_READ
 * @brief I2C read transaction type
 */
#define I2C_READ 1

/**
 * @def I2C_OK
 * @brief I2C operation successful status
 */
#define I2C_OK 0

/**
 * @def I2C_NOK
 * @brief I2C operation failed general status
 */
#define I2C_NOK 1

/**
 * @def I2C_NACK
 * @brief I2C operation failed with NACK status
 */
#define I2C_NACK 2

/**
 * @def I2C_NOK_LA
 * @brief I2C operation failed with lost arbitration status
 */
#define I2C_NOK_LA 3 /* Lost arbitration */

/**
 * @def I2C_NOK_TOUT
 * @brief I2C operation failed with timeout status
 */
#define I2C_NOK_TOUT 4 /* time out */

/* The TWI controller reports a bus error with a zero status code. */
#define I2C_BUS_ERROR 0x00

/**
 * @def I2C_START_TRANSMIT
 * @brief I2C status code for successful start condition transmission
 */
#define I2C_START_TRANSMIT 0x08

/**
 * @def I2C_RESTART_TRANSMIT
 * @brief I2C status code for successful repeated start condition transmission
 */
#define I2C_RESTART_TRANSMIT 0x10

/**
 * @def I2C_ADDRWRITE_ACK
 * @brief I2C status code for successful slave address write with ACK
 */
#define I2C_ADDRWRITE_ACK 0x18
#define I2C_ADDRWRITE_NACK 0x20

/**
 * @def I2C_ADDRREAD_ACK
 * @brief I2C status code for successful slave address read with ACK
 */
#define I2C_ADDRREAD_ACK 0x40
#define I2C_ADDRREAD_NACK 0x48

/**
 * @def I2C_DATAWRITE_ACK
 * @brief I2C status code for successful data write with ACK
 */
#define I2C_DATAWRITE_ACK 0x28
#define I2C_DATAWRITE_NACK 0x30
#define I2C_ARB_LOST 0x38

/**
 * @def I2C_READY
 * @brief I2C status code for bus ready state
 */
#define I2C_READY 0xf8

/**
 * @def I2C_DATAREAD_NACK
 * @brief I2C status code for successful data read with NACK
 */
#define I2C_DATAREAD_NACK 0x58

/**
 * @def I2C_DATAREAD_ACK
 * @brief I2C status code for successful data read with ACK
 */
#define I2C_DATAREAD_ACK 0x50

static int32_t sunxi_i2c_wait_status(sunxi_i2c_t *i2c_dev, uint32_t expected)
{
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *)i2c_dev->base;
	int32_t time = 0xffff;

	while (time--) {
		uint32_t status = i2c->status;
		if (status == expected)
			return I2C_OK;
		switch (status) {
		case I2C_BUS_ERROR:
			return -I2C_NOK;
		case I2C_ADDRWRITE_NACK:
		case I2C_ADDRREAD_NACK:
		case I2C_DATAWRITE_NACK:
			return -I2C_NACK;
		case I2C_ARB_LOST:
			return -I2C_NOK_LA;
		default:
			break;
		}
	}
	return -I2C_NOK_TOUT;
}

#ifdef I2C_DEBUG
__attribute__((unused)) static void i2c_debug(sunxi_i2c_t *i2c_dev)
{
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *)i2c_dev->base;
	pr_debug("i2c->addr  :\t0x%x:0x%x\n", &i2c->addr, i2c->addr);
	pr_debug("i2c->xaddr :\t0x%x:0x%x\n", &i2c->xaddr, i2c->xaddr);
	pr_debug("i2c->data  :\t0x%x:0x%x\n", &i2c->data, i2c->data);
	pr_debug("i2c->ctl   :\t0x%x:0x%x\n", &i2c->ctl, i2c->ctl);
	pr_debug("i2c->status:\t0x%x:0x%x\n", &i2c->status, i2c->status);
	pr_debug("i2c->clk   :\t0x%x:0x%x\n", &i2c->clk, i2c->clk);
	pr_debug("i2c->srst  :\t0x%x:0x%x\n", &i2c->srst, i2c->srst);
	pr_debug("i2c->eft   :\t0x%x:0x%x\n", &i2c->eft, i2c->eft);
	pr_debug("i2c->lcr   :\t0x%x:0x%x\n", &i2c->lcr, i2c->lcr);
	pr_debug("i2c->dvfs  :\t0x%x:0x%x\n", &i2c->dvfs, i2c->dvfs);
}
#endif

/**
 * @brief Send a byte address over I2C
 * @details Sends a single byte address to the I2C device and waits for acknowledgment.
 * @param i2c_dev Pointer to the I2C device structure
 * @param byteaddr Byte address to send
 * @return I2C_OK on success, negative error code on failure
 */
static int32_t sunxi_i2c_send_byteaddr(sunxi_i2c_t *i2c_dev, uint32_t byteaddr)
{
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *)i2c_dev->base;

	int32_t time = 0xffff;

	i2c->data = byteaddr & 0xff;
	i2c->ctl |= (0x01 << 3); /*write 1 to clean int flag*/

	while ((time--) && (!(i2c->ctl & 0x08)))
		;

	if (time <= 0) {
		return -I2C_NOK_TOUT;
	}

	return sunxi_i2c_wait_status(i2c_dev, I2C_DATAWRITE_ACK);
}

/**
 * @brief Generate I2C start condition
 * @details Generates a start condition on the I2C bus to initiate communication.
 * @param i2c_dev Pointer to the I2C device structure
 * @return I2C_OK on success, negative error code on failure
 */
static int32_t sunxi_i2c_send_start(sunxi_i2c_t *i2c_dev)
{
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *)i2c_dev->base;

	int32_t time = 0xffff;

	i2c->eft = 0;
	i2c->srst = 1;
	i2c->ctl |= TWI_CTL_STA;

	while ((time--) && (!(i2c->ctl & TWI_CTL_INTFLG)))
		;
	if (time <= 0) {
		return -I2C_NOK_TOUT;
	}

	return sunxi_i2c_wait_status(i2c_dev, I2C_START_TRANSMIT);
}

/**
 * @brief Send I2C slave address with read/write bit
 * @details Transmits the slave address along with read/write bit and waits for acknowledgment.
 * @param i2c_dev Pointer to the I2C device structure
 * @param saddr Slave device address
 * @param rw Read (1) or Write (0) operation
 * @return I2C_OK on success, negative error code on failure
 */
static int32_t sunxi_i2c_send_slave_addr(sunxi_i2c_t *i2c_dev, uint32_t saddr, uint32_t rw)
{
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *)i2c_dev->base;
	int32_t time = 0xffff;
	uint32_t expected;

	rw &= 1;
	i2c->data = ((saddr & 0xff) << 1) | rw;
	i2c->ctl |= TWI_CTL_INTFLG; /*write 1 to clean int flag*/

	while ((time--) && (!(i2c->ctl & TWI_CTL_INTFLG)))
		;

	if (time <= 0) {
		return -I2C_NOK_TOUT;
	}

	expected = rw == I2C_WRITE ? I2C_ADDRWRITE_ACK : I2C_ADDRREAD_ACK;
	return sunxi_i2c_wait_status(i2c_dev, expected);
}

/**
 * @brief Generate I2C repeated start condition
 * @details Generates a repeated start condition on the I2C bus to change operation mode.
 * @param i2c_dev Pointer to the I2C device structure
 * @return I2C_OK on success, negative error code on failure
 */
static int32_t sunxi_i2c_send_restart(sunxi_i2c_t *i2c_dev)
{
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *)i2c_dev->base;
	int32_t time = 0xffff;
	uint32_t tmp_val;
	tmp_val = i2c->ctl;

	tmp_val |= 0x20;
	i2c->ctl = tmp_val;

	while ((time--) && (!(i2c->ctl & 0x08)))
		;
	if (time <= 0) {
		return -I2C_NOK_TOUT;
	}

	return sunxi_i2c_wait_status(i2c_dev, I2C_RESTART_TRANSMIT);
}

/**
 * @brief Generate I2C stop condition
 * @details Generates a stop condition on the I2C bus to terminate communication.
 * @param i2c_dev Pointer to the I2C device structure
 * @return I2C_OK on success, negative error code on failure
 */
static int32_t sunxi_i2c_stop(sunxi_i2c_t *i2c_dev)
{
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *)i2c_dev->base;
	int32_t time = 0xffff;
	i2c->ctl |= (0x01 << 4);
	i2c->ctl |= (0x01 << 3);
	while ((time--) && (i2c->ctl & 0x10))
		;
	if (time <= 0) {
		return -I2C_NOK_TOUT;
	}
	return sunxi_i2c_wait_status(i2c_dev, I2C_READY);
}

static int32_t sunxi_i2c_receive_byte(sunxi_i2c_t *i2c_dev, uint8_t *data_addr, uint32_t expected)
{
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *)i2c_dev->base;
	int32_t time = 0xffff;
	int32_t ret;

	i2c->ctl |= TWI_CTL_INTFLG;
	while ((time--) && (!(i2c->ctl & TWI_CTL_INTFLG)))
		;
	if (time <= 0)
		return -I2C_NOK_TOUT;

	ret = sunxi_i2c_wait_status(i2c_dev, expected);
	if (ret)
		return ret;

	*data_addr = i2c->data;
	return I2C_OK;
}

/**
 * @brief Receive data from I2C device
 * @details Receives multiple bytes of data from an I2C device, handling ACK/NACK appropriately.
 * @param i2c_dev Pointer to the I2C device structure
 * @param data_addr Buffer to store received data
 * @param data_count Number of bytes to receive
 * @return I2C_OK on success, negative error code on failure
 */
static int32_t sunxi_i2c_get_data(sunxi_i2c_t *i2c_dev, uint8_t *data_addr, uint32_t data_count)
{
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *)i2c_dev->base;
	int32_t ret;
	uint32_t i;

	if (data_addr == NULL || data_count == 0)
		return -I2C_NOK;

	if (data_count > 1) {
		for (i = 0; i < data_count - 1; i++) {
			i2c->ctl |= TWI_CTL_ACK;
			ret = sunxi_i2c_receive_byte(i2c_dev, data_addr + i, I2C_DATAREAD_ACK);
			if (ret)
				return ret;
		}
		i2c->ctl &= ~TWI_CTL_ACK;
	}

	return sunxi_i2c_receive_byte(i2c_dev, data_addr + data_count - 1, I2C_DATAREAD_NACK);
}

/**
 * @brief Sends data to the I2C device.
 *
 * This function sends a specified number of bytes of data to the I2C 
 * device. It writes data to the data register and waits for the 
 * acknowledgment from the device for each byte sent. If a timeout occurs 
 * while waiting for acknowledgment, the function will return an error code.
 *
 * @param i2c_dev Pointer to the I2C device structure containing the 
 *                device's configuration and register base address.
 * @param data_addr Pointer to the data buffer that contains the data to 
 *                  be sent.
 * @param data_count The number of bytes to send from the data buffer.
 *
 * @return I2C_OK on success, or a negative error code on failure. 
 *         Possible error codes include:
 *         - I2C_NOK_TOUT: Timeout occurred while waiting for the I2C device 
 *           to acknowledge data.
 */
static int32_t sunxi_i2c_send_data(sunxi_i2c_t *i2c_dev, uint8_t *data_addr, uint32_t data_count)
{
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *)i2c_dev->base;
	int32_t time = 0xffff;
	int32_t ret = I2C_OK;
	uint32_t i;

	if (data_count != 0U && data_addr == NULL)
		return -I2C_NOK;

	for (i = 0; i < data_count; i++) {
		time = 0xffff;
		i2c->data = data_addr[i];
		i2c->ctl |= TWI_CTL_INTFLG;

		while ((time--) && (!(i2c->ctl & 0x08)))
			;
		if (time <= 0)
			return -I2C_NOK_TOUT;

		ret = sunxi_i2c_wait_status(i2c_dev, I2C_DATAWRITE_ACK);
		if (ret)
			return ret;
	}

	return ret;
}

/**
 * @brief Internal I2C read function
 * @details Performs a complete I2C read transaction, including start, address, register, and data transfer.
 * @param i2c_dev Pointer to the I2C device structure
 * @param chip Device address
 * @param addr Register address to read from
 * @param alen Length of the register address (1-3 bytes)
 * @param buffer Buffer to store read data
 * @param len Number of bytes to read
 * @return I2C_OK on success, negative error code on failure
 */
static int _sunxi_i2c_read(sunxi_i2c_t *i2c_dev, uint8_t chip, uint32_t addr, int alen, uint8_t *buffer, int len)
{
	int i, ret, addrlen;
	char *slave_reg;

	if (i2c_dev == NULL || buffer == NULL || len <= 0 || alen < 1 || alen > 3)
		return -I2C_NOK;

	ret = sunxi_i2c_send_start(i2c_dev);
	if (ret) {
		goto i2c_read_err_occur;
	}

	ret = sunxi_i2c_send_slave_addr(i2c_dev, chip, I2C_WRITE);
	if (ret) {
		goto i2c_read_err_occur;
	}

	/* Send the register address, most-significant byte first. */
	if (alen >= 3)
		addrlen = 2;
	else if (alen <= 1)
		addrlen = 0;
	else
		addrlen = 1;

	slave_reg = (char *)&addr;
	for (i = addrlen; i >= 0; i--) {
		ret = sunxi_i2c_send_byteaddr(i2c_dev, slave_reg[i] & 0xff);
		if (ret) {
			goto i2c_read_err_occur;
		}
	}

	ret = sunxi_i2c_send_restart(i2c_dev);
	if (ret) {
		goto i2c_read_err_occur;
	}

	ret = sunxi_i2c_send_slave_addr(i2c_dev, chip, I2C_READ);
	if (ret) {
		goto i2c_read_err_occur;
	}

	/* Receive data. */
	ret = sunxi_i2c_get_data(i2c_dev, buffer, len);
	if (ret) {
		goto i2c_read_err_occur;
	}

i2c_read_err_occur:
	sunxi_i2c_stop(i2c_dev);

	return ret;
}

/**
 * @brief Internal I2C write function
 * @details Performs a complete I2C write transaction, including start, address, register, and data transfer.
 * @param i2c_dev Pointer to the I2C device structure
 * @param chip Device address
 * @param addr Register address to write to
 * @param alen Length of the register address (1-3 bytes)
 * @param buffer Buffer containing data to write
 * @param len Number of bytes to write
 * @return I2C_OK on success, negative error code on failure
 */
static int _sunxi_i2c_write(sunxi_i2c_t *i2c_dev, uint8_t chip, uint32_t addr, int alen, uint8_t *buffer, int len)
{
	int i, ret, addrlen;
	char *slave_reg;

	if (i2c_dev == NULL || (buffer == NULL && len > 0) || len < 0 || alen < 1 || alen > 3)
		return -I2C_NOK;

	ret = sunxi_i2c_send_start(i2c_dev);
	if (ret) {
		goto i2c_write_err_occur;
	}

	ret = sunxi_i2c_send_slave_addr(i2c_dev, chip, I2C_WRITE);
	if (ret) {
		goto i2c_write_err_occur;
	}

	/* Send the register address, most-significant byte first. */
	if (alen >= 3) {
		addrlen = 2;
	} else if (alen <= 1) {
		addrlen = 0;
	} else {
		addrlen = 1;
	}

	slave_reg = (char *)&addr;
	for (i = addrlen; i >= 0; i--) {
		ret = sunxi_i2c_send_byteaddr(i2c_dev, slave_reg[i] & 0xff);
		if (ret) {
			goto i2c_write_err_occur;
		}
	}

	ret = sunxi_i2c_send_data(i2c_dev, buffer, len);
	if (ret) {
		goto i2c_write_err_occur;
	}

i2c_write_err_occur:
	sunxi_i2c_stop(i2c_dev);

	return ret;
}

/**
 * @brief Write a single byte to I2C device register
 * @details Writes a single byte to a specified register on an I2C device.
 * @param i2c_dev Pointer to the I2C device structure
 * @param addr I2C device address
 * @param reg Register address to write to
 * @param data Byte value to write
 * @return I2C_OK on success, I2C_NOK if I2C controller is not initialized
 */
int sunxi_i2c_write(sunxi_i2c_t *i2c_dev, uint8_t addr, uint32_t reg, uint8_t data)
{
	if (i2c_dev == NULL || !i2c_dev->status)
		return -I2C_NOK;

	return _sunxi_i2c_write(i2c_dev, addr, reg, 1, &data, 1);
}

/**
 * @brief Read a single byte from I2C device register
 * @details Reads a single byte from a specified register on an I2C device.
 * @param i2c_dev Pointer to the I2C device structure
 * @param addr I2C device address
 * @param reg Register address to read from
 * @param data Pointer to store the read byte
 * @return I2C_OK on success, I2C_NOK if I2C controller is not initialized
 */
int sunxi_i2c_read(sunxi_i2c_t *i2c_dev, uint8_t addr, uint32_t reg, uint8_t *data)
{
	if (i2c_dev == NULL || !i2c_dev->status)
		return -I2C_NOK;

	return _sunxi_i2c_read(i2c_dev, addr, reg, 1, data, 1);
}

/**
 * @brief Resets the I2C bus for the specified I2C device.
 *
 * This function attempts to reset the I2C control by toggling the SCL 
 * and SDA lines until the bus is idle. It first checks the control 
 * register to determine if a reset is needed, and then it performs 
 * the necessary toggling of the lines.
 *
 * @param i2c_dev Pointer to the I2C device structure that contains 
 *                the base address for the I2C hardware registers.
 *
 * @note The function uses a timeout mechanism to prevent infinite loops 
 *       in case the reset process fails.
 *
 * @warning Ensure that this function is called when the I2C device 
 *          is in a known state to avoid unintended behavior.
 *
 * @return I2C_OK when the controller is reset and the bus is idle;
 *         -I2C_NOK_TOUT when the controller or line-release sequence times out.
 */
static int sunxi_i2c_bus_reset(sunxi_i2c_t *i2c_dev)
{
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *)i2c_dev->base;
	int timeout = 0;

	/* Reset I2C control */
	timeout = 0xffff;
	i2c->eft = 0; /* clear error flags before the controller reset */
	i2c->srst = 1; // Initiate the reset
	while ((i2c->srst) && (timeout)) {
		timeout--;
	}
	if (i2c->srst != 0U)
		return -I2C_NOK_TOUT;

	if ((i2c->lcr & 0x30) != 0x30) {
		/* Toggle I2C SCL and SDA until bus is idle */
		i2c->lcr = 0x05;
		udelay(500);
		timeout = 10;
		while ((timeout > 0) && ((i2c->lcr & 0x02) != 2)) {
			/* Control SCL and SDA output high level */
			i2c->lcr |= 0x08;
			i2c->lcr |= 0x02;
			udelay(1000);
			/* Control SCL and SDA output low level */
			i2c->lcr &= ~0x08;
			i2c->lcr &= ~0x02;
			udelay(1000);
			timeout--;
		}
		if ((i2c->lcr & 0x02) != 2) {
			i2c->lcr = 0x0;
			return -I2C_NOK_TOUT;
		}
		i2c->lcr = 0x0; // Clear control register
		udelay(500);
	}

	return I2C_OK;
}

/**
 * @brief Configures the clock settings for the specified I2C device.
 *
 * This function calculates and sets the appropriate clock divider values
 * based on the desired I2C speed and the source clock frequency. It finds
 * suitable values for clk_m and clk_n, and adjusts the clock control register
 * accordingly.
 *
 * @param i2c_dev Pointer to the I2C device structure that contains 
 *                the base address for the I2C hardware registers and 
 *                clock configuration parameters.
 *
 * @note The function adjusts the clock settings to ensure the I2C speed 
 *       is achievable with the given source clock. It handles both 100kHz 
 *       and 400kHz configurations.
 *
 * @warning Ensure that the parent clock frequency is set correctly 
 *          before calling this function to avoid incorrect clock settings.
 *
 * @return I2C_OK when a valid divider is programmed; -I2C_NOK when the
 *         requested speed or source clock cannot be represented by the
 *         controller's divider fields.
 */
static int sunxi_i2c_set_clock(sunxi_i2c_t *i2c_dev)
{
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *)i2c_dev->base;
	uint32_t clk_m = 0, clk_n = 0, _2_pow_clk_n = 1, duty = 0, src_clk = 0;
	uint32_t divider, sclk_real; /* the real clock frequency */
	bool found = false;

	/* I2C_CLK = parent_clk / ( 2^CLK_N * (CLK_M + 1) *10) */
	if (i2c_dev->speed != SUNXI_I2C_SPEED_100K && i2c_dev->speed != SUNXI_I2C_SPEED_400K)
		return -I2C_NOK;
	if (i2c_dev->i2c_clk.parent_clk < 10U * i2c_dev->speed)
		return -I2C_NOK;
	src_clk = i2c_dev->i2c_clk.parent_clk / 10U;

	divider = src_clk / i2c_dev->speed; /* 400kHz or 100kHz */
	sclk_real = 0; /* the real clock frequency */

	/* Search for clk_n and clk_m values */
	if (divider == 0) {
		clk_m = 1;
		goto set_clk;
	}

	/* 3 bits max value is 8 */
	while (clk_n < 8) {
		/* (m+1)*2^n = divider --> m = divider/2^n - 1 */
		clk_m = (divider / _2_pow_clk_n) - 1;

		/* 4 bits max value is 16 */
		while (clk_m < 16) {
			/* Calculate real clock frequency */
			sclk_real = src_clk / (clk_m + 1) / _2_pow_clk_n;
			if (sclk_real <= i2c_dev->speed) {
				found = true;
				goto set_clk;
			} else {
				clk_m++;
			}
		}
		clk_n++;
		_2_pow_clk_n *= 2; /* Multiple by 2 */
	}
	if (!found)
		return -I2C_NOK;

set_clk:
	i2c->clk &= ~(TWI_CLK_DIV_M | TWI_CLK_DIV_N | TWI_CLK_DUTY | TWI_CLK_DUTY_30_EN);
	i2c->clk |= ((clk_m << 3) | clk_n);
	if (i2c_dev->speed == SUNXI_I2C_SPEED_400K) {
		duty = TWI_CLK_DUTY_30_EN;
		i2c->clk |= duty;
	} else {
		/* Standard mode uses the default 50/50 duty cycle. */
		duty = 0U;
	}

#ifdef I2C_DEBUG
	i2c_debug(i2c_dev);
#endif
	return I2C_OK;
}

/**
 * @brief Opens the clock for the specified I2C device.
 *
 * This function de-asserts the reset signal for the I2C clock 
 * and enables the clock gating. It first de-asserts the reset, 
 * then clears the clock gating bit, waits for a brief period, 
 * and finally re-enables the clock.
 *
 * @param i2c_dev Pointer to the I2C device structure that contains 
 *                the clock control registers' base address and offsets.
 *
 * @note The function uses a delay to ensure that the clock gating 
 *       changes take effect before proceeding.
 */
static inline void sunxi_i2c_bus_clk_open(sunxi_i2c_t *i2c_dev)
{
	/* Match SPL hal_clk_disable/enable: disable gate and assert reset,
	 * then release reset before enabling the gate. */
	clrbits_le32(i2c_dev->i2c_clk.gate_reg_base, BIT(i2c_dev->i2c_clk.gate_reg_offset));
	clrbits_le32(i2c_dev->i2c_clk.rst_reg_base, BIT(i2c_dev->i2c_clk.rst_reg_offset));
	udelay(10);
	setbits_le32(i2c_dev->i2c_clk.rst_reg_base, BIT(i2c_dev->i2c_clk.rst_reg_offset));
	setbits_le32(i2c_dev->i2c_clk.gate_reg_base, BIT(i2c_dev->i2c_clk.gate_reg_offset));
}

/**
 * @brief Enables the I2C bus for the specified I2C device.
 *
 * This function sets the control register to enable the I2C bus 
 * and clears any error flags by writing to the error flag register.
 *
 * @param i2c_dev Pointer to the I2C device structure that contains 
 *                the base address for the I2C hardware registers.
 *
 * @note This function should be called before performing any I2C 
 *       transactions to ensure that the bus is enabled.
 */
static inline void sunxi_i2c_bus_en(sunxi_i2c_t *i2c_dev)
{
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *)i2c_dev->base;

	i2c->ctl |= 0x40; // Set the control register to enable the I2C bus
	i2c->eft = 0; // Clear error flags
}

/**
 * @brief Initialize I2C controller and bus
 * @details Initializes the I2C controller by configuring GPIO pins, enabling clocks,
 *          resetting the bus, setting clock frequency, and enabling the I2C controller.
 * @param i2c_dev Pointer to the I2C device structure containing configuration parameters
 */
void sunxi_i2c_init(sunxi_i2c_t *i2c_dev)
{
	if (i2c_dev == NULL)
		return;
	i2c_dev->status = false;

	/* Config I2C SCL and SDA pins */
	sunxi_gpio_init(&i2c_dev->gpio.gpio_scl);
	sunxi_gpio_set_pull(&i2c_dev->gpio.gpio_scl, GPIO_PULL_UP);
	sunxi_gpio_init(&i2c_dev->gpio.gpio_sda);
	sunxi_gpio_set_pull(&i2c_dev->gpio.gpio_sda, GPIO_PULL_UP);

	pr_debug("base = %p, id = %d\n", (void *)i2c_dev->base, i2c_dev->id);

	sunxi_i2c_bus_clk_open(i2c_dev);

	if (sunxi_i2c_bus_reset(i2c_dev))
		return;

	if (sunxi_i2c_set_clock(i2c_dev))
		return;

	sunxi_i2c_bus_en(i2c_dev);

	pr_debug("Bus open done.\n");

#ifdef I2C_DEBUG
	i2c_debug(i2c_dev);
#endif

	i2c_dev->status = true;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-i2c");
