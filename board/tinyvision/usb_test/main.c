/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <jmp.h>
#include <mmu.h>
#include <smalloc.h>
#include <sstdlib.h>
#include <string.h>

#include <log.h>

#include <sys-dram.h>
#include <sys-rtc.h>
#include <usb.h>

#define CONFIG_HEAP_BASE (0x40800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

extern sunxi_serial_t uart_dbg;

extern dram_para_t dram_para;

int main(void) {
    sunxi_serial_init(&uart_dbg);

    /* Display the bootloader banner. */
    show_banner();

    sunxi_clk_init();

    printk(LOG_LEVEL_INFO, "Hello World!\n");

    /* Initialize the DRAM and enable memory management unit (MMU). */
    uint64_t dram_size = sunxi_dram_init(&dram_para);
    arm32_mmu_enable(SDRAM_BASE, dram_size);

    /* Debug message to indicate that MMU is enabled. */
    printk(LOG_LEVEL_DEBUG, "enable mmu ok\n");

    /* Initialize the small memory allocator. */
    smalloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

    /* Set up Real-Time Clock (RTC) hardware. */
    rtc_set_vccio_det_spare();

    /* Check if system voltage is within limits. */
    sys_ldo_check();

    /* Dump information about the system clocks. */
    sunxi_clk_dump();

    dma_init();

    dma_test((uint32_t *) 0x41008000, (uint32_t *) 0x40008000);

    sunxi_usb_attach(SUNXI_USB_DEVICE_DETECT);

    if (sunxi_usb_init()) {
        printk(LOG_LEVEL_INFO, "USB init failed.\n");
    }

    printk(LOG_LEVEL_INFO, "USB init OK.\n");

    abort();

    return 0;
}