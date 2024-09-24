#![no_std]
#![no_main]
use allwinner_hal::uart::{Config, Serial};
use allwinner_rt::{entry, Clocks, Peripherals};
use embedded_io::Write;
use panic_halt as _;
use syterkit_100ask_d1_h::mctl;

#[entry]
fn main(p: Peripherals, c: Clocks) {
    let tx = p.gpio.pb8.into_function::<6>();
    let rx = p.gpio.pb9.into_function::<6>();
    let mut serial = Serial::new(p.uart0, (tx, rx), Config::default(), &c, &p.ccu);

    writeln!(serial, "DDR init").unwrap();
    let ram_size = mctl::init();
    writeln!(serial, "{}M 🐏", ram_size).unwrap();
}
