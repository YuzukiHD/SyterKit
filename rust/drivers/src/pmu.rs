// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::{c_name, DriverResult, INVALID_ARGUMENT};

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum PmuModel {
    Axp1530,
    Axp2101,
    Axp2202,
    Axp333,
    Axp8191,
}

/// Borrowed wrapper for one AXP PMU instance.
pub struct Pmu<'a> {
    raw: &'a mut raw::axp_pmu_t,
}

impl<'a> Pmu<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::axp_pmu_t) -> Self {
        Self { raw }
    }

    pub fn configure(&mut self, model: PmuModel, i2c: &mut raw::sunxi_i2c_t) -> DriverResult<()> {
        let result = unsafe {
            match model {
                PmuModel::Axp1530 => raw::pmu_axp1530_config(self.raw, i2c),
                PmuModel::Axp2101 => raw::pmu_axp2101_config(self.raw, i2c),
                PmuModel::Axp2202 => raw::pmu_axp2202_config(self.raw, i2c),
                PmuModel::Axp333 => raw::pmu_axp333_config(self.raw, i2c),
                PmuModel::Axp8191 => raw::pmu_axp8191_config(self.raw, i2c),
            }
        };
        syterkit_lib::status(result)
    }

    pub fn initialize(&mut self, model: PmuModel) -> DriverResult<()> {
        let result = unsafe {
            match model {
                PmuModel::Axp1530 => raw::pmu_axp1530_init(self.raw),
                PmuModel::Axp2101 => raw::pmu_axp2101_init(self.raw),
                PmuModel::Axp2202 => raw::pmu_axp2202_init(self.raw),
                PmuModel::Axp333 => raw::pmu_axp333_init(self.raw),
                PmuModel::Axp8191 => raw::pmu_axp8191_init(self.raw),
            }
        };
        syterkit_lib::status(result)
    }

    /// Names must be NUL-terminated because the C API receives `char *`.
    pub fn get_voltage(&mut self, model: PmuModel, name: &[u8]) -> DriverResult<i32> {
        let name = c_name(name)?;
        let value = unsafe {
            match model {
                PmuModel::Axp1530 => raw::pmu_axp1530_get_vol(self.raw, name),
                PmuModel::Axp2101 => raw::pmu_axp2101_get_vol(self.raw, name),
                PmuModel::Axp2202 => raw::pmu_axp2202_get_vol(self.raw, name),
                PmuModel::Axp333 => raw::pmu_axp333_get_vol(self.raw, name),
                PmuModel::Axp8191 => raw::pmu_axp8191_get_vol(self.raw, name),
            }
        };
        if value < 0 {
            Err(value)
        } else {
            Ok(value)
        }
    }

    pub fn set_voltage(
        &mut self,
        model: PmuModel,
        name: &[u8],
        voltage: i32,
        enabled: bool,
    ) -> DriverResult<()> {
        let name = c_name(name)?;
        let enabled = i32::from(enabled);
        let result = unsafe {
            match model {
                PmuModel::Axp1530 => raw::pmu_axp1530_set_vol(self.raw, name, voltage, enabled),
                PmuModel::Axp2101 => raw::pmu_axp2101_set_vol(self.raw, name, voltage, enabled),
                PmuModel::Axp2202 => raw::pmu_axp2202_set_vol(self.raw, name, voltage, enabled),
                PmuModel::Axp333 => raw::pmu_axp333_set_vol(self.raw, name, voltage, enabled),
                PmuModel::Axp8191 => raw::pmu_axp8191_set_vol(self.raw, name, voltage, enabled),
            }
        };
        syterkit_lib::status(result)
    }

    pub fn set_voltage_with_table(
        &mut self,
        name: &[u8],
        voltage: i32,
        enabled: bool,
        table: &mut [raw::axp_contrl_info],
    ) -> DriverResult<()> {
        let name = c_name(name)?;
        let table_len = u8::try_from(table.len()).map_err(|_| INVALID_ARGUMENT)?;
        syterkit_lib::status(unsafe {
            raw::axp_set_vol(
                self.raw,
                name,
                voltage,
                i32::from(enabled),
                table.as_mut_ptr(),
                table_len,
            )
        })
    }

    pub fn get_voltage_with_table(
        &mut self,
        name: &[u8],
        table: &mut [raw::axp_contrl_info],
    ) -> DriverResult<i32> {
        let name = c_name(name)?;
        let table_len = u8::try_from(table.len()).map_err(|_| INVALID_ARGUMENT)?;
        let value = unsafe { raw::axp_get_vol(self.raw, name, table.as_mut_ptr(), table_len) };
        if value < 0 {
            Err(value)
        } else {
            Ok(value)
        }
    }

    pub fn dump(&mut self, model: PmuModel) {
        unsafe {
            match model {
                PmuModel::Axp1530 => raw::pmu_axp1530_dump(self.raw),
                PmuModel::Axp2101 => raw::pmu_axp2101_dump(self.raw),
                PmuModel::Axp2202 => raw::pmu_axp2202_dump(self.raw),
                PmuModel::Axp333 => raw::pmu_axp333_dump(self.raw),
                PmuModel::Axp8191 => raw::pmu_axp8191_dump(self.raw),
            }
        }
    }

    pub fn set_axp1530_dual_phase(&mut self, model: PmuModel) -> DriverResult<()> {
        if model != PmuModel::Axp1530 {
            return Err(INVALID_ARGUMENT);
        }
        syterkit_lib::status(unsafe { raw::pmu_axp1530_set_dual_phase(self.raw) })
    }
}

