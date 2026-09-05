// SPDX-License-Identifier: GPL-2.0+

use core::ffi::{c_char, c_int, c_void};
use core::sync::atomic::{AtomicBool, AtomicI32, AtomicU32, Ordering};

use syterkit_ffi::raw;

static SERIAL_WRITES: AtomicI32 = AtomicI32::new(0);
static GPIO_VALUE: AtomicI32 = AtomicI32::new(0);
static I2C_REGISTER: AtomicU32 = AtomicU32::new(0);
static CPU_PLL: AtomicU32 = AtomicU32::new(0);
static CORE_CALLS: AtomicU32 = AtomicU32::new(0);
static UART_WRITES: AtomicU32 = AtomicU32::new(0);
static UART_READY: AtomicBool = AtomicBool::new(true);
static PRINTK_CALLS: AtomicU32 = AtomicU32::new(0);
static PRINTK_LEVEL: AtomicI32 = AtomicI32::new(0);

#[no_mangle]
pub static mut uart_dbg: raw::sunxi_serial_t = unsafe { core::mem::zeroed() };

#[no_mangle]
pub extern "C" fn sunxi_serial_init_stdout() -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn sunxi_serial_putc(_arg: *mut c_void, _value: c_char) {
    SERIAL_WRITES.fetch_add(1, Ordering::Relaxed);
}

#[no_mangle]
pub extern "C" fn sunxi_serial_tstc(_arg: *mut c_void) -> c_int {
    1
}

#[no_mangle]
pub extern "C" fn sunxi_serial_getc(_arg: *mut c_void) -> c_char {
    b'B' as c_char
}

#[no_mangle]
pub extern "C" fn sunxi_gpio_init(_gpio: *const raw::gpio_mux_t) {}

#[no_mangle]
pub extern "C" fn sun300iw1_clk_get_hosc_rate() -> u32 {
    40
}

#[no_mangle]
pub extern "C" fn sun55iw3_clk_set_cpu_pll(frequency_mhz: u32) {
    CPU_PLL.store(frequency_mhz, Ordering::Relaxed);
}

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
pub extern "C" fn printk_string(level: c_int, _message: *const c_char) {
    PRINTK_CALLS.fetch_add(1, Ordering::Relaxed);
    PRINTK_LEVEL.store(level, Ordering::Relaxed);
}

#[no_mangle]
pub extern "C" fn set_rpio_power_mode() {
    CORE_CALLS.fetch_add(1, Ordering::Relaxed);
}

#[no_mangle]
pub extern "C" fn sys_ldo_check() {
    CORE_CALLS.fetch_add(1, Ordering::Relaxed);
}

#[no_mangle]
pub extern "C" fn neon_enable() {
    CORE_CALLS.fetch_add(1, Ordering::Relaxed);
}

#[no_mangle]
pub extern "C" fn uart_putchar(_value: c_int) -> c_int {
    UART_WRITES.fetch_add(1, Ordering::Relaxed);
    0
}

#[no_mangle]
pub extern "C" fn uart_getchar() -> c_int {
    b'R' as c_int
}

#[no_mangle]
pub extern "C" fn tstc() -> c_int {
    UART_READY.load(Ordering::Relaxed) as c_int
}

#[no_mangle]
pub extern "C" fn uart_log_console_ready() {
    UART_READY.store(true, Ordering::Relaxed);
}

#[no_mangle]
pub extern "C" fn sunxi_gpio_set_value(_gpio: *const raw::gpio_mux_t, value: c_int) {
    GPIO_VALUE.store(value, Ordering::Relaxed);
}

#[no_mangle]
pub extern "C" fn sunxi_gpio_read(_gpio: *const raw::gpio_mux_t) -> c_int {
    GPIO_VALUE.load(Ordering::Relaxed)
}

#[no_mangle]
pub extern "C" fn sunxi_gpio_set_pull(_gpio: *const raw::gpio_mux_t, _pull: raw::gpio_pull_t) {}

#[no_mangle]
pub extern "C" fn sunxi_gpio_set_drv(_gpio: *const raw::gpio_mux_t, _drive: raw::gpio_drv_t) {}

#[no_mangle]
pub extern "C" fn sunxi_i2c_init(_i2c: *mut raw::sunxi_i2c_t) {}

