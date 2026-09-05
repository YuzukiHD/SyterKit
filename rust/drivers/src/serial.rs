// SPDX-License-Identifier: GPL-2.0+

use core::{
    ffi::{c_char, c_void},
    marker::PhantomData,
};

use embedded_io::{ErrorKind, ErrorType, Read, Write};

use syterkit_ffi::raw;
use syterkit_lib::DriverResult;

/// A Rust-owned handle for the device-tree-selected UART.
pub struct StdoutUart {
    raw: *mut raw::sunxi_serial_t,
    _not_send_or_sync: PhantomData<*mut ()>,
}

/// Hardware errors reported while receiving from a UART.
#[derive(Debug, Clone, Copy, Eq, PartialEq)]
#[non_exhaustive]
pub enum SerialError {
    Overrun,
    Parity,
    Framing,
    Break,
}

impl embedded_io::Error for SerialError {
    fn kind(&self) -> ErrorKind {
        match self {
            Self::Overrun => ErrorKind::Other,
            Self::Parity | Self::Framing | Self::Break => ErrorKind::InvalidData,
        }
    }
}

#[derive(Clone, Copy)]
struct LineStatus(u32);

impl LineStatus {
    const DATA_READY: u32 = 1 << 0;
    const OVERRUN_ERROR: u32 = 1 << 1;
    const PARITY_ERROR: u32 = 1 << 2;
    const FRAMING_ERROR: u32 = 1 << 3;
    const BREAK_INTERRUPT: u32 = 1 << 4;

    fn data_ready(self) -> bool {
        self.0 & Self::DATA_READY != 0
    }

    fn receive_error(self) -> Option<SerialError> {
        if self.0 & Self::OVERRUN_ERROR != 0 {
            Some(SerialError::Overrun)
        } else if self.0 & Self::PARITY_ERROR != 0 {
            Some(SerialError::Parity)
        } else if self.0 & Self::FRAMING_ERROR != 0 {
            Some(SerialError::Framing)
        } else if self.0 & Self::BREAK_INTERRUPT != 0 {
            Some(SerialError::Break)
        } else {
            None
        }
    }
}

fn line_status(serial: *mut c_void) -> LineStatus {
    LineStatus(unsafe { raw::sunxi_serial_get_status(serial) })
}

/// Read at least one byte from a UART, then drain any bytes already available.
///
/// An empty buffer returns immediately. For a non-empty buffer this function
/// blocks until a byte or receive error is reported by the UART.
pub fn read_blocking(
    serial: &mut raw::sunxi_serial_t,
    buf: &mut [u8],
) -> Result<usize, SerialError> {
    let Some((first, remaining)) = buf.split_first_mut() else {
        return Ok(0);
    };
    let serial = (serial as *mut raw::sunxi_serial_t).cast::<c_void>();

    loop {
        let status = line_status(serial);
        if let Some(error) = status.receive_error() {
            return Err(error);
        }
        if status.data_ready() {
            break;
        }
    }

    *first = unsafe { raw::sunxi_serial_getc(serial) as u8 };
    let mut read = 1;

    for byte in remaining {
        let status = line_status(serial);
        if status.receive_error().is_some() || !status.data_ready() {
            break;
        }
        *byte = unsafe { raw::sunxi_serial_getc(serial) as u8 };
        read += 1;
    }

    Ok(read)
}

/// Write all bytes to a UART using the blocking SyterKit serial ABI.
pub fn write_blocking(serial: &mut raw::sunxi_serial_t, buf: &[u8]) -> usize {
    let serial = (serial as *mut raw::sunxi_serial_t).cast::<c_void>();
    for &byte in buf {
        unsafe { raw::sunxi_serial_putc(serial, byte as c_char) };
    }
    buf.len()
}

fn flush_blocking(serial: &mut raw::sunxi_serial_t) {
    unsafe { raw::sunxi_serial_flush((serial as *mut raw::sunxi_serial_t).cast::<c_void>()) };
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
}

impl ErrorType for SerialPort<'_> {
    type Error = SerialError;
}

impl Read for SerialPort<'_> {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize, Self::Error> {
        read_blocking(self.raw, buf)
    }
}

impl Write for SerialPort<'_> {
    fn write(&mut self, buf: &[u8]) -> Result<usize, Self::Error> {
        Ok(write_blocking(self.raw, buf))
    }

    fn flush(&mut self) -> Result<(), Self::Error> {
        flush_blocking(self.raw);
        Ok(())
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

    fn raw_mut(&mut self) -> &mut raw::sunxi_serial_t {
        unsafe { &mut *self.raw }
    }
}

