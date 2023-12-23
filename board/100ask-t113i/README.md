# SyterKit Common 

## start.S

This code snippet is an ARM assembly language program that includes initialization settings and exception handlers. Here's a breakdown of its functionalities:

1. Initialization Settings: It sets registers and writes specific values to configure the processor's working mode, interrupt enable, etc.

2. Set Vector Table: It writes the address of the vector table to the Vector Base Address Register, which is used for handling exceptions and interrupts.

3. Enable NEON/VFP Unit: It configures the processor to enable the NEON (Advanced SIMD) and VFP (Floating-Point) units.

4. Clear BSS Section: It zeroes out variables in the BSS section.

5. Disable Interrupts: It disables FIQ and IRQ interrupts and switches the processor to SVC32 mode.

6. Set Timer Frequency: It sets the timer frequency to 24M.

7. Call the main Function: It jumps to the main function to execute the main logic.

## eabi_compat.c

This code snippet appears to be providing implementations for the functions `abort`, `raise`, and `__aeabi_unwind_cpp_pr0`. Here's a breakdown of their functionalities:

1. `void abort(void)`: This function creates an infinite loop, causing the program to hang indefinitely. It is typically used to indicate a critical error or unrecoverable condition in a program.

2. `int raise(int signum)`: This function is a placeholder and always returns 0. In standard C, this function is used to raise a signal and initiate the corresponding signal handler. However, in this implementation, it does nothing and simply returns 0.

3. `void __aeabi_unwind_cpp_pr0(void)`: This is a dummy function that serves as a placeholder to avoid linker complaints. Its purpose is to satisfy the linker when using C++ exceptions and unwinding, but it does not contain any actual functionality.
