#include <byteorder.h>
#include <endian.h>
#include <io.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

#define ARM_SVC_CALL_COUNT 0x8000ff00
#define ARM_SVC_UID 0x8000ff01
#define ARM_SVC_VERSION 0x8000ff03
#define ARM_SVC_RUNNSOS 0x8000ff04
#define PSCI_CPU_OFF 0x84000002
#define PSCI_CPU_ON_AARCH32 0x84000003
#define PSCI_SYSTEM_OFF 0x84000008
#define SUNXI_CPU_ON_AARCH32 0x84000010
#define SUNXI_CPU_OFF_AARCH32 0x84000011
#define SUNXI_CPU_WFI_AARCH32 0x84000012
/* arisc */
#define ARM_SVC_ARISC_STARTUP 0x8000ff10
#define ARM_SVC_ARISC_WAIT_READY 0x8000ff11
#define ARM_SVC_ARISC_READ_PMU 0x8000ff12
#define ARM_SVC_ARISC_WRITE_PMU 0x8000ff13
#define ARM_SVC_ARISC_FAKE_POWER_OFF_REQ_ARCH32 0x83000019
#define ARM_SVC_FAKE_POWER_OFF 0x8000ff14
#define ARM_SVC_UBOOT_POWER_OFF 0x8000ff15
/* efuse */
#define ARM_SVC_EFUSE_READ (0x8000fe00)
#define ARM_SVC_EFUSE_WRITE (0x8000fe01)
#define ARM_SVC_EFUSE_PROBE_SECURE_ENABLE_AARCH32 (0x8000fe03)
#define ARM_SVC_EFUSE_CUSTOMER_RESERVED_HANDLE (0x8000fe05)

#define CONFIG_DTB_LOAD_ADDR (0x4a200000)
#define CONFIG_KERNEL_LOAD_ADDR (0x40080000)

extern uint32_t __sunxi_smc_call(smc_call_arg_t arg0, smc_call_arg_t arg1, smc_call_arg_t arg2, smc_call_arg_t arg3);

uint32_t sunxi_smc_call_atf(smc_call_arg_t arg0, smc_call_arg_t arg1, smc_call_arg_t arg2, smc_call_arg_t arg3, smc_call_arg_t pResult) {
	return __sunxi_smc_call(arg0, arg1, arg2, arg3);
}

uint32_t arm_svc_run_os(smc_call_arg_t kernel, smc_call_arg_t fdt, smc_call_arg_t arg2) {
	return sunxi_smc_call_atf(ARM_SVC_RUNNSOS, kernel, fdt, arg2, 0);
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


void sys_boot() {
	arm_svc_run_os(CONFIG_KERNEL_LOAD_ADDR, CONFIG_DTB_LOAD_ADDR, 1);
}