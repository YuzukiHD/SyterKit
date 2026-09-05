// SPDX-License-Identifier: GPL-2.0+

#![no_std]

#[cfg(test)]
extern crate std;

use core::ffi::{c_char, c_int};
use core::fmt;

#[cfg(not(test))]
use core::panic::PanicInfo;

pub use syterkit_drivers::*;
use syterkit_ffi::raw;
pub use syterkit_lib::{BlockDevice, DriverResult, INVALID_ARGUMENT};

/// Log severities accepted by the root C `printk` implementation.
#[repr(i32)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum LogLevel {
    Mute = 0,
    Error = 1,
    Warning = 2,
    Info = 3,
    Debug = 4,
    Trace = 5,
    Backtrace = 6,
}

/// Maximum Rust-formatted message size, excluding the terminating NUL byte.
pub const PRINTK_BUFFER_SIZE: usize = 256;

struct PrintkBuffer {
    bytes: [u8; PRINTK_BUFFER_SIZE + 1],
    length: usize,
}

impl PrintkBuffer {
    const fn new() -> Self {
        Self {
            bytes: [0; PRINTK_BUFFER_SIZE + 1],
            length: 0,
        }
    }
}

impl fmt::Write for PrintkBuffer {
    fn write_str(&mut self, value: &str) -> fmt::Result {
        let available = PRINTK_BUFFER_SIZE - self.length;
        if value.len() > available {
            let end = self.length + available;
            self.bytes[self.length..end].copy_from_slice(&value.as_bytes()[..available]);
            self.length += available;
            return Err(fmt::Error);
        }

        let end = self.length + value.len();
        self.bytes[self.length..end].copy_from_slice(value.as_bytes());
        self.length = end;
        Ok(())
    }
}

/// Format a Rust message and send it through the root C `printk` path.
///
/// Formatting happens in a bounded stack buffer because the bare-metal
/// component layer has no allocator. The message is rejected when it cannot
/// fit; truncated kernel log records are more difficult to diagnose than a
/// clear status code.
pub fn printk(level: LogLevel, args: fmt::Arguments<'_>) -> DriverResult<()> {
    let mut message = PrintkBuffer::new();
    fmt::write(&mut message, args).map_err(|_| INVALID_ARGUMENT)?;

    message.bytes[message.length] = 0;
    unsafe {
        raw::printk_string(level as c_int, message.bytes.as_ptr().cast::<c_char>());
    }
    Ok(())
}

/// Format and emit an informational message using the normal Rust print API.
pub fn print(args: fmt::Arguments<'_>) {
    let _ = printk(LogLevel::Info, args);
}

/// Print an error-level message through the root C `printk` path.
pub fn eprint(args: fmt::Arguments<'_>) {
    let _ = printk(LogLevel::Error, args);
}

/// Format and emit a message through the root C `printk` path.
#[macro_export]
macro_rules! printk {
    ($level:expr, $($arg:tt)*) => {
        $crate::printk($level, ::core::format_args!($($arg)*))
    };
}

/// Print an informational message through the root C `printk` path.
#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => {
        $crate::print(::core::format_args!($($arg)*))
    };
}

/// Print an informational message followed by a newline.
#[macro_export]
macro_rules! println {
    () => {
        $crate::print(::core::format_args!("\n"))
    };
    ($fmt:literal) => {
        $crate::print(::core::format_args!(concat!($fmt, "\n")))
    };
    ($fmt:literal, $($args:tt)*) => {
        $crate::print(::core::format_args!(concat!($fmt, "\n"), $($args)*))
    };
}

/// Print an error-level message followed by a newline.
#[macro_export]
macro_rules! eprintln {
    () => {
        $crate::eprint(::core::format_args!("\n"))
    };
    ($fmt:literal) => {
        $crate::eprint(::core::format_args!(concat!($fmt, "\n")))
    };
    ($fmt:literal, $($args:tt)*) => {
        $crate::eprint(::core::format_args!(concat!($fmt, "\n"), $($args)*))
    };
}

