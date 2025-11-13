/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <common.h>
#include <log.h>

#include <sys-clk.h>

#include "reg-ncat.h"
#include "sys-i2c.h"

#define I2C_WRITE 0
#define I2C_READ 1

#define I2C_OK 0
#define I2C_NOK 1
#define I2C_NACK 2
#define I2C_NOK_LA 3   /* Lost arbitration */
#define I2C_NOK_TOUT 4 /* time out */

#define I2C_START_TRANSMIT 0x08
#define I2C_RESTART_TRANSMIT 0x10
#define I2C_ADDRWRITE_ACK 0x18
#define I2C_ADDRREAD_ACK 0x40
#define I2C_DATAWRITE_ACK 0x28
#define I2C_READY 0xf8
#define I2C_DATAREAD_NACK 0x58
#define I2C_DATAREAD_ACK 0x50

#ifdef I2C_DEBUG
__attribute__((unused)) static void i2c_debug(sunxi_i2c_t *i2c_dev) {
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;
	printk_debug("i2c->addr  :\t0x%x:0x%x\n", &i2c->addr, i2c->addr);
	printk_debug("i2c->xaddr :\t0x%x:0x%x\n", &i2c->xaddr, i2c->xaddr);
	printk_debug("i2c->data  :\t0x%x:0x%x\n", &i2c->data, i2c->data);
	printk_debug("i2c->ctl   :\t0x%x:0x%x\n", &i2c->ctl, i2c->ctl);
	printk_debug("i2c->status:\t0x%x:0x%x\n", &i2c->status, i2c->status);
	printk_debug("i2c->clk   :\t0x%x:0x%x\n", &i2c->clk, i2c->clk);
	printk_debug("i2c->srst  :\t0x%x:0x%x\n", &i2c->srst, i2c->srst);
	printk_debug("i2c->eft   :\t0x%x:0x%x\n", &i2c->eft, i2c->eft);
	printk_debug("i2c->lcr   :\t0x%x:0x%x\n", &i2c->lcr, i2c->lcr);
	printk_debug("i2c->dvfs  :\t0x%x:0x%x\n", &i2c->dvfs, i2c->dvfs);
}
#endif

static int32_t sunxi_i2c_send_byteaddr(sunxi_i2c_t *i2c_dev, uint32_t byteaddr) {
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;

	int32_t time = 0xffff;
	uint32_t tmp_val;

	i2c->data = byteaddr & 0xff;
	i2c->ctl |= (0x01 << 3); /*write 1 to clean int flag*/

	while ((time--) && (!(i2c->ctl & 0x08)))
		;

	if (time <= 0) {
		return -I2C_NOK_TOUT;
	}

	tmp_val = i2c->status;
	if (tmp_val != I2C_DATAWRITE_ACK) {
		return -I2C_DATAWRITE_ACK;
	}

	return I2C_OK;
}

static int32_t sunxi_i2c_send_start(sunxi_i2c_t *i2c_dev) {
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;

	int32_t time = 0xffff;
	uint32_t tmp_val;

	i2c->eft = 0;
	i2c->srst = 1;
	i2c->ctl |= TWI_CTL_STA;

	while ((time--) && (!(i2c->ctl & TWI_CTL_INTFLG)))
		;
	if (time <= 0) {
		return -I2C_NOK_TOUT;
	}

	tmp_val = i2c->status;
	if (tmp_val != I2C_START_TRANSMIT) {
		return -I2C_START_TRANSMIT;
	}

	return I2C_OK;
}


static int32_t sunxi_i2c_send_slave_addr(sunxi_i2c_t *i2c_dev, uint32_t saddr, uint32_t rw) {
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;
	int32_t time = 0xffff;
	uint32_t tmp_val;

	rw &= 1;
	i2c->data = ((saddr & 0xff) << 1) | rw;
	i2c->ctl |= TWI_CTL_INTFLG; /*write 1 to clean int flag*/

	while ((time--) && (!(i2c->ctl & TWI_CTL_INTFLG)))
		;

	if (time <= 0) {
		return -I2C_NOK_TOUT;
	}

	tmp_val = i2c->status;
	if (rw == I2C_WRITE) {
		if (tmp_val != I2C_ADDRWRITE_ACK) {
			return -I2C_ADDRWRITE_ACK;
		}
	}

	else {
		if (tmp_val != I2C_ADDRREAD_ACK) {
			return -I2C_ADDRREAD_ACK;
		}
	}

	return I2C_OK;
}

static int32_t sunxi_i2c_send_restart(sunxi_i2c_t *i2c_dev) {
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;
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

	tmp_val = i2c->status;
	if (tmp_val != I2C_RESTART_TRANSMIT) {
		return -I2C_RESTART_TRANSMIT;
	}

	return I2C_OK;
}

static int32_t sunxi_i2c_stop(sunxi_i2c_t *i2c_dev) {
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;
	int32_t time = 0xffff;
	uint32_t tmp_val;
	i2c->ctl |= (0x01 << 4);
	i2c->ctl |= (0x01 << 3);
	while ((time--) && (i2c->ctl & 0x10))
		;
	if (time <= 0) {
		return -I2C_NOK_TOUT;
	}
	time = 0xffff;
	while ((time--) && (i2c->status != I2C_READY))
		;
	tmp_val = i2c->status;
	if (tmp_val != I2C_READY) {
		return -I2C_NOK_TOUT;
	}

	return I2C_OK;
}

