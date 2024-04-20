#![feature(asm_const)]
#![no_std]
#![no_main]

use panic_halt as _;

fn main() {}

const STACK_SIZE: usize = 4 * 1024;
static STACK: [u8; STACK_SIZE] = [0u8; STACK_SIZE];

#[naked_function::naked]
#[export_name = "_start"]
pub unsafe extern "C" fn start() -> ! {
    asm!(
        /* disable interrupt */
        "csrw mie, zero",

        /* enable theadisaee */
        "li t1, (0x1 << 22) | (1 << 21) | (1 << 15)
        csrs 0x7C0, t1", // mxstatus

        /* invaild ICACHE/DCACHE/BTB/BHT */
        "li t2, 0x30013
        csrs 0x7C2, t2", // mcor

        /* Config pmp register */
        "li t0, (0xfffffffc >> 2)
        csrw pmpaddr0, t0
        li t0, ((0x0 << 7) | (0x1 << 3) | (0x1 << 2) | (0x1 << 1) | (0x1 << 0))
        csrw pmpcfg0, t0",

        /* Mask all interrupts */
        "csrw mideleg, zero
        csrw medeleg, zero
        csrw mie, zero
        csrw mip, zero",

        // /* Setup exception vectors */
        // "la t1, _image_start
        // LREG t1, (t1)
        // la t2, _start
        // sub t0, t1, t2
        // la a0, vectors
        // add a0, a0, t0
        // csrw mtvec, a0",

        /* Enable fpu and accelerator and vector if present */
        "li t0, 0x00006000 | 0x00018000 | (3 << 23)", // MSTATUS_FS | MSTATUS_XS | (3 << 23)
        "csrs mstatus, t0",

        "la sp, {stack_end}",
        "li t1, {stack_size}",
        "add sp, sp, t1",

        // TODO: clear_bss

        // TODO: set_timer_count

        "call {rust_main}",

        stack_end = sym STACK,
        stack_size = const STACK_SIZE,
        rust_main = sym main,
    )
}
