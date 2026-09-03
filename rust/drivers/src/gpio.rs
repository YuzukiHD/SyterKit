// SPDX-License-Identifier: GPL-2.0+

use core::{convert::Infallible, marker::PhantomData};
use embedded_hal::digital::{ErrorType, InputPin, OutputPin, PinState};
use syterkit_ffi::raw;
#[cfg(syterkit_config_driver_gpio_v2_pow)]
use syterkit_lib::{DriverResult, INVALID_ARGUMENT};

/// Pull configuration accepted by the C GPIO driver.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum GpioPull {
    Up,
    Down,
    None,
}

impl GpioPull {
    fn into_raw(self) -> raw::gpio_pull_t {
        match self {
            Self::Up => raw::gpio_pull_t_GPIO_PULL_UP,
            Self::Down => raw::gpio_pull_t_GPIO_PULL_DOWN,
            Self::None => raw::gpio_pull_t_GPIO_PULL_NONE,
        }
    }
}

/// Board-level GPIO power-mode setup.
pub struct Gpio;

impl Gpio {
    pub const fn new() -> Self {
        Self
    }

    pub fn initialize_power_mode(&self) {
        unsafe { raw::sunxi_gpio_power_mode_init() };
    }
}

pub const GPIO: Gpio = Gpio::new();

/// A typed wrapper around one C `gpio_mux_t` descriptor.
pub struct GpioPin {
    raw: raw::gpio_mux_t,
    _not_send_or_sync: PhantomData<*mut ()>,
}

impl GpioPin {
    /// Create a GPIO descriptor from a board-owned MMIO configuration.
    ///
    /// The descriptor must contain a valid controller base address and pin
    /// mapping. The C driver dereferences that address during operations.
    pub const unsafe fn from_raw(raw: raw::gpio_mux_t) -> Self {
        Self {
            raw,
            _not_send_or_sync: PhantomData,
        }
    }

    /// Create a descriptor from its scalar C fields.
    pub const unsafe fn new(base: usize, pin: raw::gpio_t, bank: u8, mux: u8) -> Self {
        Self::from_raw(raw::gpio_mux_t {
            base,
            pin,
            bank,
            mux,
        })
    }

    /// Access the original C descriptor for APIs not yet wrapped here.
    pub const fn as_raw(&self) -> &raw::gpio_mux_t {
        &self.raw
    }

    /// Configure this pin through the C driver.
    pub fn initialize(&self) {
        unsafe { raw::sunxi_gpio_init(&self.raw) };
    }

    pub fn set_pull(&self, pull: GpioPull) {
        unsafe { raw::sunxi_gpio_set_pull(&self.raw, pull.into_raw()) };
    }

    pub fn set_drive_strength(&self, drive: raw::gpio_drv_t) {
        unsafe { raw::sunxi_gpio_set_drv(&self.raw, drive) };
    }

    #[cfg(syterkit_config_driver_gpio_v2_pow)]
    pub fn io_voltage(&self) -> DriverResult<u32> {
        let value = unsafe { raw::sunxi_gpio_get_io_voltage(&self.raw) };
        if value < 0 {
            Err(value)
        } else {
            Ok(value as u32)
        }
    }

    #[cfg(syterkit_config_driver_gpio_v2_pow)]
    pub fn set_io_voltage(&self, voltage_uv: u32) -> DriverResult<()> {
        if voltage_uv != raw::GPIO_IO_VOLTAGE_1V8 && voltage_uv != raw::GPIO_IO_VOLTAGE_3V3 {
            return Err(INVALID_ARGUMENT);
        }
        syterkit_lib::status(unsafe { raw::sunxi_gpio_set_io_voltage(&self.raw, voltage_uv) })
    }
}

impl ErrorType for GpioPin {
    type Error = Infallible;
}

impl InputPin for GpioPin {
    fn is_high(&mut self) -> Result<bool, Self::Error> {
        Ok(unsafe { raw::sunxi_gpio_read(&self.raw) != 0 })
    }

    fn is_low(&mut self) -> Result<bool, Self::Error> {
        Ok(unsafe { raw::sunxi_gpio_read(&self.raw) == 0 })
    }
}

impl OutputPin for GpioPin {
    fn set_low(&mut self) -> Result<(), Self::Error> {
        self.set_state(PinState::Low)
    }

    fn set_high(&mut self) -> Result<(), Self::Error> {
        self.set_state(PinState::High)
    }

