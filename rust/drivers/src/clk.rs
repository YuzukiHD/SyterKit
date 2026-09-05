// SPDX-License-Identifier: GPL-2.0+

use syterkit_ffi::raw;

/// Entry points for the board's global clock tree.
pub struct ClockTree;

impl ClockTree {
    pub const fn new() -> Self {
        Self
    }

    pub fn preinitialize(&self) {
        unsafe { raw::sunxi_clk_preinit() };
    }

    pub fn initialize(&self) {
        unsafe { raw::sunxi_clk_init() };
    }

    pub fn reset(&self) {
        unsafe { raw::sunxi_clk_reset() };
    }

    pub fn dump(&self) {
        unsafe { raw::sunxi_clk_dump() };
    }

    /// Read the board's high-speed oscillator rate in MHz.
    pub fn get_hosc_rate(&self) -> u32 {
        unsafe { raw::sun300iw1_clk_get_hosc_rate() }
    }

    /// Set the CPU PLL frequency in MHz.
    pub fn set_cpu_pll(&self, frequency_mhz: u32) {
        unsafe { raw::sun55iw3_clk_set_cpu_pll(frequency_mhz) };
    }

    /// Read the high-speed oscillator rate on Sun300IW1.
    #[inline]
    pub fn sun300iw1_hosc_rate(&self) -> u32 {
        self.get_hosc_rate()
    }

    /// Set the CPU PLL on Sun55IW3.
    #[inline]
    pub fn sun55iw3_cpu_pll(&self, frequency_mhz: u32) {
        self.set_cpu_pll(frequency_mhz);
    }
}

pub const CLOCK_TREE: ClockTree = ClockTree::new();

#[cfg(test)]
mod tests {
    use super::CLOCK_TREE;
    use core::sync::atomic::{AtomicU32, Ordering};

    static CALLS: AtomicU32 = AtomicU32::new(0);
    static HOSC_RATE: AtomicU32 = AtomicU32::new(24);
    static CPU_PLL: AtomicU32 = AtomicU32::new(0);

    #[no_mangle]
    pub extern "C" fn sunxi_clk_preinit() {
        CALLS.fetch_add(1, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn sunxi_clk_init() {
        CALLS.fetch_add(1, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn sunxi_clk_reset() {
        CALLS.fetch_add(1, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn sunxi_clk_dump() {
        CALLS.fetch_add(1, Ordering::Relaxed);
    }

    #[no_mangle]
    pub extern "C" fn sun300iw1_clk_get_hosc_rate() -> u32 {
        HOSC_RATE.load(Ordering::Relaxed)
    }

    #[no_mangle]
    pub extern "C" fn sun55iw3_clk_set_cpu_pll(frequency_mhz: u32) {
        CPU_PLL.store(frequency_mhz, Ordering::Relaxed);
    }

    #[test]
    fn clock_tree_forwards_global_lifecycle_calls() {
        CALLS.store(0, Ordering::Relaxed);
        CLOCK_TREE.preinitialize();
        CLOCK_TREE.initialize();
        CLOCK_TREE.reset();
        CLOCK_TREE.dump();
        assert_eq!(CALLS.load(Ordering::Relaxed), 4);
    }

    #[test]
    fn clock_tree_provides_common_soc_clock_operations() {
        HOSC_RATE.store(40, Ordering::Relaxed);
        CPU_PLL.store(0, Ordering::Relaxed);

        assert_eq!(CLOCK_TREE.get_hosc_rate(), 40);
        CLOCK_TREE.set_cpu_pll(1008);
        assert_eq!(CPU_PLL.load(Ordering::Relaxed), 1008);

        HOSC_RATE.store(24, Ordering::Relaxed);
        assert_eq!(CLOCK_TREE.sun300iw1_hosc_rate(), 24);
        CLOCK_TREE.sun55iw3_cpu_pll(816);
        assert_eq!(CPU_PLL.load(Ordering::Relaxed), 816);
    }
}
