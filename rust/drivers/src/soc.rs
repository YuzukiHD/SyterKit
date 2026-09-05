// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::DriverResult;

/// SoC identification operations that do not require a mutable descriptor.
pub struct Soc;

impl Soc {
    pub const fn new() -> Self {
        Self
    }

    pub fn platform_id(&self) -> u32 {
        unsafe { raw::sunxi_soc_platform_id() }
    }

    /// Initialize the board's Network System Interconnect when present.
    pub fn initialize_nsi(&self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_nsi_init() })
    }
}

pub const SOC: Soc = Soc::new();

#[cfg(test)]
mod tests {
    use super::SOC;

    #[no_mangle]
    pub extern "C" fn sunxi_soc_platform_id() -> u32 {
        0x1234_5678
    }

    #[no_mangle]
    pub extern "C" fn sunxi_nsi_init() -> i32 {
        0
    }

    #[test]
    fn soc_reads_platform_id() {
        assert_eq!(SOC.platform_id(), 0x1234_5678);
        SOC.initialize_nsi().unwrap();
    }
}