    fn set_state(&mut self, state: PinState) -> Result<(), Self::Error> {
        let value = match state {
            PinState::Low => raw::GPIO_LEVEL_LOW,
            PinState::High => raw::GPIO_LEVEL_HIGH,
        };
        unsafe { raw::sunxi_gpio_set_value(&self.raw, value as i32) };
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::{GpioPin, GpioPull, GPIO};
    use core::ffi::c_int;
    use core::sync::atomic::{AtomicBool, AtomicI32, AtomicU32, Ordering};
    use embedded_hal::digital::{InputPin, OutputPin, StatefulOutputPin};
    use std::sync::Mutex;

    static TEST_LOCK: Mutex<()> = Mutex::new(());
    static GPIO_INITIALIZED: AtomicBool = AtomicBool::new(false);
    static GPIO_VALUE: AtomicI32 = AtomicI32::new(0);
    static GPIO_PULL: AtomicU32 = AtomicU32::new(0);
    static GPIO_DRIVE: AtomicU32 = AtomicU32::new(0);
    #[cfg(syterkit_config_driver_gpio_v2_pow)]
    static GPIO_VOLTAGE: AtomicI32 = AtomicI32::new(1_800_000);

    #[no_mangle]
    pub extern "C" fn sunxi_gpio_init(_gpio: *const syterkit_ffi::raw::gpio_mux_t) {
        GPIO_INITIALIZED.store(true, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn sunxi_gpio_set_value(
        _gpio: *const syterkit_ffi::raw::gpio_mux_t,
        value: c_int,
    ) {
        GPIO_VALUE.store(value, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn sunxi_gpio_read(_gpio: *const syterkit_ffi::raw::gpio_mux_t) -> c_int {
        GPIO_VALUE.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_gpio_set_pull(
        _gpio: *const syterkit_ffi::raw::gpio_mux_t,
        pull: syterkit_ffi::raw::gpio_pull_t,
    ) {
        GPIO_PULL.store(pull, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn sunxi_gpio_set_drv(
        _gpio: *const syterkit_ffi::raw::gpio_mux_t,
        drive: syterkit_ffi::raw::gpio_drv_t,
    ) {
        GPIO_DRIVE.store(drive, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn sunxi_gpio_power_mode_init() {}

    #[cfg(syterkit_config_driver_gpio_v2_pow)]
    #[no_mangle]
    pub extern "C" fn sunxi_gpio_get_io_voltage(
        _gpio: *const syterkit_ffi::raw::gpio_mux_t,
    ) -> c_int {
        GPIO_VOLTAGE.load(Ordering::Relaxed)
    }

    #[cfg(syterkit_config_driver_gpio_v2_pow)]
    #[no_mangle]
    pub extern "C" fn sunxi_gpio_set_io_voltage(
        _gpio: *const syterkit_ffi::raw::gpio_mux_t,
        voltage: u32,
    ) -> c_int {
        GPIO_VOLTAGE.store(voltage as c_int, Ordering::Relaxed);
        0
    }

    #[test]
    fn gpio_pin_translates_safe_operations_to_c_abi() {
        let _guard = TEST_LOCK.lock().unwrap();
        GPIO_INITIALIZED.store(false, Ordering::Relaxed);
        GPIO_VALUE.store(0, Ordering::Relaxed);
        GPIO_PULL.store(0, Ordering::Relaxed);
        GPIO_DRIVE.store(0, Ordering::Relaxed);

        GPIO.initialize_power_mode();
        let mut pin = unsafe { GpioPin::new(0x1000, 7, 2, 1) };
        pin.initialize();
        pin.set_pull(GpioPull::Down);
        pin.set_drive_strength(3);
        pin.set_high().ok();

        assert!(GPIO_INITIALIZED.load(Ordering::Relaxed));
        assert_eq!(GPIO_PULL.load(Ordering::Relaxed), 1);
        assert_eq!(GPIO_DRIVE.load(Ordering::Relaxed), 3);
        assert!(pin.is_high().unwrap());
        pin.set_low().ok();
        assert!(!pin.is_high().unwrap());

        #[cfg(syterkit_config_driver_gpio_v2_pow)]
        {
            assert_eq!(pin.io_voltage(), Ok(1_800_000));
            pin.set_io_voltage(3_300_000).unwrap();
            assert_eq!(pin.io_voltage(), Ok(3_300_000));
            assert_eq!(pin.set_io_voltage(5_000_000), Err(-1));
        }
    }
}
