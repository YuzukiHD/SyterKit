/* SPDX-License-Identifier: GPL-2.0+ */

	.syntax unified
	.cpu cortex-a7
	.section .text.start, "ax", %progbits
	.arm
	.globl _start
	.type _start, %function
_start:
	ldr sp, =__stack_srv_end
	ldr r0, =test_boot
	blx r0
	mov r0, #1
	ldr r1, =qemu_sys_exit
	blx r1
	.size _start, . - _start

	.thumb
	.thumb_func
	.globl qemu_sys_write
	.type qemu_sys_write, %function
qemu_sys_write:
	push {r7, lr}
	movs r7, #4
	svc #0
	pop {r7, pc}
	.size qemu_sys_write, . - qemu_sys_write

	.thumb_func
	.globl qemu_sys_exit
	.type qemu_sys_exit, %function
qemu_sys_exit:
	movs r7, #1
	svc #0
1:
	b 1b
	.size qemu_sys_exit, . - qemu_sys_exit
