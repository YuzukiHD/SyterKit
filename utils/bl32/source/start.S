.globl _start
_start: 
	b	reset

reset:
	stmfd sp!, {lr}
	bl  clear_bss

	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #0x00001000	@ enable I-cache
	mcr	p15, 0, r0, c1, c0, 0

	bl set_timer_count

	bl sunxi_serial_init

	bl print_banner

	bl sys_main

	ldmfd sp!, {lr}
	mov pc, lr
	
clear_bss:
	ldr	r0, =__bss_start
	ldr	r1, =__bss_end
	mov	r2, #0x00000000

/* clear loop...		    */
clbss_l:
	str	r2, [r0]
	add	r0, r0, #4
	cmp	r0, r1
	bne	clbss_l

	mov pc, lr
