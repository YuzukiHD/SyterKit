// SPDX-License-Identifier: GPL-2.0+

#![no_std]

#[cfg(test)]
extern crate std;

use core::ffi::{c_char, c_void};

use syterkit_ffi::raw;

/// Result type used by Rust wrappers around C driver status codes.
pub type DriverResult<T> = core::result::Result<T, i32>;

/// Common invalid-argument status used by SyterKit's C drivers.
pub const INVALID_ARGUMENT: i32 = -1;

/// Validate a byte slice used as a C string without allocating in `no_std`.
///
/// The caller retains ownership of the bytes for the duration of the C call.
#[inline]
pub fn c_name(name: &[u8]) -> DriverResult<*mut c_char> {
    if name.last() == Some(&0) {
        Ok(name.as_ptr() as *mut c_char)
    } else {
        Err(INVALID_ARGUMENT)
    }
}

/// Convert the common SyterKit C status convention to a Rust result.
#[inline]
pub fn status(code: i32) -> DriverResult<()> {
    if code == 0 {
        Ok(())
    } else {
        Err(code)
    }
}

/// A block-oriented device exposed through the Rust component layer.
///
/// The C drivers use different argument orders and return conventions. This
/// trait keeps those details below the component boundary while retaining the
/// device's native logical block size.
pub trait BlockDevice {
    fn block_size(&self) -> usize;

    fn read_blocks(&mut self, lba: u64, buffer: &mut [u8]) -> DriverResult<usize>;

    fn write_blocks(&mut self, lba: u64, buffer: &[u8]) -> DriverResult<usize>;
}

/// Validate a byte buffer for a request containing `blocks` logical blocks.
///
/// Extra bytes are allowed so callers can pass a larger staging buffer, but
/// the requested byte range must fit in it. Zero-sized requests are rejected
/// because the C block APIs do not have a consistent zero-request contract.
#[inline]
pub fn checked_block_bytes(
    block_size: usize,
    blocks: u64,
    buffer_len: usize,
) -> DriverResult<usize> {
    if block_size == 0 || blocks == 0 {
        return Err(INVALID_ARGUMENT);
    }
    let blocks = usize::try_from(blocks).map_err(|_| INVALID_ARGUMENT)?;
    let required = block_size.checked_mul(blocks).ok_or(INVALID_ARGUMENT)?;
    if buffer_len < required {
        Err(INVALID_ARGUMENT)
    } else {
        Ok(required)
    }
}

/// Convert a byte buffer length into a non-zero 32-bit block count.
#[inline]
pub fn checked_block_count(block_size: usize, buffer_len: usize) -> DriverResult<u32> {
    if block_size == 0 || buffer_len == 0 || buffer_len % block_size != 0 {
        return Err(INVALID_ARGUMENT);
    }
    u32::try_from(buffer_len / block_size).map_err(|_| INVALID_ARGUMENT)
}

/// Convert a legacy block-count return value into a Rust result.
///
/// SyterKit's block helpers return zero on failure and the number of blocks on
/// success. Partial transfers are preserved, while impossible over-reports
/// are rejected at the component boundary.
#[inline]
pub fn transferred_blocks(transferred: u32, requested: u32) -> DriverResult<usize> {
    if requested == 0 || transferred == 0 || transferred > requested {
        Err(INVALID_ARGUMENT)
    } else {
        Ok(transferred as usize)
    }
}

/// FatFs physical-drive adapter backed by the existing C disk I/O glue.
pub struct Disk<'a> {
    device: &'a mut raw::sdmmc_pdata_t,
    drive: u8,
}

impl<'a> Disk<'a> {
    /// Register a caller-owned SD/MMC device with the C disk layer.
    ///
    /// The C layer stores the device pointer, so the device storage must
    /// remain valid for as long as this drive is registered.
    pub unsafe fn attach(drive: u8, device: &'a mut raw::sdmmc_pdata_t) -> DriverResult<Self> {
        if u32::from(drive) >= raw::FF_VOLUMES {
            return Err(INVALID_ARGUMENT);
        }
        let result = unsafe { raw::disk_set_device(drive, device) };
        if result == raw::DRESULT_RES_OK {
            Ok(Self { device, drive })
        } else {
            Err(result as i32)
        }
    }

    pub const fn drive(&self) -> u8 {
        self.drive
    }

    pub fn status(&self) -> u8 {
        unsafe { raw::disk_status(self.drive) }
    }

    pub fn is_ready(&self) -> bool {
        unsafe { raw::disk_initialize(self.drive) == 0 }
    }

    pub fn read(&mut self, sector: u64, buffer: &mut [u8]) -> DriverResult<()> {
        let count = sector_count(buffer)?;
        let sector = raw::LBA_t::try_from(sector).map_err(|_| INVALID_ARGUMENT)?;
        let result = unsafe { raw::disk_read(self.drive, buffer.as_mut_ptr(), sector, count) };
        disk_result(result)
    }

