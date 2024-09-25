#![no_std]
#![no_main]

use panic_halt as _;
use syterkit::{entry, println, Clocks, Peripherals};

#[entry]
fn main(p: Peripherals, c: Clocks) {
    println!("Welcome to SyterKit 100ask-d1-h package!");
    println!("Please refer to each files in `bin` path for examples.");
}
