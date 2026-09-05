// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::{DriverResult, INVALID_ARGUMENT};

/// Borrowed wrapper for an ARM GIC descriptor.
pub struct Gic<'a> {
    raw: &'a mut raw::sunxi_gic_t,
}

impl<'a> Gic<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::sunxi_gic_t) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_gic_init(self.raw) })
    }

    pub fn exit(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_gic_exit(self.raw) })
    }

    pub fn startup() -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_gic_startup() })
    }

    pub fn initialize_cpu(cpu: i32) -> DriverResult<()> {
        if cpu < 0 {
            return Err(INVALID_ARGUMENT);
        }
        syterkit_lib::status(unsafe { raw::sunxi_gic_cpu_interface_init(cpu) })
    }

    pub fn exit_cpu() -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_gic_cpu_interface_exit() })
    }

    /// Dispatch an ARM exception frame through the C IRQ implementation.
    pub unsafe fn dispatch(regs: &mut raw::arm_regs_t) {
        unsafe { raw::do_irq(regs) };
    }
}

#[cfg(test)]
mod tests {
    use super::Gic;
    use core::ffi::c_int;

    #[no_mangle]
    pub extern "C" fn sunxi_gic_init(_gic: *mut syterkit_ffi::raw::sunxi_gic_t) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn sunxi_gic_exit(_gic: *mut syterkit_ffi::raw::sunxi_gic_t) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn sunxi_gic_startup() -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn sunxi_gic_cpu_interface_init(_cpu: c_int) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn sunxi_gic_cpu_interface_exit() -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn do_irq(_regs: *mut syterkit_ffi::raw::arm_regs_t) {}

    #[test]
    fn gic_wraps_instance_and_cpu_lifecycle() {
        let mut raw_gic: syterkit_ffi::raw::sunxi_gic_t = unsafe { core::mem::zeroed() };
        let mut gic = unsafe { Gic::from_raw(&mut raw_gic) };
        gic.initialize().unwrap();
        Gic::startup().unwrap();
        Gic::initialize_cpu(0).unwrap();
        Gic::exit_cpu().unwrap();
        let mut regs: syterkit_ffi::raw::arm_regs_t = unsafe { core::mem::zeroed() };
        unsafe { Gic::dispatch(&mut regs) };
        gic.exit().unwrap();
        assert_eq!(Gic::initialize_cpu(-1), Err(-1));
    }
}
