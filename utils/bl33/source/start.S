.globl _start
_start: 
	b	reset

reset:
	stp x29, x30, [sp, -16]!      // Save the return address on the stack
	bl  clear_bss                 // Call the clear_bss subroutine

	mrs	x0, S3_1_C15_C0_0         // Read the control register
	orr	x0, x0, #0x00001000       // Enable I-cache by setting a specific bit
	msr	S3_1_C15_C0_0, x0         // Write the modified value back to the control register

	bl  sys_main                  // Call the sys_init_dram subroutine

	ldp x29, x30, [sp], 16        // Restore the return address from the stack
	ret                           // Return control to the caller

clear_bss:
	ldr	x0, =__bss_start          // Load the address of __bss_start into register x0
	ldr	x1, =__bss_end            // Load the address of __bss_end into register x1
	mov	x2, #0x00000000           // Set the value to be stored in BSS to zero

clbss_l:
	str	w2, [x0]                  // Store the value of x2 (zero) at the address in x0
	add	x0, x0, #4                // Increment the address by 4 bytes
	cmp	x0, x1                    // Compare the current address with the end address of BSS
	b.ne	clbss_l               // If not equal, loop back to clbss_l

	ret                           // Return control to the caller
