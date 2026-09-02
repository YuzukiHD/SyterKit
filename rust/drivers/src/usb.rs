// SPDX-License-Identifier: GPL-2.0+

use core::ffi::c_void;

use syterkit_ffi::raw;
use syterkit_lib::{DriverResult, INVALID_ARGUMENT};

/// Global USB gadget manager operations.
pub struct UsbManager;

impl UsbManager {
    pub const fn new() -> Self {
        Self
    }

    pub fn initialize(&self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_usb_init() })
    }

    pub unsafe fn attach_function(&self, function: &raw::sunxi_usb_function_t) {
        unsafe { raw::sunxi_usb_attach_function(function) };
    }

    pub fn attach(&self) {
        unsafe { raw::sunxi_usb_attach() };
    }

    pub fn run_once(&self) -> DriverResult<i32> {
        let result = unsafe { raw::sunxi_usb_extern_loop() };
        if result < 0 {
            Err(result)
        } else {
            Ok(result)
        }
    }

    pub fn endpoint_reset(&self) {
        unsafe { raw::sunxi_usb_ep_reset() };
    }

    pub fn dump(&self, controller_base: u32, endpoint: u32) {
        unsafe { raw::sunxi_usb_dump(controller_base, endpoint) };
    }

    pub fn bulk_endpoint_reset(&self) {
        unsafe { raw::sunxi_usb_bulk_ep_reset() };
    }

    pub unsafe fn irq(&self, context: *mut c_void) {
        unsafe { raw::sunxi_usb_irq(context) };
    }

    pub fn receive_by_dma(&self, buffer: &mut [u8]) -> DriverResult<()> {
        let length = u32::try_from(buffer.len()).map_err(|_| INVALID_ARGUMENT)?;
        syterkit_lib::status(unsafe {
            raw::sunxi_usb_start_recv_by_dma(buffer.as_mut_ptr().cast(), length)
        })
    }

    pub fn dma_receive_status(&self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_usb_get_dma_rx_status() })
    }

    pub fn send_setup(&self, buffer: &[u8]) -> DriverResult<()> {
        let length = u32::try_from(buffer.len()).map_err(|_| INVALID_ARGUMENT)?;
        let pointer = if buffer.is_empty() {
            core::ptr::null()
        } else {
            buffer.as_ptr()
        };
        syterkit_lib::status(unsafe { raw::sunxi_usb_send_setup(length, pointer.cast()) })
    }

    pub fn set_address(&self, address: u32) -> DriverResult<()> {
        if address > 127 {
            return Err(INVALID_ARGUMENT);
        }
        syterkit_lib::status(unsafe { raw::sunxi_usb_set_address(address) })
    }

    pub fn send_data(&self, buffer: &[u8]) -> DriverResult<()> {
        let length = u32::try_from(buffer.len()).map_err(|_| INVALID_ARGUMENT)?;
        let pointer = if buffer.is_empty() {
            core::ptr::null_mut()
        } else {
            buffer.as_ptr() as *mut u8
        };
        syterkit_lib::status(unsafe { raw::sunxi_usb_send_data(pointer.cast(), length) })
    }

    pub fn endpoint_max_packet(&self) -> DriverResult<u32> {
        nonnegative(unsafe { raw::sunxi_usb_get_ep_max() })
    }

    pub fn endpoint_in_address(&self) -> DriverResult<u32> {
        nonnegative(unsafe { raw::sunxi_usb_get_ep_in_type() })
    }

    pub fn endpoint_out_address(&self) -> DriverResult<u32> {
        nonnegative(unsafe { raw::sunxi_usb_get_ep_out_type() })
    }
}

pub const USB: UsbManager = UsbManager::new();

/// Platform power and clock setup for one USB controller descriptor.
pub struct UsbPlatform<'a> {
    raw: &'a raw::sunxi_usb_t,
}

impl<'a> UsbPlatform<'a> {
    pub unsafe fn from_raw(raw: &'a raw::sunxi_usb_t) -> Self {
        Self { raw }
    }

    pub fn initialize(&self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::sunxi_usb_platform_init(self.raw) })
    }

    pub fn deinitialize(&self) {
        unsafe { raw::sunxi_usb_platform_deinit(self.raw) };
    }
}

/// Owned handle returned by the C USB controller manager.
pub struct UsbController {
    handle: usize,
}

impl UsbController {
    pub fn open(otg: u32) -> DriverResult<Self> {
        let handle = unsafe { raw::usb_controller_open_otg(otg) };
        if handle == 0 {
            Err(INVALID_ARGUMENT)
        } else {
            Ok(Self { handle })
        }
    }

