	.text
	.global _start
_start:
	/* Invalidate L1 I/D */
	mov	r0, #0			        /* set up for MCR */
	mcr	p15, 0, r0, c8, c7,  0	/* invalidate TLBs */
	mcr	p15, 0, r0, c7, c5,  0	/* invalidate icache */
	mcr	p15, 0, r0, c7, c5,  6	/* invalidate BP array */
	DSB	/* DSB */
	ISB	/* ISB */
	b reset

reset:
	ldr r0, =__stack_srv_end
	str sp, [r0, #0]
	str lr, [r0, #4]
	mrs lr, cpsr
	str lr, [r0, #8]
	mrc p15, 0, lr, c1, c0, 0
	str lr, [r0, #12]
	mrc p15, 0, lr, c12, c0, 0
	str lr, [r0, #16]
	mrc p15, 0, lr, c1, c0, 0
	str lr, [r0, #20]

	bl sys_init_dram

	ldr r0, =__stack_srv_end
	ldr sp, [r0, #0]
	ldr lr, [r0, #4]
	ldr r1, [r0, #20]
	mcr p15, 0, r1, c1, c0, 0
	ldr r1, [r0, #16]
	mcr p15, 0, r1, c12, c0, 0
	ldr r1, [r0, #12]
	mcr p15, 0, r1, c1, c0, 0
	ldr r1, [r0, #8]
	msr cpsr, r1

	bx lr