#[no_mangle]
pub extern "C" fn sunxi_i2c_write(
    _i2c: *mut raw::sunxi_i2c_t,
    _address: u8,
    register: u32,
    _value: u8,
) -> c_int {
    I2C_REGISTER.store(register, Ordering::Relaxed);
    0
}

#[no_mangle]
pub extern "C" fn sunxi_i2c_read(
    _i2c: *mut raw::sunxi_i2c_t,
    _address: u8,
    register: u32,
    value: *mut u8,
) -> c_int {
    I2C_REGISTER.store(register, Ordering::Relaxed);
    unsafe { *value = 0x5a };
    0
}

#[no_mangle]
pub extern "C" fn sunxi_spi_transfer(
    _spi: *mut raw::sunxi_spi_t,
    _mode: raw::spi_io_mode_t,
    _tx: *mut c_void,
    _tx_len: u32,
    _rx: *mut c_void,
    _rx_len: u32,
) -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn sunxi_pwm_set_config(
    _pwm: *mut raw::sunxi_pwm_t,
    _channel: c_int,
    _config: *mut raw::sunxi_pwm_config_t,
) -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn rtc_set_bootmode_flag(_rtc: *const raw::sunxi_rtc_t, _flag: u8) -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn sunxi_efuse_write(
    _sid: *const raw::sunxi_sid_t,
    _offset: u32,
    _value: u32,
) -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn sunxi_dma_start(
    _handle: usize,
    _source: usize,
    _destination: usize,
    _bytes: u32,
) -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn sunxi_dma_test(
    _dma: *mut raw::sunxi_dma_t,
    _source: *mut u32,
    _destination: *mut u32,
    _bytes: u32,
) -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn rtc_set_vccio_det_spare(_rtc: *const raw::sunxi_rtc_t) {}

#[no_mangle]
pub extern "C" fn sunxi_gpio_power_mode_init() {}

#[no_mangle]
pub extern "C" fn sunxi_nsi_init() -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn do_irq(_regs: *mut raw::arm_regs_t) {}

#[no_mangle]
pub extern "C" fn sunxi_sdhci_xfer(
    _host: *mut raw::sunxi_sdhci_t,
    _command: *mut raw::mmc_cmd_t,
    _data: *mut raw::mmc_data_t,
) -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn sunxi_spif_set_config(
    _spif: *mut raw::sunxi_spif_t,
    _config: *const raw::spif_cfg,
) -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn pcie_controller_dbi_read(
    _controller: *mut raw::pcie_controller,
    _offset: u32,
    _size: u8,
    _value: *mut u32,
) -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn ufs_read(
    _device: *mut raw::ufs_device,
    _lba: u64,
    _blocks: u32,
    _buffer: *mut c_void,
) -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn usb_controller_open_otg(_otg: u32) -> usize {
    1
}

#[no_mangle]
pub extern "C" fn sunxi_usb_send_data(_buffer: *mut c_void, _length: u32) -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn get_arch_counter() -> u64 {
    0
}

#[no_mangle]
pub extern "C" fn time_ms() -> u32 {
    0
}

#[no_mangle]
pub extern "C" fn time_us() -> u64 {
    0
}

#[no_mangle]
pub extern "C" fn get_init_timestamp() -> u32 {
    0
}

#[no_mangle]
pub extern "C" fn timer_create(
    _timer: *mut raw::timer_t,
    _callback: Option<unsafe extern "C" fn(*mut c_void, u32)>,
    _context: *mut c_void,
) {
}

#[no_mangle]
pub extern "C" fn timer_start(_timer: *mut raw::timer_t, _max_run_count: u32, _interval: u32) {}

#[no_mangle]
pub extern "C" fn timer_handle() {}

#[no_mangle]
pub extern "C" fn disk_set_device(_drive: u8, _device: *mut raw::sdmmc_pdata) -> raw::DRESULT {
    raw::DRESULT_RES_OK
}

#[no_mangle]
pub extern "C" fn disk_initialize(_drive: u8) -> raw::DSTATUS {
    0
}

#[no_mangle]
pub extern "C" fn disk_status(_drive: u8) -> raw::DSTATUS {
    0
}

#[no_mangle]
pub extern "C" fn disk_read(
    _drive: u8,
    _buffer: *mut u8,
    _sector: raw::LBA_t,
    _count: raw::UINT,
) -> raw::DRESULT {
    raw::DRESULT_RES_OK
}

