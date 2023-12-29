/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <reg-ncat.h>

#include <sys-dram.h>

extern sunxi_serial_t uart_dbg;

void jmp_to_arm64(uint32_t addr) {

    /* set the cpu boot entry addr: */
    writel(addr, RVBARADDR0_L);
    writel(0, RVBARADDR0_H);

    /* set cpu to AA64 execution state when the cpu boots into after a warm reset */
    asm volatile("mrc p15,0,r2,c12,c0,2");
    asm volatile("orr r2,r2,#(0x3<<0)");
    asm volatile("dsb");
    asm volatile("mcr p15,0,r2,c12,c0,2");
    asm volatile("isb");
_loop:
    asm volatile("wfi");
    goto _loop;
}

int main(void) {
    sunxi_serial_init(&uart_dbg);

    show_banner();

    sunxi_clk_init();

    sunxi_clk_dump();

    printk(LOG_LEVEL_INFO, "Hello World! Now Running ARM64 Test = 0x%08x\n", read32(0x09010040));

    jmp_to_arm64(0x48000);

    printk(LOG_LEVEL_INFO, "Hello World! Now Running ARM64 Test = 0x%08x\n", read32(0x09010040));

    abort();

    return 0;
}