impl ErrorType for StdoutUart {
    type Error = SerialError;
}

impl Read for StdoutUart {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize, Self::Error> {
        read_blocking(self.raw_mut(), buf)
    }
}

impl Write for StdoutUart {
    fn write(&mut self, buf: &[u8]) -> Result<usize, Self::Error> {
        Ok(write_blocking(self.raw_mut(), buf))
    }

    fn flush(&mut self) -> Result<(), Self::Error> {
        flush_blocking(self.raw_mut());
        Ok(())
    }
}

impl core::fmt::Write for StdoutUart {
    fn write_str(&mut self, value: &str) -> core::fmt::Result {
        self.write_all(value.as_bytes())
            .map_err(|_| core::fmt::Error)
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
    use core::sync::atomic::{AtomicI32, AtomicU32, AtomicUsize, Ordering};
    use embedded_io::{Read, Write};
    use std::sync::Mutex;

    static TEST_LOCK: Mutex<()> = Mutex::new(());
    static SERIAL_INIT_RESULT: AtomicI32 = AtomicI32::new(0);
    static SERIAL_WRITE_COUNT: AtomicUsize = AtomicUsize::new(0);
    static SERIAL_FLUSH_COUNT: AtomicUsize = AtomicUsize::new(0);
    static SERIAL_STATUS: AtomicU32 = AtomicU32::new(1);
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
    pub extern "C" fn sunxi_serial_flush(_arg: *mut c_void) {
        SERIAL_FLUSH_COUNT.fetch_add(1, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn sunxi_serial_get_status(_arg: *mut c_void) -> u32 {
        SERIAL_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_serial_tstc(_arg: *mut c_void) -> c_int {
        (SERIAL_STATUS.load(Ordering::Relaxed) & 1) as c_int
    }

    #[no_mangle]
    pub extern "C" fn sunxi_serial_getc(_arg: *mut c_void) -> c_char {
        SERIAL_RX_VALUE.load(Ordering::Relaxed) as c_char
    }

    #[test]
    fn stdout_uart_implements_embedded_io() {
        let _guard = TEST_LOCK.lock().unwrap();
        SERIAL_INIT_RESULT.store(0, Ordering::Relaxed);
        SERIAL_WRITE_COUNT.store(0, Ordering::Relaxed);
        SERIAL_FLUSH_COUNT.store(0, Ordering::Relaxed);
        SERIAL_STATUS.store(1, Ordering::Relaxed);
        SERIAL_RX_VALUE.store(b'R' as u32, Ordering::Relaxed);

        let mut uart = StdoutUart::init().unwrap();
        uart.write_all(b"abc").unwrap();
        uart.flush().unwrap();
        assert_eq!(SERIAL_WRITE_COUNT.load(Ordering::Relaxed), 3);
        assert_eq!(SERIAL_FLUSH_COUNT.load(Ordering::Relaxed), 1);

        let mut byte = [0];
        assert_eq!(uart.read(&mut byte).unwrap(), 1);
        assert_eq!(byte, [b'R']);

        assert_eq!(uart.read(&mut []).unwrap(), 0);
    }

    #[test]
    fn stdout_uart_reports_receive_errors() {
        let _guard = TEST_LOCK.lock().unwrap();
        SERIAL_INIT_RESULT.store(0, Ordering::Relaxed);
        let mut uart = StdoutUart::init().unwrap();
        let mut byte = [0];

        for (status, expected) in [
            (1 << 1, super::SerialError::Overrun),
            (1 << 2, super::SerialError::Parity),
            (1 << 3, super::SerialError::Framing),
            (1 << 4, super::SerialError::Break),
        ] {
            SERIAL_STATUS.store(status, Ordering::Relaxed);
            assert_eq!(uart.read(&mut byte), Err(expected));
        }

        SERIAL_STATUS.store(1, Ordering::Relaxed);
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
        uart.write_all(b"ok").unwrap();
        uart.flush().unwrap();
        assert_eq!(SERIAL_WRITE_COUNT.load(Ordering::Relaxed), 2);

        SERIAL_RX_VALUE.store(b'S' as u32, Ordering::Relaxed);
        let mut byte = [0];
        assert_eq!(uart.read(&mut byte).unwrap(), 1);
        assert_eq!(byte, [b'S']);
    }
}