    pub fn write(&mut self, sector: u64, buffer: &[u8]) -> DriverResult<()> {
        let count = sector_count(buffer)?;
        let sector = raw::LBA_t::try_from(sector).map_err(|_| INVALID_ARGUMENT)?;
        let result = unsafe { raw::disk_write(self.drive, buffer.as_ptr(), sector, count) };
        disk_result(result)
    }

    pub fn ioctl(&mut self, command: u8, buffer: Option<&mut [u8]>) -> DriverResult<()> {
        let pointer: *mut u8 = buffer.map_or(core::ptr::null_mut(), |buffer| buffer.as_mut_ptr());
        let result = unsafe { raw::disk_ioctl(self.drive, command, pointer.cast::<c_void>()) };
        disk_result(result)
    }

    pub fn sync(&mut self) -> DriverResult<()> {
        self.ioctl(raw::CTRL_SYNC as u8, None)
    }

    /// Expose the registered C device for driver-specific operations.
    pub const fn device(&self) -> &raw::sdmmc_pdata_t {
        self.device
    }
}

pub const FATFS_SECTOR_SIZE: usize = raw::FF_MIN_SS as usize;

fn sector_count(buffer: &[u8]) -> DriverResult<raw::UINT> {
    checked_block_count(FATFS_SECTOR_SIZE, buffer.len())
        .and_then(|count| raw::UINT::try_from(count).map_err(|_| INVALID_ARGUMENT))
}

fn disk_result(result: raw::DRESULT) -> DriverResult<()> {
    if result == raw::DRESULT_RES_OK {
        Ok(())
    } else {
        Err(result as i32)
    }
}

impl BlockDevice for Disk<'_> {
    fn block_size(&self) -> usize {
        FATFS_SECTOR_SIZE
    }

    fn read_blocks(&mut self, lba: u64, buffer: &mut [u8]) -> DriverResult<usize> {
        let count = checked_block_count(self.block_size(), buffer.len())?;
        let sector = raw::LBA_t::try_from(lba).map_err(|_| INVALID_ARGUMENT)?;
        let result = unsafe { raw::disk_read(self.drive, buffer.as_mut_ptr(), sector, count) };
        disk_result(result)?;
        Ok(count as usize)
    }

    fn write_blocks(&mut self, lba: u64, buffer: &[u8]) -> DriverResult<usize> {
        let count = checked_block_count(self.block_size(), buffer.len())?;
        let sector = raw::LBA_t::try_from(lba).map_err(|_| INVALID_ARGUMENT)?;
        let result = unsafe { raw::disk_write(self.drive, buffer.as_ptr(), sector, count) };
        disk_result(result)?;
        Ok(count as usize)
    }
}

#[cfg(test)]
mod tests {
    use super::{status, BlockDevice, Disk, FATFS_SECTOR_SIZE};
    use core::ffi::{c_uint, c_void};
    use core::sync::atomic::{AtomicI32, AtomicU32, Ordering};
    use std::sync::Mutex;

    use syterkit_ffi::raw;

    static TEST_LOCK: Mutex<()> = Mutex::new(());
    static DISK_SET_RESULT: AtomicI32 = AtomicI32::new(0);
    static DISK_STATUS: AtomicU32 = AtomicU32::new(0);
    static LAST_DRIVE: AtomicU32 = AtomicU32::new(u32::MAX);
    static LAST_SECTOR: AtomicU32 = AtomicU32::new(0);
    static LAST_COUNT: AtomicU32 = AtomicU32::new(0);
    static LAST_IOCTL: AtomicU32 = AtomicU32::new(u32::MAX);

    #[no_mangle]
    pub extern "C" fn disk_set_device(
        drive: u8,
        _device: *mut syterkit_ffi::raw::sdmmc_pdata,
    ) -> syterkit_ffi::raw::DRESULT {
        LAST_DRIVE.store(drive as u32, Ordering::Relaxed);
        DISK_SET_RESULT.load(Ordering::Relaxed) as syterkit_ffi::raw::DRESULT
    }

    #[no_mangle]
    pub extern "C" fn disk_status(drive: u8) -> syterkit_ffi::raw::DSTATUS {
        LAST_DRIVE.store(drive as u32, Ordering::Relaxed);
        DISK_STATUS.load(Ordering::Relaxed) as syterkit_ffi::raw::DSTATUS
    }

    #[no_mangle]
    pub extern "C" fn disk_initialize(drive: u8) -> syterkit_ffi::raw::DSTATUS {
        LAST_DRIVE.store(drive as u32, Ordering::Relaxed);
        DISK_STATUS.load(Ordering::Relaxed) as syterkit_ffi::raw::DSTATUS
    }

