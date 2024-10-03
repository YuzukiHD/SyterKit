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
    pub inner: TransmitHalf<UART0, 0, Function<'static, 'B', 8, 6>>,
}

unsafe impl Send for SyterKitStdoutInner {}

#[doc(hidden)]
pub struct SyterKitStdinInner {
    pub inner: ReceiveHalf<UART0, 0, Function<'static, 'B', 9, 6>>,
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
    inner: spin::MutexGuard<'static, Option<SyterKitStdoutInner>>,
}

impl embedded_io::ErrorType for Stdout {
    type Error = core::convert::Infallible;
}

impl embedded_io::Write for Stdout {
    #[inline]
    fn write(&mut self, buf: &[u8]) -> Result<usize, Self::Error> {
        if let Some(stdout) = &mut *self.inner {
            stdout.inner.write_all(buf).ok();
        }
        Ok(buf.len())
    }
    #[inline]
    fn flush(&mut self) -> Result<(), Self::Error> {
        if let Some(stdout) = &mut *self.inner {
            stdout.inner.flush().ok();
        }
        Ok(())
    }
}

/// A handle to the standard input stream of a runtime.
pub struct Stdin {
    inner: spin::MutexGuard<'static, Option<SyterKitStdinInner>>,
}

impl embedded_io::ErrorType for Stdin {
    type Error = core::convert::Infallible;
}

impl embedded_io::Read for Stdin {
    #[inline]
    fn read(&mut self, buf: &mut [u8]) -> Result<usize, Self::Error> {
        if let Some(stdin) = &mut *self.inner {
            stdin.inner.read(buf).ok();
        }
        Ok(buf.len())
    }
}

/// Constructs a new handle to the standard output of the current environment.
#[inline]
pub fn stdout() -> Stdout {
    Stdout {
        inner: STDOUT.lock(),
    }
}

/// Constructs a new handle to the standard input of the current environment.
#[inline]
pub fn stdin() -> Stdin {
    Stdin {
        inner: STDIN.lock(),
    }
}
