#![no_std]
#![no_main]

use embedded_hal::digital::{InputPin, OutputPin};
use panic_halt as _;
use syterkit::{entry, Clocks, Peripherals};

#[entry]
fn main(p: Peripherals, _c: Clocks) {
    // light up led
    let mut pb5 = p.gpio.pb5.into_output();
    pb5.set_high().unwrap();
    let mut pc1 = p.gpio.pc1.into_output();
    pc1.set_high().unwrap();

    let mut pb0 = p.gpio.pb7.into_input();

    pb0.with_output(|pad| pad.set_high()).unwrap();

    let _input_high = pb0.is_high();
}
