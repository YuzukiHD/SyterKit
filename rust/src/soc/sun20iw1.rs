//! D1-H, D1s, F133, F133-A, F133-B series.
use allwinner_hal::{
    ccu::{Clocks, CpuClockSource},
    gpio::Function,
    prelude::*,
};
use allwinner_rt::soc::d1::{CCU, COM, GPIO, PHY, PLIC, SMHC0, SMHC1, SMHC2, SPI0, UART0};
use embedded_time::rate::Extensions;

/// SyterKit runtime peripheral ownership and configurations.
pub struct Peripherals {
    /// General Purpose Input/Output peripheral.
    pub gpio: Pads,
    /// Clock control unit peripheral.
    pub ccu: CCU,
    // uart0 is removed; it is occupied by stdin/stdout `Serial` structure.
    /// Serial Peripheral Interface peripheral 0.
    /// Common control peripheral of DDR SDRAM.
    pub com: COM,
    /// Memory controller physical layer (PHY) of DDR SDRAM.
    pub phy: PHY,
    /// SD/MMC Host Controller peripheral 0.
    pub smhc0: SMHC0,
    /// SD/MMC Host Controller peripheral 1.
    pub smhc1: SMHC1,
    /// SD/MMC Host Controller peripheral 2.
    pub smhc2: SMHC2,
    /// Serial Peripheral Interface peripheral 0.
    pub spi0: SPI0,
    /// Platform-local Interrupt Controller.
    pub plic: PLIC,
}

impl Peripherals {
    /// Split SyterKit peripherals from `allwinner-rt` peripherals.
    #[inline]
    pub fn configure_uart0(
        src: allwinner_rt::soc::d1::Peripherals,
    ) -> (
        Self,
        UART0,
        Function<'static, 'B', 8, 6>,
        Function<'static, 'B', 9, 6>,
    ) {
        let pb8 = src.gpio.pb8.into_function::<6>();
        let pb9 = src.gpio.pb9.into_function::<6>();
        let uart0 = src.uart0;
        let p = Self {
            gpio: Pads::__new(),
            ccu: src.ccu,
            com: src.com,
            phy: src.phy,
            smhc0: src.smhc0,
            smhc1: src.smhc1,
            smhc2: src.smhc2,
            spi0: src.spi0,
            plic: src.plic,
        };
        (p, uart0, pb8, pb9)
    }
}

/// Initialize clock configurations.
///
/// Macro Internal - DO NOT USE on ordinary code.
#[doc(hidden)]
pub fn __clock_init(ccu: &CCU) -> Clocks {
    // TODO rewrite according to function `set_pll_cpux_axi` in src/drivers/sun20iw1/sys-clk.c
    unsafe {
        ccu.cpu_axi_config
            .modify(|val| val.set_clock_source(CpuClockSource::PllPeri1x))
    };
    unsafe {
        ccu.pll_cpu_control.modify(|val| val.set_pll_n(42 - 1));
        ccu.pll_cpu_control.modify(|val| val.disable_lock());
        ccu.pll_cpu_control.modify(|val| val.enable_lock())
    };
    while !ccu.pll_cpu_control.read().is_locked() {
        core::hint::spin_loop();
    }
    unsafe {
        ccu.cpu_axi_config
            .modify(|val| val.set_clock_source(CpuClockSource::PllCpu));
    };
    // TODO move default frequencies into allwinner_hal::soc::d1::Clocks, reuse default
    // frequencies and follow SyterKit defined clock frequencies for non-rom-default ones.
    Clocks {
        psi: 600_000_000.Hz(),
        apb1: 24_000_000.Hz(),
    }
}

