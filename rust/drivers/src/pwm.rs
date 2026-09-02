// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::{DriverResult, INVALID_ARGUMENT};

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum PwmMode {
    Cycle,
    Pulse,
}

impl PwmMode {
    fn into_raw(self) -> raw::sunxi_pwm_mode_t {
        match self {
            Self::Cycle => raw::sunxi_pwm_mode_t_PWM_MODE_CYCLE,
            Self::Pulse => raw::sunxi_pwm_mode_t_PWM_MODE_PLUSE,
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum PwmPolarity {
    Inverted,
    Normal,
}

impl PwmPolarity {
    fn into_raw(self) -> raw::sunxi_pwm_polarity_t {
        match self {
            Self::Inverted => raw::sunxi_pwm_polarity_t_PWM_POLARITY_INVERSED,
            Self::Normal => raw::sunxi_pwm_polarity_t_PWM_POLARITY_NORMAL,
        }
    }
}

/// Safe Rust-owned values for one C `sunxi_pwm_config_t` operation.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct PwmConfig {
    pub duty_ns: u32,
    pub period_ns: u32,
    pub polarity: PwmPolarity,
    pub mode: PwmMode,
    pub pulse_count: u32,
}

impl PwmConfig {
    pub const fn new(duty_ns: u32, period_ns: u32) -> Self {
        Self {
            duty_ns,
            period_ns,
            polarity: PwmPolarity::Normal,
            mode: PwmMode::Cycle,
            pulse_count: 0,
        }
    }

    fn into_raw(self) -> raw::sunxi_pwm_config_t {
        raw::sunxi_pwm_config_t {
            duty_ns: self.duty_ns,
            period_ns: self.period_ns,
            polarity: self.polarity.into_raw(),
            pwm_mode: self.mode.into_raw(),
            pluse_count: self.pulse_count,
        }
    }
}

/// Exclusive Rust access to one C `sunxi_pwm_t` controller descriptor.
pub struct Pwm<'a> {
    raw: &'a mut raw::sunxi_pwm_t,
}

impl<'a> Pwm<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::sunxi_pwm_t) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self) {
        unsafe { raw::sunxi_pwm_init(self.raw) };
    }

    pub fn deinitialize(&mut self) {
        unsafe { raw::sunxi_pwm_deinit(self.raw) };
    }

    pub fn set_config(&mut self, channel: usize, config: PwmConfig) -> DriverResult<()> {
        let channel = i32::try_from(channel).map_err(|_| INVALID_ARGUMENT)?;
        let mut raw_config = config.into_raw();
        let result = unsafe { raw::sunxi_pwm_set_config(self.raw, channel, &mut raw_config) };
        syterkit_lib::status(result)
    }

    pub fn release(&mut self, channel: usize) -> DriverResult<()> {
        let channel = i32::try_from(channel).map_err(|_| INVALID_ARGUMENT)?;
        let result = unsafe { raw::sunxi_pwm_release(self.raw, channel) };
        syterkit_lib::status(result)
    }
}

#[cfg(test)]
mod tests {
    use super::{Pwm, PwmConfig, PwmMode, PwmPolarity};
    use core::ffi::c_int;
    use core::sync::atomic::{AtomicI32, AtomicU32, Ordering};
    use std::sync::Mutex;

    static TEST_LOCK: Mutex<()> = Mutex::new(());
    static PWM_STATUS: AtomicI32 = AtomicI32::new(0);
    static PWM_LAST_CHANNEL: AtomicI32 = AtomicI32::new(-1);
    static PWM_LAST_DUTY: AtomicU32 = AtomicU32::new(0);
    static PWM_LAST_PERIOD: AtomicU32 = AtomicU32::new(0);
    static PWM_LAST_POLARITY: AtomicU32 = AtomicU32::new(0);
    static PWM_LAST_MODE: AtomicU32 = AtomicU32::new(0);
    static PWM_LAST_PULSE_COUNT: AtomicU32 = AtomicU32::new(0);

    #[no_mangle]
    pub extern "C" fn sunxi_pwm_init(_pwm: *mut syterkit_ffi::raw::sunxi_pwm_t) {}

    #[no_mangle]
    pub extern "C" fn sunxi_pwm_deinit(_pwm: *mut syterkit_ffi::raw::sunxi_pwm_t) {}

    #[no_mangle]
    pub extern "C" fn sunxi_pwm_set_config(
        _pwm: *mut syterkit_ffi::raw::sunxi_pwm_t,
        channel: c_int,
        config: *mut syterkit_ffi::raw::sunxi_pwm_config_t,
    ) -> c_int {
        let config = unsafe { &*config };
        PWM_LAST_CHANNEL.store(channel, Ordering::Relaxed);
        PWM_LAST_DUTY.store(config.duty_ns, Ordering::Relaxed);
        PWM_LAST_PERIOD.store(config.period_ns, Ordering::Relaxed);
        PWM_LAST_POLARITY.store(config.polarity, Ordering::Relaxed);
        PWM_LAST_MODE.store(config.pwm_mode, Ordering::Relaxed);
        PWM_LAST_PULSE_COUNT.store(config.pluse_count, Ordering::Relaxed);
        PWM_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_pwm_release(
        _pwm: *mut syterkit_ffi::raw::sunxi_pwm_t,
        _channel: c_int,
    ) -> c_int {
        PWM_STATUS.load(Ordering::Relaxed)
    }

    #[test]
    fn pwm_maps_typed_configuration_and_status() {
        let _guard = TEST_LOCK.lock().unwrap();
        PWM_STATUS.store(0, Ordering::Relaxed);
        let mut raw_pwm: syterkit_ffi::raw::sunxi_pwm_t = unsafe { core::mem::zeroed() };
        let mut pwm = unsafe { Pwm::from_raw(&mut raw_pwm) };
        pwm.initialize();
        let mut config = PwmConfig::new(10_000, 20_000);
        config.polarity = PwmPolarity::Inverted;
        config.mode = PwmMode::Pulse;
        config.pulse_count = 4;
        pwm.set_config(3, config).unwrap();
        assert_eq!(PWM_LAST_CHANNEL.load(Ordering::Relaxed), 3);
        assert_eq!(PWM_LAST_DUTY.load(Ordering::Relaxed), 10_000);
        assert_eq!(PWM_LAST_PERIOD.load(Ordering::Relaxed), 20_000);
        assert_eq!(PWM_LAST_POLARITY.load(Ordering::Relaxed), 0);
        assert_eq!(PWM_LAST_MODE.load(Ordering::Relaxed), 1);
        assert_eq!(PWM_LAST_PULSE_COUNT.load(Ordering::Relaxed), 4);

        PWM_STATUS.store(-6, Ordering::Relaxed);
        assert_eq!(pwm.release(3), Err(-6));
        assert_eq!(pwm.set_config(usize::MAX, config), Err(-1));
        pwm.deinitialize();
    }
}