/// Thin Rust facade for the existing C `core/` interfaces.
///
/// The implementation remains in the repository's root `core/` directory.
/// This type only gives Rust applications typed access to that established
/// ABI; device-specific behavior remains in C and in the driver wrappers.
pub struct Core;

impl Core {
    pub const fn new() -> Self {
        Self
    }

    /// Print the C core startup banner.
    pub fn show_banner(&self) {
        unsafe { raw::show_banner() };
    }

    /// Print the startup banner with an application-selected toolchain string.
    pub fn show_banner_with_build_info(&self, build_info: &[u8]) -> DriverResult<()> {
        let build_info = syterkit_lib::c_name(build_info)?;
        unsafe { raw::show_banner_with_build_info(build_info) };
        Ok(())
    }

    /// Run the board cleanup hook before leaving SyterKit.
    pub fn clean_data(&self) {
        unsafe { raw::clean_syterkit_data() };
    }

    /// Format and emit a message through the root C `printk` path.
    pub fn printk(&self, level: LogLevel, args: fmt::Arguments<'_>) -> DriverResult<()> {
        crate::printk(level, args)
    }

    /// Format and emit an informational message through the root C `printk` path.
    pub fn print(&self, args: fmt::Arguments<'_>) {
        crate::print(args)
    }

    /// Format and emit an error-level message through the root C `printk` path.
    pub fn eprint(&self, args: fmt::Arguments<'_>) {
        crate::eprint(args)
    }

    /// Initialize the C heap with a caller-selected memory range.
    pub fn malloc_init(&self, heap_start: usize, heap_size: usize) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::malloc_init(heap_start, heap_size) })
    }

    /// Dump a readable memory range through the root C logger.
    pub unsafe fn dump_hex(&self, start_address: usize, count: u32) {
        unsafe { raw::dump_hex(start_address, count) };
    }

    /// Attach the default shell command table.
    pub fn attach_shell(&self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::syterkit_shell_attach(core::ptr::null()) })
    }

    /// Enter the default shell and stop the application when it exits.
    pub fn run_shell(&self) -> ! {
        let _ = self.attach_shell();
        unsafe { raw::abort() }
    }

    /// Write one byte through the root C core UART interface.
    pub fn putc(&self, value: u8) -> DriverResult<()> {
        let result = unsafe { raw::uart_putchar(value as c_int) };
        if result == 0 {
            Ok(())
        } else {
            Err(result)
        }
    }

    /// Write a byte slice through the root C core UART interface.
    pub fn write_bytes(&self, bytes: &[u8]) -> DriverResult<()> {
        for &byte in bytes {
            self.putc(byte)?;
        }
        Ok(())
    }

    /// Read one character using the root C core UART interface.
    pub fn getc(&self) -> i32 {
        unsafe { raw::uart_getchar() }
    }

    /// Read one character without blocking when the UART has no input.
    pub fn try_getc(&self) -> Option<i32> {
        if unsafe { raw::tstc() } != 0 {
            Some(self.getc())
        } else {
            None
        }
    }

    /// Mark the root C UART console ready and flush buffered early output.
    pub fn console_ready(&self) {
        unsafe { raw::uart_log_console_ready() };
    }
}

pub const CORE: Core = Core::new();

#[cfg(syterkit_rust_app)]
mod app {
    include!(env!("SYTERKIT_RUST_APP_SOURCE"));
}

#[cfg(syterkit_rust_app)]
#[no_mangle]
pub extern "C" fn main() -> i32 {
    app::main()
}

#[cfg(test)]
mod tests {
    use super::{LogLevel, CORE, INVALID_ARGUMENT, PRINTK_BUFFER_SIZE};
    use core::ffi::{c_char, c_int};
    use core::fmt;
    use core::sync::atomic::{AtomicBool, AtomicI32, AtomicU32, AtomicUsize, Ordering};
    use std::sync::Mutex;

