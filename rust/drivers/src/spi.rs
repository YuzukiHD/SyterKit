// SPDX-License-Identifier: GPL-2.0+

use core::ffi::c_void;

use syterkit_ffi::raw;
use syterkit_lib::{DriverResult, INVALID_ARGUMENT};

/// SPI wire mode accepted by the C controller driver.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum SpiIoMode {
    Single,
    DualRx,
    QuadRx,
    QuadIo,
}

impl SpiIoMode {
    fn into_raw(self) -> raw::spi_io_mode_t {
        match self {
            Self::Single => raw::spi_io_mode_t_SPI_IO_SINGLE,
            Self::DualRx => raw::spi_io_mode_t_SPI_IO_DUAL_RX,
            Self::QuadRx => raw::spi_io_mode_t_SPI_IO_QUAD_RX,
            Self::QuadIo => raw::spi_io_mode_t_SPI_IO_QUAD_IO,
        }
    }
}

/// Exclusive Rust access to one C `sunxi_spi_t` controller descriptor.
pub struct SpiBus<'a> {
    raw: &'a mut raw::sunxi_spi_t,
}

impl<'a> SpiBus<'a> {
    /// Borrow a board-owned descriptor filled by device-tree setup.
    pub unsafe fn from_raw(raw: &'a mut raw::sunxi_spi_t) -> Self {
        Self { raw }
    }

    pub fn initialize(&mut self) -> DriverResult<()> {
        let result = unsafe { raw::sunxi_spi_init(self.raw) };
        syterkit_lib::status(result)
    }

    pub fn disable(&mut self) {
        unsafe { raw::sunxi_spi_disable(self.raw) };
    }

    pub fn update_clock(&mut self, clock_hz: u32) -> DriverResult<()> {
        let result = unsafe { raw::sunxi_spi_update_clk(self.raw, clock_hz) };
        syterkit_lib::status(result)
    }

    pub fn select(&mut self, chip_select: u8) -> DriverResult<()> {
        let result = unsafe { raw::sunxi_spi_select(self.raw, chip_select) };
        syterkit_lib::status(result)
    }

    /// Transfer optional transmit and receive buffers.
    ///
    /// A negative C return value is propagated as an error. A non-negative
    /// return value is the number of bytes transferred by the C driver.
    pub fn transfer(
        &mut self,
        mode: SpiIoMode,
        tx: Option<&[u8]>,
        rx: Option<&mut [u8]>,
    ) -> DriverResult<usize> {
        let (tx_ptr, tx_len) = match tx {
            Some(buffer) => (buffer.as_ptr().cast_mut().cast::<c_void>(), buffer.len()),
            None => (core::ptr::null_mut(), 0),
        };
        let (rx_ptr, rx_len) = match rx {
            Some(buffer) => (buffer.as_mut_ptr().cast::<c_void>(), buffer.len()),
            None => (core::ptr::null_mut(), 0),
        };
        let tx_len = u32::try_from(tx_len).map_err(|_| INVALID_ARGUMENT)?;
        let rx_len = u32::try_from(rx_len).map_err(|_| INVALID_ARGUMENT)?;
        let result = unsafe {
            raw::sunxi_spi_transfer(self.raw, mode.into_raw(), tx_ptr, tx_len, rx_ptr, rx_len)
        };
        if result < 0 {
            Err(result)
        } else {
            Ok(result as usize)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::{SpiBus, SpiIoMode};
    use core::ffi::{c_int, c_void};
    use core::sync::atomic::{AtomicI32, AtomicU32, Ordering};
    use std::sync::Mutex;

    static TEST_LOCK: Mutex<()> = Mutex::new(());
    static SPI_STATUS: AtomicI32 = AtomicI32::new(0);
    static SPI_LAST_MODE: AtomicU32 = AtomicU32::new(0);
    static SPI_LAST_TX_LEN: AtomicU32 = AtomicU32::new(0);
    static SPI_LAST_RX_LEN: AtomicU32 = AtomicU32::new(0);

    #[no_mangle]
    pub extern "C" fn sunxi_spi_init(_spi: *mut syterkit_ffi::raw::sunxi_spi_t) -> c_int {
        SPI_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_spi_disable(_spi: *mut syterkit_ffi::raw::sunxi_spi_t) {}

    #[no_mangle]
    pub extern "C" fn sunxi_spi_update_clk(
        _spi: *mut syterkit_ffi::raw::sunxi_spi_t,
        _clock_hz: u32,
    ) -> c_int {
        SPI_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_spi_select(
        _spi: *mut syterkit_ffi::raw::sunxi_spi_t,
        _chip_select: u8,
    ) -> c_int {
        SPI_STATUS.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sunxi_spi_transfer(
        _spi: *mut syterkit_ffi::raw::sunxi_spi_t,
        mode: syterkit_ffi::raw::spi_io_mode_t,
        txbuf: *mut c_void,
        txlen: u32,
        rxbuf: *mut c_void,
        rxlen: u32,
    ) -> c_int {
        SPI_LAST_MODE.store(mode, Ordering::Relaxed);
        SPI_LAST_TX_LEN.store(txlen, Ordering::Relaxed);
        SPI_LAST_RX_LEN.store(rxlen, Ordering::Relaxed);
        if !txbuf.is_null() && txlen != 0 {
            let _ = unsafe { core::slice::from_raw_parts(txbuf.cast::<u8>(), txlen as usize) };
        }
        if !rxbuf.is_null() && rxlen != 0 {
            let buffer =
                unsafe { core::slice::from_raw_parts_mut(rxbuf.cast::<u8>(), rxlen as usize) };
            buffer.fill(0xa5);
        }
        let status = SPI_STATUS.load(Ordering::Relaxed);
        if status == 0 {
            (txlen + rxlen) as c_int
        } else {
            status
        }
    }

    #[test]
    fn spi_bus_maps_control_and_full_duplex_transfer() {
        let _guard = TEST_LOCK.lock().unwrap();
        SPI_STATUS.store(0, Ordering::Relaxed);
        let mut raw_spi: syterkit_ffi::raw::sunxi_spi_t = unsafe { core::mem::zeroed() };
        let mut bus = unsafe { SpiBus::from_raw(&mut raw_spi) };
        bus.initialize().unwrap();
        bus.update_clock(24_000_000).unwrap();
        bus.select(1).unwrap();
        let mut rx = [0; 2];
        assert_eq!(
            bus.transfer(SpiIoMode::QuadIo, Some(&[1, 2, 3]), Some(&mut rx)),
            Ok(5)
        );
        assert_eq!(SPI_LAST_MODE.load(Ordering::Relaxed), 3);
        assert_eq!(SPI_LAST_TX_LEN.load(Ordering::Relaxed), 3);
        assert_eq!(SPI_LAST_RX_LEN.load(Ordering::Relaxed), 2);
        assert_eq!(rx, [0xa5; 2]);

        SPI_STATUS.store(-8, Ordering::Relaxed);
        assert_eq!(bus.transfer(SpiIoMode::Single, None, None), Err(-8));
        bus.disable();
    }
}
