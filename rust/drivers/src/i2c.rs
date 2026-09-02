// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::DriverResult;

/// Exclusive Rust access to one C `sunxi_i2c_t` controller descriptor.
pub struct I2cBus<'a> {
    raw: &'a mut raw::sunxi_i2c_t,
}

impl<'a> I2cBus<'a> {
    /// Borrow a board-owned descriptor.
    ///
    /// The descriptor must be initialized with valid MMIO, clock, and GPIO
    /// fields before the C driver is called.
    pub unsafe fn from_raw(raw: &'a mut raw::sunxi_i2c_t) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self) {
        unsafe { raw::sunxi_i2c_init(self.raw) };
    }

    pub fn write(&mut self, address: u8, register: u32, value: u8) -> DriverResult<()> {
        let result = unsafe { raw::sunxi_i2c_write(self.raw, address, register, value) };
        syterkit_lib::status(result)
    }

    pub fn read(&mut self, address: u8, register: u32) -> DriverResult<u8> {
        let mut value = 0;
        let result = unsafe { raw::sunxi_i2c_read(self.raw, address, register, &mut value) };
        if result == 0 {
            Ok(value)
        } else {
            Err(result)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::I2cBus;
    use core::ffi::c_int;
    use core::sync::atomic::{AtomicBool, AtomicI32, AtomicU32, Ordering};
    use std::sync::Mutex;

    static TEST_LOCK: Mutex<()> = Mutex::new(());
    static I2C_INITIALIZED: AtomicBool = AtomicBool::new(false);
    static I2C_LAST_ADDR: AtomicU32 = AtomicU32::new(0);
    static I2C_LAST_REG: AtomicU32 = AtomicU32::new(0);
    static I2C_LAST_VALUE: AtomicU32 = AtomicU32::new(0);
    static I2C_STATUS: AtomicI32 = AtomicI32::new(0);

    #[no_mangle]
    pub extern "C" fn sunxi_i2c_init(_i2c: *mut syterkit_ffi::raw::sunxi_i2c_t) {
        I2C_INITIALIZED.store(true, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn sunxi_i2c_write(
        _i2c: *mut syterkit_ffi::raw::sunxi_i2c_t,
        addr: u8,
        reg: u32,
        value: u8,
    ) -> c_int {
        I2C_LAST_ADDR.store(addr as u32, Ordering::Relaxed);
        I2C_LAST_REG.store(reg, Ordering::Relaxed);
        I2C_LAST_VALUE.store(value as u32, Ordering::Relaxed);
        I2C_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_i2c_read(
        _i2c: *mut syterkit_ffi::raw::sunxi_i2c_t,
        addr: u8,
        reg: u32,
        value: *mut u8,
    ) -> c_int {
        I2C_LAST_ADDR.store(addr as u32, Ordering::Relaxed);
        I2C_LAST_REG.store(reg, Ordering::Relaxed);
        unsafe { *value = 0xa5 };
        I2C_STATUS.load(Ordering::Relaxed)
    }

    #[test]
    fn i2c_bus_maps_status_and_register_transfers() {
        let _guard = TEST_LOCK.lock().unwrap();
        I2C_INITIALIZED.store(false, Ordering::Relaxed);
        I2C_LAST_ADDR.store(0, Ordering::Relaxed);
        I2C_LAST_REG.store(0, Ordering::Relaxed);
        I2C_LAST_VALUE.store(0, Ordering::Relaxed);
        I2C_STATUS.store(0, Ordering::Relaxed);

        let mut raw_i2c: syterkit_ffi::raw::sunxi_i2c_t = unsafe { core::mem::zeroed() };
        let mut bus = unsafe { I2cBus::from_raw(&mut raw_i2c) };
        bus.initialize();
        bus.write(0x50, 0x1234, 0x5a).unwrap();
        assert_eq!(I2C_LAST_ADDR.load(Ordering::Relaxed), 0x50);
        assert_eq!(I2C_LAST_REG.load(Ordering::Relaxed), 0x1234);
        assert_eq!(I2C_LAST_VALUE.load(Ordering::Relaxed), 0x5a);
        assert_eq!(bus.read(0x50, 0x1234).unwrap(), 0xa5);
        assert!(I2C_INITIALIZED.load(Ordering::Relaxed));

        I2C_STATUS.store(-9, Ordering::Relaxed);
        assert!(matches!(bus.write(0x50, 0, 0), Err(-9)));
        assert!(matches!(bus.read(0x50, 0), Err(-9)));
    }
}
