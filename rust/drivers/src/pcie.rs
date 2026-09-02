// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;
use syterkit_lib::DriverResult;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum PcieMode {
    RootComplex,
    Endpoint,
}

impl PcieMode {
    fn into_raw(self) -> raw::pcie_mode {
        match self {
            Self::RootComplex => raw::pcie_mode_PCIE_MODE_RC,
            Self::Endpoint => raw::pcie_mode_PCIE_MODE_EP,
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum AtuType {
    Memory,
    Io,
    ConfigType0,
    ConfigType1,
}

impl AtuType {
    fn into_raw(self) -> raw::pcie_atu_type {
        match self {
            Self::Memory => raw::pcie_atu_type_PCIE_ATU_TYPE_MEM,
            Self::Io => raw::pcie_atu_type_PCIE_ATU_TYPE_IO,
            Self::ConfigType0 => raw::pcie_atu_type_PCIE_ATU_TYPE_CFG0,
            Self::ConfigType1 => raw::pcie_atu_type_PCIE_ATU_TYPE_CFG1,
        }
    }
}

/// Borrowed wrapper for the high-level PCIe runtime object.
pub struct Pcie<'a> {
    raw: &'a mut raw::pcie,
}

impl<'a> Pcie<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::pcie) -> Self {
        Self { raw }
    }

    pub fn default_config(mode: PcieMode) -> raw::pcie_config {
        let mut config: raw::pcie_config = unsafe { core::mem::zeroed() };
        unsafe { raw::pcie_config_sun55iw6(&mut config, mode.into_raw()) };
        config
    }

    pub fn platform_power_on(config: &raw::pcie_config) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_platform_power_on(config) })
    }

    pub fn initialize(&mut self, config: &raw::pcie_config) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_init(self.raw, config) })
    }

    pub fn initialize_dt(&mut self, node: i32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_init_dt(self.raw, node) })
    }

    pub fn wait_for_link(&mut self, timeout_us: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_wait_for_link(self.raw, timeout_us) })
    }

    pub fn exit(&mut self) {
        unsafe { raw::pcie_exit(self.raw) };
    }

    pub fn controller(&mut self) -> PcieController<'_> {
        unsafe { PcieController::from_raw(&mut self.raw.controller) }
    }

    pub fn rc_initialize(
        &mut self,
        config: &raw::pcie_config,
        rc_config: &raw::pcie_rc_config,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_rc_init(self.raw, config, rc_config) })
    }

    pub fn rc_setup(&mut self, config: &raw::pcie_rc_config) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_rc_setup(self.raw, config) })
    }

    pub fn default_rc_config() -> raw::pcie_rc_config {
        let mut config: raw::pcie_rc_config = unsafe { core::mem::zeroed() };
        unsafe { raw::pcie_rc_config_default(&mut config) };
        config
    }

    pub fn rc_initialize_dt(
        &mut self,
        node: i32,
        config: &raw::pcie_rc_config,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_rc_init_dt(self.raw, node, config) })
    }

    pub fn rc_start(&mut self, timeout_us: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_rc_start(self.raw, timeout_us) })
    }

    pub fn rc_stop(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_rc_stop(self.raw) })
    }

    pub fn rc_link_up(&mut self) -> bool {
        unsafe { raw::pcie_rc_link_up(self.raw) != 0 }
    }

    pub fn rc_read_config(&mut self, bdf: u32, offset: u32, size: u8) -> DriverResult<u32> {
        let mut value = 0;
        syterkit_lib::status(unsafe {
            raw::pcie_rc_read_config(self.raw, bdf, offset, size, &mut value)
        })?;
        Ok(value)
    }

    pub fn rc_write_config(
        &mut self,
        bdf: u32,
        offset: u32,
        size: u8,
        value: u32,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::pcie_rc_write_config(self.raw, bdf, offset, size, value)
        })
    }

    pub fn rc_program_outbound(
        &mut self,
        index: u8,
        kind: AtuType,
        cpu_addr: u64,
        pci_addr: u64,
        size: u64,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::pcie_rc_program_outbound(
                self.raw,
                index,
                kind.into_raw(),
                cpu_addr,
                pci_addr,
                size,
            )
        })
    }

    pub fn ep_initialize(&mut self, config: &raw::pcie_config) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_ep_init(self.raw, config) })
    }

    pub fn ep_initialize_dt(&mut self, node: i32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_ep_init_dt(self.raw, node) })
    }

    pub fn ep_write_header(
        &mut self,
        function: u8,
        header: &raw::pcie_ep_header,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_ep_write_header(self.raw, function, header) })
    }

    pub fn ep_set_bar(&mut self, function: u8, bar: &raw::pcie_ep_bar) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_ep_set_bar(self.raw, function, bar) })
    }

    pub fn ep_clear_bar(&mut self, function: u8, bar: u8) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_ep_clear_bar(self.raw, function, bar) })
    }

    pub fn ep_program_inbound(
        &mut self,
        function: u8,
        index: u8,
        kind: AtuType,
        local_addr: u64,
        pci_addr: u64,
        size: u64,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::pcie_ep_program_inbound(
                self.raw,
                function,
                index,
                kind.into_raw(),
                local_addr,
                pci_addr,
                size,
            )
        })
    }

    pub fn ep_program_outbound(
        &mut self,
        index: u8,
        kind: AtuType,
        local_addr: u64,
        pci_addr: u64,
        size: u64,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::pcie_ep_program_outbound(
                self.raw,
                index,
                kind.into_raw(),
                local_addr,
                pci_addr,
                size,
            )
        })
    }

    pub fn ep_configure_msi(
        &mut self,
        function: u8,
        multiple_message_capable: u8,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::pcie_ep_configure_msi(self.raw, function, multiple_message_capable)
        })
    }

    pub fn ep_start(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_ep_start(self.raw) })
    }

    pub fn ep_stop(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_ep_stop(self.raw) })
    }

    pub fn ep_link_up(&mut self) -> bool {
        unsafe { raw::pcie_ep_link_up(self.raw) != 0 }
    }
}