#[no_mangle]
pub extern "C" fn disk_write(
    _drive: u8,
    _buffer: *const u8,
    _sector: raw::LBA_t,
    _count: raw::UINT,
) -> raw::DRESULT {
    raw::DRESULT_RES_OK
}

#[no_mangle]
pub extern "C" fn disk_ioctl(_drive: u8, _command: u8, _buffer: *mut c_void) -> raw::DRESULT {
    raw::DRESULT_RES_OK
}

#[no_mangle]
pub extern "C" fn sdmmc_initialize() -> c_int {
    0
}

#[no_mangle]
pub extern "C" fn sdmmc_block_read(_start: u32, blocks: u32, destination: *mut c_void) -> u32 {
    if !destination.is_null() {
        unsafe {
            core::slice::from_raw_parts_mut(
                destination.cast::<u8>(),
                blocks as usize * raw::FF_MIN_SS as usize,
            )
            .fill(0x7e);
        }
    }
    blocks
}

#[test]
fn generated_bindings_expose_c_driver_signatures() {
    let serial_init: unsafe extern "C" fn() -> c_int = raw::sunxi_serial_init_stdout;
    let serial_putc: unsafe extern "C" fn(*mut c_void, c_char) = raw::sunxi_serial_putc;
    let gpio_set: unsafe extern "C" fn(*const raw::gpio_mux_t, c_int) = raw::sunxi_gpio_set_value;
    let i2c_read: unsafe extern "C" fn(*mut raw::sunxi_i2c_t, u8, u32, *mut u8) -> c_int =
        raw::sunxi_i2c_read;
    let get_hosc: unsafe extern "C" fn() -> u32 = raw::sun300iw1_clk_get_hosc_rate;
    let set_cpu_pll: unsafe extern "C" fn(u32) = raw::sun55iw3_clk_set_cpu_pll;

    SERIAL_WRITES.store(0, Ordering::Relaxed);
    let gpio = raw::gpio_mux_t {
        base: 0,
        pin: 7,
        bank: 1,
        mux: raw::GPIO_OUTPUT as u8,
    };
    let mut i2c = unsafe { core::mem::zeroed::<raw::sunxi_i2c_t>() };
    let mut value = 0;

    unsafe {
        assert_eq!(serial_init(), 0);
        serial_putc(core::ptr::null_mut(), b'X' as c_char);
        gpio_set(&gpio, raw::GPIO_LEVEL_HIGH as c_int);
        assert_eq!(i2c_read(&mut i2c, 0x50, 0x1234, &mut value), 0);
        assert_eq!(get_hosc(), 40);
        set_cpu_pll(1008);
    }

    assert_eq!(SERIAL_WRITES.load(Ordering::Relaxed), 1);
    assert_eq!(
        GPIO_VALUE.load(Ordering::Relaxed),
        raw::GPIO_LEVEL_HIGH as c_int
    );
    assert_eq!(I2C_REGISTER.load(Ordering::Relaxed), 0x1234);
    assert_eq!(value, 0x5a);
    assert_eq!(CPU_PLL.load(Ordering::Relaxed), 1008);
}

#[test]
fn generated_bindings_expose_root_core_signatures() {
    let show_banner: unsafe extern "C" fn() = raw::show_banner;
    let show_banner_with_build_info: unsafe extern "C" fn(*const c_char) =
        raw::show_banner_with_build_info;
    let clean_data: unsafe extern "C" fn() = raw::clean_syterkit_data;
    let set_rpio_power: unsafe extern "C" fn() = raw::set_rpio_power_mode;
    let check_ldo: unsafe extern "C" fn() = raw::sys_ldo_check;
    let enable_neon: unsafe extern "C" fn() = raw::neon_enable;
    let putc: unsafe extern "C" fn(c_int) -> c_int = raw::uart_putchar;
    let getc: unsafe extern "C" fn() -> c_int = raw::uart_getchar;
    let tstc: unsafe extern "C" fn() -> c_int = raw::tstc;
    let console_ready: unsafe extern "C" fn() = raw::uart_log_console_ready;

    CORE_CALLS.store(0, Ordering::Relaxed);
    UART_WRITES.store(0, Ordering::Relaxed);
    UART_READY.store(true, Ordering::Relaxed);

    unsafe {
        show_banner();
        show_banner_with_build_info(b"rustc test\0".as_ptr().cast());
        clean_data();
        set_rpio_power();
        check_ldo();
        enable_neon();
        assert_eq!(putc(b'X' as c_int), 0);
        assert_eq!(getc(), b'R' as c_int);
        assert_eq!(tstc(), 1);
        console_ready();
    }

    assert_eq!(CORE_CALLS.load(Ordering::Relaxed), 6);
    assert_eq!(UART_WRITES.load(Ordering::Relaxed), 1);
}