/// Dump information about the system clocks.
pub fn clock_dump(ccu: &CCU) {
    let cpu_clock_source = ccu.cpu_axi_config.read().clock_source();
    let clock_name = match cpu_clock_source {
        CpuClockSource::Hosc => "OSC24M",
        CpuClockSource::Clk32K => "CLK32",
        CpuClockSource::Clk16MRC => "CLK16M_RC",
        CpuClockSource::PllCpu => "PLL_CPU",
        CpuClockSource::PllPeri1x => "PLL_PERI(1X)",
        CpuClockSource::PllPeri2x => "PLL_PERI(2X)",
        CpuClockSource::PllPeri800M => "PLL_PERI(800M)",
    };

    let val = ccu.pll_cpu_control.read();
    let n = (val.pll_n() + 1) as u32;
    let m = (val.pll_m() + 1) as u32;
    let cpu_freq = 24 * n / m;
    println!("CLK: CPU PLL={} FREQ={}MHz", clock_name, cpu_freq);

    let val = ccu.pll_peri0_control.read();
    if val.is_pll_enabled() {
        let n = (val.pll_n() + 1) as u32;
        let m = (val.pll_m() + 1) as u32;
        let p0 = (val.pll_p0() + 1) as u32;
        let p1 = (val.pll_p1() + 1) as u32;
        let peri2x_freq = 24 * n / (m * p0);
        let peri1x_freq = 24 * n / (m * p0 * 2);
        let peri800m_freq = 24 * n / (m * p1);
        println!(
            "CLK: PLL_peri (2X)={}MHz, (1X)={}MHz, (800M)={}MHz",
            peri2x_freq, peri1x_freq, peri800m_freq
        );
    } else {
        println!("CLK: PLL_peri is disabled");
    }

    let val = ccu.pll_ddr_control.read();
    if val.is_pll_enabled() {
        let n = (val.pll_n() + 1) as u32;
        let m0 = (val.pll_m0() + 1) as u32;
        let m1 = (val.pll_m1() + 1) as u32;
        let ddr_freq = 24 * n / (m0 * m1);
        println!("CLK: PLL_ddr={}MHz", ddr_freq);
    } else {
        println!("CLK: PLL_ddr is disabled");
    }
}

#[allow(unused)]
macro_rules! impl_gpio_pins {
    ($($px: ident:($P: expr, $N: expr);)+) => {
/// GPIO pads available from SyterKit.
pub struct Pads {
    $(
    pub $px: ::allwinner_rt::soc::d1::Pad<$P, $N>,
    )+
}

impl Pads {
    #[doc(hidden)]
    #[inline]
    pub fn __new() -> Self {
        Self {
            $(
            $px: ::allwinner_rt::soc::d1::Pad::__new(),
            )+
        }
    }
}
    };
}

// pb8, pb9 is removed for they are configured as Function<6> for UART0.
impl_gpio_pins! {
    pb0: ('B', 0);
    pb1: ('B', 1);
    pb2: ('B', 2);
    pb3: ('B', 3);
    pb4: ('B', 4);
    pb5: ('B', 5);
    pb6: ('B', 6);
    pb7: ('B', 7);
    pb10: ('B', 10);
    pb11: ('B', 11);
    pb12: ('B', 12);
    pc0: ('C', 0);
    pc1: ('C', 1);
    pc2: ('C', 2);
    pc3: ('C', 3);
    pc4: ('C', 4);
    pc5: ('C', 5);
    pc6: ('C', 6);
    pc7: ('C', 7);
    pd0: ('D', 0);
    pd1: ('D', 1);
    pd2: ('D', 2);
    pd3: ('D', 3);
    pd4: ('D', 4);
    pd5: ('D', 5);
    pd6: ('D', 6);
    pd7: ('D', 7);
    pd8: ('D', 8);
    pd9: ('D', 9);
    pd10: ('D', 10);
    pd11: ('D', 11);
    pd12: ('D', 12);
    pd13: ('D', 13);
    pd14: ('D', 14);
    pd15: ('D', 15);
    pd16: ('D', 16);
    pd17: ('D', 17);
    pd18: ('D', 18);
    pd19: ('D', 19);
    pd20: ('D', 20);
    pd21: ('D', 21);
    pd22: ('D', 22);
    pe0: ('E', 0);
    pe1: ('E', 1);
    pe2: ('E', 2);
    pe3: ('E', 3);
    pe4: ('E', 4);
    pe5: ('E', 5);
    pe6: ('E', 6);
    pe7: ('E', 7);
    pe8: ('E', 8);
    pe9: ('E', 9);
    pe10: ('E', 10);
    pe11: ('E', 11);
    pe12: ('E', 12);
    pe13: ('E', 13);
    pe14: ('E', 14);
    pe15: ('E', 15);
    pe16: ('E', 16);
    pe17: ('E', 17);
    pf0: ('F', 0);
    pf1: ('F', 1);
    pf2: ('F', 2);
    pf3: ('F', 3);
    pf4: ('F', 4);
    pf5: ('F', 5);
    pf6: ('F', 6);
    pg0: ('G', 0);
    pg1: ('G', 1);
    pg2: ('G', 2);
    pg3: ('G', 3);
    pg4: ('G', 4);
    pg5: ('G', 5);
    pg6: ('G', 6);
    pg7: ('G', 7);
    pg8: ('G', 8);
    pg9: ('G', 9);
    pg10: ('G', 10);
    pg11: ('G', 11);
    pg12: ('G', 12);
    pg13: ('G', 13);
    pg14: ('G', 14);
    pg15: ('G', 15);
    pg16: ('G', 16);
    pg17: ('G', 17);
    pg18: ('G', 18);
}
