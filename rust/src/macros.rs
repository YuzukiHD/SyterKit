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

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => {
        $crate::_print(core::format_args!($($arg)*));
    }
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\r\n"));
    ($($arg:tt)*) => {{
        $crate::_print(core::format_args!($($arg)*));
        $crate::println!();
    }}
}
