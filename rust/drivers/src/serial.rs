// SPDX-License-Identifier: GPL-2.0+

use core::ffi::{c_char, c_void};
use core::marker::PhantomData;

use syterkit_ffi::raw;
use syterkit_lib::DriverResult;

/// A Rust-owned handle for the device-tree-selected UART.
pub struct StdoutUart {
    raw: *mut raw::sunxi_serial_t,
    _not_send_or_sync: PhantomData<*mut ()>,
}

/// Borrowed wrapper for a caller-owned UART descriptor.
pub struct SerialPort<'a> {
    raw: &'a mut raw::sunxi_serial_t,
}

impl<'a> SerialPort<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::sunxi_serial_t) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self) {
        unsafe { raw::sunxi_serial_init(self.raw) };
    }

    pub fn putc(&mut self, value: u8) {
        unsafe {
            raw::sunxi_serial_putc(
                (self.raw as *mut raw::sunxi_serial_t).cast::<c_void>(),
                value as c_char,
            )
        };
    }

    pub fn write_bytes(&mut self, bytes: &[u8]) {
        for &byte in bytes {
            self.putc(byte);
        }
    }

    pub fn has_data(&mut self) -> bool {
        unsafe {
            raw::sunxi_serial_tstc((self.raw as *mut raw::sunxi_serial_t).cast::<c_void>()) != 0
        }
    }

    pub fn getc(&mut self) -> u8 {
        unsafe {
            raw::sunxi_serial_getc((self.raw as *mut raw::sunxi_serial_t).cast::<c_void>()) as u8
        }
    }
}

impl StdoutUart {
    /// Initialize and claim the global SyterKit stdout descriptor.
    pub fn init() -> DriverResult<Self> {
        let result = unsafe { raw::sunxi_serial_init_stdout() };
        if result != 0 {
            return Err(result);
        }

        Ok(Self {
            raw: core::ptr::addr_of_mut!(raw::uart_dbg),
            _not_send_or_sync: PhantomData,
        })
    }

    /// Send one byte through this UART.
    pub fn putc(&mut self, value: u8) {
        unsafe { raw::sunxi_serial_putc(self.raw.cast::<c_void>(), value as c_char) };
    }

    /// Send a byte slice through this UART.
    pub fn write_bytes(&mut self, bytes: &[u8]) {
        for &byte in bytes {
            self.putc(byte);
        }
    }

    /// Return whether a byte is available without consuming it.
    pub fn has_data(&mut self) -> bool {
        unsafe { raw::sunxi_serial_tstc(self.raw.cast::<c_void>()) != 0 }
    }

    /// Receive one byte when the UART FIFO is ready.
    pub fn try_getc(&mut self) -> Option<u8> {
        if self.has_data() {
            Some(self.getc())
        } else {
            None
        }
    }

    /// Receive one byte. The C driver defines the result for an empty FIFO.
    pub fn getc(&mut self) -> u8 {
        unsafe { raw::sunxi_serial_getc(self.raw.cast::<c_void>()) as u8 }
    }
}

impl core::fmt::Write for StdoutUart {
    fn write_str(&mut self, value: &str) -> core::fmt::Result {
        self.write_bytes(value.as_bytes());
        Ok(())
    }
}

/// Write through the global C stdout descriptor from a C-facing core shim.
pub unsafe fn putc_stdout(value: u8) {
    let uart = core::ptr::addr_of_mut!(raw::uart_dbg);
    unsafe { raw::sunxi_serial_putc(uart.cast::<c_void>(), value as c_char) };
}

#[cfg(test)]
mod tests {
    use super::{SerialPort, StdoutUart};
    use core::ffi::{c_char, c_int, c_void};
    use core::sync::atomic::{AtomicBool, AtomicI32, AtomicU32, AtomicUsize, Ordering};
    use std::sync::Mutex;

    static TEST_LOCK: Mutex<()> = Mutex::new(());
    static SERIAL_INIT_RESULT: AtomicI32 = AtomicI32::new(0);
    static SERIAL_WRITE_COUNT: AtomicUsize = AtomicUsize::new(0);
    static SERIAL_RX_READY: AtomicBool = AtomicBool::new(true);
    static SERIAL_RX_VALUE: AtomicU32 = AtomicU32::new(b'R' as u32);

    #[no_mangle]
    pub static mut uart_dbg: syterkit_ffi::raw::sunxi_serial_t = unsafe { core::mem::zeroed() };

    #[no_mangle]
    pub extern "C" fn sunxi_serial_init_stdout() -> c_int {
        SERIAL_INIT_RESULT.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_serial_init(_uart: *mut syterkit_ffi::raw::sunxi_serial_t) {}

    #[no_mangle]
    pub extern "C" fn sunxi_serial_putc(_arg: *mut c_void, _value: c_char) {
        SERIAL_WRITE_COUNT.fetch_add(1, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn sunxi_serial_tstc(_arg: *mut c_void) -> c_int {
        SERIAL_RX_READY.load(Ordering::Relaxed) as c_int
    }

    #[no_mangle]
    pub extern "C" fn sunxi_serial_getc(_arg: *mut c_void) -> c_char {
        SERIAL_RX_VALUE.load(Ordering::Relaxed) as c_char
    }

    #[test]
    fn stdout_uart_supports_output_and_nonblocking_input() {
        let _guard = TEST_LOCK.lock().unwrap();
        SERIAL_INIT_RESULT.store(0, Ordering::Relaxed);
        SERIAL_WRITE_COUNT.store(0, Ordering::Relaxed);
        SERIAL_RX_READY.store(true, Ordering::Relaxed);
        SERIAL_RX_VALUE.store(b'R' as u32, Ordering::Relaxed);

        let mut uart = StdoutUart::init().unwrap();
        uart.write_bytes(b"abc");
        assert_eq!(SERIAL_WRITE_COUNT.load(Ordering::Relaxed), 3);
        assert_eq!(uart.try_getc(), Some(b'R'));

        SERIAL_RX_READY.store(false, Ordering::Relaxed);
        assert_eq!(uart.try_getc(), None);
    }

    #[test]
    fn stdout_uart_propagates_initialization_errors() {
        let _guard = TEST_LOCK.lock().unwrap();
        SERIAL_INIT_RESULT.store(-7, Ordering::Relaxed);
        assert!(matches!(StdoutUart::init(), Err(-7)));
        SERIAL_INIT_RESULT.store(0, Ordering::Relaxed);
    }

    #[test]
    fn caller_owned_serial_port_uses_the_same_c_abi() {
        let _guard = TEST_LOCK.lock().unwrap();
        SERIAL_WRITE_COUNT.store(0, Ordering::Relaxed);
        let mut raw_uart: syterkit_ffi::raw::sunxi_serial_t = unsafe { core::mem::zeroed() };
        let mut uart = unsafe { SerialPort::from_raw(&mut raw_uart) };
        uart.initialize();
        uart.write_bytes(b"ok");
        assert_eq!(SERIAL_WRITE_COUNT.load(Ordering::Relaxed), 2);
    }
}
