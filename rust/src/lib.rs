#![no_std]

#[macro_use]
mod macros;

pub mod config;
mod dynamic_info;
pub mod mctl;
mod sdcard;
pub mod soc;
mod stdio;
mod time_source;

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

pub use config::{parse_config, Config};
pub use dynamic_info::DynamicInfo;
pub use sdcard::{load_from_sdcard, SdCardError};
pub use stdio::{stdin, stdout, Stdin, Stdout};
pub use time_source::{time_source, TimeSource};

/// SyterKit prelude.
pub mod prelude {
    pub use allwinner_hal::prelude::*;
}

// macro internal code, used by `print` and `println`.
#[doc(hidden)]
pub use stdio::{
    SyterKitStdinInner, SyterKitStdoutInner, _print, set_logger_stdout, STDIN, STDOUT,
};

// macro internal code, used in `entry` proc macro.
#[doc(hidden)]
pub use {allwinner_hal, allwinner_rt};
