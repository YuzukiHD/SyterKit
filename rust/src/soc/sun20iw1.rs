//! D1-H, D1s, F133, F133-A, F133-B series.
use allwinner_hal::{
    ccu::{Clocks, CpuClockSource},
    gpio::{Disabled, Function},
};
use allwinner_rt::soc::d1::{CCU, COM, GPIO, PHY, PLIC, SMHC0, SMHC1, SMHC2, SPI0, UART0};
use embedded_time::rate::Extensions;

/// SyterKit runtime peripheral ownership and configurations.
pub struct Peripherals<'a> {
    /// General Purpose Input/Output peripheral.
    pub gpio: Pads<'a>,
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

impl<'a> Peripherals<'a> {
    /// Split SyterKit peripherals from `allwinner-rt` peripherals.
    #[inline]
    pub fn configure_uart0(
        src: allwinner_rt::soc::d1::Peripherals<'a>,
    ) -> (
        Self,
        UART0,
        Function<'a, 'B', 8, 6>,
        Function<'a, 'B', 9, 6>,
    ) {
        let pb8 = src.gpio.pb8.into_function::<6>();
        let pb9 = src.gpio.pb9.into_function::<6>();
        let uart0 = src.uart0;
        let p = Self {
            gpio: Pads::__init(),
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
        CpuClockSource::Osc24M => "OSC24M",
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

// pb8, pb9 is removed for they are configured as Function<6> for UART0.
impl_gpio_pins! {
    pb0: ('B', 0, Disabled);
    pb1: ('B', 1, Disabled);
    pb2: ('B', 2, Disabled);
    pb3: ('B', 3, Disabled);
    pb4: ('B', 4, Disabled);
    pb5: ('B', 5, Disabled);
    pb6: ('B', 6, Disabled);
    pb7: ('B', 7, Disabled);
    pb10: ('B', 10, Disabled);
    pb11: ('B', 11, Disabled);
    pb12: ('B', 12, Disabled);
    pc0: ('C', 0, Disabled);
    pc1: ('C', 1, Disabled);
    pc2: ('C', 2, Disabled);
    pc3: ('C', 3, Disabled);
    pc4: ('C', 4, Disabled);
    pc5: ('C', 5, Disabled);
    pc6: ('C', 6, Disabled);
    pc7: ('C', 7, Disabled);
    pd0: ('D', 0, Disabled);
    pd1: ('D', 1, Disabled);
    pd2: ('D', 2, Disabled);
    pd3: ('D', 3, Disabled);
    pd4: ('D', 4, Disabled);
    pd5: ('D', 5, Disabled);
    pd6: ('D', 6, Disabled);
    pd7: ('D', 7, Disabled);
    pd8: ('D', 8, Disabled);
    pd9: ('D', 9, Disabled);
    pd10: ('D', 10, Disabled);
    pd11: ('D', 11, Disabled);
    pd12: ('D', 12, Disabled);
    pd13: ('D', 13, Disabled);
    pd14: ('D', 14, Disabled);
    pd15: ('D', 15, Disabled);
    pd16: ('D', 16, Disabled);
    pd17: ('D', 17, Disabled);
    pd18: ('D', 18, Disabled);
    pd19: ('D', 19, Disabled);
    pd20: ('D', 20, Disabled);
    pd21: ('D', 21, Disabled);
    pd22: ('D', 22, Disabled);
    pe0: ('E', 0, Disabled);
    pe1: ('E', 1, Disabled);
    pe2: ('E', 2, Disabled);
    pe3: ('E', 3, Disabled);
    pe4: ('E', 4, Disabled);
    pe5: ('E', 5, Disabled);
    pe6: ('E', 6, Disabled);
    pe7: ('E', 7, Disabled);
    pe8: ('E', 8, Disabled);
    pe9: ('E', 9, Disabled);
    pe10: ('E', 10, Disabled);
    pe11: ('E', 11, Disabled);
    pe12: ('E', 12, Disabled);
    pe13: ('E', 13, Disabled);
    pe14: ('E', 14, Disabled);
    pe15: ('E', 15, Disabled);
    pe16: ('E', 16, Disabled);
    pe17: ('E', 17, Disabled);
    pf0: ('F', 0, Disabled);
    pf1: ('F', 1, Disabled);
    pf2: ('F', 2, Disabled);
    pf3: ('F', 3, Disabled);
    pf4: ('F', 4, Disabled);
    pf5: ('F', 5, Disabled);
    pf6: ('F', 6, Disabled);
    pg0: ('G', 0, Disabled);
    pg1: ('G', 1, Disabled);
    pg2: ('G', 2, Disabled);
    pg3: ('G', 3, Disabled);
    pg4: ('G', 4, Disabled);
    pg5: ('G', 5, Disabled);
    pg6: ('G', 6, Disabled);
    pg7: ('G', 7, Disabled);
    pg8: ('G', 8, Disabled);
    pg9: ('G', 9, Disabled);
    pg10: ('G', 10, Disabled);
    pg11: ('G', 11, Disabled);
    pg12: ('G', 12, Disabled);
    pg13: ('G', 13, Disabled);
    pg14: ('G', 14, Disabled);
    pg15: ('G', 15, Disabled);
    pg16: ('G', 16, Disabled);
    pg17: ('G', 17, Disabled);
    pg18: ('G', 18, Disabled);
}

impl<'a> Pads<'a> {
    #[inline]
    #[doc(hidden)]
    pub fn __init() -> Self {
        static _GPIO: GPIO = unsafe { core::mem::transmute(()) };
        Pads {
            pb0: unsafe { Disabled::__new(&_GPIO) },
            pb1: unsafe { Disabled::__new(&_GPIO) },
            pb2: unsafe { Disabled::__new(&_GPIO) },
            pb3: unsafe { Disabled::__new(&_GPIO) },
            pb4: unsafe { Disabled::__new(&_GPIO) },
            pb5: unsafe { Disabled::__new(&_GPIO) },
            pb6: unsafe { Disabled::__new(&_GPIO) },
            pb7: unsafe { Disabled::__new(&_GPIO) },
            pb10: unsafe { Disabled::__new(&_GPIO) },
            pb11: unsafe { Disabled::__new(&_GPIO) },
            pb12: unsafe { Disabled::__new(&_GPIO) },
            pc0: unsafe { Disabled::__new(&_GPIO) },
            pc1: unsafe { Disabled::__new(&_GPIO) },
            pc2: unsafe { Disabled::__new(&_GPIO) },
            pc3: unsafe { Disabled::__new(&_GPIO) },
            pc4: unsafe { Disabled::__new(&_GPIO) },
            pc5: unsafe { Disabled::__new(&_GPIO) },
            pc6: unsafe { Disabled::__new(&_GPIO) },
            pc7: unsafe { Disabled::__new(&_GPIO) },
            pd0: unsafe { Disabled::__new(&_GPIO) },
            pd1: unsafe { Disabled::__new(&_GPIO) },
            pd2: unsafe { Disabled::__new(&_GPIO) },
            pd3: unsafe { Disabled::__new(&_GPIO) },
            pd4: unsafe { Disabled::__new(&_GPIO) },
            pd5: unsafe { Disabled::__new(&_GPIO) },
            pd6: unsafe { Disabled::__new(&_GPIO) },
            pd7: unsafe { Disabled::__new(&_GPIO) },
            pd8: unsafe { Disabled::__new(&_GPIO) },
            pd9: unsafe { Disabled::__new(&_GPIO) },
            pd10: unsafe { Disabled::__new(&_GPIO) },
            pd11: unsafe { Disabled::__new(&_GPIO) },
            pd12: unsafe { Disabled::__new(&_GPIO) },
            pd13: unsafe { Disabled::__new(&_GPIO) },
            pd14: unsafe { Disabled::__new(&_GPIO) },
            pd15: unsafe { Disabled::__new(&_GPIO) },
            pd16: unsafe { Disabled::__new(&_GPIO) },
            pd17: unsafe { Disabled::__new(&_GPIO) },
            pd18: unsafe { Disabled::__new(&_GPIO) },
            pd19: unsafe { Disabled::__new(&_GPIO) },
            pd20: unsafe { Disabled::__new(&_GPIO) },
            pd21: unsafe { Disabled::__new(&_GPIO) },
            pd22: unsafe { Disabled::__new(&_GPIO) },
            pe0: unsafe { Disabled::__new(&_GPIO) },
            pe1: unsafe { Disabled::__new(&_GPIO) },
            pe2: unsafe { Disabled::__new(&_GPIO) },
            pe3: unsafe { Disabled::__new(&_GPIO) },
            pe4: unsafe { Disabled::__new(&_GPIO) },
            pe5: unsafe { Disabled::__new(&_GPIO) },
            pe6: unsafe { Disabled::__new(&_GPIO) },
            pe7: unsafe { Disabled::__new(&_GPIO) },
            pe8: unsafe { Disabled::__new(&_GPIO) },
            pe9: unsafe { Disabled::__new(&_GPIO) },
            pe10: unsafe { Disabled::__new(&_GPIO) },
            pe11: unsafe { Disabled::__new(&_GPIO) },
            pe12: unsafe { Disabled::__new(&_GPIO) },
            pe13: unsafe { Disabled::__new(&_GPIO) },
            pe14: unsafe { Disabled::__new(&_GPIO) },
            pe15: unsafe { Disabled::__new(&_GPIO) },
            pe16: unsafe { Disabled::__new(&_GPIO) },
            pe17: unsafe { Disabled::__new(&_GPIO) },
            pf0: unsafe { Disabled::__new(&_GPIO) },
            pf1: unsafe { Disabled::__new(&_GPIO) },
            pf2: unsafe { Disabled::__new(&_GPIO) },
            pf3: unsafe { Disabled::__new(&_GPIO) },
            pf4: unsafe { Disabled::__new(&_GPIO) },
            pf5: unsafe { Disabled::__new(&_GPIO) },
            pf6: unsafe { Disabled::__new(&_GPIO) },
            pg0: unsafe { Disabled::__new(&_GPIO) },
            pg1: unsafe { Disabled::__new(&_GPIO) },
            pg2: unsafe { Disabled::__new(&_GPIO) },
            pg3: unsafe { Disabled::__new(&_GPIO) },
            pg4: unsafe { Disabled::__new(&_GPIO) },
            pg5: unsafe { Disabled::__new(&_GPIO) },
            pg6: unsafe { Disabled::__new(&_GPIO) },
            pg7: unsafe { Disabled::__new(&_GPIO) },
            pg8: unsafe { Disabled::__new(&_GPIO) },
            pg9: unsafe { Disabled::__new(&_GPIO) },
            pg10: unsafe { Disabled::__new(&_GPIO) },
            pg11: unsafe { Disabled::__new(&_GPIO) },
            pg12: unsafe { Disabled::__new(&_GPIO) },
            pg13: unsafe { Disabled::__new(&_GPIO) },
            pg14: unsafe { Disabled::__new(&_GPIO) },
            pg15: unsafe { Disabled::__new(&_GPIO) },
            pg16: unsafe { Disabled::__new(&_GPIO) },
            pg17: unsafe { Disabled::__new(&_GPIO) },
            pg18: unsafe { Disabled::__new(&_GPIO) },
        }
    }
}
