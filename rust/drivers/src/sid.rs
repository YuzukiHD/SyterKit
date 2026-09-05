// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::DriverResult;

/// Borrowed access to a C `sunxi_sid_t` eFuse controller descriptor.
pub struct Sid<'a> {
    raw: &'a raw::sunxi_sid_t,
}

impl<'a> Sid<'a> {
    pub unsafe fn from_raw(raw: &'a raw::sunxi_sid_t) -> Self {
        Self { raw }
    }

    pub fn read_sram(&self, offset: u32) -> u32 {
        unsafe { raw::sunxi_efuse_sram_read(self.raw, offset) }
    }

    pub fn read(&self, offset: u32) -> u32 {
        unsafe { raw::sunxi_efuse_read(self.raw, offset) }
    }

    pub fn write(&self, offset: u32, value: u32) -> DriverResult<()> {
        let result = unsafe { raw::sunxi_efuse_write(self.raw, offset, value) };
        syterkit_lib::status(result)
    }

    /// Writing eFuse is irreversible on hardware; this method preserves the
    /// C driver's explicit operation without hiding that property.
    pub unsafe fn dump(&self) {
        unsafe { raw::sunxi_efuse_dump(self.raw) };
    }
}

#[cfg(test)]
mod tests {
    use super::Sid;
    use core::ffi::c_int;
    use core::sync::atomic::{AtomicI32, AtomicU32, Ordering};
    use std::sync::Mutex;

    static TEST_LOCK: Mutex<()> = Mutex::new(());
    static SID_VALUE: AtomicU32 = AtomicU32::new(0);
    static SID_STATUS: AtomicI32 = AtomicI32::new(0);

    #[no_mangle]
    pub extern "C" fn sunxi_efuse_sram_read(
        _sid: *const syterkit_ffi::raw::sunxi_sid_t,
        _offset: u32,
    ) -> u32 {
        SID_VALUE.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_efuse_read(
        _sid: *const syterkit_ffi::raw::sunxi_sid_t,
        _offset: u32,
    ) -> u32 {
        SID_VALUE.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_efuse_write(
        _sid: *const syterkit_ffi::raw::sunxi_sid_t,
        _offset: u32,
        value: u32,
    ) -> c_int {
        SID_VALUE.store(value, Ordering::Relaxed);
        SID_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_efuse_dump(_sid: *const syterkit_ffi::raw::sunxi_sid_t) {}

    #[test]
    fn sid_preserves_e_fuse_reads_and_write_status() {
        let _guard = TEST_LOCK.lock().unwrap();
        SID_VALUE.store(0, Ordering::Relaxed);
        SID_STATUS.store(0, Ordering::Relaxed);
        let raw_sid = syterkit_ffi::raw::sunxi_sid_t {
            dt_node: 0,
            base: 0x1000,
            size: 0x100,
            sram_base: 0x2000,
            efuse_hv_switch: 0,
        };
        let sid = unsafe { Sid::from_raw(&raw_sid) };
        sid.write(0x88, 0xcafebabe).unwrap();
        assert_eq!(sid.read(0x88), 0xcafebabe);
        assert_eq!(sid.read_sram(0x88), 0xcafebabe);
        unsafe { sid.dump() };

        SID_STATUS.store(-4, Ordering::Relaxed);
        assert_eq!(sid.write(0x88, 0), Err(-4));
    }
}
