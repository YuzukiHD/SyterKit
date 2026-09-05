// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::DriverResult;

/// Borrowed access to the RTC persistent-data area.
pub struct Rtc<'a> {
    raw: &'a raw::sunxi_rtc_t,
}

impl<'a> Rtc<'a> {
    pub unsafe fn from_raw(raw: &'a raw::sunxi_rtc_t) -> Self {
        Self { raw }
    }

    pub fn write_data(&self, index: i32, value: u32) {
        unsafe { raw::rtc_write_data(self.raw, index, value) };
    }

    pub fn read_data(&self, index: i32) -> u32 {
        unsafe { raw::rtc_read_data(self.raw, index) }
    }

    pub fn set_fel_flag(&self) {
        unsafe { raw::rtc_set_fel_flag(self.raw) };
    }

    pub fn set_start_time_ms(&self) {
        unsafe { raw::rtc_set_start_time_ms(self.raw) };
    }

    pub fn set_dram_parameter(&self, address: u32) {
        unsafe { raw::rtc_set_dram_para(self.raw, address) };
    }

    /// Configure the board-specific spare register used for VCCIO detection.
    pub fn set_vccio_detect_spare(&self) {
        unsafe { raw::rtc_set_vccio_det_spare(self.raw) };
    }

    pub fn fel_flag_is_set(&self) -> bool {
        unsafe { raw::rtc_probe_fel_flag(self.raw) != 0 }
    }

    pub fn clear_fel_flag(&self) {
        unsafe { raw::rtc_clear_fel_flag(self.raw) };
    }

    pub fn set_bootmode_flag(&self, flag: u8) -> DriverResult<()> {
        let result = unsafe { raw::rtc_set_bootmode_flag(self.raw, flag) };
        syterkit_lib::status(result)
    }

    pub fn bootmode_flag(&self) -> DriverResult<u8> {
        let result = unsafe { raw::rtc_get_bootmode_flag(self.raw) };
        if result < 0 {
            Err(result)
        } else {
            Ok(result as u8)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Rtc;
    use core::ffi::c_int;
    use core::sync::atomic::{AtomicI32, AtomicU32, Ordering};
    use std::sync::Mutex;

    static TEST_LOCK: Mutex<()> = Mutex::new(());
    static RTC_VALUE: AtomicU32 = AtomicU32::new(0);
    static RTC_BOOTMODE: AtomicI32 = AtomicI32::new(0);
    static RTC_STATUS: AtomicI32 = AtomicI32::new(0);

    #[no_mangle]
    pub extern "C" fn rtc_write_data(
        _rtc: *const syterkit_ffi::raw::sunxi_rtc_t,
        _index: c_int,
        value: u32,
    ) {
        RTC_VALUE.store(value, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn rtc_read_data(
        _rtc: *const syterkit_ffi::raw::sunxi_rtc_t,
        _index: c_int,
    ) -> u32 {
        RTC_VALUE.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn rtc_set_fel_flag(_rtc: *const syterkit_ffi::raw::sunxi_rtc_t) {
        RTC_VALUE.store(0x5AA5A55A, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn rtc_set_start_time_ms(_rtc: *const syterkit_ffi::raw::sunxi_rtc_t) {}

    #[no_mangle]
    pub extern "C" fn rtc_set_dram_para(_rtc: *const syterkit_ffi::raw::sunxi_rtc_t, address: u32) {
        RTC_VALUE.store(address, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn rtc_set_vccio_det_spare(_rtc: *const syterkit_ffi::raw::sunxi_rtc_t) {}

    #[no_mangle]
    pub extern "C" fn rtc_probe_fel_flag(_rtc: *const syterkit_ffi::raw::sunxi_rtc_t) -> u32 {
        (RTC_VALUE.load(Ordering::Relaxed) == 0x5AA5A55A) as u32
    }

    #[no_mangle]
    pub extern "C" fn rtc_clear_fel_flag(_rtc: *const syterkit_ffi::raw::sunxi_rtc_t) {
        RTC_VALUE.store(0, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn rtc_set_bootmode_flag(
        _rtc: *const syterkit_ffi::raw::sunxi_rtc_t,
        flag: u8,
    ) -> c_int {
        RTC_BOOTMODE.store(flag as c_int, Ordering::Relaxed);
        RTC_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn rtc_get_bootmode_flag(_rtc: *const syterkit_ffi::raw::sunxi_rtc_t) -> c_int {
        RTC_BOOTMODE.load(Ordering::Relaxed)
    }

    #[test]
    fn rtc_maps_persistent_flags_and_status() {
        let _guard = TEST_LOCK.lock().unwrap();
        RTC_VALUE.store(0, Ordering::Relaxed);
        RTC_STATUS.store(0, Ordering::Relaxed);
        let raw_rtc = syterkit_ffi::raw::sunxi_rtc_t {
            dt_node: 0,
            data_base: 0x1000,
            data_size: 32,
        };
        let rtc = unsafe { Rtc::from_raw(&raw_rtc) };
        rtc.write_data(1, 0x1234);
        assert_eq!(rtc.read_data(1), 0x1234);
        rtc.set_fel_flag();
        assert!(rtc.fel_flag_is_set());
        rtc.clear_fel_flag();
        assert!(!rtc.fel_flag_is_set());
        rtc.set_dram_parameter(0x8000);
        assert_eq!(RTC_VALUE.load(Ordering::Relaxed), 0x8000);
        rtc.set_vccio_detect_spare();
        rtc.set_bootmode_flag(7).unwrap();
        assert_eq!(rtc.bootmode_flag(), Ok(7));

        RTC_STATUS.store(-5, Ordering::Relaxed);
        assert_eq!(rtc.set_bootmode_flag(8), Err(-5));
        RTC_BOOTMODE.store(-5, Ordering::Relaxed);
        assert_eq!(rtc.bootmode_flag(), Err(-5));
    }
}
