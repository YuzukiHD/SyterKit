#![no_std]

#[macro_use]
mod macros;

pub mod mctl;
pub mod soc;
mod stdio;

pub use allwinner_hal::ccu::Clocks;
pub use syterkit_macros::entry;

#[cfg(feature = "sun20iw1")]
pub use soc::sun20iw1::{__clock_init, clock_dump, Peripherals};

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

pub use stdio::{stdin, stdout, Stdin, Stdout};

// macro internal code, used by `print` and `println`.
#[doc(hidden)]
pub use stdio::_print;

// macro internal code, used in `entry` proc macro.
#[doc(hidden)]
pub use {allwinner_hal, allwinner_rt};
