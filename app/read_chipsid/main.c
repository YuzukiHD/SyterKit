/* SPDX-License-Identifier: Apache-2.0 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <sys-sid.h>

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

	dump_efuse();

	return 0;
}