// SPDX-License-Identifier: GPL-2.0+

use core::ffi::c_void;

use syterkit_ffi::raw;
use syterkit_lib::{
    checked_block_bytes, checked_block_count, transferred_blocks, BlockDevice, DriverResult,
    INVALID_ARGUMENT,
};

pub const MMC_BLOCK_SIZE: usize = 512;

/// Borrowed wrapper for one SDHCI/MMC host descriptor.
pub struct Sdhci<'a> {
    raw: &'a mut raw::sunxi_sdhci_t,
}

impl<'a> Sdhci<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::sunxi_sdhci_t) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_sdhci_init(self.raw) })
    }

    pub fn core_initialize(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_sdhci_core_init(self.raw) })
    }

    pub fn set_ios(&mut self) {
        unsafe { raw::sunxi_sdhci_set_ios(self.raw) };
    }

    pub fn update_phase(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_sdhci_update_phase(self.raw) })
    }

    pub fn transfer(
        &mut self,
        command: &mut raw::mmc_cmd_t,
        data: Option<&mut raw::mmc_data_t>,
    ) -> DriverResult<()> {
        let data = data.map_or(core::ptr::null_mut(), |data| data as *mut _);
        syterkit_lib::status(unsafe { raw::sunxi_sdhci_xfer(self.raw, command, data) })
    }

    pub fn transfer_timeout(
        &mut self,
        command: &mut raw::mmc_cmd_t,
        data: Option<&mut raw::mmc_data_t>,
        timeout_us: u32,
    ) -> DriverResult<()> {
        let data = data.map_or(core::ptr::null_mut(), |data| data as *mut _);
        syterkit_lib::status(unsafe {
            raw::sunxi_sdhci_xfer_timeout(self.raw, command, data, timeout_us)
        })
    }

    pub fn set_clock(&mut self, clock_hz: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_sdhci_set_mclk(self.raw, clock_hz) })
    }

    pub fn clock(&mut self) -> u32 {
        unsafe { raw::sunxi_sdhci_get_mclk(self.raw) }
    }

    pub fn block_size(&self) -> usize {
        let block_size = self.raw.mmc.blksz as usize;
        if block_size == 0 {
            MMC_BLOCK_SIZE
        } else {
            block_size
        }
    }

    pub fn set_io_voltage(&mut self, gpio: &raw::gpio_mux_t, voltage_uv: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_sdhci_set_io_voltage(self.raw, gpio, voltage_uv) })
    }

    pub fn set_skew(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_sdhci_set_skew(self.raw) })
    }

    pub fn initialize_card(&mut self) -> DriverResult<()> {
        let handle = (self.raw as *mut raw::sunxi_sdhci_t).cast::<c_void>();
        syterkit_lib::status(unsafe { raw::sunxi_mmc_init(handle) })
    }

    /// Read blocks using the C block layer. The return value is the number of
    /// blocks reported by C; the legacy API uses zero for failure.
    pub fn read_blocks(&mut self, destination: &mut [u8], start: u32, blocks: u32) -> u32 {
        if checked_block_bytes(self.block_size(), u64::from(blocks), destination.len()).is_err() {
            return 0;
        }
        unsafe {
            raw::sunxi_mmc_blk_read(
                (self.raw as *mut raw::sunxi_sdhci_t).cast::<c_void>(),
                destination.as_mut_ptr().cast::<c_void>(),
                start,
                blocks,
            )
        }
    }

    pub fn write_blocks(&mut self, source: &[u8], start: u32, blocks: u32) -> u32 {
        if checked_block_bytes(self.block_size(), u64::from(blocks), source.len()).is_err() {
            return 0;
        }
        unsafe {
            raw::sunxi_mmc_blk_write(
                (self.raw as *mut raw::sunxi_sdhci_t).cast::<c_void>(),
                source.as_ptr().cast_mut().cast::<c_void>(),
                start,
                blocks,
            )
        }
    }

    pub fn dump_registers(&mut self) {
        unsafe { raw::sunxi_sdhci_dump_reg(self.raw) };
    }

    /// Switch an MMC EXT_CSD field while high-speed timing support is enabled.
    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn high_speed_switch_card(&mut self, set: u8, index: u8, value: u8) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_mmc_hs_switch_card(self.raw, set, index, value) })
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn high_speed_wait_status(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_mmc_hs_wait_status(self.raw) })
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn high_speed_set_clock(&mut self, clock_hz: u32) {
        unsafe { raw::sunxi_mmc_hs_set_clock(self.raw, clock_hz) };
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn high_speed_set_bus_width(&mut self, width: u32) {
        unsafe { raw::sunxi_mmc_hs_set_bus_width(self.raw, width) };
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn high_speed_switch_bus_mode(&mut self, speed_mode: u32, width: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::sunxi_mmc_hs_switch_bus_mode(self.raw, speed_mode, width)
        })
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn switch_hs200(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_mmc_mmc_switch_hs200(self.raw) })
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn prepare_hs200(&mut self, width: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_mmc_mmc_prepare_hs200(self.raw, width) })
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn downgrade_high_speed(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_mmc_mmc_downgrade_high_speed(self.raw) })
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn set_hs400_mode(&mut self, enabled: bool) {
        unsafe { raw::sunxi_mmc_hs400_mode_set(self.raw, i8::from(enabled)) };
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn switch_hs400(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_mmc_mmc_switch_hs400(self.raw) })
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn prepare_hs400(&mut self, width: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_mmc_mmc_prepare_hs400(self.raw, width) })
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn execute_tuning(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_mmc_execute_tuning(self.raw) })
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn execute_hs400_command_tuning(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_mmc_execute_hs400_command_tuning(self.raw) })
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    pub fn execute_hs400_tuning(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_mmc_execute_hs400_tuning(self.raw) })
    }
}

