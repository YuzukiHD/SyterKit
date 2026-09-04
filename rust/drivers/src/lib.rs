// SPDX-License-Identifier: GPL-2.0+

#![no_std]

pub mod clic;
pub mod clk;
pub mod dma;
pub mod dram;
pub mod gic;
pub mod gpio;
pub mod i2c;
pub mod intc;
pub mod mmc;
pub mod mtd;
pub mod pcie;
pub mod plic;
pub mod pmu;
pub mod psram;
pub mod pwm;
pub mod remoteproc;
pub mod rtc;
pub mod serial;
pub mod sid;
pub mod soc;
pub mod spi;
pub mod spif;
pub mod timer;
pub mod ufs;
pub mod usb;

pub use clic::Clic;
pub use clk::{ClockTree, CLOCK_TREE};
pub use dma::{DmaChannel, DmaController};
pub use dram::Dram;
pub use gic::Gic;
pub use gpio::{Gpio, GpioPin, GpioPull, GPIO};
pub use i2c::I2cBus;
pub use intc::InterruptController;
pub use mmc::{MmcTuning, SdMmc, SdMmcMedia, Sdhci};
pub use mtd::{SpiNand, SpiNor, SpifNor};
pub use pcie::{AtuType, Pcie, PcieController, PcieMode, PciePhy};
pub use plic::Plic;
pub use pmu::{Pmu, PmuModel};
pub use psram::Psram;
pub use pwm::{Pwm, PwmConfig, PwmMode, PwmPolarity};
pub use remoteproc::RemoteProcessor;
pub use rtc::Rtc;
pub use serial::{SerialError, SerialPort, StdoutUart};
pub use sid::Sid;
pub use soc::{Soc, SOC};
pub use spi::{SpiBus, SpiIoMode};
pub use spif::{Spif, SpifConfig};
pub use timer::{SoftwareTimer, Timer, TimerCallback, TIMER};
pub use ufs::{SunxiUfs, UfsDevice, UfsHost, UfsScsi};
pub use usb::{UsbController, UsbDevice, UsbDma, UsbDmaChannel, UsbManager, UsbPlatform, USB};

#[cfg(test)]
extern crate std;
