// SPDX-License-Identifier: GPL-2.0+

use core::ffi::c_void;

use syterkit_ffi::raw;
use syterkit_lib::{DriverResult, INVALID_ARGUMENT};

/// Common interrupt-controller operations shared by GIC, PLIC, and CLIC.
pub struct InterruptController;

impl InterruptController {
    /// Initialize the architecture-selected interrupt controller.
    pub fn initialize() -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::arch_interrupt_init() })
    }

    /// Shut down the architecture-selected interrupt controller.
    pub fn exit() -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::arch_interrupt_exit() })
    }

    /// Install a C ABI callback for an IRQ.
    ///
    /// The callback and its context must remain valid until the handler is
    /// freed. This is unsafe because C retains both pointers after return.
    pub unsafe fn install_handler(
        irq: i32,
        handler: Option<unsafe extern "C" fn(*mut c_void)>,
        data: *mut c_void,
    ) -> DriverResult<()> {
        if irq < 0 {
            return Err(INVALID_ARGUMENT);
        }
        unsafe { raw::irq_install_handler(irq, handler, data) };
        Ok(())
    }

    pub fn free_handler(irq: i32) -> DriverResult<()> {
        if irq < 0 {
            return Err(INVALID_ARGUMENT);
        }
        unsafe { raw::irq_free_handler(irq) };
        Ok(())
    }

    pub fn enable(irq: i32) -> DriverResult<()> {
        if irq < 0 {
            return Err(INVALID_ARGUMENT);
        }
        syterkit_lib::status(unsafe { raw::irq_enable(irq) })
    }

    pub fn disable(irq: i32) -> DriverResult<()> {
        if irq < 0 {
            return Err(INVALID_ARGUMENT);
        }
        syterkit_lib::status(unsafe { raw::irq_disable(irq) })
    }

    /// Dispatch an interrupt cause reported by the architecture trap entry.
    pub fn handle(cause: usize) -> bool {
        unsafe { raw::intc_handle_irq(cause as _) != 0 }
    }
}

#[cfg(test)]
mod tests {
    use super::InterruptController;
    use core::ffi::{c_int, c_void};
    use core::sync::atomic::{AtomicI32, AtomicUsize, Ordering};

    static IRQ: AtomicI32 = AtomicI32::new(-1);
    static ENABLED: AtomicI32 = AtomicI32::new(0);
    static CAUSE: AtomicUsize = AtomicUsize::new(0);

    #[no_mangle]
    pub extern "C" fn arch_interrupt_init() -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn arch_interrupt_exit() -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn irq_install_handler(
        irq: c_int,
        _handler: Option<unsafe extern "C" fn(*mut c_void)>,
        _data: *mut c_void,
    ) {
        IRQ.store(irq, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn irq_free_handler(irq: c_int) {
        IRQ.store(-irq, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn irq_enable(irq: c_int) -> c_int {
        ENABLED.store(irq, Ordering::Relaxed);
        0
    }

    #[no_mangle]
    pub extern "C" fn irq_disable(irq: c_int) -> c_int {
        ENABLED.store(-irq, Ordering::Relaxed);
        0
    }

    #[no_mangle]
    pub extern "C" fn intc_handle_irq(cause: usize) -> i8 {
        CAUSE.store(cause, Ordering::Relaxed);
        1
    }

    unsafe extern "C" fn handler(_data: *mut c_void) {}

    #[test]
    fn common_interrupt_api_dispatches_and_validates() {
        InterruptController::initialize().unwrap();
        unsafe {
            InterruptController::install_handler(5, Some(handler), core::ptr::null_mut()).unwrap();
        }
        InterruptController::enable(5).unwrap();
        assert!(InterruptController::handle(0x21));
        assert_eq!(IRQ.load(Ordering::Relaxed), 5);
        assert_eq!(ENABLED.load(Ordering::Relaxed), 5);
        assert_eq!(CAUSE.load(Ordering::Relaxed), 0x21);
        InterruptController::disable(5).unwrap();
        InterruptController::free_handler(5).unwrap();
        InterruptController::exit().unwrap();
        assert_eq!(InterruptController::enable(-1), Err(-1));
    }
}
