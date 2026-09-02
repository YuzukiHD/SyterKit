// SPDX-License-Identifier: GPL-2.0+

use crate::{eprintln, println, Psram, Spif, SpifNor, StdoutUart};
use crate::{CLOCK_TREE, CORE, TIMER};
use syterkit_ffi::raw;

/* These adapters belong to this application. Their C implementations wrap
 * header-local device-tree helpers, so they are intentionally not part of
 * the generated project-wide binding. */
mod app_ffi {
    use super::raw;
    use core::ffi::{c_char, c_int};

    extern "C" {
        pub fn sunxi_psram_dt_read_alias_ffi(
            psram: *mut raw::sunxi_psram_t,
            alias: *const c_char,
        ) -> c_int;
        pub fn sunxi_spif_dt_read_alias_ffi(
            spif: *mut raw::sunxi_spif_t,
            alias: *const c_char,
        ) -> c_int;
        pub fn spif_nor_dt_read_alias_ffi(
            nor: *mut raw::spif_nor_t,
            alias: *const c_char,
            spif: *mut raw::sunxi_spif_t,
        ) -> c_int;
    }
}

const YUZUKINEKO_PSRAM_BASE: usize = raw::SUNXI_PSRAM_BASE as usize;
const YUZUKINEKO_PSRAM_SIZE: usize = 0x0100_0000;
const YUZUKINEKO_HEAP_BASE: usize = YUZUKINEKO_PSRAM_BASE + 0x0010_0000;
const YUZUKINEKO_HEAP_SIZE: usize = YUZUKINEKO_PSRAM_SIZE - 0x0010_0000;
const NOR_READ_SIZE: u32 = 1024 * 1024;
const RUST_TOOLCHAIN: &[u8] = concat!(env!("SYTERKIT_RUST_TOOLCHAIN"), "\0").as_bytes();

pub fn main() -> i32 {
    let _stdout = match StdoutUart::init() {
        Ok(stdout) => stdout,
        Err(error) => return error,
    };

    let _ = CORE.show_banner_with_build_info(RUST_TOOLCHAIN);
    CLOCK_TREE.initialize();
    println!("Hello World!");
    CLOCK_TREE.dump();

    #[cfg(syterkit_config_driver_psram)]
    {
        let mut raw_psram: raw::sunxi_psram_t = unsafe { core::mem::zeroed() };
        if unsafe {
            app_ffi::sunxi_psram_dt_read_alias_ffi(
                &mut raw_psram,
                b"psram0\0".as_ptr().cast(),
            )
        } != 0
        {
            eprintln!("PSRAM: invalid devicetree configuration");
            return -1;
        }

        let mut psram = unsafe { Psram::from_raw(&mut raw_psram) };

        let _ = psram.initialize();
        println!("LPSRAM Size = {} MB", psram.size_mb());
    }

    if let Err(error) = CORE.malloc_init(YUZUKINEKO_HEAP_BASE, YUZUKINEKO_HEAP_SIZE) {
        eprintln!("Heap: PSRAM heap initialization failed ({})", error);
        return error;
    }

    let mut raw_spif: raw::sunxi_spif_t = unsafe { core::mem::zeroed() };
    if unsafe {
        app_ffi::sunxi_spif_dt_read_alias_ffi(&mut raw_spif, b"spif0\0".as_ptr().cast())
    } != 0
    {
        eprintln!("SPIF: invalid devicetree configuration");
        return -1;
    }

    let mut raw_nor: raw::spif_nor_t = unsafe { core::mem::zeroed() };
    if unsafe {
        app_ffi::spif_nor_dt_read_alias_ffi(
            &mut raw_nor,
            b"spif-nor0\0".as_ptr().cast(),
            &mut raw_spif,
        )
    } != 0
    {
        eprintln!("SPIF NOR: invalid devicetree configuration");
        return -1;
    }

    let mut spif = unsafe { Spif::from_raw(&mut raw_spif) };
    if let Err(error) = spif.initialize() {
        eprintln!("SPIF: controller init failed ({})", error);
        return error;
    }

    let mut nor = unsafe { SpifNor::from_raw(&mut raw_nor) };
    if let Err(error) = nor.detect() {
        eprintln!("SPI NOR: no supported flash detected ({})", error);
        return error;
    }

    let time_start = TIMER.milliseconds();
    let nor_read_done = unsafe {
        let buffer = core::slice::from_raw_parts_mut(
            YUZUKINEKO_PSRAM_BASE as *mut u8,
            NOR_READ_SIZE as usize,
        );
        match nor.read(0, buffer) {
            Ok(done) => done,
            Err(error) => {
                eprintln!("SPI NOR: read failed ({})", error);
                return error;
            }
        }
    };
    let time_end = TIMER.milliseconds();
    let delta = (time_end - time_start).max(1);
    let kib_per_second = (u64::from(nor_read_done) * 1000)
        / (u64::from(delta) * 1024);
    println!(
        "SPI NOR: read {}KiB in {}ms, {}KiB/s",
        nor_read_done / 1024,
        delta,
        kib_per_second
    );

    unsafe { CORE.dump_hex(YUZUKINEKO_PSRAM_BASE, 0x40) };
    CORE.run_shell();

    #[allow(unreachable_code)]
    {
        0
    }
}
