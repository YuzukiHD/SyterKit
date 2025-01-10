#include <linkage.h>
#include <riscv64.h>

	.global _start
_start:
	li t1, 0x1 << 22
	csrs mxstatus, t1
	li t1, 0x30013
	csrs mcor, t1
	j reset

reset:
	addi sp, sp, -32
	sd s0, 8(sp)
	sd s1, 16(sp)
	sd ra, 24(sp)
	mv s0, a0

	li t0, 0x07090108
	sw zero, (t0)
	jal main

	ld ra, 24(sp)
	ld s0, 8(sp)
	ld s1, 16(sp)
	addi sp, sp, 32
	ret
