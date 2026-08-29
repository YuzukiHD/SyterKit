/**
 * @file memcmp.S
 * @brief ARM assembly implementation of memcmp().
 *
 * Word-wise comparison routine that returns a negative, zero, or positive
 * value based on the first differing byte of the two buffers.
 */
    .text

    .global memcmp
    .type memcmp, %function
    .align 4

memcmp:
	/* if(len == 0) return 0 */
	cmp		r2, #0
	moveq	r0, #0
	moveq	pc, lr

	/* subtract one to have the index of the last character to check */
	sub		r2, r2, #1

	/* ip == last src address to compare */
	add		ip, r0, r2

1:	ldrb	r2, [r0], #1
	ldrb	r3, [r1], #1
	cmp		ip, r0
	cmpcs	r2, r3
	beq		1b
	sub		r0, r2, r3

	mov		pc, lr