/// Borrowed wrapper for the SD/MMC block-layer state.
pub struct SdMmc<'a> {
    raw: &'a mut raw::sdmmc_pdata_t,
}

/// Global state reset for the optional MMC tuning cache.
pub struct MmcTuning;

#[cfg(syterkit_config_driver_mmc_tuning)]
impl MmcTuning {
    pub fn reset() {
        unsafe { raw::sunxi_mmc_tuning_reset() };
    }
}

impl<'a> SdMmc<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::sdmmc_pdata_t) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self, host: &mut Sdhci<'_>) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sdmmc_init(self.raw, host.raw) })
    }

    pub fn is_online(&self) -> bool {
        self.raw.online != 0
    }

    pub fn block_size(&self) -> usize {
        if self.raw.hci.is_null() {
            MMC_BLOCK_SIZE
        } else {
            let block_size = unsafe { (*self.raw.hci).mmc.blksz as usize };
            if block_size == 0 {
                MMC_BLOCK_SIZE
            } else {
                block_size
            }
        }
    }

    pub fn read_blocks(&mut self, destination: &mut [u8], block: u32, count: u32) -> u32 {
        if checked_block_bytes(self.block_size(), u64::from(count), destination.len()).is_err() {
            return 0;
        }
        unsafe { raw::sdmmc_blk_read(self.raw, destination.as_mut_ptr(), block, count) }
    }

    pub fn write_blocks(&mut self, source: &[u8], block: u32, count: u32) -> u32 {
        if checked_block_bytes(self.block_size(), u64::from(count), source.len()).is_err() {
            return 0;
        }
        unsafe { raw::sdmmc_blk_write(self.raw, source.as_ptr().cast_mut(), block, count) }
    }

    pub fn require_buffer_size(buffer: &[u8], block_size: usize, blocks: u32) -> DriverResult<()> {
        checked_block_bytes(block_size, u64::from(blocks), buffer.len()).map(|_| ())
    }
}

/// Wrapper for the legacy global FatFs SD/MMC media entry points.
pub struct SdMmcMedia;

impl SdMmcMedia {
    pub fn initialize() -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sdmmc_initialize() })
    }

    pub fn block_read(start: u32, blocks: u32, destination: &mut [u8]) -> DriverResult<usize> {
        checked_block_bytes(MMC_BLOCK_SIZE, u64::from(blocks), destination.len())?;
        let transferred = unsafe {
            raw::sdmmc_block_read(start, blocks, destination.as_mut_ptr().cast::<c_void>())
        };
        transferred_blocks(transferred, blocks)
    }
}

