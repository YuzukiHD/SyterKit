#include <byteorder.h>
#include <endian.h>
#include <io.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

#include <rtc.h>

#define CONFIG_DTB_LOAD_ADDR (0x44000000)
#define CONFIG_KERNEL_LOAD_ADDR (0x40080000)

#define LINUX_IMAGE_MAGIC 0x644d5241
typedef struct {
    uint32_t code0;       /* Executable code */
    uint32_t code1;       /* Executable code */
    uint64_t text_offset; /* Image load offset, little endian */
    uint64_t image_size;  /* Effective Image size, little endian */
    uint64_t flags;       /* kernel flags, little endian */
    uint64_t res2;        /* reserved */
    uint64_t res3;        /* reserved */
    uint64_t res4;        /* reserved */
    uint32_t magic;       /* Magic number, little endian, "ARM\x64" */
    uint32_t res5;        /* reserved (used for PE COFF offset) */
} linux_image_header_t;

#define ARM_SVC_VERSION 0x8000ff03
#define ARM_SVC_RUNNSOS 0x8000ff04

extern uint32_t __sunxi_smc_call(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);

void print_banner(void) {
    printf(" _____     _           _____ __    ___ ___ \n");
    printf("|   __|_ _| |_ ___ ___| __  |  |  |_  |_  |\n");
    printf("|__   | | |  _| -_|  _| __ -|  |__|_  |_  |\n");
    printf("|_____|_  |_| |___|_| |_____|_____|___|___|\n");
    printf("      |___|                                \n");
    printf("\n");
}

uint32_t sunxi_smc_call_atf(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    return __sunxi_smc_call(arg0, arg1, arg2, arg3);
}

void sys_main(void) {
    printf("Hello Syter BL33!\n");

    register uint32_t r0 __asm__("r0");
    __asm__("mrs r0, CPSR"
            :
            :
            : "%r0");
    printf("EL = %u\n", r0 & 0x1F);

    sunxi_smc_call_atf(ARM_SVC_RUNNSOS, 0x4b000000, 1, 1);

    printf("Hello BL33!\n");
}
