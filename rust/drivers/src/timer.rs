// SPDX-License-Identifier: GPL-2.0+

use core::ffi::c_void;

use syterkit_ffi::raw;
use syterkit_lib::{DriverResult, INVALID_ARGUMENT};

/// Architecture-independent access to the C timer and delay services.
pub struct Timer;

impl Timer {
    pub const fn new() -> Self {
        Self
    }

    /// Capture the current time as the C log/timer epoch.
    pub fn set_epoch(&self) {
        unsafe { raw::set_timer_count() };
    }

    /// Return the raw architecture timer counter.
    pub fn counter(&self) -> u64 {
        unsafe { raw::get_arch_counter() }
    }

    pub fn milliseconds(&self) -> u32 {
        unsafe { raw::time_ms() }
    }

    pub fn microseconds(&self) -> u64 {
        unsafe { raw::time_us() }
    }

    pub fn epoch_microseconds(&self) -> u32 {
        unsafe { raw::get_init_timestamp() }
    }

    pub fn delay_us(&self, microseconds: u32) {
        unsafe { raw::udelay(microseconds) };
    }

    pub fn delay_ms(&self, milliseconds: u32) {
        unsafe { raw::mdelay(milliseconds) };
    }

    pub fn spin(&self, loops: u32) {
        unsafe { raw::sdelay(loops) };
    }

    /// Initialize a caller-owned software timer.
    ///
    /// C retains the callback and context after this method returns. The
    /// caller must keep both valid until the timer is no longer scheduled.
    pub unsafe fn create<'a>(
        &self,
        storage: &'a mut raw::timer_t,
        callback: TimerCallback,
        context: *mut c_void,
    ) -> SoftwareTimer<'a> {
        unsafe { raw::timer_create(storage, Some(callback), context) };
        SoftwareTimer { raw: storage }
    }

    /// Advance all active software timers by one scheduler tick.
    pub fn handle(&self) {
        unsafe { raw::timer_handle() };
    }
}

pub const TIMER: Timer = Timer::new();

/// Callback ABI accepted by the C software timer list.
pub type TimerCallback = unsafe extern "C" fn(*mut c_void, u32);

/// Borrowed software timer storage owned by the caller.
pub struct SoftwareTimer<'a> {
    raw: &'a mut raw::timer_t,
}

impl SoftwareTimer<'_> {
    /// Start this timer. C keeps a pointer to the embedded task, so the
    /// storage and callback context must remain valid while it is active.
    pub unsafe fn start(&mut self, max_run_count: u32, interval: u32) -> DriverResult<()> {
        if interval == 0 {
            return Err(INVALID_ARGUMENT);
        }
        unsafe { raw::timer_start(self.raw, max_run_count, interval) };
        Ok(())
    }

    pub const fn interval(&self) -> u32 {
        self.raw.interval
    }

    pub const fn as_raw(&self) -> &raw::timer_t {
        self.raw
    }
}

#[cfg(test)]
mod tests {
    use super::{SoftwareTimer, Timer, TimerCallback};
    use core::ffi::c_void;
    use core::sync::atomic::{AtomicU32, AtomicU64, Ordering};
    use std::sync::Mutex;

    static TEST_LOCK: Mutex<()> = Mutex::new(());
    static COUNTER: AtomicU64 = AtomicU64::new(0);
    static MILLIS: AtomicU32 = AtomicU32::new(0);
    static MICROS: AtomicU64 = AtomicU64::new(0);
    static EPOCH: AtomicU32 = AtomicU32::new(0);
    static CREATED: AtomicU32 = AtomicU32::new(0);
    static STARTED: AtomicU32 = AtomicU32::new(0);
    static HANDLED: AtomicU32 = AtomicU32::new(0);

    #[no_mangle]
    pub extern "C" fn set_timer_count() {
        EPOCH.store(1, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn get_arch_counter() -> u64 {
        COUNTER.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn time_ms() -> u32 {
        MILLIS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn time_us() -> u64 {
        MICROS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn get_init_timestamp() -> u32 {
        EPOCH.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn udelay(_microseconds: u32) {}

    #[no_mangle]
    pub extern "C" fn mdelay(_milliseconds: u32) {}

    #[no_mangle]
    pub extern "C" fn sdelay(_loops: u32) {}

    #[no_mangle]
    pub extern "C" fn timer_create(
        storage: *mut syterkit_ffi::raw::timer_t,
        _callback: Option<TimerCallback>,
        _context: *mut c_void,
    ) {
        CREATED.fetch_add(1, Ordering::Relaxed);
        unsafe { (*storage).interval = 0 };
    }

    #[no_mangle]
    pub extern "C" fn timer_start(
        storage: *mut syterkit_ffi::raw::timer_t,
        _max_run_count: u32,
        interval: u32,
    ) {
        STARTED.fetch_add(1, Ordering::Relaxed);
        unsafe { (*storage).interval = interval };
    }

    #[no_mangle]
    pub extern "C" fn timer_handle() {
        HANDLED.fetch_add(1, Ordering::Relaxed);
    }

    unsafe extern "C" fn callback(_context: *mut c_void, _event: u32) {}

    #[test]
    fn timer_wraps_architecture_and_software_services() {
        let _guard = TEST_LOCK.lock().unwrap();
        COUNTER.store(0x1122, Ordering::Relaxed);
        MILLIS.store(33, Ordering::Relaxed);
        MICROS.store(44, Ordering::Relaxed);
        EPOCH.store(0, Ordering::Relaxed);
        CREATED.store(0, Ordering::Relaxed);
        STARTED.store(0, Ordering::Relaxed);
        HANDLED.store(0, Ordering::Relaxed);

        let timer = Timer::new();
        assert_eq!(timer.counter(), 0x1122);
        assert_eq!(timer.milliseconds(), 33);
        assert_eq!(timer.microseconds(), 44);
        timer.set_epoch();
        assert_eq!(timer.epoch_microseconds(), 1);
        timer.delay_us(1);
        timer.delay_ms(1);
        timer.spin(1);

        let mut storage: syterkit_ffi::raw::timer_t = unsafe { core::mem::zeroed() };
        let mut software: SoftwareTimer<'_> =
            unsafe { timer.create(&mut storage, callback, core::ptr::null_mut()) };
        assert_eq!(software.interval(), 0);
        assert_eq!(unsafe { software.start(2, 10) }, Ok(()));
        assert_eq!(software.interval(), 10);
        assert_eq!(unsafe { software.start(2, 0) }, Err(-1));
        timer.handle();

        assert_eq!(CREATED.load(Ordering::Relaxed), 1);
        assert_eq!(STARTED.load(Ordering::Relaxed), 1);
        assert_eq!(HANDLED.load(Ordering::Relaxed), 1);
    }
}
