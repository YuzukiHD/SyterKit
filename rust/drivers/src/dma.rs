// SPDX-License-Identifier: GPL-2.0+

use core::ffi::c_void;
use core::marker::PhantomData;

use syterkit_ffi::raw;
use syterkit_lib::{DriverResult, INVALID_ARGUMENT};

/// Borrowed access to one C DMA controller.
pub struct DmaController<'a> {
    raw: &'a mut raw::sunxi_dma_t,
}

/// A requested DMA channel. The controller borrow prevents using the same
/// controller while this channel remains owned by Rust.
pub struct DmaChannel<'a> {
    handle: usize,
    _controller: PhantomData<&'a mut raw::sunxi_dma_t>,
}

impl<'a> DmaController<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::sunxi_dma_t) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self) {
        unsafe { raw::sunxi_dma_init(self.raw) };
    }

    pub fn exit(&mut self) {
        unsafe { raw::sunxi_dma_exit(self.raw) };
    }

    pub fn request(&mut self, dma_type: u32) -> DriverResult<DmaChannel<'_>> {
        let handle = unsafe { raw::sunxi_dma_request(self.raw, dma_type) };
        if handle == 0 {
            Err(INVALID_ARGUMENT)
        } else {
            Ok(DmaChannel {
                handle,
                _controller: PhantomData,
            })
        }
    }

    pub fn request_from_last(&mut self, dma_type: u32) -> DriverResult<DmaChannel<'_>> {
        let handle = unsafe { raw::sunxi_dma_request_from_last(self.raw, dma_type) };
        if handle == 0 {
            Err(INVALID_ARGUMENT)
        } else {
            Ok(DmaChannel {
                handle,
                _controller: PhantomData,
            })
        }
    }

    /// Run the C driver's built-in DRAM-to-DRAM integrity test.
    ///
    /// The C implementation writes four `u32` values per iteration and
    /// rounds the byte length up to four bytes. Requiring equal, four-word
    /// aligned slices keeps those accesses inside the Rust-owned buffers.
    pub fn test(&mut self, source: &mut [u32], destination: &mut [u32]) -> DriverResult<()> {
        if source.len() != destination.len() || source.len() % 4 != 0 {
            return Err(INVALID_ARGUMENT);
        }
        let bytes = source
            .len()
            .checked_mul(core::mem::size_of::<u32>())
            .and_then(|length| u32::try_from(length).ok())
            .ok_or(INVALID_ARGUMENT)?;
        let result = unsafe {
            raw::sunxi_dma_test(
                self.raw,
                source.as_mut_ptr(),
                destination.as_mut_ptr(),
                bytes,
            )
        };
        syterkit_lib::status(result)
    }
}

impl DmaChannel<'_> {
    pub const fn handle(&self) -> usize {
        self.handle
    }

    pub fn configure(&mut self, config: &mut raw::sunxi_dma_set_t) -> DriverResult<()> {
        let result = unsafe { raw::sunxi_dma_setting(self.handle, config) };
        syterkit_lib::status(result)
    }

    /// Start a transfer using addresses understood by the C DMA driver.
    ///
    /// This is unsafe because addresses may refer to MMIO or memory whose
    /// lifetime cannot be represented by Rust slices.
    pub unsafe fn start(
        &mut self,
        source: usize,
        destination: usize,
        bytes: u32,
    ) -> DriverResult<()> {
        let result = unsafe { raw::sunxi_dma_start(self.handle, source, destination, bytes) };
        syterkit_lib::status(result)
    }

    pub fn stop(&mut self) -> DriverResult<()> {
        let result = unsafe { raw::sunxi_dma_stop(self.handle) };
        syterkit_lib::status(result)
    }

    pub fn status(&self) -> i32 {
        unsafe { raw::sunxi_dma_querystatus(self.handle) }
    }

    pub unsafe fn install_interrupt(&mut self, context: *mut c_void) -> DriverResult<()> {
        let result = unsafe { raw::sunxi_dma_install_int(self.handle, context) };
        syterkit_lib::status(result)
    }

    pub fn enable_interrupt(&mut self) -> DriverResult<()> {
        let result = unsafe { raw::sunxi_dma_enable_int(self.handle) };
        syterkit_lib::status(result)
    }

    pub fn disable_interrupt(&mut self) -> DriverResult<()> {
        let result = unsafe { raw::sunxi_dma_disable_int(self.handle) };
        syterkit_lib::status(result)
    }

    pub fn free_interrupt(&mut self) -> DriverResult<()> {
        let result = unsafe { raw::sunxi_dma_free_int(self.handle) };
        syterkit_lib::status(result)
    }

    pub fn release(mut self) -> DriverResult<()> {
        self.release_inner()
    }

    fn release_inner(&mut self) -> DriverResult<()> {
        if self.handle == 0 {
            return Ok(());
        }
        let result = unsafe { raw::sunxi_dma_release(self.handle) };
        if result == 0 {
            self.handle = 0;
            Ok(())
        } else {
            Err(result)
        }
    }
}

