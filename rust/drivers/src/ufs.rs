// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::{
    checked_block_bytes, checked_block_count, BlockDevice, DriverResult, INVALID_ARGUMENT,
};

/// Borrowed wrapper for the UFSHCI host controller.
pub struct UfsHost<'a> {
    raw: &'a mut raw::ufshc_host,
}

impl<'a> UfsHost<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::ufshc_host) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self, config: &raw::ufshc_config) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::ufshc_init(self.raw, config) })
    }

    pub fn exit(&mut self) {
        unsafe { raw::ufshc_exit(self.raw) };
    }

    pub fn execute(&mut self, request: &mut raw::ufshc_request) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::ufshc_exec(self.raw, request) })
    }

    pub fn uic_command(&mut self, args: &raw::ufshc_uic_cmd_args) -> DriverResult<u32> {
        let mut result = 0;
        syterkit_lib::status(unsafe { raw::ufshc_uic_command(self.raw, args, &mut result) })?;
        Ok(result)
    }

    pub fn dme_get(&mut self, attribute: u32, peer: bool) -> DriverResult<u32> {
        let mut value = 0;
        syterkit_lib::status(unsafe {
            raw::ufshc_dme_get(self.raw, attribute, &mut value, i8::from(peer))
        })?;
        Ok(value)
    }

    pub fn dme_set(&mut self, attribute: u32, value: u32, peer: bool) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::ufshc_dme_set(self.raw, attribute, value, i8::from(peer))
        })
    }

    pub fn dme_get_selected(
        &mut self,
        attribute: u32,
        selector: u16,
        peer: bool,
    ) -> DriverResult<u32> {
        let mut value = 0;
        syterkit_lib::status(unsafe {
            raw::ufshc_dme_get_sel(self.raw, attribute, selector, &mut value, i8::from(peer))
        })?;
        Ok(value)
    }

    pub fn dme_set_selected(
        &mut self,
        attribute: u32,
        selector: u16,
        value: u32,
        peer: bool,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::ufshc_dme_set_sel(self.raw, attribute, selector, value, i8::from(peer))
        })
    }

    pub fn maximum_power_mode(&mut self) -> DriverResult<raw::ufshc_power_mode> {
        let mut mode = unsafe { core::mem::zeroed() };
        syterkit_lib::status(unsafe { raw::ufshc_get_max_power_mode(self.raw, &mut mode) })?;
        Ok(mode)
    }

    pub fn change_power_mode(&mut self, mode: &raw::ufshc_power_mode) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::ufshc_change_power_mode(self.raw, mode) })
    }

    pub fn nop(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::ufshc_nop(self.raw) })
    }

    pub fn query_flag(&mut self, id: u8, set: bool) -> DriverResult<bool> {
        let mut value: raw::bool_ = 0;
        syterkit_lib::status(unsafe {
            raw::ufshc_query_flag(self.raw, id, i8::from(set), &mut value)
        })?;
        Ok(value != 0)
    }

    pub fn query_flag_opcode(&mut self, id: u8, opcode: u8) -> DriverResult<bool> {
        let mut value: raw::bool_ = 0;
        syterkit_lib::status(unsafe {
            raw::ufshc_query_flag_op(self.raw, id, opcode, &mut value)
        })?;
        Ok(value != 0)
    }

    pub fn query_attribute(
        &mut self,
        id: u8,
        index: u8,
        selector: u8,
        value: u32,
        write: bool,
    ) -> DriverResult<u32> {
        let mut value = value;
        syterkit_lib::status(unsafe {
            raw::ufshc_query_attribute(self.raw, id, index, selector, &mut value, i8::from(write))
        })?;
        Ok(value)
    }

    pub fn query_descriptor(
        &mut self,
        id: u8,
        index: u8,
        selector: u8,
        buffer: &mut [u8],
    ) -> DriverResult<usize> {
        let mut actual = 0;
        syterkit_lib::status(unsafe {
            raw::ufshc_query_descriptor(
                self.raw,
                id,
                index,
                selector,
                buffer.as_mut_ptr().cast(),
                buffer.len(),
                &mut actual,
            )
        })?;
        Ok(actual.min(buffer.len()))
    }

    pub fn query_descriptor_opcode(
        &mut self,
        opcode: u8,
        id: u8,
        index: u8,
        selector: u8,
        buffer: &mut [u8],
    ) -> DriverResult<usize> {
        let mut actual = 0;
        syterkit_lib::status(unsafe {
            raw::ufshc_query_descriptor_op(
                self.raw,
                opcode,
                id,
                index,
                selector,
                buffer.as_mut_ptr().cast(),
                buffer.len(),
                &mut actual,
            )
        })?;
        Ok(actual.min(buffer.len()))
    }

    pub fn task_request(&mut self, lun: u8, function: u8, task_id: u16) -> DriverResult<u8> {
        let mut response = 0;
        syterkit_lib::status(unsafe {
            raw::ufshc_task_request(self.raw, lun, function, task_id, &mut response)
        })?;
        Ok(response)
    }
}

