#![no_std]
use allwinner_hal::{gpio::Function, uart::Serial};
use allwinner_rt::soc::d1::UART0;
use embedded_io::Write;
use spin::Mutex;

#[macro_use]
mod macros;

pub mod mctl;
pub mod soc;

pub use allwinner_hal::ccu::Clocks;
pub use syterkit_macros::entry;

#[cfg(feature = "sun20iw1")]
pub use soc::sun20iw1::{clock_dump, clock_init, Peripherals};

/// Print SyterKit banner.
pub fn show_banner() {
    println!(" _____     _           _____ _ _   ");
    println!("|   __|_ _| |_ ___ ___|  |  |_| |_ ");
    println!("|__   | | |  _| -_|  _|    -| | _| ");
    println!("|_____|_  |_| |___|_| |__|__|_|_|  ");
    println!("      |___|                        ");
    println!("***********************************");
    println!(
        " syterkit v{} Commit: {}",
        env!("CARGO_PKG_VERSION"),
        env!("SYTERKIT_GIT_HASH")
    );
    println!(" github.com/YuzukiHD/SyterKit      ");
    println!("***********************************");
    println!(" Built by: rustc {}", env!("SYTERKIT_RUSTC_VERSION"));
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

// macro internal code, used in `entry` proc macro.
#[doc(hidden)]
pub use {allwinner_hal, allwinner_rt};
