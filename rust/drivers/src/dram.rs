// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;

/// Borrowed access to the board-populated DRAM descriptor.
pub struct Dram<'a> {
    raw: &'a mut raw::sunxi_dram_t,
}

impl<'a> Dram<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::sunxi_dram_t) -> Self {
        Self { raw }
    }

    /// Initialize DRAM and return the size reported by the C implementation.
    pub fn initialize(&mut self) -> u32 {
        unsafe { raw::sunxi_dram_init(self.raw) }
    }

    pub fn size_bytes(&self) -> u32 {
        unsafe { raw::sunxi_get_dram_size(self.raw) }
    }

    pub const fn as_raw(&self) -> &raw::sunxi_dram_t {
        self.raw
    }
}

#[cfg(test)]
mod tests {
    use super::Dram;
    use core::sync::atomic::{AtomicU32, Ordering};

    static DRAM_SIZE: AtomicU32 = AtomicU32::new(0);
    static DRAM_INIT_SIZE: AtomicU32 = AtomicU32::new(0);

    #[no_mangle]
    pub extern "C" fn sunxi_get_dram_size(_dram: *const syterkit_ffi::raw::sunxi_dram_t) -> u32 {
        DRAM_SIZE.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_dram_init(_dram: *mut syterkit_ffi::raw::sunxi_dram_t) -> u32 {
        DRAM_INIT_SIZE.load(Ordering::Relaxed)
    }

    #[test]
    fn dram_forwards_size_and_initialization_result() {
        DRAM_SIZE.store(0x4000_0000, Ordering::Relaxed);
        DRAM_INIT_SIZE.store(0x4000_0000, Ordering::Relaxed);
        let mut raw_dram: syterkit_ffi::raw::sunxi_dram_t = unsafe { core::mem::zeroed() };
        let mut dram = unsafe { Dram::from_raw(&mut raw_dram) };
        assert_eq!(dram.initialize(), 0x4000_0000);
        assert_eq!(dram.size_bytes(), 0x4000_0000);
    }
}