/// Borrowed wrapper for the controller sub-object of a PCIe instance.
pub struct PcieController<'a> {
    raw: &'a mut raw::pcie_controller,
}

impl<'a> PcieController<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::pcie_controller) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self, config: &raw::pcie_controller_config) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_controller_init(self.raw, config) })
    }

    /// Initialize with a caller-owned C operations table.
    ///
    /// The table must outlive the controller and every callback must obey the
    /// C ABI. This is unsafe because the C controller stores the pointer.
    pub unsafe fn initialize_with_ops(
        &mut self,
        config: &raw::pcie_controller_config,
        ops: &raw::pcie_controller_ops,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_controller_init_with_ops(self.raw, config, ops) })
    }

    pub fn exit(&mut self) {
        unsafe { raw::pcie_controller_exit(self.raw) };
    }

    pub fn read_dbi(&mut self, offset: u32, size: u8) -> DriverResult<u32> {
        let mut value = 0;
        syterkit_lib::status(unsafe {
            raw::pcie_controller_dbi_read(self.raw, offset, size, &mut value)
        })?;
        Ok(value)
    }

    pub fn write_dbi(&mut self, offset: u32, size: u8, value: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::pcie_controller_dbi_write(self.raw, offset, size, value)
        })
    }

    pub fn enable_dbi_ro_write(&mut self, enable: bool) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::pcie_controller_dbi_ro_write_enable(self.raw, i8::from(enable))
        })
    }

    pub fn set_endpoint_bar(
        &mut self,
        function: u8,
        bar: u8,
        enable: bool,
        bar_64bit: bool,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::pcie_controller_set_ep_bar(
                self.raw,
                function,
                bar,
                i8::from(enable),
                i8::from(bar_64bit),
            )
        })
    }

    pub fn read_app(&mut self, offset: u32) -> DriverResult<u32> {
        let mut value = 0;
        syterkit_lib::status(unsafe {
            raw::pcie_controller_app_read(self.raw, offset, &mut value)
        })?;
        Ok(value)
    }

    pub fn write_app(&mut self, offset: u32, value: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_controller_app_write(self.raw, offset, value) })
    }

    pub fn set_mode(&mut self, mode: PcieMode) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_controller_set_mode(self.raw, mode.into_raw()) })
    }

    pub fn set_link(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_controller_set_link(self.raw) })
    }

    pub fn change_speed(&mut self, generation: u8) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_controller_change_speed(self.raw, generation) })
    }

    pub fn set_ltssm(&mut self, enable: bool) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_controller_ltssm(self.raw, i8::from(enable)) })
    }

    pub fn link_up(&mut self) -> bool {
        unsafe { raw::pcie_controller_link_up(self.raw) != 0 }
    }

    pub fn wait_link(&mut self, timeout_us: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_controller_wait_link(self.raw, timeout_us) })
    }

    pub fn program_atu(&mut self, region: &raw::pcie_atu_region) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_controller_program_atu(self.raw, region) })
    }

    pub fn disable_atu(
        &mut self,
        direction: raw::pcie_atu_direction,
        index: u8,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::pcie_controller_disable_atu(self.raw, direction, index)
        })
    }

    pub fn read_config(&mut self, bdf: u32, offset: u32, size: u8) -> DriverResult<u32> {
        let mut value = 0;
        syterkit_lib::status(unsafe {
            raw::pcie_controller_cfg_read(self.raw, bdf, offset, size, &mut value)
        })?;
        Ok(value)
    }

    pub fn write_config(
        &mut self,
        bdf: u32,
        offset: u32,
        size: u8,
        value: u32,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::pcie_controller_cfg_write(self.raw, bdf, offset, size, value)
        })
    }

    pub fn find_capability(&mut self, function_offset: u32, capability: u8) -> DriverResult<u32> {
        nonnegative(unsafe {
            raw::pcie_controller_find_capability(self.raw, function_offset, capability)
        })
    }

    pub fn find_extended_capability(
        &mut self,
        function_offset: u32,
        capability: u16,
    ) -> DriverResult<u32> {
        nonnegative(unsafe {
            raw::pcie_controller_find_ext_capability(self.raw, function_offset, capability)
        })
    }
}