static int32_t sunxi_i2c_get_data(sunxi_i2c_t *i2c_dev, uint8_t *data_addr, uint32_t data_count) {
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;
	int32_t time = 0xffff;
	uint32_t tmp_val;
	uint32_t i;
	if (data_count == 1) {
		i2c->ctl |= (0x01 << 3);
		while ((time--) && (!(i2c->ctl & 0x08)))
			;
		if (time <= 0) {
			return -I2C_NOK_TOUT;
		}
		for (time = 0; time < 100; time++)
			;
		*data_addr = i2c->data;

		tmp_val = i2c->status;
		if (tmp_val != I2C_DATAREAD_NACK) {
			return -I2C_DATAREAD_NACK;
		}
	} else {
		for (i = 0; i < data_count - 1; i++) {
			time = 0xffff;
			/*host should send ack every time when a data packet finished*/
			tmp_val = i2c->ctl | (0x01 << 2);
			tmp_val = i2c->ctl | (0x01 << 3);
			tmp_val |= 0x04;
			i2c->ctl = tmp_val;
			/*i2c->ctl |=(0x01<<3);*/

			while ((time--) && (!(i2c->ctl & 0x08)))
				;
			if (time <= 0) {
				return -I2C_NOK_TOUT;
			}
			for (time = 0; time < 100; time++)
				;
			time = 0xffff;
			data_addr[i] = i2c->data;
			while ((time--) && (i2c->status != I2C_DATAREAD_ACK))
				;
			if (time <= 0) {
				return -I2C_NOK_TOUT;
			}
		}

		time = 0xffff;
		i2c->ctl &= 0xFb; /*the last data packet,not send ack*/
		i2c->ctl |= (0x01 << 3);
		while ((time--) && (!(i2c->ctl & 0x08)))
			;
		if (time <= 0) {
			return -I2C_NOK_TOUT;
		}
		for (time = 0; time < 100; time++)
			;
		data_addr[data_count - 1] = i2c->data;
		while ((time--) && (i2c->status != I2C_DATAREAD_NACK))
			;
		if (time <= 0) {
			return -I2C_NOK_TOUT;
		}
	}

	return I2C_OK;
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
static int32_t sunxi_i2c_send_data(sunxi_i2c_t *i2c_dev, uint8_t *data_addr, uint32_t data_count) {
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;
	int32_t time = 0xffff;
	uint32_t i;

	for (i = 0; i < data_count; i++) {
		time = 0xffff;
		i2c->data = data_addr[i];// Write data to the I2C data register
		i2c->ctl |= (0x01 << 3); // Trigger data transmission

		// Wait for the transmission to complete (timeout)
		while ((time--) && (!(i2c->ctl & 0x08)))
			;
		if (time <= 0) {
			return -I2C_NOK_TOUT;// Timeout error
		}

		time = 0xffff;

		// Wait for ACK from the I2C device (timeout)
		while ((time--) && (i2c->status != I2C_DATAWRITE_ACK)) { ; }
		if (time <= 0) {
			return -I2C_NOK_TOUT;// Timeout error
		}
	}

	return I2C_OK;// Success
}


/**
 * @brief sunxi_i2c read function
 *
 * @param i2c_dev Pointer to the sunxi_i2c controller device structure
 * @param chip Device address
 * @param addr Register to be read from in the device
 * @param alen Length of the register address
 * @param buffer Buffer to store the read data
 * @param len Length of the data to be read
 * @return int Number of status
 */
static int _sunxi_i2c_read(sunxi_i2c_t *i2c_dev, uint8_t chip, uint32_t addr, int alen, uint8_t *buffer, int len) {
	int i, ret, addrlen;
	char *slave_reg;

	ret = sunxi_i2c_send_start(i2c_dev);
	if (ret) {
		goto i2c_read_err_occur;
	}

	ret = sunxi_i2c_send_slave_addr(i2c_dev, chip, I2C_WRITE);
	if (ret) {
		goto i2c_read_err_occur;
	}

	/*send byte address*/
	if (alen >= 3) {
		addrlen = 2;
	} else if (alen <= 1) {
		addrlen = 0;
	} else {
		addrlen = 1;
	}
	slave_reg = (char *) &addr;

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

	/*get data*/
	ret = sunxi_i2c_get_data(i2c_dev, buffer, len);
	if (ret) {
		goto i2c_read_err_occur;
	}

i2c_read_err_occur:
	sunxi_i2c_stop(i2c_dev);

	return ret;
}

/**
 * @brief sunxi_i2c write function
 *
 * @param i2c_dev Pointer to the sunxi_i2c controller device structure
 * @param chip Device address
 * @param addr Register to be read/written in the device
 * @param alen Length of the register address
 * @param buffer Data to be written/read
 * @param len Length of the data
 * @return int Number of status
 */
static int _sunxi_i2c_write(sunxi_i2c_t *i2c_dev, uint8_t chip, uint32_t addr, int alen, uint8_t *buffer, int len) {
	int i, ret, ret0, addrlen;
	char *slave_reg;

	ret0 = -1;
	ret = sunxi_i2c_send_start(i2c_dev);
	if (ret) {
		goto i2c_write_err_occur;
	}

	ret = sunxi_i2c_send_slave_addr(i2c_dev, chip, I2C_WRITE);
	if (ret) {
		goto i2c_write_err_occur;
	}

	/*send byte address*/
	if (alen >= 3) {
		addrlen = 2;
	} else if (alen <= 1) {
		addrlen = 0;
	} else {
		addrlen = 1;
	}

	slave_reg = (char *) &addr;
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
	ret0 = 0;

i2c_write_err_occur:
	sunxi_i2c_stop(i2c_dev);

	return ret0;
}

int sunxi_i2c_write(sunxi_i2c_t *i2c_dev, uint8_t addr, uint32_t reg, uint8_t data) {
	if (!i2c_dev->status)
		return I2C_NOK;

	return _sunxi_i2c_write(i2c_dev, addr, reg, 1, &data, 1);
}

int sunxi_i2c_read(sunxi_i2c_t *i2c_dev, uint8_t addr, uint32_t reg, uint8_t *data) {
	if (!i2c_dev->status)
		return I2C_NOK;

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
 */
static void sunxi_i2c_bus_reset(sunxi_i2c_t *i2c_dev) {
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;
	int timeout = 0;

	/* Reset I2C control */
	timeout = 0xffff;
	i2c->srst = 1;// Initiate the reset
	while ((i2c->srst) && (timeout)) { timeout--; }

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
		i2c->lcr = 0x0;// Clear control register
		udelay(500);
	}
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
 */
static void sunxi_i2c_set_clock(sunxi_i2c_t *i2c_dev) {
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;
	uint32_t clk_m = 0, clk_n = 0, _2_pow_clk_n = 1, duty = 0, src_clk = 0;
	uint32_t divider, sclk_real; /* the real clock frequency */

	/* I2C_CLK = parent_clk / ( 2^CLK_N * (CLK_M + 1) *10) */
	src_clk = i2c_dev->i2c_clk.parent_clk / 10;

	divider = src_clk / i2c_dev->speed; /* 400kHz or 100kHz */
	sclk_real = 0;						/* the real clock frequency */

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
				goto set_clk;
			} else {
				clk_m++;
			}
		}
		clk_n++;
		_2_pow_clk_n *= 2; /* Multiple by 2 */
	}