    pub fn set_base(otg: u32, base: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::usb_controller_set_base(otg, base) })
    }

    pub const fn handle(&self) -> usize {
        self.handle
    }

    pub fn close(mut self) -> DriverResult<()> {
        self.close_inner()
    }

    fn close_inner(&mut self) -> DriverResult<()> {
        if self.handle == 0 {
            return Ok(());
        }
        let result = unsafe { raw::usb_controller_close_otg(self.handle) };
        if result == 0 {
            self.handle = 0;
            Ok(())
        } else {
            Err(result)
        }
    }

    pub fn vbus_status(&self) -> u32 {
        unsafe { raw::usb_controller_get_vbus_status(self.handle) }
    }

    pub fn active_endpoint(&self) -> u32 {
        unsafe { raw::usb_controller_get_active_ep(self.handle) }
    }

    pub fn force_id_status(&self, id_type: u32) {
        unsafe { raw::usb_controller_force_id_status(self.handle, id_type) };
    }

    pub fn force_vbus_valid(&self, vbus_type: u32) {
        unsafe { raw::usb_controller_force_vbus_valid(self.handle, vbus_type) };
    }

    pub fn enable_id_pull(&self) {
        unsafe { raw::usb_controller_id_pull_enable(self.handle) };
    }

    pub fn disable_id_pull(&self) {
        unsafe { raw::usb_controller_id_pull_disable(self.handle) };
    }

    pub fn enable_dpdm_pull(&self) {
        unsafe { raw::usb_controller_dpdm_pull_enable(self.handle) };
    }

    pub fn disable_dpdm_pull(&self) {
        unsafe { raw::usb_controller_dpdm_pull_disable(self.handle) };
    }

    pub fn select_bus(&self, io_type: u32, endpoint_type: u32, endpoint: u32) {
        unsafe { raw::usb_controller_select_bus(self.handle, io_type, endpoint_type, endpoint) };
    }

    pub fn disable_all_misc_interrupts(&self) {
        unsafe { raw::usb_controller_int_disable_usb_misc_all(self.handle) };
    }

    pub fn disable_all_endpoint_interrupts(&self, endpoint_type: u32) {
        unsafe { raw::usb_controller_int_disable_ep_all(self.handle, endpoint_type) };
    }

    pub fn enable_misc_interrupts(&self, mask: u32) {
        unsafe { raw::usb_controller_int_enable_usb_misc_uint(self.handle, mask) };
    }

    pub fn disable_misc_interrupts(&self, mask: u32) {
        unsafe { raw::usb_controller_int_disable_usb_misc_uint(self.handle, mask) };
    }

    pub fn enable_endpoint_interrupt(&self, endpoint_type: u32, endpoint: u32) {
        unsafe { raw::usb_controller_int_enable_ep(self.handle, endpoint_type, endpoint) };
    }

    pub fn endpoint_interrupt_pending(&self, endpoint_type: u32) -> u32 {
        unsafe { raw::usb_controller_int_ep_pending(self.handle, endpoint_type) }
    }

    pub fn clear_endpoint_interrupt(&self, endpoint_type: u32, endpoint: u8) {
        unsafe { raw::usb_controller_int_clear_ep_pending(self.handle, endpoint_type, endpoint) };
    }

    pub fn clear_all_endpoint_interrupts(&self, endpoint_type: u32) {
        unsafe { raw::usb_controller_int_clear_ep_pending_all(self.handle, endpoint_type) };
    }

    pub fn misc_interrupt_pending(&self) -> u32 {
        unsafe { raw::usb_controller_int_misc_pending(self.handle) }
    }

    pub fn clear_misc_interrupt(&self, mask: u32) {
        unsafe { raw::usb_controller_int_clear_misc_pending(self.handle, mask) };
    }

    pub fn clear_all_misc_interrupts(&self) {
        unsafe { raw::usb_controller_int_clear_misc_pending_all(self.handle) };
    }

    pub fn disable_endpoint_interrupt(&self, endpoint_type: u32, endpoint: u8) {
        unsafe { raw::usb_controller_int_disable_ep(self.handle, endpoint_type, endpoint) };
    }

    pub fn select_active_endpoint(&self, endpoint: u8) {
        unsafe { raw::usb_controller_select_active_ep(self.handle, endpoint) };
    }

    pub fn configure_fifo(
        &self,
        endpoint_type: u32,
        double_fifo: bool,
        fifo_size: u32,
        fifo_address: u32,
    ) {
        unsafe {
            raw::usb_controller_config_fifo(
                self.handle,
                endpoint_type,
                u32::from(double_fifo),
                fifo_size,
                fifo_address,
            )
        };
    }

    pub fn configure_fifo_base(&self, sram_base: u32) {
        unsafe { raw::usb_controller_config_fifo_base(self.handle, sram_base) };
    }

    pub fn read_fifo_length(&self, endpoint_type: u32) -> u32 {
        unsafe { raw::usb_controller_read_len_from_fifo(self.handle, endpoint_type) }
    }

    pub fn fifo_start_address(&self) -> u32 {
        unsafe { raw::usb_controller_get_port_fifo_start_addr(self.handle) }
    }

    pub fn fifo_size(&self) -> u32 {
        unsafe { raw::usb_controller_get_port_fifo_size(self.handle) }
    }

    pub fn select_fifo(&self, endpoint: u32) -> u32 {
        unsafe { raw::usb_controller_select_fifo(self.handle, endpoint) }
    }

    pub fn configure_tx_fifo_default(address: u32) {
        unsafe { raw::usb_controller_config_fifo_tx_ep_default(address) };
    }

    pub fn configure_tx_fifo(address: u32, double_fifo: bool, size: u32, fifo_address: u32) {
        unsafe {
            raw::usb_controller_config_fifo_tx_ep(
                address,
                u32::from(double_fifo),
                size,
                fifo_address,
            )
        };
    }

    pub fn configure_rx_fifo_default(address: u32) {
        unsafe { raw::usb_controller_config_fifo_rx_ep_default(address) };
    }

    pub fn configure_rx_fifo(address: u32, double_fifo: bool, size: u32, fifo_address: u32) {
        unsafe {
            raw::usb_controller_config_fifo_rx_ep(
                address,
                u32::from(double_fifo),
                size,
                fifo_address,
            )
        };
    }

    pub fn write_packet(&self, fifo: u32, data: &[u8]) -> DriverResult<u32> {
        let length = u32::try_from(data.len()).map_err(|_| INVALID_ARGUMENT)?;
        Ok(unsafe {
            raw::usb_controller_write_packet(self.handle, fifo, length, data.as_ptr().cast())
        })
    }

    pub fn read_packet(&self, fifo: u32, buffer: &mut [u8]) -> DriverResult<u32> {
        let length = u32::try_from(buffer.len()).map_err(|_| INVALID_ARGUMENT)?;
        Ok(unsafe {
            raw::usb_controller_read_packet(self.handle, fifo, length, buffer.as_mut_ptr().cast())
        })
    }

    pub fn device(&self) -> UsbDevice<'_> {
        UsbDevice { controller: self }
    }
}

