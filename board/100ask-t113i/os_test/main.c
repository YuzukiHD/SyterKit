/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <os.h>
#include <timer.h>

extern sunxi_serial_t uart_dbg;

static void timer_500ms_cb(void *arg, uint32_t event) {
	printk_info("Timer 500ms callback\n");
}

static void timer_1500ms_run2_cb(void *arg, uint32_t event) {
	printk_info("Timer 500ms run 2 times callback\n");
}

int main(void) {
	sunxi_serial_init(&uart_dbg);

	sunxi_clk_init();

	printk_info("Hello World!\n");

	timer_t timer_500ms;
	timer_create(&timer_500ms, timer_500ms_cb, NULL);
	timer_start(&timer_500ms, TIMER_ALWAYS_RUN, 500);

	timer_t timer_1500ms_run2;
	timer_create(&timer_1500ms_run2, timer_1500ms_run2_cb, NULL);
	timer_start(&timer_1500ms_run2, 2, 1500);

	while (1) {
		timer_handle();
		mdelay(1);
	}

	return 0;
}