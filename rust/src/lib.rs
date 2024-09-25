#![no_std]
use allwinner_hal::{gpio::Function, uart::Serial};
use allwinner_rt::soc::d1::{CCU, COM, PLIC, SPI0, UART0};
use embedded_io::Write;
use spin::Mutex;

#[macro_use]
pub mod macros;

pub mod mctl;
pub mod soc;

pub use allwinner_hal::ccu::Clocks;
pub use syterkit_macros::entry;

#[cfg(feature = "sun20iw1")]
pub use soc::sun20iw1::clock_dump;

/// ROM runtime peripheral ownership and configurations.
pub struct Peripherals<'a> {
    /// General Purpose Input/Output peripheral.
    pub gpio: soc::sun20iw1::Pads<'a>,
    // uart0 is removed; it is occupied by stdin/stdout `Serial` structure.
    /// Serial Peripheral Interface peripheral 0.
    pub spi0: SPI0,
    /// Common control peripheral of DDR SDRAM.
    pub com: COM,
    /// Clock control unit peripheral.
    pub ccu: CCU,
    /// Platform-local Interrupt Controller.
    pub plic: PLIC,
}

impl<'a> Peripherals<'a> {
    /// Split SyterKit peripherals from `allwinner-rt` peripherals.
    #[inline]
    pub fn configure_uart0(
        src: allwinner_rt::soc::d1::Peripherals<'a>,
    ) -> (
        Self,
        UART0,
        Function<'a, 'B', 8, 6>,
        Function<'a, 'B', 9, 6>,
    ) {
        let pb8 = src.gpio.pb8.into_function::<6>();
        let pb9 = src.gpio.pb9.into_function::<6>();
        let uart0 = src.uart0;
        let p = Self {
            gpio: soc::sun20iw1::Pads::__init(),
            spi0: src.spi0,
            com: src.com,
            ccu: src.ccu,
            plic: src.plic,
        };
        (p, uart0, pb8, pb9)
    }
}

/// Print SyterKit banner.
pub fn show_banner() {
    println!(" _____     _           _____ _ _   ");
    println!("|   __|_ _| |_ ___ ___|  |  |_| |_ ");
    println!("|__   | | |  _| -_|  _|    -| | _| ");
    println!("|_____|_  |_| |___|_| |__|__|_|_|  ");
    println!("      |___|                        ");
    println!("***********************************");
    println!(" syterkit v{}", env!("CARGO_PKG_VERSION")); // TODO: Git commit hash
    println!(" github.com/YuzukiHD/SyterKit      ");
    println!("***********************************");
    println!(" Built by: rustc"); // TODO: Detect Rustc version
    println!();
}

#[doc(hidden)]
pub static CONSOLE: Mutex<Option<SyterKitConsole<'static>>> = Mutex::new(None);

#[doc(hidden)]
pub struct SyterKitConsole<'a> {
    pub inner: Serial<UART0, 0, (Function<'a, 'B', 8, 6>, Function<'a, 'B', 9, 6>)>,
}

unsafe impl<'a> Send for SyterKitConsole<'a> {}

// macro internal, used in `print` and `println` macros in `macros.rs`.
#[doc(hidden)]
#[inline]
pub fn _print(args: core::fmt::Arguments) {
    if let Some(serial) = &mut *CONSOLE.lock() {
        serial.inner.write_fmt(args).unwrap();
    }
}

// macro internal code.
#[doc(hidden)]
pub use {allwinner_hal, allwinner_rt};