impl Drop for UsbController {
    fn drop(&mut self) {
        let _ = self.close_inner();
    }
}

/// USB device-mode operations borrowing an open controller handle.
pub struct UsbDevice<'a> {
    controller: &'a UsbController,
}

impl UsbDevice<'_> {
    fn handle(&self) -> usize {
        self.controller.handle
    }

    pub fn set_address_default(&self) {
        unsafe { raw::usb_device_set_address_default(self.handle()) };
    }

    pub fn set_address(&self, address: u8) {
        unsafe { raw::usb_device_set_address(self.handle(), address) };
    }

    pub fn transfer_mode(&self) -> u32 {
        unsafe { raw::usb_device_query_transfer_mode(self.handle()) }
    }

    pub fn configure_transfer_mode(&self, transfer_type: u8, speed: u8) {
        unsafe { raw::usb_device_config_transfer_mode(self.handle(), transfer_type, speed) };
    }

    pub fn connect(&self, enabled: bool) {
        unsafe { raw::usb_device_connect_switch(self.handle(), u32::from(enabled)) };
    }

    pub fn power_status(&self) -> u32 {
        unsafe { raw::usb_device_query_power_status(self.handle()) }
    }

    pub fn configure_endpoint(
        &self,
        transfer_type: u32,
        endpoint_type: u32,
        double_fifo: bool,
        max_packet: u32,
    ) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::usb_device_config_ep(
                self.handle(),
                transfer_type,
                endpoint_type,
                u32::from(double_fifo),
                max_packet,
            )
        })
    }

    pub fn configure_endpoint_default(&self, endpoint_type: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::usb_device_config_ep_default(self.handle(), endpoint_type)
        })
    }

    pub fn configure_endpoint_dma(&self, endpoint_type: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::usb_device_config_ep_dma(self.handle(), endpoint_type) })
    }

    pub fn clear_endpoint_dma(&self, endpoint_type: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::usb_device_clear_ep_dma(self.handle(), endpoint_type) })
    }

    pub fn endpoint_stalled(&self, endpoint_type: u32) -> DriverResult<bool> {
        nonnegative(unsafe { raw::usb_device_get_ep_stall(self.handle(), endpoint_type) })
            .map(|value| value != 0)
    }

    pub fn send_stall(&self, endpoint_type: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::usb_device_ep_send_stall(self.handle(), endpoint_type) })
    }

    pub fn clear_stall(&self, endpoint_type: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::usb_device_ep_clear_stall(self.handle(), endpoint_type)
        })
    }

    pub fn setup_end(&self) -> u32 {
        unsafe { raw::usb_device_ctrl_get_setup_end(self.handle()) }
    }

    pub fn clear_setup_end(&self) {
        unsafe { raw::usb_device_ctrl_clear_setup_end(self.handle()) };
    }

    pub fn write_data_status(&self, endpoint_type: u32, complete: bool) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::usb_device_write_data_status(self.handle(), endpoint_type, u32::from(complete))
        })
    }

    pub fn read_data_status(&self, endpoint_type: u32, complete: bool) -> DriverResult<()> {
        syterkit_lib::status(unsafe {
            raw::usb_device_read_data_status(self.handle(), endpoint_type, u32::from(complete))
        })
    }

    pub fn read_data_ready(&self, endpoint_type: u32) -> u32 {
        unsafe { raw::usb_device_get_read_data_ready(self.handle(), endpoint_type) }
    }

    pub fn write_data_ready(&self, endpoint_type: u32) -> u32 {
        unsafe { raw::usb_device_get_write_data_ready(self.handle(), endpoint_type) }
    }

    pub fn write_fifo_empty(&self, endpoint_type: u32) -> u32 {
        unsafe { raw::usb_device_get_write_data_ready_fifo_empty(self.handle(), endpoint_type) }
    }

    pub fn iso_update_enable(&self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::usb_device_iso_update_enable(self.handle()) })
    }

    pub fn flush_fifo(&self, endpoint_type: u32) {
        unsafe { raw::usb_device_flush_fifo(self.handle(), endpoint_type) };
    }
}

