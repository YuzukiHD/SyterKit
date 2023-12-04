/* SPDX-License-Identifier: Apache-2.0 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <config.h>

extern uint32_t _start;
extern uint32_t __spl_start;
extern uint32_t __spl_end;
extern uint32_t __spl_size;
extern uint32_t __stack_srv_start;
extern uint32_t __stack_srv_end;
extern uint32_t __stack_ddr_srv_start;
extern uint32_t __stack_ddr_srv_end;

sunxi_uart_t uart_dbg = {
	.base = 0x02500000,
	.id = 0,
	.gpio_tx = { GPIO_PIN(PORTH, 9), GPIO_PERIPH_MUX5 },
	.gpio_rx = { GPIO_PIN(PORTH, 10), GPIO_PERIPH_MUX5 },
};

void show_banner(void)
{
	uint32_t id[4];

	printk(LOG_LEVEL_MUTE, "\r\n");
	printk(LOG_LEVEL_INFO, " _____     _           _____ _ _   \r\n");
	printk(LOG_LEVEL_INFO, "|   __|_ _| |_ ___ ___|  |  |_| |_ \r\n");
	printk(LOG_LEVEL_INFO, "|__   | | |  _| -_|  _|    -| | _| \r\n");
	printk(LOG_LEVEL_INFO, "|_____|_  |_| |___|_| |__|__|_|_|  \r\n");
	printk(LOG_LEVEL_INFO, "      |___|                        \r\n");
	printk(LOG_LEVEL_INFO, "***********************************\r\n");
	printk(LOG_LEVEL_INFO, " %s V0.1.1 Commit: %s\r\n", PROJECT_NAME,
	       PROJECT_GIT_HASH);
	printk(LOG_LEVEL_INFO, "***********************************\r\n");

	id[0] = read32(0x03006200 + 0x0);
	id[1] = read32(0x03006200 + 0x4);
	id[2] = read32(0x03006200 + 0x8);
	id[3] = read32(0x03006200 + 0xc);

	printk(LOG_LEVEL_INFO, "Chip ID is: %08x%08x%08x%08x\r\n", id[0], id[1],
	       id[2], id[3]);
}

int main(void)
{
	sunxi_uart_init(&uart_dbg);

	show_banner();

	sunxi_clk_init();

	sunxi_dram_init();

	return 0;
}