// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::DriverResult;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct SpifConfig {
    pub valid: u32,
    pub speed_hz: u32,
    pub rx_dtr: u32,
    pub tx_dtr: u32,
    pub sample_mode: u32,
    pub sample_delay: u32,
}

impl SpifConfig {
    fn into_raw(self) -> raw::spif_cfg {
        raw::spif_cfg {
            valid: self.valid,
            speed_hz: self.speed_hz,
            rx_dtr_en: self.rx_dtr,
            tx_dtr_en: self.tx_dtr,
            sample_mode: self.sample_mode,
            sample_delay: self.sample_delay,
        }
    }
}

/// Borrowed wrapper for the high-speed SPI flash controller.
pub struct Spif<'a> {
    raw: &'a mut raw::sunxi_spif_t,
}

impl<'a> Spif<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::sunxi_spif_t) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_spif_init(self.raw) })
    }

    pub fn disable(&mut self) {
        unsafe { raw::sunxi_spif_disable(self.raw) };
    }

    pub fn select(&mut self, chip_select: u8) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_spif_select(self.raw, chip_select) })
    }

    pub fn update_clock(&mut self, speed_hz: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_spif_update_clk(self.raw, speed_hz) })
    }

    pub fn set_config(&mut self, config: SpifConfig) -> DriverResult<()> {
        let config = config.into_raw();
        syterkit_lib::status(unsafe { raw::sunxi_spif_set_config(self.raw, &config) })
    }

    /// Execute a prebuilt C SPI-memory operation. The operation may contain
    /// raw data pointers and therefore its validity is the caller's contract.
    pub unsafe fn execute(&mut self, operation: &raw::spi_mem_op) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_spif_exec_op(self.raw, operation) })
    }
}

#[cfg(test)]
mod tests {
    use super::{Spif, SpifConfig};
    use core::ffi::c_int;

    #[no_mangle]
    pub extern "C" fn sunxi_spif_init(_spif: *mut syterkit_ffi::raw::sunxi_spif_t) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_spif_disable(_spif: *mut syterkit_ffi::raw::sunxi_spif_t) {}
    #[no_mangle]
    pub extern "C" fn sunxi_spif_select(
        _spif: *mut syterkit_ffi::raw::sunxi_spif_t,
        _cs: u8,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_spif_update_clk(
        _spif: *mut syterkit_ffi::raw::sunxi_spif_t,
        _speed: u32,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_spif_set_config(
        _spif: *mut syterkit_ffi::raw::sunxi_spif_t,
        config: *const syterkit_ffi::raw::spif_cfg,
    ) -> c_int {
        assert_eq!(unsafe { (*config).speed_hz }, 20_000_000);
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_spif_exec_op(
        _spif: *mut syterkit_ffi::raw::sunxi_spif_t,
        _op: *const syterkit_ffi::raw::spi_mem_op,
    ) -> c_int {
        0
    }

    #[test]
    fn spif_wraps_controller_configuration() {
        let mut raw_spif: syterkit_ffi::raw::sunxi_spif_t = unsafe { core::mem::zeroed() };
        let mut spif = unsafe { Spif::from_raw(&mut raw_spif) };
        spif.initialize().unwrap();
        spif.select(0).unwrap();
        spif.update_clock(20_000_000).unwrap();
        spif.set_config(SpifConfig {
            valid: 1,
            speed_hz: 20_000_000,
            rx_dtr: 0,
            tx_dtr: 0,
            sample_mode: 0,
            sample_delay: 0,
        })
        .unwrap();
        spif.disable();
    }
}