/// One channel in the legacy USB DMA manager.
pub struct UsbDmaChannel {
    index: u32,
}

impl UsbDmaChannel {
    pub fn request() -> DriverResult<Self> {
        let index = unsafe { raw::usb_dma_request() };
        if index <= 0 {
            Err(if index < 0 { index } else { INVALID_ARGUMENT })
        } else {
            Ok(Self {
                index: index as u32,
            })
        }
    }

    pub const fn index(&self) -> u32 {
        self.index
    }

    pub fn configure(&mut self, direction: u32, endpoint: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::usb_dma_setting(self.index, direction, endpoint) })
    }

    pub fn set_packet_length(&mut self, length: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::usb_dma_set_pktlen(self.index, length) })
    }

    pub fn start(&mut self, address: u32, bytes: u32) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::usb_dma_start(self.index, address, bytes) })
    }

    pub fn stop(&mut self) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::usb_dma_stop(self.index) })
    }

    fn release_inner(&mut self) -> DriverResult<()> {
        if self.index == 0 {
            return Ok(());
        }
        let result = unsafe { raw::usb_dma_release(self.index) };
        if result == 0 {
            self.index = 0;
            Ok(())
        } else {
            Err(result)
        }
    }

    pub fn release(mut self) -> DriverResult<()> {
        self.release_inner()
    }
}

impl Drop for UsbDmaChannel {
    fn drop(&mut self) {
        let _ = self.release_inner();
    }
}

pub struct UsbDma;

