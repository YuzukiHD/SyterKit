// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::DriverResult;

/// Borrowed wrapper for a C remote processor instance.
pub struct RemoteProcessor<'a> {
    raw: &'a mut raw::sunxi_remoteproc_t,
}

impl<'a> RemoteProcessor<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::sunxi_remoteproc_t) -> Self {
        Self { raw }
    }

    pub fn reset(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_remoteproc_reset(self.raw) })
    }

    pub fn load(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_remoteproc_load(self.raw) })
    }

    pub fn load_buffer(&mut self, firmware: &[u8]) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::sunxi_remoteproc_load_buffer(self.raw, firmware.as_ptr().cast(), firmware.len())
        })
    }

    pub fn prepare(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_remoteproc_prepare(self.raw) })
    }

    pub fn start(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_remoteproc_start(self.raw) })
    }

    pub fn dump(&self) {
        unsafe { raw::sunxi_remoteproc_dump(self.raw) };
    }
}

#[cfg(test)]
mod tests {
    use super::RemoteProcessor;
    use core::ffi::{c_int, c_void};
    use core::sync::atomic::{AtomicI32, AtomicUsize, Ordering};

    static RPROC_STATUS: AtomicI32 = AtomicI32::new(0);
    static RPROC_BYTES: AtomicUsize = AtomicUsize::new(0);

    #[no_mangle]
    pub extern "C" fn sunxi_remoteproc_reset(
        _rproc: *mut syterkit_ffi::raw::sunxi_remoteproc_t,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_remoteproc_load(
        _rproc: *mut syterkit_ffi::raw::sunxi_remoteproc_t,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_remoteproc_load_buffer(
        _rproc: *mut syterkit_ffi::raw::sunxi_remoteproc_t,
        _firmware: *const c_void,
        size: usize,
    ) -> c_int {
        RPROC_BYTES.store(size, Ordering::Relaxed);
        RPROC_STATUS.load(Ordering::Relaxed)
    }
    #[no_mangle]
    pub extern "C" fn sunxi_remoteproc_prepare(
        _rproc: *mut syterkit_ffi::raw::sunxi_remoteproc_t,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_remoteproc_start(
        _rproc: *mut syterkit_ffi::raw::sunxi_remoteproc_t,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_remoteproc_dump(_rproc: *const syterkit_ffi::raw::sunxi_remoteproc_t) {}

    #[test]
    fn remoteproc_wraps_firmware_lifecycle() {
        RPROC_STATUS.store(0, Ordering::Relaxed);
        let mut raw_rproc: syterkit_ffi::raw::sunxi_remoteproc_t = unsafe { core::mem::zeroed() };
        let mut rproc = unsafe { RemoteProcessor::from_raw(&mut raw_rproc) };
        rproc.reset().unwrap();
        rproc.load().unwrap();
        rproc.load_buffer(&[1, 2, 3, 4]).unwrap();
        assert_eq!(RPROC_BYTES.load(Ordering::Relaxed), 4);
        rproc.prepare().unwrap();
        rproc.start().unwrap();
        rproc.dump();
        RPROC_STATUS.store(-3, Ordering::Relaxed);
        assert_eq!(rproc.load_buffer(&[],), Err(-3));
    }
}
