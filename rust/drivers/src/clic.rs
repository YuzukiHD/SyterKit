// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::DriverResult;

/// Borrowed wrapper for a RISC-V CLIC descriptor.
pub struct Clic<'a> {
    raw: &'a mut raw::sunxi_clic_t,
}

impl<'a> Clic<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::sunxi_clic_t) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_clic_init(self.raw) })
    }

    pub fn exit(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_clic_exit(self.raw) })
    }

    pub fn startup() -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_clic_startup() })
    }
}

#[cfg(test)]
mod tests {
    use super::Clic;
    use core::ffi::c_int;

    #[no_mangle]
    pub extern "C" fn sunxi_clic_init(_clic: *mut syterkit_ffi::raw::sunxi_clic_t) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn sunxi_clic_exit(_clic: *mut syterkit_ffi::raw::sunxi_clic_t) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn sunxi_clic_startup() -> c_int {
        0
    }

    #[test]
    fn clic_wraps_instance_lifecycle() {
        let mut raw_clic: syterkit_ffi::raw::sunxi_clic_t = unsafe { core::mem::zeroed() };
        let mut clic = unsafe { Clic::from_raw(&mut raw_clic) };
        clic.initialize().unwrap();
        Clic::startup().unwrap();
        clic.exit().unwrap();
    }
}