/// Borrowed wrapper for a PCIe PHY instance.
pub struct PciePhy<'a> {
    raw: &'a mut raw::pcie_phy,
}

impl<'a> PciePhy<'a> {
    pub unsafe fn from_raw(raw: &'a mut raw::pcie_phy) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self, config: &raw::pcie_phy_config) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::pcie_phy_init(self.raw, config) })
    }

    pub fn exit(&mut self) {
        unsafe { raw::pcie_phy_exit(self.raw) };
    }
}

fn nonnegative(value: i32) -> DriverResult<u32> {
    if value < 0 {
        Err(value)
    } else {
        Ok(value as u32)
    }
}

#[cfg(test)]
mod tests {
    use super::{Pcie, PcieMode};
    use core::ffi::c_int;

    #[no_mangle]
    pub extern "C" fn pcie_config_sun55iw6(
        config: *mut syterkit_ffi::raw::pcie_config,
        mode: syterkit_ffi::raw::pcie_mode,
    ) {
        unsafe {
            (*config).mode = mode;
        }
    }
    #[no_mangle]
    pub extern "C" fn pcie_platform_power_on(
        _config: *const syterkit_ffi::raw::pcie_config,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn pcie_init(
        _pcie: *mut syterkit_ffi::raw::pcie,
        _config: *const syterkit_ffi::raw::pcie_config,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn pcie_wait_for_link(
        _pcie: *mut syterkit_ffi::raw::pcie,
        _timeout: u32,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn pcie_exit(_pcie: *mut syterkit_ffi::raw::pcie) {}
    #[no_mangle]
    pub extern "C" fn pcie_controller_init(
        _controller: *mut syterkit_ffi::raw::pcie_controller,
        _config: *const syterkit_ffi::raw::pcie_controller_config,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn pcie_controller_exit(_controller: *mut syterkit_ffi::raw::pcie_controller) {}
    #[no_mangle]
    pub extern "C" fn pcie_controller_dbi_read(
        _controller: *mut syterkit_ffi::raw::pcie_controller,
        _offset: u32,
        _size: u8,
        value: *mut u32,
    ) -> c_int {
        unsafe {
            *value = 0xfeed;
        }
        0
    }
    #[no_mangle]
    pub extern "C" fn pcie_controller_dbi_write(
        _controller: *mut syterkit_ffi::raw::pcie_controller,
        _offset: u32,
        _size: u8,
        _value: u32,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn pcie_controller_app_read(
        _controller: *mut syterkit_ffi::raw::pcie_controller,
        _offset: u32,
        value: *mut u32,
    ) -> c_int {
        unsafe {
            *value = 0xbeef;
        }
        0
    }
    #[no_mangle]
    pub extern "C" fn pcie_controller_app_write(
        _controller: *mut syterkit_ffi::raw::pcie_controller,
        _offset: u32,
        _value: u32,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn pcie_controller_set_mode(
        _controller: *mut syterkit_ffi::raw::pcie_controller,
        _mode: syterkit_ffi::raw::pcie_mode,
    ) -> c_int {
        0
    }

    #[test]
    fn pcie_wraps_platform_and_controller_interfaces() {
        let config = Pcie::default_config(PcieMode::RootComplex);
        assert_eq!(config.mode, 0);
        Pcie::platform_power_on(&config).unwrap();
        let mut raw_pcie: syterkit_ffi::raw::pcie = unsafe { core::mem::zeroed() };
        let mut pcie = unsafe { Pcie::from_raw(&mut raw_pcie) };
        pcie.initialize(&config).unwrap();
        pcie.wait_for_link(1000).unwrap();
        {
            let mut controller = pcie.controller();
            controller.initialize(&config.controller).unwrap();
            assert_eq!(controller.read_dbi(0, 4), Ok(0xfeed));
            controller.write_dbi(0, 4, 1).unwrap();
            assert_eq!(controller.read_app(0), Ok(0xbeef));
            controller.write_app(0, 1).unwrap();
            controller.set_mode(PcieMode::Endpoint).unwrap();
            controller.exit();
        }
        pcie.exit();
    }
}
