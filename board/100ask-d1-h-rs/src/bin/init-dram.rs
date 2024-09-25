#![no_std]
#![no_main]

use panic_halt as _;
use syterkit::{entry, mctl, println, Clocks, Peripherals};

#[entry]
fn main(p: Peripherals, c: Clocks) {
    println!("DDR Init");
    let ram_size = mctl::init();
    println!("{}M 🐏", ram_size);
}