#[cfg(test)]
mod tests {
    use super::{Pmu, PmuModel};
    use core::ffi::{c_char, c_int};
    use core::sync::atomic::{AtomicI32, Ordering};

    static PMU_VALUE: AtomicI32 = AtomicI32::new(0);

    #[no_mangle]
    pub extern "C" fn axp_set_vol(
        _pmu: *mut syterkit_ffi::raw::axp_pmu_t,
        _name: *mut c_char,
        value: c_int,
        _enabled: c_int,
        _table: *mut syterkit_ffi::raw::axp_contrl_info,
        _table_len: u8,
    ) -> c_int {
        PMU_VALUE.store(value, Ordering::Relaxed);
        0
    }

    #[no_mangle]
    pub extern "C" fn axp_get_vol(
        _pmu: *mut syterkit_ffi::raw::axp_pmu_t,
        _name: *mut c_char,
        _table: *mut syterkit_ffi::raw::axp_contrl_info,
        _table_len: u8,
    ) -> c_int {
        PMU_VALUE.load(Ordering::Relaxed)
    }

    macro_rules! mock_pmu {
        ($config:ident, $init:ident, $get:ident, $set:ident, $dump:ident) => {
            #[no_mangle]
            pub extern "C" fn $config(
                _pmu: *mut syterkit_ffi::raw::axp_pmu_t,
                _i2c: *mut syterkit_ffi::raw::sunxi_i2c_t,
            ) -> c_int {
                0
            }
            #[no_mangle]
            pub extern "C" fn $init(_pmu: *mut syterkit_ffi::raw::axp_pmu_t) -> c_int {
                0
            }
            #[no_mangle]
            pub extern "C" fn $get(
                _pmu: *mut syterkit_ffi::raw::axp_pmu_t,
                _name: *mut c_char,
            ) -> c_int {
                PMU_VALUE.load(Ordering::Relaxed)
            }
            #[no_mangle]
            pub extern "C" fn $set(
                _pmu: *mut syterkit_ffi::raw::axp_pmu_t,
                _name: *mut c_char,
                value: c_int,
                _enabled: c_int,
            ) -> c_int {
                PMU_VALUE.store(value, Ordering::Relaxed);
                0
            }
            #[no_mangle]
            pub extern "C" fn $dump(_pmu: *mut syterkit_ffi::raw::axp_pmu_t) {}
        };
    }

    mock_pmu!(
        pmu_axp1530_config,
        pmu_axp1530_init,
        pmu_axp1530_get_vol,
        pmu_axp1530_set_vol,
        pmu_axp1530_dump
    );
    mock_pmu!(
        pmu_axp2101_config,
        pmu_axp2101_init,
        pmu_axp2101_get_vol,
        pmu_axp2101_set_vol,
        pmu_axp2101_dump
    );
    mock_pmu!(
        pmu_axp2202_config,
        pmu_axp2202_init,
        pmu_axp2202_get_vol,
        pmu_axp2202_set_vol,
        pmu_axp2202_dump
    );
    mock_pmu!(
        pmu_axp333_config,
        pmu_axp333_init,
        pmu_axp333_get_vol,
        pmu_axp333_set_vol,
        pmu_axp333_dump
    );
    mock_pmu!(
        pmu_axp8191_config,
        pmu_axp8191_init,
        pmu_axp8191_get_vol,
        pmu_axp8191_set_vol,
        pmu_axp8191_dump
    );

    #[no_mangle]
    pub extern "C" fn pmu_axp1530_set_dual_phase(_pmu: *mut syterkit_ffi::raw::axp_pmu_t) -> c_int {
        0
    }

    #[test]
    fn pmu_validates_names_and_dispatches_model_api() {
        PMU_VALUE.store(0, Ordering::Relaxed);
        let mut raw_pmu: syterkit_ffi::raw::axp_pmu_t = unsafe { core::mem::zeroed() };
        let mut raw_i2c: syterkit_ffi::raw::sunxi_i2c_t = unsafe { core::mem::zeroed() };
        let mut pmu = unsafe { Pmu::from_raw(&mut raw_pmu) };
        assert_eq!(pmu.configure(PmuModel::Axp2101, &mut raw_i2c), Ok(()));
        pmu.initialize(PmuModel::Axp2101).unwrap();
        pmu.set_voltage(PmuModel::Axp2101, b"vdd\0", 900_000, true)
            .unwrap();
        assert_eq!(pmu.get_voltage(PmuModel::Axp2101, b"vdd\0"), Ok(900_000));
        assert_eq!(pmu.get_voltage(PmuModel::Axp2101, b"vdd"), Err(-1));
        assert_eq!(pmu.set_axp1530_dual_phase(PmuModel::Axp2101), Err(-1));

        let mut table = [unsafe { core::mem::zeroed::<syterkit_ffi::raw::axp_contrl_info>() }];
        pmu.set_voltage_with_table(b"vdd\0", 1_000_000, true, &mut table)
            .unwrap();
        assert_eq!(
            pmu.get_voltage_with_table(b"vdd\0", &mut table),
            Ok(1_000_000)
        );
    }
}
