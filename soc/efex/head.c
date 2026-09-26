/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

/* eFEX uses the 32-bit Boot ROM header ABI on ARM and RISC-V. */
struct boot_file_head {
	uint32_t jump_instruction;
	uint8_t magic[8];
	uint32_t check_sum;
	uint32_t length;
	uint32_t pub_head_size;
	uint8_t pub_head_vsn[4];
	uint32_t ret_addr; /* Boot ROM-visible eFEX result buffer. */
	uint32_t run_addr;
	uint32_t boot_cpu;
	uint8_t platform[8];
};

_Static_assert(sizeof(struct boot_file_head) == 0x30,
	       "eFEX requires a 32-bit boot header");

extern uint32_t __spl_size[];
extern uint32_t __code_start_address[];
extern uint32_t __efex_result_start[];

const struct boot_file_head boot_head __attribute__((section(".boot0_head"), used)) = {
#ifdef CONFIG_ARCH_ARM32
	.jump_instruction = 0xea00000e, /* b image + 0x40 */
#else
	.jump_instruction = 0x0400006f, /* jal x0, image + 0x40 */
#endif
	.magic = "eGON.BT0",
	.check_sum = 0x12345678,
	.length = (uint32_t)(uintptr_t)__spl_size,
	.pub_head_size = sizeof(struct boot_file_head),
	.pub_head_vsn = "3000",
	.ret_addr = (uint32_t)(uintptr_t)__efex_result_start,
	.run_addr = (uint32_t)(uintptr_t)__code_start_address,
	.platform = { 0, 0, '3', '.', '0', '.', '0', 0 },
};
