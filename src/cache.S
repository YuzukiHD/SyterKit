.globl __v7_flush_dcache_all
__v7_flush_dcache_all:
	dmb
	mrc	p15, 1, r0, c0, c0, 1
	mov	r3, r0, lsr #23
	ands	r3, r3, #14
	beq	finished
start_flush_levels:
	mov	r10, #0
flush_levels:
	add	r2, r10, r10, lsr #1
	mov	r1, r0, lsr r2
	and	r1, r1, #7
	cmp	r1, #2
	blt	skip
	mcr	p15, 2, r10, c0, c0, 0
	isb
	mrc	p15, 1, r1, c0, c0, 0
	and	r2, r1, #7
	add	r2, r2, #4
	movw	r4, #1023
	ands	r4, r4, r1, lsr #3
	clz	r5, r4
	movw	r7, #32767
	ands	r7, r7, r1, lsr #13
loop1:
	mov	r9, r7
loop2:
	orr	r11, r10, r4, lsl r5
	orr	r11, r11, r9, lsl r2
	mcr	p15, 0, r11, c7, c14, 2
	subs	r9, r9, #1
	bge	loop2
	subs	r4, r4, #1
	bge	loop1
skip:
	add	r10, r10, #2
	cmp	r3, r10
	bgt	flush_levels
finished:
	mov	r10, #0
	mcr	p15, 2, r10, c0, c0, 0
	dsb	st
	isb
	bx	lr

.globl v7_flush_dcache_all
v7_flush_dcache_all:
	stmfd	sp!, {r4-r7, r9-r11, lr}
	bl	__v7_flush_dcache_all
	ldmfd	sp!, {r4-r7, r9-r11, lr}
	bx	lr