/// Borrowed wrapper for one initialized UFS logical unit.
pub struct UfsDevice<'a> {
    raw: &'a mut raw::ufs_device,
}

impl<'a> UfsDevice<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::ufs_device) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self, config: &raw::ufshc_config) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::ufs_init(self.raw, config) })
    }

    pub fn initialize_lun(&mut self, config: &raw::ufshc_config, lun: u8) -> DriverResult<()> {
        if lun > 0x7f {
            return Err(INVALID_ARGUMENT);
        }
        syterkit_lib::status(unsafe { raw::ufs_init_lun(self.raw, config, lun) })
    }

    pub fn exit(&mut self) {
        unsafe { raw::ufs_exit(self.raw) };
    }

    pub fn read(&mut self, lba: u64, blocks: u32, buffer: &mut [u8]) -> DriverResult<()> {
        checked_block_bytes(self.block_size(), u64::from(blocks), buffer.len())?;
        syterkit_lib::status(unsafe {
            raw::ufs_read(self.raw, lba, blocks, buffer.as_mut_ptr().cast())
        })
    }

    pub fn write(&mut self, lba: u64, blocks: u32, buffer: &[u8]) -> DriverResult<()> {
        checked_block_bytes(self.block_size(), u64::from(blocks), buffer.len())?;
        syterkit_lib::status(unsafe {
            raw::ufs_write(self.raw, lba, blocks, buffer.as_ptr().cast())
        })
    }

    pub fn capacity(&self) -> u64 {
        unsafe { raw::ufs_capacity(self.raw) }
    }

    pub fn block_size(&self) -> usize {
        unsafe { raw::ufs_block_size(self.raw) as usize }
    }

    pub fn manufacturer_id(&self) -> u16 {
        unsafe { raw::ufs_manufacturer_id(self.raw) }
    }

    pub fn read_blocks(&mut self, lba: u64, blocks: u32, buffer: &mut [u8]) -> u32 {
        if checked_block_bytes(self.block_size(), u64::from(blocks), buffer.len()).is_err() {
            return 0;
        }
        unsafe { raw::ufs_blk_read(self.raw, buffer.as_mut_ptr().cast(), lba, blocks) }
    }

    pub fn write_blocks(&mut self, lba: u64, blocks: u32, buffer: &[u8]) -> u32 {
        if checked_block_bytes(self.block_size(), u64::from(blocks), buffer.len()).is_err() {
            return 0;
        }
        unsafe { raw::ufs_blk_write(self.raw, buffer.as_ptr().cast(), lba, blocks) }
    }
}

/// Borrowed wrapper for one UFS SCSI logical unit.
pub struct UfsScsi<'a> {
    raw: &'a mut raw::ufs_scsi_device,
}