impl UsbDma {
    pub fn initialize(handle: usize) -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::usb_dma_init(handle) })
    }

    pub fn interrupt_status() -> i32 {
        unsafe { raw::usb_dma_int_query() }
    }

    pub fn clear_interrupt() -> DriverResult<()> {
        syterkit_lib::status(unsafe { raw::usb_dma_int_clear() })
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
    use super::{UsbController, UsbDma, UsbDmaChannel, UsbManager};
    use core::ffi::{c_int, c_void};

    #[no_mangle]
    pub extern "C" fn sunxi_usb_platform_init(
        _usb: *const syterkit_ffi::raw::sunxi_usb_t,
    ) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_usb_platform_deinit(_usb: *const syterkit_ffi::raw::sunxi_usb_t) {}
    #[no_mangle]
    pub extern "C" fn sunxi_usb_init() -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_usb_attach_function(
        _function: *const syterkit_ffi::raw::sunxi_usb_function_t,
    ) {
    }
    #[no_mangle]
    pub extern "C" fn sunxi_usb_attach() {}
    #[no_mangle]
    pub extern "C" fn sunxi_usb_extern_loop() -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_usb_ep_reset() {}
    #[no_mangle]
    pub extern "C" fn sunxi_usb_bulk_ep_reset() {}
    #[no_mangle]
    pub extern "C" fn sunxi_usb_irq(_context: *mut c_void) {}
    #[no_mangle]
    pub extern "C" fn sunxi_usb_start_recv_by_dma(_buffer: *mut c_void, _length: u32) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_usb_get_dma_rx_status() -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_usb_send_setup(_length: u32, _buffer: *const c_void) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_usb_set_address(_address: u32) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_usb_send_data(_buffer: *mut c_void, _length: u32) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn sunxi_usb_get_ep_max() -> c_int {
        512
    }
    #[no_mangle]
    pub extern "C" fn sunxi_usb_get_ep_in_type() -> c_int {
        1
    }
    #[no_mangle]
    pub extern "C" fn sunxi_usb_get_ep_out_type() -> c_int {
        2
    }
    #[no_mangle]
    pub extern "C" fn usb_controller_open_otg(_otg: u32) -> usize {
        0x1000
    }
    #[no_mangle]
    pub extern "C" fn usb_controller_set_base(_otg: u32, _base: u32) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn usb_controller_close_otg(_handle: usize) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn usb_controller_get_vbus_status(_handle: usize) -> u32 {
        1
    }
    #[no_mangle]
    pub extern "C" fn usb_controller_get_active_ep(_handle: usize) -> u32 {
        3
    }
    #[no_mangle]
    pub extern "C" fn usb_controller_write_packet(
        _handle: usize,
        _fifo: u32,
        count: u32,
        _buffer: *const c_void,
    ) -> u32 {
        count
    }
    #[no_mangle]
    pub extern "C" fn usb_controller_read_packet(
        _handle: usize,
        _fifo: u32,
        count: u32,
        buffer: *mut c_void,
    ) -> u32 {
        if !buffer.is_null() {
            unsafe {
                core::slice::from_raw_parts_mut(buffer.cast::<u8>(), count as usize).fill(0x5a);
            }
        }
        count
    }
    #[no_mangle]
    pub extern "C" fn usb_dma_init(_handle: usize) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn usb_dma_request() -> c_int {
        1
    }
    #[no_mangle]
    pub extern "C" fn usb_dma_release(_index: u32) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn usb_dma_setting(_index: u32, _dir: u32, _ep: u32) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn usb_dma_set_pktlen(_index: u32, _len: u32) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn usb_dma_start(_index: u32, _addr: u32, _bytes: u32) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn usb_dma_stop(_index: u32) -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn usb_dma_int_query() -> c_int {
        0
    }
    #[no_mangle]
    pub extern "C" fn usb_dma_int_clear() -> c_int {
        0
    }

    #[test]
    fn usb_manager_controller_and_dma_wrap_c_interfaces() {
        let manager = UsbManager::new();
        manager.initialize().unwrap();
        manager.set_address(7).unwrap();
        manager.send_setup(&[1, 2]).unwrap();
        manager.send_data(&[3, 4]).unwrap();
        assert_eq!(manager.endpoint_max_packet(), Ok(512));
        assert_eq!(manager.endpoint_in_address(), Ok(1));
        assert_eq!(manager.endpoint_out_address(), Ok(2));

        let controller = UsbController::open(0).unwrap();
        assert_eq!(controller.vbus_status(), 1);
        let mut packet = [0u8; 4];
        assert_eq!(controller.write_packet(1, &packet), Ok(4));
        assert_eq!(controller.read_packet(1, &mut packet), Ok(4));
        assert_eq!(packet, [0x5a; 4]);
        controller.close().unwrap();

        UsbDma::initialize(0x1000).unwrap();
        let mut dma = UsbDmaChannel::request().unwrap();
        dma.configure(1, 2).unwrap();
        dma.set_packet_length(64).unwrap();
        dma.start(0x2000, 64).unwrap();
        dma.stop().unwrap();
        dma.release().unwrap();
        UsbDma::clear_interrupt().unwrap();
    }
}
