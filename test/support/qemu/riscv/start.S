/* SPDX-License-Identifier: GPL-2.0+ */

	.section .text.start, "ax", @progbits
	.globl _start
	.type _start, @function
_start:
	la sp, __stack_srv_end
	call test_boot
	li a0, 1
	tail qemu_sys_exit
	.size _start, . - _start

	.globl qemu_sys_write
	.type qemu_sys_write, @function
qemu_sys_write:
	li a7, 64
	ecall
	ret
	.size qemu_sys_write, . - qemu_sys_write

	.globl qemu_sys_exit
	.type qemu_sys_exit, @function
qemu_sys_exit:
	li a7, 93
	ecall
1:
	j 1b
	.size qemu_sys_exit, . - qemu_sys_exit