    #[no_mangle]
    pub extern "C" fn disk_read(
        drive: u8,
        buffer: *mut u8,
        sector: syterkit_ffi::raw::LBA_t,
        count: c_uint,
    ) -> syterkit_ffi::raw::DRESULT {
        LAST_DRIVE.store(drive as u32, Ordering::Relaxed);
        LAST_SECTOR.store(sector as u32, Ordering::Relaxed);
        LAST_COUNT.store(count, Ordering::Relaxed);
        unsafe {
            core::slice::from_raw_parts_mut(buffer, count as usize * FATFS_SECTOR_SIZE).fill(0xa5);
        }
        0
    }

    #[no_mangle]
    pub extern "C" fn disk_write(
        drive: u8,
        _buffer: *const u8,
        sector: syterkit_ffi::raw::LBA_t,
        count: c_uint,
    ) -> syterkit_ffi::raw::DRESULT {
        LAST_DRIVE.store(drive as u32, Ordering::Relaxed);
        LAST_SECTOR.store(sector as u32, Ordering::Relaxed);
        LAST_COUNT.store(count, Ordering::Relaxed);
        0
    }

    #[no_mangle]
    pub extern "C" fn disk_ioctl(
        drive: u8,
        command: u8,
        _buffer: *mut c_void,
    ) -> syterkit_ffi::raw::DRESULT {
        LAST_DRIVE.store(drive as u32, Ordering::Relaxed);
        LAST_IOCTL.store(command as u32, Ordering::Relaxed);
        0
    }

    #[test]
    fn status_preserves_c_success_and_error_codes() {
        assert_eq!(status(0), Ok(()));
        assert_eq!(status(-7), Err(-7));
    }

    #[test]
    fn disk_wraps_fatfs_block_io_and_checks_buffers() {
        let _guard = TEST_LOCK.lock().unwrap();
        DISK_SET_RESULT.store(0, Ordering::Relaxed);
        DISK_STATUS.store(0, Ordering::Relaxed);
        LAST_COUNT.store(0, Ordering::Relaxed);
        LAST_IOCTL.store(u32::MAX, Ordering::Relaxed);

        let mut device: syterkit_ffi::raw::sdmmc_pdata_t = unsafe { core::mem::zeroed() };
        let mut disk = unsafe { Disk::attach(0, &mut device) }.unwrap();
        assert_eq!(disk.drive(), 0);
        assert!(disk.is_ready());
        assert_eq!(disk.status(), 0);

        let mut read_buffer = [0u8; FATFS_SECTOR_SIZE * 2];
        disk.read(7, &mut read_buffer).unwrap();
        assert_eq!(read_buffer, [0xa5; FATFS_SECTOR_SIZE * 2]);
        assert_eq!(LAST_SECTOR.load(Ordering::Relaxed), 7);
        assert_eq!(LAST_COUNT.load(Ordering::Relaxed), 2);

        let write_buffer = [0x5a; FATFS_SECTOR_SIZE];
        disk.write(9, &write_buffer).unwrap();
        assert_eq!(LAST_SECTOR.load(Ordering::Relaxed), 9);
        assert_eq!(LAST_COUNT.load(Ordering::Relaxed), 1);
        disk.sync().unwrap();
        assert_eq!(LAST_IOCTL.load(Ordering::Relaxed), raw::CTRL_SYNC as u32);

        assert_eq!(disk.read(0, &mut [0; FATFS_SECTOR_SIZE - 1]), Err(-1));
        assert_eq!(disk.write(0, &[0; FATFS_SECTOR_SIZE + 1]), Err(-1));
        drop(disk);
        assert!(matches!(unsafe { Disk::attach(1, &mut device) }, Err(-1)));

        DISK_SET_RESULT.store(4, Ordering::Relaxed);
        assert!(matches!(unsafe { Disk::attach(0, &mut device) }, Err(4)));
    }

    #[test]
    fn block_device_returns_transferred_sector_count() {
        let _guard = TEST_LOCK.lock().unwrap();
        DISK_SET_RESULT.store(0, Ordering::Relaxed);
        let mut device: syterkit_ffi::raw::sdmmc_pdata_t = unsafe { core::mem::zeroed() };
        let mut disk = unsafe { Disk::attach(0, &mut device) }.unwrap();
        let mut buffer = [0u8; FATFS_SECTOR_SIZE * 2];
        assert_eq!(BlockDevice::read_blocks(&mut disk, 3, &mut buffer), Ok(2));
        assert_eq!(BlockDevice::write_blocks(&mut disk, 4, &buffer), Ok(2));
        assert_eq!(BlockDevice::read_blocks(&mut disk, 0, &mut [0; 1]), Err(-1));
    }
}
