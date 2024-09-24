#![no_std]
#![no_main]

use panic_halt as _;
use syterkit_100ask_d1_h::{entry, println, Clocks, Peripherals};

#[entry]
fn main(p: Peripherals, c: Clocks) {
    println!("Hello World!");
}