#[test]
fn generated_bindings_expose_generic_printk_signature() {
    let printk_string: unsafe extern "C" fn(c_int, *const c_char) = raw::printk_string;

    PRINTK_CALLS.store(0, Ordering::Relaxed);
    PRINTK_LEVEL.store(0, Ordering::Relaxed);

    unsafe {
        printk_string(3, b"binding test\0".as_ptr().cast());
    }

    assert_eq!(PRINTK_CALLS.load(Ordering::Relaxed), 1);
    assert_eq!(PRINTK_LEVEL.load(Ordering::Relaxed), 3);
}

#[test]
fn generated_bindings_preserve_public_constants_and_layouts() {
    assert_eq!(raw::GPIO_INPUT as u32, 0);
    assert_eq!(raw::GPIO_OUTPUT as u32, 1);
    assert_eq!(raw::GPIO_LEVEL_HIGH as u32, 1);
    assert_eq!(raw::gpio_pull_t_GPIO_PULL_UP as u32, 0);
    assert_eq!(raw::gpio_pull_t_GPIO_PULL_DOWN as u32, 1);
    assert_eq!(core::mem::size_of::<raw::gpio_mux_t>(), 16);
    assert_eq!(core::mem::align_of::<raw::gpio_mux_t>(), 8);
}

#[test]
fn generated_bindings_cover_expanded_driver_abi() {
    let spi_transfer: unsafe extern "C" fn(
        *mut raw::sunxi_spi_t,
        raw::spi_io_mode_t,
        *mut c_void,
        u32,
        *mut c_void,
        u32,
    ) -> c_int = raw::sunxi_spi_transfer;
    let pwm_set_config: unsafe extern "C" fn(
        *mut raw::sunxi_pwm_t,
        c_int,
        *mut raw::sunxi_pwm_config_t,
    ) -> c_int = raw::sunxi_pwm_set_config;
    let rtc_bootmode: unsafe extern "C" fn(*const raw::sunxi_rtc_t, u8) -> c_int =
        raw::rtc_set_bootmode_flag;
    let sid_write: unsafe extern "C" fn(*const raw::sunxi_sid_t, u32, u32) -> c_int =
        raw::sunxi_efuse_write;
    let dma_start: unsafe extern "C" fn(usize, usize, usize, u32) -> c_int = raw::sunxi_dma_start;
    let dma_test: unsafe extern "C" fn(*mut raw::sunxi_dma_t, *mut u32, *mut u32, u32) -> c_int =
        raw::sunxi_dma_test;
    let rtc_vccio: unsafe extern "C" fn(*const raw::sunxi_rtc_t) = raw::rtc_set_vccio_det_spare;
    let gpio_power: unsafe extern "C" fn() = raw::sunxi_gpio_power_mode_init;
    let nsi_init: unsafe extern "C" fn() -> c_int = raw::sunxi_nsi_init;
    let arm_irq: unsafe extern "C" fn(*mut raw::arm_regs_t) = raw::do_irq;
    let mmc_xfer: unsafe extern "C" fn(
        *mut raw::sunxi_sdhci_t,
        *mut raw::mmc_cmd_t,
        *mut raw::mmc_data_t,
    ) -> c_int = raw::sunxi_sdhci_xfer;
    let spif_config: unsafe extern "C" fn(*mut raw::sunxi_spif_t, *const raw::spif_cfg) -> c_int =
        raw::sunxi_spif_set_config;
    let pcie_dbi_read: unsafe extern "C" fn(*mut raw::pcie_controller, u32, u8, *mut u32) -> c_int =
        raw::pcie_controller_dbi_read;
    let ufs_read: unsafe extern "C" fn(*mut raw::ufs_device, u64, u32, *mut c_void) -> c_int =
        raw::ufs_read;
    let usb_open: unsafe extern "C" fn(u32) -> usize = raw::usb_controller_open_otg;
    let usb_send: unsafe extern "C" fn(*mut c_void, u32) -> c_int = raw::sunxi_usb_send_data;
    let mut spi = unsafe { core::mem::zeroed::<raw::sunxi_spi_t>() };
    let mut pwm = unsafe { core::mem::zeroed::<raw::sunxi_pwm_t>() };
    let mut config = unsafe { core::mem::zeroed::<raw::sunxi_pwm_config_t>() };
    let rtc = unsafe { core::mem::zeroed::<raw::sunxi_rtc_t>() };
    let sid = unsafe { core::mem::zeroed::<raw::sunxi_sid_t>() };
    let mut host = unsafe { core::mem::zeroed::<raw::sunxi_sdhci_t>() };
    let mut command = unsafe { core::mem::zeroed::<raw::mmc_cmd_t>() };
    let mut ufs = unsafe { core::mem::zeroed::<raw::ufs_device>() };
    let mut value = 0;
    let mut buffer = [0u8; 2];
    let mut source = [0u32; 4];
    let mut destination = [0u32; 4];

    unsafe {
        spi_transfer(
            &mut spi,
            raw::spi_io_mode_t_SPI_IO_SINGLE,
            core::ptr::null_mut(),
            0,
            core::ptr::null_mut(),
            0,
        );
        pwm_set_config(&mut pwm, 0, &mut config);
        rtc_bootmode(&rtc, 1);
        sid_write(&sid, 0, 1);
        dma_start(1, 2, 3, 4);
        dma_test(
            core::ptr::null_mut(),
            source.as_mut_ptr(),
            destination.as_mut_ptr(),
            16,
        );
        rtc_vccio(&rtc);
        gpio_power();
        assert_eq!(nsi_init(), 0);
        arm_irq(core::ptr::null_mut());
        mmc_xfer(&mut host, &mut command, core::ptr::null_mut());
        spif_config(core::ptr::null_mut(), core::ptr::null());
        pcie_dbi_read(core::ptr::null_mut(), 0, 4, &mut value);
        ufs_read(&mut ufs, 0, 1, buffer.as_mut_ptr().cast());
        assert_eq!(usb_open(0), 1);
        usb_send(buffer.as_mut_ptr().cast(), buffer.len() as u32);
    }

    assert_eq!(raw::spi_io_mode_t_SPI_IO_QUAD_IO as u32, 3);
    assert_eq!(raw::sunxi_pwm_mode_t_PWM_MODE_PLUSE as u32, 1);
    assert_eq!(raw::SUNXI_USB_MAX_CONTROLLERS, 3);
    assert_eq!(raw::SUNXI_REMOTEPROC_MAX_FIRMWARES, 4);
}