set_clk:
	i2c->clk &= ~(TWI_CLK_DIV_M | TWI_CLK_DIV_N);
	i2c->clk |= ((clk_m << 3) | clk_n);
	if (i2c_dev->speed == SUNXI_I2C_SPEED_400K) {
		duty = TWI_CLK_DUTY_30_EN;
		i2c->clk |= duty;
	} else {
		duty = TWI_CLK_DUTY;
		i2c->clk &= ~(duty);
	}

#ifdef I2C_DEBUG
	i2c_debug(i2c_dev);
#endif
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
static inline void sunxi_i2c_bus_clk_open(sunxi_i2c_t *i2c_dev) {
	// De-assert the clock reset
	setbits_le32(i2c_dev->i2c_clk.rst_reg_base, BIT(i2c_dev->i2c_clk.rst_reg_offset));

	// Enable the clock by clearing the gate bit
	clrbits_le32(i2c_dev->i2c_clk.gate_reg_base, BIT(i2c_dev->i2c_clk.gate_reg_offset));
	mdelay(1);// Wait for the clock to stabilize

	// Re-assert the clock gating
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
static inline void sunxi_i2c_bus_en(sunxi_i2c_t *i2c_dev) {
	struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;

	i2c->ctl |= 0x40;// Set the control register to enable the I2C bus
	i2c->eft = 0;	 // Clear error flags
}


void sunxi_i2c_init(sunxi_i2c_t *i2c_dev) {
	/* Config I2C SCL and SDA pins */
	sunxi_gpio_init(i2c_dev->gpio.gpio_scl.pin, i2c_dev->gpio.gpio_scl.mux);
	sunxi_gpio_set_pull(i2c_dev->gpio.gpio_scl.pin, GPIO_PULL_UP);
	sunxi_gpio_init(i2c_dev->gpio.gpio_sda.pin, i2c_dev->gpio.gpio_sda.mux);
	sunxi_gpio_set_pull(i2c_dev->gpio.gpio_sda.pin, GPIO_PULL_UP);

	printk_debug("I2C: base = 0x%08x, id = %d\n", i2c_dev->base, i2c_dev->id);

	sunxi_i2c_bus_clk_open(i2c_dev);

	sunxi_i2c_bus_reset(i2c_dev);

	sunxi_i2c_set_clock(i2c_dev);

	sunxi_i2c_bus_en(i2c_dev);

	printk_debug("I2C: Bus open done.\n");

#ifdef I2C_DEBUG
	i2c_debug(i2c_dev);
#endif

	i2c_dev->status = true;
}