    static TEST_LOCK: Mutex<()> = Mutex::new(());
    static CORE_CALLS: AtomicU32 = AtomicU32::new(0);
    static UART_WRITES: AtomicU32 = AtomicU32::new(0);
    static UART_READY: AtomicBool = AtomicBool::new(true);
    static UART_VALUE: AtomicI32 = AtomicI32::new(b'R' as i32);
    static PRINTK_CALLS: AtomicU32 = AtomicU32::new(0);
    static PRINTK_LEVEL: AtomicI32 = AtomicI32::new(0);
    static HEAP_START: AtomicUsize = AtomicUsize::new(0);
    static HEAP_SIZE: AtomicUsize = AtomicUsize::new(0);
    static DUMP_ADDRESS: AtomicUsize = AtomicUsize::new(0);
    static DUMP_COUNT: AtomicU32 = AtomicU32::new(0);
    static SHELL_CALLS: AtomicU32 = AtomicU32::new(0);
    static PRINTK_MESSAGE: Mutex<[u8; PRINTK_BUFFER_SIZE + 1]> =
        Mutex::new([0; PRINTK_BUFFER_SIZE + 1]);

    #[no_mangle]
    pub extern "C" fn show_banner() {
        CORE_CALLS.fetch_add(1, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn show_banner_with_build_info(_build_info: *const c_char) {
        CORE_CALLS.fetch_add(1, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn clean_syterkit_data() {
        CORE_CALLS.fetch_add(1, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn printk_string(level: c_int, message: *const c_char) {
        PRINTK_CALLS.fetch_add(1, Ordering::Relaxed);
        PRINTK_LEVEL.store(level, Ordering::Relaxed);

        let mut output = PRINTK_MESSAGE.lock().unwrap();
        output.fill(0);
        if message.is_null() {
            return;
        }

        let mut index = 0;
        while index < output.len() - 1 {
            let value = unsafe { *message.cast::<u8>().add(index) };
            if value == 0 {
                break;
            }
            output[index] = value;
            index += 1;
        }
    }

    #[no_mangle]
    pub extern "C" fn uart_putchar(_value: i32) -> i32 {
        UART_WRITES.fetch_add(1, Ordering::Relaxed);
        0
    }

    #[no_mangle]
    pub extern "C" fn uart_getchar() -> i32 {
        UART_VALUE.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn tstc() -> i32 {
        UART_READY.load(Ordering::Relaxed) as i32
    }

    #[no_mangle]
    pub extern "C" fn uart_log_console_ready() {
        UART_READY.store(true, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn malloc_init(heap_start: usize, heap_size: usize) -> c_int {
        HEAP_START.store(heap_start, Ordering::Relaxed);
        HEAP_SIZE.store(heap_size, Ordering::Relaxed);
        0
    }

    #[no_mangle]
    pub extern "C" fn dump_hex(start_address: usize, count: u32) {
        DUMP_ADDRESS.store(start_address, Ordering::Relaxed);
        DUMP_COUNT.store(count, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn syterkit_shell_attach(
        _commands: *const syterkit_ffi::raw::msh_command_entry,
    ) -> c_int {
        SHELL_CALLS.fetch_add(1, Ordering::Relaxed);
        0
    }

    #[no_mangle]
    pub extern "C" fn abort() -> ! {
        loop {
            core::hint::spin_loop();
        }
    }

    #[test]
    fn core_forwards_root_core_lifecycle_hooks() {
        let _guard = TEST_LOCK.lock().unwrap();
        CORE_CALLS.store(0, Ordering::Relaxed);
        CORE.show_banner();
        CORE.clean_data();
        assert_eq!(CORE_CALLS.load(Ordering::Relaxed), 2);
    }

    #[test]
    fn core_can_override_the_banner_build_identity() {
        let _guard = TEST_LOCK.lock().unwrap();
        CORE_CALLS.store(0, Ordering::Relaxed);
        assert_eq!(CORE.show_banner_with_build_info(b"rustc test\0"), Ok(()));
        assert_eq!(
            CORE.show_banner_with_build_info(b"rustc test"),
            Err(INVALID_ARGUMENT)
        );
        assert_eq!(CORE_CALLS.load(Ordering::Relaxed), 1);
    }

    #[test]
    fn core_wraps_root_core_uart_interface() {
        let _guard = TEST_LOCK.lock().unwrap();
        UART_WRITES.store(0, Ordering::Relaxed);
        UART_READY.store(true, Ordering::Relaxed);
        UART_VALUE.store(b'R' as i32, Ordering::Relaxed);

        assert_eq!(CORE.write_bytes(b"ok"), Ok(()));
        assert_eq!(UART_WRITES.load(Ordering::Relaxed), 2);
        assert_eq!(CORE.getc(), b'R' as i32);
        assert_eq!(CORE.try_getc(), Some(b'R' as i32));

        UART_READY.store(false, Ordering::Relaxed);
        assert_eq!(CORE.try_getc(), None);
        CORE.console_ready();
        assert_eq!(CORE.try_getc(), Some(b'R' as i32));
    }

    #[test]
    fn core_formats_messages_for_generic_printk() {
        let _guard = TEST_LOCK.lock().unwrap();
        PRINTK_CALLS.store(0, Ordering::Relaxed);
        PRINTK_LEVEL.store(0, Ordering::Relaxed);

        crate::println!("voltage={}mV", 3300);

        assert_eq!(PRINTK_CALLS.load(Ordering::Relaxed), 1);
        assert_eq!(PRINTK_LEVEL.load(Ordering::Relaxed), LogLevel::Info as i32);
        let output = PRINTK_MESSAGE.lock().unwrap();
        assert_eq!(&output[..15], b"voltage=3300mV\n");
    }

    struct LongMessage;

    impl fmt::Display for LongMessage {
        fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
            for _ in 0..=PRINTK_BUFFER_SIZE {
                formatter.write_str("x")?;
            }
            Ok(())
        }
    }

    #[test]
    fn core_rejects_printk_messages_that_do_not_fit() {
        let _guard = TEST_LOCK.lock().unwrap();
        PRINTK_CALLS.store(0, Ordering::Relaxed);

        assert_eq!(
            CORE.printk(LogLevel::Debug, format_args!("{}", LongMessage)),
            Err(INVALID_ARGUMENT)
        );
        assert_eq!(PRINTK_CALLS.load(Ordering::Relaxed), 0);
    }

    #[test]
    fn core_wraps_heap_memory_dump_and_shell_lifecycle() {
        let _guard = TEST_LOCK.lock().unwrap();
        HEAP_START.store(0, Ordering::Relaxed);
        HEAP_SIZE.store(0, Ordering::Relaxed);
        DUMP_ADDRESS.store(0, Ordering::Relaxed);
        DUMP_COUNT.store(0, Ordering::Relaxed);
        SHELL_CALLS.store(0, Ordering::Relaxed);

        CORE.malloc_init(0x4010_0000, 0x00f0_0000).unwrap();
        unsafe { CORE.dump_hex(0x4000_0000, 0x40) };
        CORE.attach_shell().unwrap();

        assert_eq!(HEAP_START.load(Ordering::Relaxed), 0x4010_0000);
        assert_eq!(HEAP_SIZE.load(Ordering::Relaxed), 0x00f0_0000);
        assert_eq!(DUMP_ADDRESS.load(Ordering::Relaxed), 0x4000_0000);
        assert_eq!(DUMP_COUNT.load(Ordering::Relaxed), 0x40);
        assert_eq!(SHELL_CALLS.load(Ordering::Relaxed), 1);
    }
}

#[cfg(not(test))]
#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    loop {
        core::hint::spin_loop();
    }
}
