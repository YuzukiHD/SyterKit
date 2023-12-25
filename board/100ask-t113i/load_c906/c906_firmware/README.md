# Tiny C906 Firmware For T113-I 

## How to build

```
mkdir build && cd build
cmake -DRISCV_ROOT_PATH=/path/to/riscv/toolchain ..
make

```

1. `mkdir build` creates a directory named "build".
2. `cd build` changes the current directory to "build".
3. `cmake -DRISCV_ROOT_PATH=/path/to/riscv/toolchain ..` configures the build system using CMake. The `-DRISCV_ROOT_PATH` option specifies the path to the RISC-V toolchain.
4. `make` compiles the project using the configured build system.

By running these commands, a build directory is created, the build process is configured with the specified RISC-V toolchain path, and the project is compiled using the `make` command.

## start.S

This code is written in assembly language for a RISC-V processor. Here's a breakdown of its functionality:

The `.global _start` directive indicates that `_start` is the entry point of the program.

At the `_start` label:
1. `li t1, 0x1 << 22` loads the immediate value `0x1` shifted left by `22` into register `t1`.
2. `csrs mxstatus, t1` sets the MXSTATUS register with the value in `t1`, enabling machine-level interrupts.
3. `li t1, 0x30013` loads the immediate value `0x30013` into register `t1`.
4. `csrs mcor, t1` sets the MCOR register with the value in `t1`, configuring the machine-level interrupt controller.
5. `j reset` jumps to the `reset` label.

At the `reset` label:
1. Memory is allocated on the stack by subtracting `32` from the stack pointer (`sp`).
2. Registers `s0`, `s1`, and `ra` are saved onto the stack using the `sd` instruction.
3. The value of `a0` is moved into `s0` register.
4. `li t0, 0x07090108` loads the immediate value `0x07090108` into register `t0`.
5. `sw zero, (t0)` stores the value of register `zero` (which holds the value `0`) at the memory address pointed to by `t0`.
6. `jal main` jumps and links to the `main` function.

After the `jal` instruction, the execution will continue at the `main` function. Once the `main` function returns:
1. The saved registers are restored from the stack.
2. The stack pointer is adjusted by adding `32` to it.
3. `ret` instruction is executed, which returns control to the calling function.

Overall, this code sets up machine-level interrupts, configures the interrupt controller, initializes the stack, and jumps to the `main` function before returning to the caller.

## main.c

This code defines a constant `OPENSBI_FW_TEXT_START` with the value `0x41fc0000`. 

The function `jmp_opensbi` takes an argument `opensbi_base` of type `uint32_t` and performs an assembly instruction that jumps to the address stored in register `a0`. After the jump, it enters an infinite loop where it executes a `WFI` (Wait for Interrupt) instruction and then jumps back to `__LOOP` label.

The `main` function initializes UART0 and prints a series of messages to the console using `sys_uart_printf` function. It counts from 0 to 8 using a `for` loop and delays for 100,000 cycles between each count. Finally, it prints a message and calls `jmp_opensbi` function with the `OPENSBI_FW_TEXT_START` constant as the argument.