#[test]
fn generated_bindings_cover_timer_and_disk_abi() {
    let timer_counter: unsafe extern "C" fn() -> u64 = raw::get_arch_counter;
    let timer_create: unsafe extern "C" fn(
        *mut raw::timer_t,
        Option<unsafe extern "C" fn(*mut c_void, u32)>,
        *mut c_void,
    ) = raw::timer_create;
    let disk_read: unsafe extern "C" fn(u8, *mut u8, raw::LBA_t, raw::UINT) -> raw::DRESULT =
        raw::disk_read;

    let mut timer: raw::timer_t = unsafe { core::mem::zeroed() };
    let mut buffer = [0u8; raw::FF_MIN_SS as usize];
    unsafe {
        assert_eq!(timer_counter(), 0);
        timer_create(&mut timer, None, core::ptr::null_mut());
        assert_eq!(disk_read(0, buffer.as_mut_ptr(), 3, 1), raw::DRESULT_RES_OK);
    }
    assert_eq!(raw::FF_MIN_SS, 512);
    assert_eq!(raw::FF_VOLUMES, 1);
}

#[test]
fn generated_bindings_cover_legacy_sdmmc_media_abi() {
    let initialize: unsafe extern "C" fn() -> c_int = raw::sdmmc_initialize;
    let block_read: unsafe extern "C" fn(u32, u32, *mut c_void) -> u32 = raw::sdmmc_block_read;
    let mut buffer = [0u8; raw::FF_MIN_SS as usize * 2];

    unsafe {
        assert_eq!(initialize(), 0);
        assert_eq!(block_read(12, 2, buffer.as_mut_ptr().cast()), 2);
    }
    assert_eq!(buffer, [0x7e; raw::FF_MIN_SS as usize * 2]);
}
