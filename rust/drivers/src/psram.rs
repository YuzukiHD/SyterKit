// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;

/// Borrowed access to the board-populated PSRAM descriptor.
pub struct Psram<'a> {
    raw: &'a mut raw::sunxi_psram_t,
}

impl<'a> Psram<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::sunxi_psram_t) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self) -> u32 {
        unsafe { raw::sunxi_psram_init(self.raw) }
    }

    pub fn size_mb(&self) -> u32 {
        unsafe { raw::sunxi_get_psram_size(self.raw) }
    }

    pub const fn as_raw(&self) -> &raw::sunxi_psram_t {
        self.raw
    }
}

#[cfg(test)]
mod tests {
    use super::Psram;
    use core::sync::atomic::{AtomicU32, Ordering};

    static PSRAM_SIZE: AtomicU32 = AtomicU32::new(0);

    #[no_mangle]
    pub extern "C" fn sunxi_get_psram_size(_psram: *const syterkit_ffi::raw::sunxi_psram_t) -> u32 {
        PSRAM_SIZE.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_psram_init(_psram: *mut syterkit_ffi::raw::sunxi_psram_t) -> u32 {
        PSRAM_SIZE.load(Ordering::Relaxed)
    }

    #[test]
    fn psram_forwards_detected_size() {
        PSRAM_SIZE.store(8, Ordering::Relaxed);
        let mut raw_psram: syterkit_ffi::raw::sunxi_psram_t = unsafe { core::mem::zeroed() };
        let mut psram = unsafe { Psram::from_raw(&mut raw_psram) };
        assert_eq!(psram.initialize(), 8);
        assert_eq!(psram.size_mb(), 8);
    }
}
