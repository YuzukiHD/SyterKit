#[allow(unused)]
macro_rules! impl_gpio_pins {
    ($($px: ident:($P: expr, $N: expr, $M: ident);)+) => {
/// GPIO pads available from SyterKit.
pub struct Pads<'a> {
    $(
    pub $px: ::allwinner_hal::gpio::$M<'a, $P, $N>,
    )+
}
    };
}

/// Prints to the standard output.
#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => {
        $crate::_print(core::format_args!($($arg)*));
    }
}

/// Prints to the standard output, with a newline.
#[macro_export]
macro_rules! println {
    () => ($crate::print!("\r\n"));
    ($($arg:tt)*) => {{
        $crate::_print(core::format_args!($($arg)*));
        $crate::println!();
    }}
}