impl BlockDevice for Sdhci<'_> {
    fn block_size(&self) -> usize {
        Sdhci::block_size(self)
    }

    fn read_blocks(&mut self, lba: u64, buffer: &mut [u8]) -> DriverResult<usize> {
        let count = checked_block_count(self.block_size(), buffer.len())?;
        let lba = u32::try_from(lba).map_err(|_| INVALID_ARGUMENT)?;
        transferred_blocks(Sdhci::read_blocks(self, buffer, lba, count), count)
    }

    fn write_blocks(&mut self, lba: u64, buffer: &[u8]) -> DriverResult<usize> {
        let count = checked_block_count(self.block_size(), buffer.len())?;
        let lba = u32::try_from(lba).map_err(|_| INVALID_ARGUMENT)?;
        transferred_blocks(Sdhci::write_blocks(self, buffer, lba, count), count)
    }
}

impl BlockDevice for SdMmc<'_> {
    fn block_size(&self) -> usize {
        SdMmc::block_size(self)
    }

    fn read_blocks(&mut self, lba: u64, buffer: &mut [u8]) -> DriverResult<usize> {
        let count = checked_block_count(self.block_size(), buffer.len())?;
        let lba = u32::try_from(lba).map_err(|_| INVALID_ARGUMENT)?;
        transferred_blocks(SdMmc::read_blocks(self, buffer, lba, count), count)
    }

    fn write_blocks(&mut self, lba: u64, buffer: &[u8]) -> DriverResult<usize> {
        let count = checked_block_count(self.block_size(), buffer.len())?;
        let lba = u32::try_from(lba).map_err(|_| INVALID_ARGUMENT)?;
        transferred_blocks(SdMmc::write_blocks(self, buffer, lba, count), count)
    }
}

#[cfg(test)]
mod tests {
    use super::{MmcTuning, SdMmc, SdMmcMedia, Sdhci, MMC_BLOCK_SIZE};
    use core::ffi::{c_int, c_void};
    use core::sync::atomic::{AtomicI32, AtomicU32, Ordering};
    use syterkit_lib::BlockDevice;

    static MMC_STATUS: AtomicI32 = AtomicI32::new(0);
    static MMC_CLOCK: AtomicU32 = AtomicU32::new(0);
    static MEDIA_INIT_STATUS: AtomicI32 = AtomicI32::new(0);
    static MEDIA_READ_RESULT: AtomicU32 = AtomicU32::new(u32::MAX);

