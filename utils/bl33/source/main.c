#include <byteorder.h>
#include <endian.h>
#include <io.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

#define CONFIG_DTB_LOAD_ADDR (0x4a200000)
#define CONFIG_KERNEL_LOAD_ADDR (0x40080000)

extern uint32_t __sunxi_smc_call(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);

static inline uint32_t sunxi_smc_call_atf(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    return __sunxi_smc_call(arg0, arg1, arg2, arg3);
}

void print_banner(void) {
    printf(" _____     _           _____ __    ___ ___ \n");
    printf("|   __|_ _| |_ ___ ___| __  |  |  |_  |_  |\n");
    printf("|__   | | |  _| -_|  _| __ -|  |__|_  |_  |\n");
    printf("|_____|_  |_| |___|_| |_____|_____|___|___|\n");
    printf("      |___|                                \n");
    printf("\n");
    printf("Hello Syter BL33!\n");
    printf("load kernel 0x%08x to aarch64 mode...\n", CONFIG_KERNEL_LOAD_ADDR);
    printf("load dtb 0x%08x...\n\n", CONFIG_DTB_LOAD_ADDR);

    printf("Start Kernel...\n\n");

    mdelay(10);
}
