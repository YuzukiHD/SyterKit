/* SPDX-License-Identifier: Apache-2.0 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>

#include <log.h>

#include <common.h>

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
	.gpio_tx = {GPIO_PIN(PORTH, 9), GPIO_PERIPH_MUX5},
	.gpio_rx = {GPIO_PIN(PORTH, 10), GPIO_PERIPH_MUX5},
};

int main(void)
{
	sunxi_uart_init(&uart_dbg);

	sunxi_clk_init();

    uint32_t id[4];

	id[0] = read32(0x03006200 + 0x0);
	id[1] = read32(0x03006200 + 0x4);
	id[2] = read32(0x03006200 + 0x8);
	id[3] = read32(0x03006200 + 0xc);

	printk(LOG_LEVEL_INFO, "Chip ID is: %08x%08x%08x%08x\r\n", id[0], id[1], id[2], id[3]);

	return 0;
}