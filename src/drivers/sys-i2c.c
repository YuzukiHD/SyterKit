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

static void i2c_debug(sunxi_i2c_t *i2c_dev) {
    struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;
    printk(LOG_LEVEL_DEBUG, "i2c->addr  :\t0x%x:0x%x\n", &i2c->addr, i2c->addr);
    printk(LOG_LEVEL_DEBUG, "i2c->xaddr :\t0x%x:0x%x\n", &i2c->xaddr, i2c->xaddr);
    printk(LOG_LEVEL_DEBUG, "i2c->data  :\t0x%x:0x%x\n", &i2c->data, i2c->data);
    printk(LOG_LEVEL_DEBUG, "i2c->ctl   :\t0x%x:0x%x\n", &i2c->ctl, i2c->ctl);
    printk(LOG_LEVEL_DEBUG, "i2c->status:\t0x%x:0x%x\n", &i2c->status, i2c->status);
    printk(LOG_LEVEL_DEBUG, "i2c->clk   :\t0x%x:0x%x\n", &i2c->clk, i2c->clk);
    printk(LOG_LEVEL_DEBUG, "i2c->srst  :\t0x%x:0x%x\n", &i2c->srst, i2c->srst);
    printk(LOG_LEVEL_DEBUG, "i2c->eft   :\t0x%x:0x%x\n", &i2c->eft, i2c->eft);
    printk(LOG_LEVEL_DEBUG, "i2c->lcr   :\t0x%x:0x%x\n", &i2c->lcr, i2c->lcr);
    printk(LOG_LEVEL_DEBUG, "i2c->dvfs  :\t0x%x:0x%x\n", &i2c->dvfs, i2c->dvfs);
}

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

static int32_t sunxi_i2c_send_data(sunxi_i2c_t *i2c_dev, uint8_t *data_addr, uint32_t data_count) {
    struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;
    int32_t time = 0xffff;
    uint32_t i;

    for (i = 0; i < data_count; i++) {
        time = 0xffff;
        i2c->data = data_addr[i];
        i2c->ctl |= (0x01 << 3);
        while ((time--) && (!(i2c->ctl & 0x08)))
            ;
        if (time <= 0) {
            return -I2C_NOK_TOUT;
        }
        time = 0xffff;
        while ((time--) && (i2c->status != I2C_DATAWRITE_ACK)) {
            ;
        }
        if (time <= 0) {
            return -I2C_NOK_TOUT;
        }
    }

    return I2C_OK;
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
    struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;
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
    struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;
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
    return _sunxi_i2c_write(i2c_dev, addr, reg, 1, &data, 1);
}

int sunxi_i2c_read(sunxi_i2c_t *i2c_dev, uint8_t addr, uint32_t reg, uint8_t *data) {
    return _sunxi_i2c_read(i2c_dev, addr, reg, 1, data, 1);
}

static void sunxi_i2c_set_clock(sunxi_i2c_t *i2c_dev) {
    struct sunxi_twi_reg *i2c = (struct sunxi_twi_reg *) i2c_dev->base;

    int i, clk_n, clk_m, pow_2_clk_n;

    /* reset i2c control  */
    i = 0xffff;
    i2c->srst = 1;
    while ((i2c->srst) && (i)) {
        i--;
    }
    if ((i2c->lcr & 0x30) != 0x30) {
        /* toggle I2C SCL and SDA until bus idle */
        i2c->lcr = 0x05;
        udelay(500);
        i = 10;
        while ((i > 0) && ((i2c->lcr & 0x02) != 2)) {
            /*control scl and sda output high level*/
            i2c->lcr |= 0x08;
            i2c->lcr |= 0x02;
            udelay(1000);
            /*control scl and sda output low level*/
            i2c->lcr &= ~0x08;
            i2c->lcr &= ~0x02;
            udelay(1000);
            i--;
        }
        i2c->lcr = 0x0;
        udelay(500);
    }
    i2c_dev->speed /= 1000; /*khz*/

    if (i2c_dev->speed < 100)
        i2c_dev->speed = 100;

    else if (i2c_dev->speed > 400)
        i2c_dev->speed = 400;

    /* Foscl=24000/(2^CLK_N*(CLK_M+1)*10) */
    clk_n = (i2c_dev->speed == 100) ? 1 : 0;
    pow_2_clk_n = 1;

    for (i = 0; i < clk_n; ++i)
        pow_2_clk_n *= 2;

    clk_m = (24000 / 10) / (pow_2_clk_n * i2c_dev->speed) - 1;

    i2c->clk = (clk_m << 3) | clk_n;
    i2c->ctl |= 0x40;
    i2c->eft = 0;
}

static void sunxi_i2c_bus_open(sunxi_i2c_t *i2c_dev) {
    int reg_value = 0;

    if (i2c_dev->id <= 5) {
        //de-assert
        reg_value = readl(CCU_BASE + CCU_TWI_BGR_REG);
        reg_value |= (1 << (16 + i2c_dev->id));
        writel(reg_value, CCU_BASE + CCU_TWI_BGR_REG);

        //gating clock pass
        reg_value = readl(CCU_BASE + CCU_TWI_BGR_REG);
        reg_value &= ~(1 << i2c_dev->id);
        writel(reg_value, CCU_BASE + CCU_TWI_BGR_REG);

        mdelay(1);
        reg_value |= (1 << i2c_dev->id);
        writel(reg_value, CCU_BASE + CCU_TWI_BGR_REG);
    } else {
        uint32_t r_bus_num = (i2c_dev->base - SUNXI_RTWI_BASE) / 0x400;
        /*de-assert*/
        reg_value = readl(SUNXI_RTWI_BRG_REG);
        reg_value &= ~(1 << (16 + r_bus_num));
        writel(reg_value, SUNXI_RTWI_BRG_REG);

        reg_value = readl(SUNXI_RTWI_BRG_REG);
        reg_value |= (1 << (16 + r_bus_num));
        writel(reg_value, SUNXI_RTWI_BRG_REG);

        /*gating clock pass*/
        reg_value = readl(SUNXI_RTWI_BRG_REG);
        reg_value &= ~(1 << r_bus_num);
        writel(reg_value, SUNXI_RTWI_BRG_REG);
        mdelay(1);
        reg_value |= (1 << r_bus_num);
        writel(reg_value, SUNXI_RTWI_BRG_REG);
    }
}

void sunxi_i2c_init(sunxi_i2c_t *i2c_dev) {
    /* Config I2C SCL and SDA pins */
    sunxi_gpio_init(i2c_dev->gpio_scl.pin, i2c_dev->gpio_scl.mux);
    sunxi_gpio_init(i2c_dev->gpio_sda.pin, i2c_dev->gpio_sda.mux);

    printk(LOG_LEVEL_DEBUG, "I2C: Init GPIO for I2C, base = 0x%08x, id = %d\n", i2c_dev->base, i2c_dev->id);

    sunxi_i2c_bus_open(i2c_dev);

    printk(LOG_LEVEL_DEBUG, "I2C: Bus open done.\n");
    sunxi_i2c_set_clock(i2c_dev);
}
