/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SUN8IW8_REG_NCAT_H__
#define __SUN8IW8_REG_NCAT_H__

#define SUNXI_CE_BASE (0x01c15000)
#define SUNXI_SS_BASE SUNXI_CE_BASE
//sys ctrl
#define SUNXI_SYSCRL_BASE (0x01c00000)
#define SUNXI_CCM_BASE (0x01c20000)
#define SUNXI_DMA_BASE (0x01c02000)
#define SUNXI_MSGBOX_BASE (0X01C17000)
#define SUNXI_SPINLOCK_BASE (0X01C18000)
#define SUNXI_HSTMR_BASE (0x01c60000)

#define SUNXI_SMC_BASE (0x01c1e000)
#define SUNXI_TIMER_BASE (0x01c20c00)

#define SUNXI_PIO_BASE (0x01c20800)
#define SUNXI_GIC_BASE (0x01c81000)

// storage
#define SUNXI_DRAMCTL0_BASE (0x01c63000)
#define SUNXI_NFC_BASE (0x01c03000)
#define SUNXI_SMHC0_BASE (0x01c0f000)
#define SUNXI_SMHC1_BASE (0x01c10000)
#define SUNXI_SMHC2_BASE (0x01c11000)

// noraml
#define SUNXI_UART0_BASE (0x01c28000)
#define SUNXI_UART1_BASE (0x01c28400)
#define SUNXI_UART2_BASE (0x01c28800)
#define SUNXI_UART3_BASE (0x01c28c00)
#define SUNXI_UART4_BASE (0x01c29000)

#define SUNXI_RTWI_BASE (0x01c2ac00)
#define SUNXI_TWI0_BASE (0x01c2ac00)
#define SUNXI_TWI1_BASE (0x01c2b000)
#define SUNXI_TWI2_BASE (0x01c2b400)
#define SUNXI_TWI3_BASE (0x01c2b800)

#define SUNXI_SPI0_BASE (0x01c68000)

#define SUNXI_RPRCM_BASE (0x01f01400)
#define SUNXI_RTWI_BRG_REG (SUNXI_RPRCM_BASE + 0x019c)
#define SUNXI_RPIO_BASE (0X01F02C00)
#define SUNXI_RTC_BASE (0x01c20400)
#define SUNXI_RTC_DATA_BASE (SUNXI_RTC_BASE + 0x100)
#define GPIO_POW_MODE_REG (0x0340)

/*
* Because the platform uses a very old clock framework,
* the following register configuration is only suitable
* for the new driver, and is not practical.
*/
#define CCU_UART_BGR_REG (0x0)
#define CCU_SPI0_CLK_REG (0x0)
#define CCU_SPI_BGR_REG (0x0)

#endif// __SUN8IW8_REG_NCAT_H__