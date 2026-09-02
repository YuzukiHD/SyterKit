// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::{DriverResult, INVALID_ARGUMENT};

/// Borrowed wrapper for an SPI NOR flash connected to the regular SPI driver.
pub struct SpiNor<'a> {
    raw: &'a mut raw::spi_nor_t,
}

impl<'a> SpiNor<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::spi_nor_t) -> Self {
        Self { raw }
    }

    pub fn detect(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::spi_nor_detect(self.raw) })
    }

    pub fn block_size(&self) -> usize {
        self.raw.info.blksz as usize
    }

    pub fn capacity(&self) -> usize {
        self.raw.info.capacity as usize
    }

    pub fn read(&mut self, address: u32, buffer: &mut [u8]) -> DriverResult<u32> {
        read_result(unsafe {
            raw::spi_nor_read(
                self.raw,
                buffer.as_mut_ptr(),
                address,
                buffer.len().try_into().map_err(|_| INVALID_ARGUMENT)?,
            )
        })
    }

    pub fn read_blocks(&mut self, block: u32, count: u32, buffer: &mut [u8]) -> u32 {
        if syterkit_lib::checked_block_bytes(self.block_size(), u64::from(count), buffer.len())
            .is_err()
        {
            return 0;
        }
        unsafe { raw::spi_nor_read_block(self.raw, buffer.as_mut_ptr(), block, count) }
    }
}

/// Borrowed wrapper for an SPI NAND flash connected to the regular SPI driver.
pub struct SpiNand<'a> {
    raw: &'a mut raw::spi_nand_t,
}

impl<'a> SpiNand<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::spi_nand_t) -> Self {
        Self { raw }
    }

    pub fn detect(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::spi_nand_detect(self.raw) })
    }

    pub fn read(&mut self, address: u32, buffer: &mut [u8]) -> DriverResult<u32> {
        read_result(unsafe {
            raw::spi_nand_read(
                self.raw,
                buffer.as_mut_ptr(),
                address,
                buffer.len().try_into().map_err(|_| INVALID_ARGUMENT)?,
            )
        })
    }
}

/// Borrowed wrapper for a NOR flash connected to the SPIF controller.
pub struct SpifNor<'a> {
    raw: &'a mut raw::spif_nor_t,
}

impl<'a> SpifNor<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::spif_nor_t) -> Self {
        Self { raw }
    }

    pub fn detect(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::spif_nor_detect(self.raw) })
    }

    pub fn block_size(&self) -> usize {
        self.raw.info.blksz as usize
    }

    pub fn capacity(&self) -> usize {
        self.raw.info.capacity as usize
    }

    pub fn read(&mut self, address: u32, buffer: &mut [u8]) -> DriverResult<u32> {
        read_result(unsafe {
            raw::spif_nor_read(
                self.raw,
                buffer.as_mut_ptr(),
                address,
                buffer.len().try_into().map_err(|_| INVALID_ARGUMENT)?,
            )
        })
    }

    pub fn read_blocks(&mut self, block: u32, count: u32, buffer: &mut [u8]) -> u32 {
        if syterkit_lib::checked_block_bytes(self.block_size(), u64::from(count), buffer.len())
            .is_err()
        {
            return 0;
        }
        unsafe { raw::spif_nor_read_block(self.raw, buffer.as_mut_ptr(), block, count) }
    }
}

fn read_result(value: u32) -> DriverResult<u32> {
    if value == 0 || value == u32::MAX {
        Err(-1)
    } else {
        Ok(value)
    }
}

#[cfg(test)]
mod tests {
    use super::{read_result, SpiNand, SpiNor, SpifNor};
    use core::ffi::c_int;