impl<'a> UfsScsi<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::ufs_scsi_device) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self, host: &mut UfsHost<'_>, lun: u8) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::ufs_scsi_init(self.raw, host.raw, lun) })
    }

    pub fn execute(
        &mut self,
        cdb: &[u8],
        data: Option<&mut [u8]>,
        write: bool,
    ) -> DriverResult<()> {
        let cdb_len = u8::try_from(cdb.len()).map_err(|_| INVALID_ARGUMENT)?;
        let (data_ptr, data_len) = match data {
            Some(buffer) => (buffer.as_mut_ptr().cast(), buffer.len()),
            None => (core::ptr::null_mut(), 0),
        };
        syterkit_lib::status(unsafe {
            raw::ufs_scsi_exec(
                self.raw,
                if cdb.is_empty() {
                    core::ptr::null()
                } else {
                    cdb.as_ptr()
                },
                cdb_len,
                data_ptr,
                data_len,
                i8::from(write),
            )
        })
    }

    pub fn test_unit_ready(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::ufs_scsi_test_unit_ready(self.raw) })
    }

    pub fn request_sense(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::ufs_scsi_request_sense(self.raw) })
    }

    pub fn block_size(&self) -> usize {
        self.raw.block_size as usize
    }

    pub fn capacity(&self) -> u64 {
        self.raw.block_count
    }

    pub fn read(&mut self, lba: u64, blocks: u32, buffer: &mut [u8]) -> DriverResult<()> {
        checked_block_bytes(self.block_size(), u64::from(blocks), buffer.len())?;
        syterkit_lib::status(unsafe {
            raw::ufs_scsi_read(self.raw, lba, blocks, buffer.as_mut_ptr().cast())
        })
    }

    pub fn write(&mut self, lba: u64, blocks: u32, buffer: &[u8]) -> DriverResult<()> {
        checked_block_bytes(self.block_size(), u64::from(blocks), buffer.len())?;
        syterkit_lib::status(unsafe {
            raw::ufs_scsi_write(self.raw, lba, blocks, buffer.as_ptr().cast())
        })
    }
}

impl BlockDevice for UfsDevice<'_> {
    fn block_size(&self) -> usize {
        UfsDevice::block_size(self)
    }

    fn read_blocks(&mut self, lba: u64, buffer: &mut [u8]) -> DriverResult<usize> {
        let count = checked_block_count(self.block_size(), buffer.len())?;
        UfsDevice::read(self, lba, count, buffer)?;
        Ok(count as usize)
    }

    fn write_blocks(&mut self, lba: u64, buffer: &[u8]) -> DriverResult<usize> {
        let count = checked_block_count(self.block_size(), buffer.len())?;
        UfsDevice::write(self, lba, count, buffer)?;
        Ok(count as usize)
    }
}

impl BlockDevice for UfsScsi<'_> {
    fn block_size(&self) -> usize {
        UfsScsi::block_size(self)
    }

    fn read_blocks(&mut self, lba: u64, buffer: &mut [u8]) -> DriverResult<usize> {
        let count = checked_block_count(self.block_size(), buffer.len())?;
        UfsScsi::read(self, lba, count, buffer)?;
        Ok(count as usize)
    }

    fn write_blocks(&mut self, lba: u64, buffer: &[u8]) -> DriverResult<usize> {
        let count = checked_block_count(self.block_size(), buffer.len())?;
        UfsScsi::write(self, lba, count, buffer)?;
        Ok(count as usize)
    }
}

/// Sunxi-specific UFS PHY, clock and calibration operations.
pub struct SunxiUfs;

impl SunxiUfs {
    pub fn variant() -> Option<&'static raw::sunxi_ufs_variant> {
        let variant = unsafe { raw::sunxi_ufs_get_variant() };
        (!variant.is_null()).then(|| unsafe { &*variant })
    }

    pub fn decode_cal_words(
        cal: &mut raw::sunxi_ufs_cal_words,
        low: u32,
        high: u32,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_decode_cal_words(cal, low, high) })
    }

    pub fn read_cal_words(cal: &mut raw::sunxi_ufs_cal_words) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_get_cal_words(cal) })
    }

    pub fn initialize_variant(variant: &mut raw::sunxi_ufs_variant) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_ufs_variant_init(variant) })
    }

    pub fn configure(variant: &raw::sunxi_ufs_variant) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_ufs_configure(variant) })
    }

    pub fn enable() -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_ufs_enable() })
    }

    pub fn disable() {
        unsafe { raw::sunxi_ufs_disable() };
    }

    pub fn prepare() -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_ufs_prepare() })
    }

    pub fn device_reset() {
        unsafe { raw::sunxi_ufs_device_reset() };
    }

    pub fn ref_clock_frequency() -> DriverResult<u32> {
        let mut value = 0;
        syterkit_lib::status(unsafe { raw::sunxi_ufs_get_ref_clk_freq(&mut value) })?;
        Ok(value)
    }

    pub fn hs_rate() -> DriverResult<u32> {
        let mut value = 0;
        syterkit_lib::status(unsafe { raw::sunxi_ufs_get_hs_rate(&mut value) })?;
        Ok(value)
    }

    pub fn link_startup(host: &mut UfsHost<'_>) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_ufs_link_startup(host.raw) })
    }

    pub fn link_up(host: &mut UfsHost<'_>) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_ufs_link_up(host.raw) })
    }
}

