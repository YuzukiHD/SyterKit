use allwinner_hal::{
    gpio::Function,
    uart::{ReceiveHalf, TransmitHalf},
};
use allwinner_rt::soc::d1::UART0;
use embedded_io::Write;
use spin::Mutex;

#[doc(hidden)]
pub static STDOUT: Mutex<Option<SyterKitStdoutInner>> = Mutex::new(None);
#[doc(hidden)]
pub static STDIN: Mutex<Option<SyterKitStdinInner>> = Mutex::new(None);

#[doc(hidden)]
pub struct SyterKitStdoutInner {
    pub inner: TransmitHalf<'static, Function<'static, 'B', 8, 6>>,
}

unsafe impl Send for SyterKitStdoutInner {}

#[doc(hidden)]
pub struct SyterKitStdinInner {
    pub inner: ReceiveHalf<'static, Function<'static, 'B', 9, 6>>,
}

unsafe impl Send for SyterKitStdinInner {}

// macro internal, used in `print` and `println` macros in `macros.rs`.
#[doc(hidden)]
#[inline]
pub fn _print(args: core::fmt::Arguments) {
    if let Some(serial) = &mut *STDOUT.lock() {
        serial.inner.write_fmt(args).ok();
    }
}

/// A handle to the global standard output stream of the current runtime.
pub struct Stdout {
    inner: &'static spin::Mutex<Option<SyterKitStdoutInner>>,
}

impl embedded_io::ErrorType for Stdout {
    type Error = core::convert::Infallible;
}

impl embedded_io::Write for Stdout {
    #[inline]
    fn write(&mut self, buf: &[u8]) -> Result<usize, Self::Error> {
        let mut lock = self.inner.lock();
        if let Some(stdout) = &mut *lock {
            stdout.inner.write_all(buf).ok();
        }
        Ok(buf.len())
    }
    #[inline]
    fn flush(&mut self) -> Result<(), Self::Error> {
        let mut lock = self.inner.lock();
        if let Some(stdout) = &mut *lock {
            stdout.inner.flush().ok();
        }
        Ok(())
    }
}

#[doc(hidden)]
#[inline]
pub fn set_logger_stdout() {
    struct StdoutLogger;
    impl log::Log for StdoutLogger {
        #[inline]
        fn enabled(&self, _metadata: &log::Metadata) -> bool {
            STDOUT.lock().is_some()
        }
        #[inline]
        fn log(&self, record: &log::Record) {
            if self.enabled(record.metadata()) {
                let mut lock = STDOUT.lock();
                if let Some(stdout) = &mut *lock {
                    write!(stdout.inner, "{} - {}\r\n", record.level(), record.args()).ok();
                }
                drop(lock);
            }
        }
        #[inline]
        fn flush(&self) {
            let mut lock = STDOUT.lock();
            if let Some(stdout) = &mut *lock {
                stdout.inner.flush().ok();
            }
        }
    }

    static STDOUT_LOGGER: StdoutLogger = StdoutLogger;
    log::set_logger(&STDOUT_LOGGER).ok();
    // TODO: make it configurable in environment variable
    log::set_max_level(log::LevelFilter::max());
}

/// A handle to the standard input stream of a runtime.
pub struct Stdin {
    inner: &'static spin::Mutex<Option<SyterKitStdinInner>>,
}

impl embedded_io::ErrorType for Stdin {
    type Error = core::convert::Infallible;
}

impl embedded_io::Read for Stdin {
    #[inline]
    fn read(&mut self, buf: &mut [u8]) -> Result<usize, Self::Error> {
        let mut lock = self.inner.lock();
        if let Some(stdin) = &mut *lock {
            stdin.inner.read(buf).ok();
        }
        Ok(buf.len())
    }
}

/// Constructs a new handle to the standard output of the current environment.
#[inline]
pub fn stdout() -> Stdout {
    Stdout { inner: &STDOUT }
}

/// Constructs a new handle to the standard input of the current environment.
#[inline]
pub fn stdin() -> Stdin {
    Stdin { inner: &STDIN }
}