    #[no_mangle]
    pub extern "C" fn spi_nor_detect(_nor: *mut syterkit_ffi::raw::spi_nor_t) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn spi_nor_read(
        _nor: *mut syterkit_ffi::raw::spi_nor_t,
        buffer: *mut u8,
        _address: u32,
        length: u32,
    ) -> u32 {
        unsafe {
            core::slice::from_raw_parts_mut(buffer, length as usize).fill(0xa1);
        }
        length
    }
    #[no_mangle]
    pub extern "C" fn spi_nor_read_block(
        _nor: *mut syterkit_ffi::raw::spi_nor_t,
        _buffer: *mut u8,
        _block: u32,
        count: u32,
    ) -> u32 {
        count
    }
    #[no_mangle]
    pub extern "C" fn spi_nand_detect(_nand: *mut syterkit_ffi::raw::spi_nand_t) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn spi_nand_read(
        _nand: *mut syterkit_ffi::raw::spi_nand_t,
        buffer: *mut u8,
        _address: u32,
        length: u32,
    ) -> u32 {
        unsafe {
            core::slice::from_raw_parts_mut(buffer, length as usize).fill(0xb2);
        }
        length
    }
    #[no_mangle]
    pub extern "C" fn spif_nor_detect(_nor: *mut syterkit_ffi::raw::spif_nor_t) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn spif_nor_read(
        _nor: *mut syterkit_ffi::raw::spif_nor_t,
        buffer: *mut u8,
        _address: u32,
        length: u32,
    ) -> u32 {
        unsafe {
            core::slice::from_raw_parts_mut(buffer, length as usize).fill(0xc3);
        }
        length
    }
    #[no_mangle]
    pub extern "C" fn spif_nor_read_block(
        _nor: *mut syterkit_ffi::raw::spif_nor_t,
        _buffer: *mut u8,
        _block: u32,
        count: u32,
    ) -> u32 {
        count
    }

    #[test]
    fn mtd_wrappers_dispatch_detection_and_reads() {
        let mut raw_nor: syterkit_ffi::raw::spi_nor_t = unsafe { core::mem::zeroed() };
        raw_nor.info.blksz = 4;
        raw_nor.info.capacity = 16;
        let mut nor = unsafe { SpiNor::from_raw(&mut raw_nor) };
        nor.detect().unwrap();
        let mut buffer = [0; 4];
        assert_eq!(nor.read(0x100, &mut buffer), Ok(4));
        assert_eq!(buffer, [0xa1; 4]);
        let mut block_buffer = [0; 12];
        assert_eq!(nor.read_blocks(1, 3, &mut block_buffer), 3);

        let mut raw_nand: syterkit_ffi::raw::spi_nand_t = unsafe { core::mem::zeroed() };
        let mut nand = unsafe { SpiNand::from_raw(&mut raw_nand) };
        nand.detect().unwrap();
        assert_eq!(nand.read(0, &mut buffer), Ok(4));
        assert_eq!(buffer, [0xb2; 4]);

        let mut raw_spif: syterkit_ffi::raw::spif_nor_t = unsafe { core::mem::zeroed() };
        raw_spif.info.blksz = 4;
        raw_spif.info.capacity = 16;
        let mut spif = unsafe { SpifNor::from_raw(&mut raw_spif) };
        spif.detect().unwrap();
        assert_eq!(spif.read(0, &mut buffer), Ok(4));
        assert_eq!(buffer, [0xc3; 4]);
        assert_eq!(spif.read_blocks(0, 2, &mut block_buffer), 2);

        assert_eq!(nor.block_size(), 4);
        assert_eq!(nor.capacity(), 16);
        assert_eq!(nor.read_blocks(1, 2, &mut buffer), 0);

        assert_eq!(spif.block_size(), 4);
        assert_eq!(spif.capacity(), 16);
        assert_eq!(spif.read_blocks(0, 2, &mut buffer), 0);
    }

    #[test]
    fn mtd_read_rejects_zero_byte_transfers() {
        assert_eq!(read_result(0), Err(-1));
        assert_eq!(read_result(u32::MAX), Err(-1));
    }
}
