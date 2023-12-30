.globl _start
_start: 
	b	reset

reset:
	stmfd sp!, {lr}        @ Save the return address on the stack
	bl  clear_bss          @ Call the clear_bss subroutine

	mrc	p15, 0, r0, c1, c0, 0    @ Read the control register
	orr	r0, r0, #0x00001000      @ Enable I-cache by setting a specific bit
	mcr	p15, 0, r0, c1, c0, 0    @ Write the modified value back to the control register

	bl  sys_init_dram       @ Call the sys_init_dram subroutine
	
	ldmfd sp!, {lr}         @ Restore the return address from the stack
	mov pc, lr              @ Return control to the caller

clear_bss:
	ldr	r0, =__bss_start     @ Load the address of __bss_start into register r0
	ldr	r1, =__bss_end       @ Load the address of __bss_end into register r1
	mov	r2, #0x00000000       @ Set the value to be stored in BSS to zero

clbss_l:
	str	r2, [r0]             @ Store the value of r2 (zero) at the address in r0
	add	r0, r0, #4           @ Increment the address by 4 bytes
	cmp	r0, r1               @ Compare the current address with the end address of BSS
	bne	clbss_l              @ If not equal, loop back to clbss_l

	mov pc, lr               @ Return control to the caller