#[cfg(test)]
mod tests {
    use super::{UfsDevice, UfsHost, UfsScsi};
    use core::ffi::{c_int, c_void};
    use syterkit_lib::BlockDevice;

    #[no_mangle]
    pub extern "C" fn ufs_init(
        device: *mut syterkit_ffi::raw::ufs_device,
        _config: *const syterkit_ffi::raw::ufshc_config,
    ) -> c_int {
        unsafe {
            (*device).initialized = 1;
        }
        0
    }
    #[no_mangle]
    pub extern "C" fn ufs_init_lun(
        device: *mut syterkit_ffi::raw::ufs_device,
        _config: *const syterkit_ffi::raw::ufshc_config,
        _lun: u8,
    ) -> c_int {
        unsafe {
            (*device).initialized = 1;
        }
        0
    }
    #[no_mangle]
    pub extern "C" fn ufs_exit(_device: *mut syterkit_ffi::raw::ufs_device) {}
    #[no_mangle]
    pub extern "C" fn ufs_read(
        _device: *mut syterkit_ffi::raw::ufs_device,
        _lba: u64,
        blocks: u32,
        buffer: *mut c_void,
    ) -> c_int {
        unsafe {
            core::slice::from_raw_parts_mut(buffer.cast::<u8>(), blocks as usize * 4096).fill(0x66);
        }
        0
    }
    #[no_mangle]
    pub extern "C" fn ufs_write(
        _device: *mut syterkit_ffi::raw::ufs_device,
        _lba: u64,
        _blocks: u32,
        _buffer: *const c_void,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn ufs_capacity(_device: *const syterkit_ffi::raw::ufs_device) -> u64 {
        0x1000
    }
    #[no_mangle]
    pub extern "C" fn ufs_block_size(_device: *const syterkit_ffi::raw::ufs_device) -> u32 {
        4096
    }
    #[no_mangle]
    pub extern "C" fn ufs_manufacturer_id(_device: *const syterkit_ffi::raw::ufs_device) -> u16 {
        0x1ce
    }
    #[no_mangle]
    pub extern "C" fn ufs_blk_read(
        _device: *mut syterkit_ffi::raw::ufs_device,
        _buffer: *mut c_void,
        _lba: u64,
        blocks: u32,
    ) -> u32 {
        blocks
    }
    #[no_mangle]
    pub extern "C" fn ufs_blk_write(
        _device: *mut syterkit_ffi::raw::ufs_device,
        _buffer: *const c_void,
        _lba: u64,
        blocks: u32,
    ) -> u32 {
        blocks
    }

    #[no_mangle]
    pub extern "C" fn ufshc_init(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _config: *const syterkit_ffi::raw::ufshc_config,
    ) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn ufshc_exit(_host: *mut syterkit_ffi::raw::ufshc_host) {}

    #[no_mangle]
    pub extern "C" fn ufshc_exec(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _request: *mut syterkit_ffi::raw::ufshc_request,
    ) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn ufshc_uic_command(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _args: *const syterkit_ffi::raw::ufshc_uic_cmd_args,
        result: *mut u32,
    ) -> c_int {
        unsafe { *result = 0x11 };
        0
    }

    #[no_mangle]
    pub extern "C" fn ufshc_dme_get(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _attribute: u32,
        value: *mut u32,
        _peer: syterkit_ffi::raw::bool_,
    ) -> c_int {
        unsafe { *value = 0x22 };
        0
    }

    #[no_mangle]
    pub extern "C" fn ufshc_dme_set(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _attribute: u32,
        _value: u32,
        _peer: syterkit_ffi::raw::bool_,
    ) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn ufshc_dme_get_sel(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _attribute: u32,
        _selector: u16,
        value: *mut u32,
        _peer: syterkit_ffi::raw::bool_,
    ) -> c_int {
        unsafe { *value = 0x33 };
        0
    }

    #[no_mangle]
    pub extern "C" fn ufshc_dme_set_sel(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _attribute: u32,
        _selector: u16,
        _value: u32,
        _peer: syterkit_ffi::raw::bool_,
    ) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn ufshc_get_max_power_mode(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        mode: *mut syterkit_ffi::raw::ufshc_power_mode,
    ) -> c_int {
        unsafe {
            (*mode).gear_tx = 4;
            (*mode).gear_rx = 4;
        }
        0
    }

    #[no_mangle]
    pub extern "C" fn ufshc_change_power_mode(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _mode: *const syterkit_ffi::raw::ufshc_power_mode,
    ) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn ufshc_nop(_host: *mut syterkit_ffi::raw::ufshc_host) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn ufshc_query_flag(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _id: u8,
        _set: syterkit_ffi::raw::bool_,
        value: *mut syterkit_ffi::raw::bool_,
    ) -> c_int {
        unsafe { *value = 1 };
        0
    }

    #[no_mangle]
    pub extern "C" fn ufshc_query_flag_op(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _id: u8,
        _opcode: u8,
        value: *mut syterkit_ffi::raw::bool_,
    ) -> c_int {
        unsafe { *value = 1 };
        0
    }

    #[no_mangle]
    pub extern "C" fn ufshc_query_attribute(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _id: u8,
        _index: u8,
        _selector: u8,
        value: *mut u32,
        _write: syterkit_ffi::raw::bool_,
    ) -> c_int {
        unsafe { *value = (*value).wrapping_add(1) };
        0
    }

    fn fill_descriptor(buffer: *mut c_void, length: usize, actual: *mut usize) -> c_int {
        unsafe {
            if !buffer.is_null() {
                core::slice::from_raw_parts_mut(buffer.cast::<u8>(), length).fill(0xa5);
            }
            *actual = length + 3;
        }
        0
    }

    #[no_mangle]
    pub extern "C" fn ufshc_query_descriptor(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _id: u8,
        _index: u8,
        _selector: u8,
        buffer: *mut c_void,
        length: usize,
        actual: *mut usize,
    ) -> c_int {
        fill_descriptor(buffer, length, actual)
    }

    #[no_mangle]
    pub extern "C" fn ufshc_query_descriptor_op(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _opcode: u8,
        _id: u8,
        _index: u8,
        _selector: u8,
        buffer: *mut c_void,
        length: usize,
        actual: *mut usize,
    ) -> c_int {
        fill_descriptor(buffer, length, actual)
    }

    #[no_mangle]
    pub extern "C" fn ufshc_task_request(
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _lun: u8,
        _function: u8,
        _task_id: u16,
        response: *mut u8,
    ) -> c_int {
        unsafe { *response = 0x44 };
        0
    }

    #[no_mangle]
    pub extern "C" fn ufs_scsi_init(
        device: *mut syterkit_ffi::raw::ufs_scsi_device,
        _host: *mut syterkit_ffi::raw::ufshc_host,
        _lun: u8,
    ) -> c_int {
        unsafe {
            (*device).present = 1;
            (*device).block_size = 4096;
            (*device).block_count = 0x1000;
        }
        0
    }

    #[no_mangle]
    pub extern "C" fn ufs_scsi_exec(
        _device: *mut syterkit_ffi::raw::ufs_scsi_device,
        _cdb: *const u8,
        _cdb_len: u8,
        data: *mut c_void,
        data_len: usize,
        _write: syterkit_ffi::raw::bool_,
    ) -> c_int {
        unsafe {
            if !data.is_null() {
                core::slice::from_raw_parts_mut(data.cast::<u8>(), data_len).fill(0x5c);
            }
        }
        0
    }

    #[no_mangle]
    pub extern "C" fn ufs_scsi_test_unit_ready(
        _device: *mut syterkit_ffi::raw::ufs_scsi_device,
    ) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn ufs_scsi_request_sense(
        _device: *mut syterkit_ffi::raw::ufs_scsi_device,
    ) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn ufs_scsi_read(
        _device: *mut syterkit_ffi::raw::ufs_scsi_device,
        _lba: u64,
        _blocks: u32,
        _buffer: *mut c_void,
    ) -> c_int {
        0
    }

    #[no_mangle]
    pub extern "C" fn ufs_scsi_write(
        _device: *mut syterkit_ffi::raw::ufs_scsi_device,
        _lba: u64,
        _blocks: u32,
        _buffer: *const c_void,
    ) -> c_int {
        0
    }

    #[test]
    fn ufs_device_wraps_lifecycle_and_block_io() {
        let config = syterkit_ffi::raw::ufshc_config {
            base: 0x1000,
            timeout_us: 1000,
        };
        let mut raw_device: syterkit_ffi::raw::ufs_device = unsafe { core::mem::zeroed() };
        let mut device = unsafe { UfsDevice::from_raw(&mut raw_device) };
        device.initialize(&config).unwrap();
        let mut buffer = [0; 4096];
        device.read(0, 1, &mut buffer).unwrap();
        assert_eq!(buffer, [0x66; 4096]);
        device.write(0, 1, &buffer).unwrap();
        assert_eq!(device.capacity(), 0x1000);
        assert_eq!(device.block_size(), 4096);
        assert_eq!(device.manufacturer_id(), 0x1ce);
        let mut blocks_buffer = [0; 8192];
        assert_eq!(device.read_blocks(0, 2, &mut blocks_buffer), 2);
        assert_eq!(device.write_blocks(0, 2, &blocks_buffer), 2);
        assert_eq!(
            BlockDevice::read_blocks(&mut device, 0, &mut blocks_buffer),
            Ok(2)
        );
        assert_eq!(
            BlockDevice::write_blocks(&mut device, 0, &blocks_buffer),
            Ok(2)
        );
        assert_eq!(device.read(0, 2, &mut buffer), Err(-1));
        assert_eq!(device.initialize_lun(&config, 0x80), Err(-1));
        device.exit();
    }

    #[test]
    fn ufs_host_and_scsi_wrappers_cover_query_and_cdb_paths() {
        let config = syterkit_ffi::raw::ufshc_config {
            base: 0x2000,
            timeout_us: 1000,
        };
        let mut raw_host: syterkit_ffi::raw::ufshc_host = unsafe { core::mem::zeroed() };
        let mut host = unsafe { UfsHost::from_raw(&mut raw_host) };
        host.initialize(&config).unwrap();
        let mut request: syterkit_ffi::raw::ufshc_request = unsafe { core::mem::zeroed() };
        host.execute(&mut request).unwrap();
        let args: syterkit_ffi::raw::ufshc_uic_cmd_args = unsafe { core::mem::zeroed() };
        assert_eq!(host.uic_command(&args), Ok(0x11));
        assert_eq!(host.dme_get(1, false), Ok(0x22));
        host.dme_set(1, 2, true).unwrap();
        assert_eq!(host.dme_get_selected(1, 2, false), Ok(0x33));
        host.dme_set_selected(1, 2, 3, true).unwrap();
        assert_eq!(host.maximum_power_mode().unwrap().gear_tx, 4);
        host.change_power_mode(&unsafe { core::mem::zeroed() })
            .unwrap();
        host.nop().unwrap();
        assert!(host.query_flag(1, false).unwrap());
        assert!(host.query_flag_opcode(1, 8).unwrap());
        assert_eq!(host.query_attribute(1, 0, 0, 9, false), Ok(10));
        let mut descriptor = [0u8; 4];
        assert_eq!(host.query_descriptor(1, 0, 0, &mut descriptor), Ok(4));
        assert_eq!(descriptor, [0xa5; 4]);
        assert_eq!(
            host.query_descriptor_opcode(6, 1, 0, 0, &mut descriptor),
            Ok(4)
        );
        assert_eq!(host.task_request(0, 1, 2), Ok(0x44));

        let mut raw_scsi: syterkit_ffi::raw::ufs_scsi_device = unsafe { core::mem::zeroed() };
        let mut scsi = unsafe { UfsScsi::from_raw(&mut raw_scsi) };
        scsi.initialize(&mut host, 0).unwrap();
        let mut data = [0u8; 4096];
        scsi.execute(&[0x28, 0, 0], Some(&mut data), false).unwrap();
        assert_eq!(&data[..4], [0x5c; 4]);
        scsi.test_unit_ready().unwrap();
        scsi.request_sense().unwrap();
        scsi.read(0, 1, &mut data).unwrap();
        scsi.write(0, 1, &data).unwrap();
        assert_eq!(scsi.block_size(), 4096);
        assert_eq!(scsi.capacity(), 0x1000);
        assert_eq!(BlockDevice::read_blocks(&mut scsi, 0, &mut data), Ok(1));
        assert_eq!(BlockDevice::write_blocks(&mut scsi, 0, &data), Ok(1));
        assert_eq!(scsi.read(0, 2, &mut data), Err(-1));
        host.exit();
    }
}
