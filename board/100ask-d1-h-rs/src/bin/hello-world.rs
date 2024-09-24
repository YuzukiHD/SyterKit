#![no_std]
#![no_main]
use allwinner_hal::uart::{Config, Serial};
use allwinner_rt::{entry, Clocks, Peripherals};
use embedded_hal::digital::{InputPin, OutputPin};
use embedded_io::Write;
use panic_halt as _;

#[entry]
fn main(p: Peripherals, c: Clocks) {
    // light up led
    let mut pb5 = p.gpio.pb5.into_output();
    pb5.set_high().unwrap();
    let mut pc1 = p.gpio.pc1.into_output();
    pc1.set_high().unwrap();

    let mut pb0 = p.gpio.pb7.into_input();

    pb0.with_output(|pad| pad.set_high()).unwrap();

    let _input_high = pb0.is_high();

    let tx = p.gpio.pb8.into_function::<6>();
    let rx = p.gpio.pb9.into_function::<6>();
    let mut serial = Serial::new(p.uart0, (tx, rx), Config::default(), &c, &p.ccu);

    writeln!(serial, "Hello World!").unwrap();
}
