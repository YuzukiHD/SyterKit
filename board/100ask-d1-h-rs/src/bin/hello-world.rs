#![no_std]
#![no_main]

use panic_halt as _;
use syterkit::{entry, println, Clocks, Peripherals};

#[entry]
fn main(_p: Peripherals, _c: Clocks) {
    println!("Hello World!");
}