impl Drop for DmaChannel<'_> {
    fn drop(&mut self) {
        let _ = self.release_inner();
    }
}

#[cfg(test)]
mod tests {
    use super::DmaController;
    use core::ffi::{c_int, c_void};
    use core::sync::atomic::{AtomicI32, AtomicUsize, Ordering};
    use std::sync::Mutex;

    static TEST_LOCK: Mutex<()> = Mutex::new(());
    static DMA_STATUS: AtomicI32 = AtomicI32::new(0);
    static DMA_TEST_STATUS: AtomicI32 = AtomicI32::new(0);
    static DMA_HANDLE: AtomicUsize = AtomicUsize::new(0x1000);
    static DMA_RELEASES: AtomicUsize = AtomicUsize::new(0);

    #[no_mangle]
    pub extern "C" fn sunxi_dma_init(_dma: *mut syterkit_ffi::raw::sunxi_dma_t) {}

    #[no_mangle]
    pub extern "C" fn sunxi_dma_exit(_dma: *mut syterkit_ffi::raw::sunxi_dma_t) {}

    #[no_mangle]
    pub extern "C" fn sunxi_dma_request(
        _dma: *mut syterkit_ffi::raw::sunxi_dma_t,
        _dma_type: u32,
    ) -> usize {
        DMA_HANDLE.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_dma_request_from_last(
        _dma: *mut syterkit_ffi::raw::sunxi_dma_t,
        _dma_type: u32,
    ) -> usize {
        DMA_HANDLE.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_dma_release(_handle: usize) -> c_int {
        DMA_RELEASES.fetch_add(1, Ordering::Relaxed);
        DMA_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_dma_setting(
        _handle: usize,
        _config: *mut syterkit_ffi::raw::sunxi_dma_set_t,
    ) -> c_int {
        DMA_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_dma_start(
        _handle: usize,
        _source: usize,
        _destination: usize,
        _bytes: u32,
    ) -> c_int {
        DMA_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_dma_stop(_handle: usize) -> c_int {
        DMA_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_dma_querystatus(_handle: usize) -> c_int {
        DMA_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_dma_install_int(_handle: usize, _context: *mut c_void) -> c_int {
        DMA_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_dma_enable_int(_handle: usize) -> c_int {
        DMA_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_dma_disable_int(_handle: usize) -> c_int {
        DMA_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_dma_free_int(_handle: usize) -> c_int {
        DMA_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_dma_test(
        _dma: *mut syterkit_ffi::raw::sunxi_dma_t,
        _source: *mut u32,
        _destination: *mut u32,
        _bytes: u32,
    ) -> c_int {
        DMA_TEST_STATUS.load(Ordering::Relaxed)
    }

    #[test]
    fn dma_channel_owns_and_releases_c_handle() {
        let _guard = TEST_LOCK.lock().unwrap();
        DMA_STATUS.store(0, Ordering::Relaxed);
        DMA_RELEASES.store(0, Ordering::Relaxed);
        let mut raw_dma: syterkit_ffi::raw::sunxi_dma_t = unsafe { core::mem::zeroed() };
        let mut controller = unsafe { DmaController::from_raw(&mut raw_dma) };
        controller.initialize();
        let mut channel = controller.request(0).unwrap();
        assert_eq!(channel.handle(), 0x1000);
        assert_eq!(channel.status(), 0);
        unsafe { channel.start(0x2000, 0x3000, 64) }.unwrap();
        channel.stop().unwrap();
        channel.release().unwrap();
        assert_eq!(DMA_RELEASES.load(Ordering::Relaxed), 1);
        controller.exit();
    }

    #[test]
    fn dma_request_reports_empty_channel_pool() {
        let _guard = TEST_LOCK.lock().unwrap();
        DMA_HANDLE.store(0, Ordering::Relaxed);
        let mut raw_dma: syterkit_ffi::raw::sunxi_dma_t = unsafe { core::mem::zeroed() };
        let mut controller = unsafe { DmaController::from_raw(&mut raw_dma) };
        assert_eq!(controller.request(0).map(|_| ()), Err(-1));
        DMA_HANDLE.store(0x1000, Ordering::Relaxed);
    }

    #[test]
    fn dma_controller_validates_and_runs_memory_test() {
        let _guard = TEST_LOCK.lock().unwrap();
        DMA_TEST_STATUS.store(0, Ordering::Relaxed);
        let mut raw_dma: syterkit_ffi::raw::sunxi_dma_t = unsafe { core::mem::zeroed() };
        let mut controller = unsafe { DmaController::from_raw(&mut raw_dma) };
        let mut source = [0u32; 4];
        let mut destination = [0u32; 4];
        controller.test(&mut source, &mut destination).unwrap();
        assert_eq!(controller.test(&mut source[..3], &mut destination), Err(-1));

        DMA_TEST_STATUS.store(-2, Ordering::Relaxed);
        assert_eq!(controller.test(&mut source, &mut destination), Err(-2));
    }
}