    #[no_mangle]
    pub extern "C" fn sunxi_sdhci_init(_host: *mut syterkit_ffi::raw::sunxi_sdhci_t) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_sdhci_core_init(_host: *mut syterkit_ffi::raw::sunxi_sdhci_t) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_sdhci_set_ios(_host: *mut syterkit_ffi::raw::sunxi_sdhci_t) {}
    #[no_mangle]
    pub extern "C" fn sunxi_sdhci_update_phase(
        _host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_sdhci_xfer(
        _host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
        _cmd: *mut syterkit_ffi::raw::mmc_cmd_t,
        _data: *mut syterkit_ffi::raw::mmc_data_t,
    ) -> c_int {
        MMC_STATUS.load(Ordering::Relaxed)
    }
    #[no_mangle]
    pub extern "C" fn sunxi_sdhci_xfer_timeout(
        host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
        cmd: *mut syterkit_ffi::raw::mmc_cmd_t,
        data: *mut syterkit_ffi::raw::mmc_data_t,
        _timeout: u32,
    ) -> c_int {
        sunxi_sdhci_xfer(host, cmd, data)
    }
    #[no_mangle]
    pub extern "C" fn sunxi_sdhci_set_mclk(
        _host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
        clock: u32,
    ) -> c_int {
        MMC_CLOCK.store(clock, Ordering::Relaxed);
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_sdhci_get_mclk(_host: *mut syterkit_ffi::raw::sunxi_sdhci_t) -> u32 {
        MMC_CLOCK.load(Ordering::Relaxed)
    }
    #[no_mangle]
    pub extern "C" fn sunxi_sdhci_set_io_voltage(
        _host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
        _gpio: *const syterkit_ffi::raw::gpio_mux_t,
        _voltage: u32,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_sdhci_set_skew(_host: *mut syterkit_ffi::raw::sunxi_sdhci_t) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_sdhci_dump_reg(_host: *mut syterkit_ffi::raw::sunxi_sdhci_t) {}

    #[cfg(syterkit_config_driver_mmc_tuning)]
    #[no_mangle]
    pub extern "C" fn sunxi_mmc_hs_switch_card(
        _host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
        _set: u8,
        _index: u8,
        _value: u8,
    ) -> c_int {
        0
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    #[no_mangle]
    pub extern "C" fn sunxi_mmc_hs_wait_status(
        _host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
    ) -> c_int {
        0
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    #[no_mangle]
    pub extern "C" fn sunxi_mmc_hs_set_clock(
        _host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
        _clock: u32,
    ) {
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    #[no_mangle]
    pub extern "C" fn sunxi_mmc_hs_set_bus_width(
        _host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
        _width: u32,
    ) {
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    #[no_mangle]
    pub extern "C" fn sunxi_mmc_hs_switch_bus_mode(
        _host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
        _speed_mode: u32,
        _width: u32,
    ) -> c_int {
        0
    }

    macro_rules! mock_tuning_status {
        ($name:ident) => {
            #[cfg(syterkit_config_driver_mmc_tuning)]
            #[no_mangle]
            pub extern "C" fn $name(_host: *mut syterkit_ffi::raw::sunxi_sdhci_t) -> c_int {
                0
            }
        };
    }

    mock_tuning_status!(sunxi_mmc_mmc_switch_hs200);
    mock_tuning_status!(sunxi_mmc_mmc_downgrade_high_speed);
    mock_tuning_status!(sunxi_mmc_mmc_switch_hs400);
    mock_tuning_status!(sunxi_mmc_execute_tuning);
    mock_tuning_status!(sunxi_mmc_execute_hs400_command_tuning);
    mock_tuning_status!(sunxi_mmc_execute_hs400_tuning);

    #[cfg(syterkit_config_driver_mmc_tuning)]
    #[no_mangle]
    pub extern "C" fn sunxi_mmc_mmc_prepare_hs200(
        _host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
        _width: u32,
    ) -> c_int {
        0
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    #[no_mangle]
    pub extern "C" fn sunxi_mmc_mmc_prepare_hs400(
        _host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
        _width: u32,
    ) -> c_int {
        0
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    #[no_mangle]
    pub extern "C" fn sunxi_mmc_hs400_mode_set(
        _host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
        _enabled: syterkit_ffi::raw::bool_,
    ) {
    }

    #[cfg(syterkit_config_driver_mmc_tuning)]
    #[no_mangle]
    pub extern "C" fn sunxi_mmc_tuning_reset() {}
    #[no_mangle]
    pub extern "C" fn sunxi_mmc_init(_host: *mut core::ffi::c_void) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_mmc_blk_read(
        _host: *mut core::ffi::c_void,
        _dst: *mut core::ffi::c_void,
        _start: u32,
        blocks: u32,
    ) -> u32 {
        blocks
    }
    #[no_mangle]
    pub extern "C" fn sunxi_mmc_blk_write(
        _host: *mut core::ffi::c_void,
        _src: *mut core::ffi::c_void,
        _start: u32,
        blocks: u32,
    ) -> u32 {
        blocks
    }
    #[no_mangle]
    pub extern "C" fn sdmmc_init(
        data: *mut syterkit_ffi::raw::sdmmc_pdata_t,
        host: *mut syterkit_ffi::raw::sunxi_sdhci_t,
    ) -> c_int {
        unsafe {
            (*data).hci = host;
            (*data).online = 1;
        }
        0
    }
    #[no_mangle]
    pub extern "C" fn sdmmc_blk_read(
        _data: *mut syterkit_ffi::raw::sdmmc_pdata_t,
        _buffer: *mut u8,
        _block: u32,
        count: u32,
    ) -> u32 {
        count
    }
    #[no_mangle]
    pub extern "C" fn sdmmc_blk_write(
        _data: *mut syterkit_ffi::raw::sdmmc_pdata_t,
        _buffer: *mut u8,
        _block: u32,
        count: u32,
    ) -> u32 {
        count
    }

    #[no_mangle]
    pub extern "C" fn sdmmc_initialize() -> c_int {
        MEDIA_INIT_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sdmmc_block_read(_start: u32, blocks: u32, destination: *mut c_void) -> u32 {
        let result = MEDIA_READ_RESULT.load(Ordering::Relaxed);
        let transferred = if result == u32::MAX { blocks } else { result };
        if !destination.is_null() {
            unsafe {
                core::slice::from_raw_parts_mut(
                    destination.cast::<u8>(),
                    transferred.min(blocks) as usize * MMC_BLOCK_SIZE,
                )
                .fill(0x3c);
            }
        }
        transferred
    }

    #[test]
    fn mmc_wraps_host_commands_and_block_access() {
        let mut raw_host: syterkit_ffi::raw::sunxi_sdhci_t = unsafe { core::mem::zeroed() };
        let mut host = unsafe { Sdhci::from_raw(&mut raw_host) };
        host.initialize().unwrap();
        host.core_initialize().unwrap();
        host.set_clock(50_000_000).unwrap();
        assert_eq!(host.clock(), 50_000_000);
        let mut cmd: syterkit_ffi::raw::mmc_cmd_t = unsafe { core::mem::zeroed() };
        host.transfer(&mut cmd, None).unwrap();
        let mut buffer = [0u8; MMC_BLOCK_SIZE * 2];
        assert_eq!(host.read_blocks(&mut buffer, 4, 2), 2);

        #[cfg(syterkit_config_driver_mmc_tuning)]
        {
            host.high_speed_switch_card(1, 185, 2).unwrap();
            host.high_speed_wait_status().unwrap();
            host.high_speed_set_clock(100_000_000);
            host.high_speed_set_bus_width(8);
            host.high_speed_switch_bus_mode(1, 8).unwrap();
            host.switch_hs200().unwrap();
            host.prepare_hs200(8).unwrap();
            host.downgrade_high_speed().unwrap();
            host.set_hs400_mode(true);
            host.switch_hs400().unwrap();
            host.prepare_hs400(8).unwrap();
            host.execute_tuning().unwrap();
            host.execute_hs400_command_tuning().unwrap();
            host.execute_hs400_tuning().unwrap();
            MmcTuning::reset();
        }

        let mut raw_data: syterkit_ffi::raw::sdmmc_pdata_t = unsafe { core::mem::zeroed() };
        let mut card = unsafe { SdMmc::from_raw(&mut raw_data) };
        card.initialize(&mut host).unwrap();
        assert!(card.is_online());
        assert_eq!(card.read_blocks(&mut buffer, 4, 2), 2);
        assert_eq!(BlockDevice::read_blocks(&mut card, 4, &mut buffer), Ok(2));
        assert_eq!(BlockDevice::write_blocks(&mut card, 4, &buffer), Ok(2));
        assert_eq!(card.block_size(), MMC_BLOCK_SIZE);
        assert!(SdMmc::require_buffer_size(&buffer, MMC_BLOCK_SIZE, 2).is_ok());
        assert!(SdMmc::require_buffer_size(&buffer, MMC_BLOCK_SIZE, 3).is_err());
        assert_eq!(card.read_blocks(&mut [0; MMC_BLOCK_SIZE], 4, 2), 0);
    }

    #[test]
    fn legacy_media_wrapper_maps_status_and_block_counts() {
        MEDIA_INIT_STATUS.store(0, Ordering::Relaxed);
        MEDIA_READ_RESULT.store(u32::MAX, Ordering::Relaxed);
        assert_eq!(SdMmcMedia::initialize(), Ok(()));
        let mut buffer = [0u8; MMC_BLOCK_SIZE * 2];
        assert_eq!(SdMmcMedia::block_read(8, 2, &mut buffer), Ok(2));
        assert_eq!(buffer, [0x3c; MMC_BLOCK_SIZE * 2]);
        assert_eq!(
            SdMmcMedia::block_read(8, 2, &mut [0; MMC_BLOCK_SIZE]),
            Err(-1)
        );

        MEDIA_READ_RESULT.store(0, Ordering::Relaxed);
        assert_eq!(SdMmcMedia::block_read(8, 2, &mut buffer), Err(-1));
        MEDIA_READ_RESULT.store(3, Ordering::Relaxed);
        assert_eq!(SdMmcMedia::block_read(8, 2, &mut buffer), Err(-1));
        MEDIA_INIT_STATUS.store(-5, Ordering::Relaxed);
        assert_eq!(SdMmcMedia::initialize(), Err(-5));
    }
}
