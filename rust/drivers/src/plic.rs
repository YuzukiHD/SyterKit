// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::DriverResult;

/// Borrowed wrapper for a RISC-V PLIC descriptor.
pub struct Plic<'a> {
    raw: &'a mut raw::sunxi_plic_t,
}

impl<'a> Plic<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::sunxi_plic_t) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_plic_init(self.raw) })
    }

    pub fn exit(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_plic_exit(self.raw) })
    }

    pub fn startup() -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_plic_startup() })
    }

    pub fn handle_irq() {
        unsafe { raw::sunxi_plic_handle_irq() };
    }
}

#[cfg(test)]
mod tests {
    use super::Plic;
    use core::ffi::c_int;

    #[no_mangle]
    pub extern "C" fn sunxi_plic_init(_plic: *mut syterkit_ffi::raw::sunxi_plic_t) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn sunxi_plic_exit(_plic: *mut syterkit_ffi::raw::sunxi_plic_t) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn sunxi_plic_startup() -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn sunxi_plic_handle_irq() {}

    #[test]
    fn plic_wraps_instance_lifecycle() {
        let mut raw_plic: syterkit_ffi::raw::sunxi_plic_t = unsafe { core::mem::zeroed() };
        let mut plic = unsafe { Plic::from_raw(&mut raw_plic) };
        plic.initialize().unwrap();
        Plic::startup().unwrap();
        Plic::handle_irq();
        plic.exit().unwrap();
    }
}
