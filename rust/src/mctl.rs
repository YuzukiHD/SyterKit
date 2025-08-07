use allwinner_hal::ccu::{DramClockSource, PeriFactorN};
use allwinner_rt::soc::d1::{CCU, PHY};
use core::ptr::{read_volatile, write_volatile};

// for verbose prints
const VERBOSE: bool = false;

pub const RAM_BASE: usize = 0x40000000;

// p49 ff
// const CCU: usize = 0x0200_1000;
// const PLL_CPU_CTRL: usize = CCU + 0x0000;
// const PLL_DDR_CTRL: usize = CCU + 0x0010;
// const MBUS_CLK: usize = CCU + 0x0540;
// const DRAM_CLK: usize = CCU + 0x0800;
// const DRAM_BGR: usize = CCU + 0x080c;

/**
 * D1 manual p152 3.4 System Configuration
 *
 * SYS_CFG Base Address 0x03000000
 *
 * | Register Name       | Offset | Description                              |
 * | ------------------- | ------ | ---------------------------------------- |
 * | DSP_BOOT_RAMMAP_REG | 0x0008 | DSP Boot SRAM Remap Control Register     |
 * | VER_REG             | 0x0024 | Version Register                         |
 * | EMAC_EPHY_CLK_REG0  | 0x0030 | EMAC-EPHY Clock Register 0               |
 * | SYS_LDO_CTRL_REG    | 0x0150 | System LDO Control Register              |
 * | RESCAL_CTRL_REG     | 0x0160 | Resistor Calibration Control Register    |
 * | RES240_CTRL_REG     | 0x0168 | 240ohms Resistor Manual Control Register |
 * | RESCAL_STATUS_REG   | 0x016C | Resistor Calibration Status Register     |
 */

const SYS_CFG: usize = 0x0300_0000; // 0x0300_0000 - 0x0300_0FFF
                                    // const VER_REG: usize = SYS_CFG + 0x0024;
                                    // const EMAC_EPHY_CLK_REG0: usize = SYS_CFG + 0x0030;
const SYS_LDO_CTRL_REG: usize = SYS_CFG + 0x0150;
const RES_CAL_CTRL_REG: usize = SYS_CFG + 0x0160;
const RES240_CTRL_REG: usize = SYS_CFG + 0x0168;
// const RES_CAL_STATUS_REG: usize = SYS_CFG + 0x016c;
// const ZQ_INTERNAL: usize = SYS_CFG + 0x016e;
const ZQ_VALUE: usize = SYS_CFG + 0x0172;

const BAR_BASE: usize = 0x0700_0000; // TODO: What do we call this?
const SOME_STATUS: usize = BAR_BASE + 0x05d4; // 0x70005d4

const FOO_BASE: usize = 0x0701_0000; // TODO: What do we call this?
const ANALOG_SYS_PWROFF_GATING_REG: usize = FOO_BASE + 0x0254;
const SOME_OTHER: usize = FOO_BASE + 0x0250; // 0x7010250

const SID_INFO: usize = SYS_CFG + 0x2228; // 0x3002228

// p32 memory mapping
// MSI + MEMC: 0x0310_2000 - 0x0330_1fff
// NOTE: MSI shares the bus clock with CE, DMAC, IOMMU and CPU_SYS; p 38
// TODO: Define *_BASE?
const MSI_MEMC_BASE: usize = 0x0310_2000; // p32 0x0310_2000 - 0x0330_1FFF

// PHY config registers; TODO: fix names
const MC_WORK_MODE_RANK0_1: usize = MSI_MEMC_BASE;
const MC_WORK_MODE_RANK0_2: usize = MSI_MEMC_BASE + 0x0004;

const UNKNOWN1: usize = MSI_MEMC_BASE + 0x0008; // 0x3102008
const UNKNOWN7: usize = MSI_MEMC_BASE + 0x000c; // 0x310200c
const UNKNOWN12: usize = MSI_MEMC_BASE + 0x0014; // 0x3102014

const DRAM_MASTER_CTL1: usize = MSI_MEMC_BASE + 0x0020;
const DRAM_MASTER_CTL2: usize = MSI_MEMC_BASE + 0x0024;
const DRAM_MASTER_CTL3: usize = MSI_MEMC_BASE + 0x0028;

// NOTE: From unused function `bit_delay_compensation` in the
// C code; could be for other platforms?
// const UNKNOWN6: usize = MSI_MEMC_BASE + 0x0100; // 0x3102100

// TODO:
// 0x0310_2200
// 0x0310_2210
// 0x0310_2214
// 0x0310_2230
// 0x0310_2234
// 0x0310_2240
// 0x0310_2244
// 0x0310_2260
// 0x0310_2264
// 0x0310_2290
// 0x0310_2294
// 0x0310_2470
// 0x0310_2474
// 0x0310_31c0
// 0x0310_31c8
// 0x0310_31d0

/*
// NOTE: From unused function `bit_delay_compensation` in the
// C code; could be for other platforms?
// DATX0IOCR x + 4 * size
// DATX0IOCR - DATX3IOCR: 11 registers per block, blocks 0x20 words apart
const DATX0IOCR: usize = MSI_MEMC_BASE + 0x0310; // 0x3102310
const DATX3IOCR: usize = MSI_MEMC_BASE + 0x0510; // 0x3102510
*/

const PHY_AC_MAP1: usize = 0x3102500;
const PHY_AC_MAP2: usize = 0x3102504;
const PHY_AC_MAP3: usize = 0x3102508;
const PHY_AC_MAP4: usize = 0x310250c;

// *_BASE?
// const PIR: usize = MSI_MEMC_BASE + 0x1000; // 0x3103000
// const UNKNOWN15: usize = MSI_MEMC_BASE + 0x1004; // 0x3103004
// const MCTL_CLK: usize = MSI_MEMC_BASE + 0x100c; // 0x310300c
// const PGSR0: usize = MSI_MEMC_BASE + 0x1010; // 0x3103010

// const STATR_X: usize = MSI_MEMC_BASE + 0x1018; // 0x3103018

// const DRAM_MR0: usize = MSI_MEMC_BASE + 0x1030; // 0x3103030
// const DRAM_MR1: usize = MSI_MEMC_BASE + 0x1034; // 0x3103034
// const DRAM_MR2: usize = MSI_MEMC_BASE + 0x1038; // 0x3103038
// const DRAM_MR3: usize = MSI_MEMC_BASE + 0x103c; // 0x310303c
// const DRAM_ODTX: usize = MSI_MEMC_BASE + 0x102c; // 0x310302c

// const PTR3: usize = MSI_MEMC_BASE + 0x1050; // 0x3103050;
// const PTR4: usize = MSI_MEMC_BASE + 0x1054; // 0x3103054;
// const DRAMTMG0: usize = MSI_MEMC_BASE + 0x1058; // 0x3103058;
// const DRAMTMG1: usize = MSI_MEMC_BASE + 0x105c; // 0x310305c;
// const DRAMTMG2: usize = MSI_MEMC_BASE + 0x1060; // 0x3103060;
// const DRAMTMG3: usize = MSI_MEMC_BASE + 0x1064; // 0x3103064;
// const DRAMTMG4: usize = MSI_MEMC_BASE + 0x1068; // 0x3103068;
// const DRAMTMG5: usize = MSI_MEMC_BASE + 0x106c; // 0x310306c;
// // const DRAMTMG6: usize = MSI_MEMC_BASE + 0x1070; // 0x3103070;
// // const DRAMTMG7: usize = MSI_MEMC_BASE + 0x1074; // 0x3103074;
// const DRAMTMG8: usize = MSI_MEMC_BASE + 0x1078; // 0x3103078;
// const UNKNOWN8: usize = MSI_MEMC_BASE + 0x107c; // 0x310307c
// const PITMG0: usize = MSI_MEMC_BASE + 0x1080; // 0x3103080;
// const UNKNOWN13: usize = MSI_MEMC_BASE + 0x108c; // 0x310308c;
// const RFSHTMG: usize = MSI_MEMC_BASE + 0x1090; // 0x3103090;
// const RFSHCTL1: usize = MSI_MEMC_BASE + 0x1094; // 0x3103094;

// const UNKNOWN10: usize = MSI_MEMC_BASE + 0x109c; // 0x310309c
// const UNKNOWN11: usize = MSI_MEMC_BASE + 0x10a0; // 0x31030a0
// const UNKNOWN16: usize = MSI_MEMC_BASE + 0x10c0; // 0x31030c0

// const PGCR0: usize = MSI_MEMC_BASE + 0x1100; // 0x3103100

const MRCTRL0: usize = MSI_MEMC_BASE + 0x1108; // 0x3103108

// const UNKNOWN14: usize = MSI_MEMC_BASE + 0x110c; // 0x310310c

// const IOCVR0: usize = MSI_MEMC_BASE + 0x1110; // 0x3103110
// const IOCVR1: usize = MSI_MEMC_BASE + 0x1114; // 0x3103114

// const DQS_GATING_X: usize = MSI_MEMC_BASE + 0x111c; // 0x310311c

// const UNKNOWN9: usize = MSI_MEMC_BASE + 0x1120; // 0x3103120
// const ZQ_CFG: usize = MSI_MEMC_BASE + 0x1140; // 0x3103140
// const UNDOC1: usize = MSI_MEMC_BASE + 0x1208; // 0x3103208;

// const DAT00IOCR: usize = MSI_MEMC_BASE + 0x1310; // 0x3103310
// const DX0GCR0: usize = MSI_MEMC_BASE + 0x1344; // 0x3103344
// const UNKNOWN4: usize = MSI_MEMC_BASE + 0x1348; // 0x3103348
// const DX1GCR0: usize = MSI_MEMC_BASE + 0x13c4; // 0x31033c4;
// const DAT01IOCR: usize = MSI_MEMC_BASE + 0x1390; // 0x3103390

// const UNKNOWN5: usize = MSI_MEMC_BASE + 0x13c8; // 0x31033C8
// const DAT03IOCR: usize = MSI_MEMC_BASE + 0x1510; // 0x3103510

// TODO: *_BASE ?
const MC_WORK_MODE_RANK1_1: usize = MSI_MEMC_BASE + 0x10_0000;
const MC_WORK_MODE_RANK1_2: usize = MSI_MEMC_BASE + 0x10_0004;

/// Type of DDR SDRAM.
#[repr(u32)]
#[derive(Clone, Copy, PartialEq, Eq)]
pub enum DramType {
    /// Double-Data-Rate Two (DDR2) Synchronous Dynamic Random Access Memory.
    Ddr2 = 2,
    /// Double-Data-Rate Three (DDR3) Synchronous Dynamic Random Access Memory.
    Ddr3 = 3,
    /// Low-Power Double-Data-Rate 2 (LPDDR2).
    Lpddr2 = 6,
    /// Low-Power Double-Data-Rate 3 (LPDDR3).
    Lpddr3 = 7,
}

#[repr(C)]
pub struct dram_parameters {
    pub dram_clk: u32,
    pub dram_type: DramType,
    pub dram_zq: u32,
    pub dram_odt_en: u32,
    pub dram_para1: u32,
    pub dram_para2: u32,
    pub dram_mr0: u32,
    pub dram_mr1: u32,
    pub dram_mr2: u32,
    pub dram_mr3: u32,
    pub dram_tpr0: u32,
    pub dram_tpr1: u32,
    pub dram_tpr2: u32,
    pub dram_tpr3: u32,
    pub dram_tpr4: u32,
    pub dram_tpr5: u32,
    pub dram_tpr6: u32,
    pub dram_tpr7: u32,
    pub dram_tpr8: u32,
    /// overrided_dram_clk = para.dram_tpr9
    pub dram_tpr9: u32,
    pub dram_tpr10: u32,
    pub dram_tpr11: u32,
    pub dram_tpr12: u32,
    /// should_override = para.dram_tpr13 & (1 << 6) != 0;
    /// dqs_gating_mode = (para.dram_tpr13 >> 2) & 0x3;
    /// should_auto_scan_dram_config = (para.dram_tpr13 & 0x1) == 0;
    pub dram_tpr13: u32,
}
// FIXME: This could be a concise struct. Let Rust piece it together.
/*
    //dram_tpr0
    tccd : [23:21]
    tfaw : [20:15]
    trrd : [14:11]
    trcd : [10:6 ]
    trc  : [ 5:0 ]

    //dram_tpr1
    txp  : [27:23]
    twtr : [22:20]
    trtp : [19:15]
    twr  : [14:11]
    trp  : [10:6 ]
    tras : [ 5:0 ]

    //dram_tpr2
    trfc : [20:12]
    trefi: [11:0 ]
*/

fn readl(reg: usize) -> u32 {
    unsafe { read_volatile(reg as *mut u32) }
}

fn writel(reg: usize, val: u32) {
    unsafe {
        write_volatile(reg as *mut u32, val);
    }
}

fn sdelay(micros: usize) {
    let millis = micros * 1000;
    unsafe {
        for _ in 0..millis {
            core::arch::asm!("nop")
        }
    }
}

fn get_pmu_exists() -> bool {
    return false;
}

const PHY_CFG1: [u32; 22] = [
    1, 9, 3, 7, 8, 18, 4, 13, 5, 6, 10, 2, 14, 12, 0, 0, 21, 17, 20, 19, 11, 22,
];
const PHY_CFG2: [u32; 22] = [
    4, 9, 3, 7, 8, 18, 1, 13, 2, 6, 10, 5, 14, 12, 0, 0, 21, 17, 20, 19, 11, 22,
];
const PHY_CFG3: [u32; 22] = [
    1, 7, 8, 12, 10, 18, 4, 13, 5, 6, 3, 2, 9, 0, 0, 0, 21, 17, 20, 19, 11, 22,
];
const PHY_CFG4: [u32; 22] = [
    4, 12, 10, 7, 8, 18, 1, 13, 2, 6, 3, 5, 9, 0, 0, 0, 21, 17, 20, 19, 11, 22,
];
const PHY_CFG5: [u32; 22] = [
    13, 2, 7, 9, 12, 19, 5, 1, 6, 3, 4, 8, 10, 0, 0, 0, 21, 22, 18, 17, 11, 20,
];
const PHY_CFG6: [u32; 22] = [
    3, 10, 7, 13, 9, 11, 1, 2, 4, 6, 8, 5, 12, 0, 0, 0, 20, 1, 0, 21, 22, 17,
];
const PHY_CFG7: [u32; 22] = [
    3, 2, 4, 7, 9, 1, 17, 12, 18, 14, 13, 8, 15, 6, 10, 5, 19, 22, 16, 21, 20, 11,
];

// TODO: verify
// This routine seems to have several remapping tables for 22 lines.
// It is unclear which lines are being remapped. It seems to pick
// table PHY_CFG7 for the Nezha board.
unsafe fn mctl_phy_ac_remapping(para: &mut dram_parameters) {
    // read SID info @ 0x228
    let fuse = (readl(SID_INFO) >> 8) & 0x4;
    // // println!("ddr_efuse_type: 0x{:x}", fuse);
    let mut phy_cfg0 = [0; 22];
    if (para.dram_tpr13 >> 18) & 0x3 > 0 {
        // // println!("phy cfg 7");
        phy_cfg0 = PHY_CFG7;
    } else {
        match fuse {
            8 => phy_cfg0 = PHY_CFG2,
            9 => phy_cfg0 = PHY_CFG3,
            10 => phy_cfg0 = PHY_CFG5,
            11 => phy_cfg0 = PHY_CFG4,
            13 | 14 => {}
            12 | _ => phy_cfg0 = PHY_CFG1,
        }
    }

    if para.dram_type == DramType::Ddr2 {
        if fuse == 15 {
            return;
        }
        phy_cfg0 = PHY_CFG6;
    }

    if para.dram_type == DramType::Ddr2 || para.dram_type == DramType::Ddr3 {
        let val = (phy_cfg0[4] << 25)
            | (phy_cfg0[3] << 20)
            | (phy_cfg0[2] << 15)
            | (phy_cfg0[1] << 10)
            | (phy_cfg0[0] << 5);
        writel(PHY_AC_MAP1, val as u32);

        let val = (phy_cfg0[10] << 25)
            | (phy_cfg0[9] << 20)
            | (phy_cfg0[8] << 15)
            | (phy_cfg0[7] << 10)
            | (phy_cfg0[6] << 5)
            | phy_cfg0[5];
        writel(PHY_AC_MAP2, val as u32);

        let val = (phy_cfg0[15] << 20)
            | (phy_cfg0[14] << 15)
            | (phy_cfg0[13] << 10)
            | (phy_cfg0[12] << 5)
            | phy_cfg0[11];
        writel(PHY_AC_MAP3, val as u32);

        let val = (phy_cfg0[21] << 25)
            | (phy_cfg0[20] << 20)
            | (phy_cfg0[19] << 15)
            | (phy_cfg0[18] << 10)
            | (phy_cfg0[17] << 5)
            | phy_cfg0[16];
        writel(PHY_AC_MAP4, val as u32);

        let val = (phy_cfg0[4] << 25)
            | (phy_cfg0[3] << 20)
            | (phy_cfg0[2] << 15)
            | (phy_cfg0[1] << 10)
            | (phy_cfg0[0] << 5)
            | 1;
        writel(PHY_AC_MAP1, val as u32);
    }
}

fn dram_vol_set(dram_para: &mut dram_parameters) {
    let vol = match dram_para.dram_type {
        DramType::Ddr2 => 47, // 1.8V
        DramType::Ddr3 => 25, // 1.5V
        _ => 0,
    };
    let mut reg = readl(SYS_LDO_CTRL_REG);
    reg &= !(0xff00);
    reg |= vol << 8;
    reg &= !(0x200000);
    writel(SYS_LDO_CTRL_REG, reg);
    sdelay(1);
}

fn set_ddr_voltage(val: usize) -> usize {
    val
}

fn handler_super_standby() {}

fn dram_enable_all_master() {
    writel(DRAM_MASTER_CTL1, 0xffffffff);
    writel(DRAM_MASTER_CTL2, 0xff);
    writel(DRAM_MASTER_CTL3, 0xffff);
    sdelay(10);
}

fn dram_disable_all_master() {
    writel(DRAM_MASTER_CTL1, 1);
    writel(DRAM_MASTER_CTL2, 0);
    writel(DRAM_MASTER_CTL3, 0);
    sdelay(10);
}

// Purpose of this routine seems to be to initialize the PLL driving
// the MBUS and sdram.
//
// should_override: para.dram_tpr13 & (1 << 6) != 0
// overrided_dram_clk: para.dram_tpr9
// origin_dram_clk: para.dram_clk
fn ccm_set_pll_ddr_clk(
    should_override: bool,
    overrided_dram_clk: u32,
    origin_dram_clk: u32,
    ccu: &CCU,
) -> u32 {
    // FIXME: This is a bit weird, especially the scaling down and up etc
    let clk = match should_override {
        true => overrided_dram_clk,
        false => origin_dram_clk,
    };
    let (m0, m1) = (2, 1);
    let n = clk * m0 * m1 / 24;

    let mut val = ccu
        .pll_ddr_control
        .read()
        .mask_pll_output()
        .set_pll_n((n - 1) as u8)
        .set_pll_m1((m1 - 1) as u8)
        .set_pll_m0((m0 - 1) as u8)
        .enable_pll()
        .enable_pll_ldo();
    unsafe { ccu.pll_ddr_control.write(val) };

    // Restart PLL locking
    val = val.disable_lock();
    unsafe { ccu.pll_ddr_control.write(val) };
    val = val.enable_lock();
    unsafe { ccu.pll_ddr_control.write(val) };

    // Wait for PLL to lock
    while !ccu.pll_ddr_control.read().is_locked() {
        core::hint::spin_loop();
    }
    // fixme: should we delay 20 us?

    // Enable PLL output
    unsafe { ccu.pll_ddr_control.modify(|val| val.unmask_pll_output()) };

    // Turn clock gate on.
    unsafe {
        ccu.dram_clock.modify(|val| {
            val.set_clock_source(DramClockSource::PllDdr)
                .set_factor_m(0)
                .set_factor_n(PeriFactorN::N1)
                .unmask_clock()
        });
    }

    n * 24 / (m0 * m1)
}

// Main purpose of sys_init seems to be to initalise the clocks for
// the sdram controller.
// TODO: verify this
fn mctl_sys_init(
    should_override: bool,
    overrided_dram_clk: u32,
    dram_clk: &mut u32,
    ccu: &CCU,
    phy: &PHY,
) {
    // assert MBUS reset
    unsafe { ccu.mbus_clock.modify(|val| val.assert_reset()) };

    // turn off sdram clock gate, assert sdram reset
    unsafe {
        ccu.dram_bgr.modify(|val| val.gate_mask().assert_reset());
        ccu.dram_clock.modify(|val| val.mask_clock());
        ccu.dram_clock.modify(|val| val.unmask_clock());
    }
    sdelay(10);

    // set ddr pll clock
    // NOTE: This passes an additional `0` in the original, but it's unused
    *dram_clk = ccm_set_pll_ddr_clk(should_override, overrided_dram_clk, *dram_clk, &ccu);
    sdelay(100);
    dram_disable_all_master();

    // release sdram reset
    unsafe { ccu.dram_bgr.modify(|val| val.gate_mask().deassert_reset()) };

    // release MBUS reset
    unsafe { ccu.mbus_clock.modify(|val| val.deassert_reset()) };

    // No need to turn back on bit 30

    sdelay(5);

    // turn on sdram clock gate
    unsafe { ccu.dram_bgr.modify(|val| val.gate_pass()) };

    // turn dram clock gate on, trigger sdr clock update
    unsafe {
        // TODO: trigger SDR clock update (bit 27)
        ccu.dram_clock.modify(|val| {
            core::mem::transmute(core::mem::transmute::<_, u32>(val.unmask_clock()) | (0x1 << 27))
        });
    }
    sdelay(5);

    // mCTL clock enable
    unsafe { phy.clken.write(0x00008000) };
    sdelay(10);
}

// Set the Vref mode for the controller
fn mctl_vrefzq_init(para: &mut dram_parameters, phy: &PHY) {
    if (para.dram_tpr13 & (1 << 17)) == 0 {
        unsafe {
            phy.iovcr0.modify(|val| {
                let val = val & 0x80808080;
                val | para.dram_tpr5 as u32
            });
        }

        if (para.dram_tpr13 & (1 << 16)) == 0 {
            unsafe {
                phy.iovcr1.modify(|val| {
                    let val = val & 0xffffff80;
                    val | para.dram_tpr6 as u32 & 0x7f
                });
            }
        }
    }
}

// The main purpose of this routine seems to be to copy an address configuration
// from the dram_para1 and dram_para2 fields to the PHY configuration registers
// (0x3102000, 0x3102004).
fn mctl_com_init(para: &mut dram_parameters, phy: &PHY) {
    // purpose ??
    let mut val = readl(UNKNOWN1) & 0xffffc0ff;
    val |= 0x2000;
    writel(UNKNOWN1, val);

    // Set sdram type and word width
    let mut val = readl(MC_WORK_MODE_RANK0_1) & 0xff000fff;
    val |= ((para.dram_type as u32) & 0x7) << 16; // DRAM type
    val |= (!para.dram_para2 & 0x1) << 12; // DQ width
    if para.dram_type != DramType::Lpddr2 && para.dram_type != DramType::Lpddr3 {
        val |= ((para.dram_tpr13 >> 5) & 0x1) << 19; // 2T or 1T
        val |= 0x400000;
    } else {
        val |= 0x480000; // type 6 and 7 must use 1T
    }
    writel(MC_WORK_MODE_RANK0_1, val);

    // init rank / bank / row for single/dual or two different ranks
    let val = para.dram_para2;
    // ((val & 0x100) && (((val >> 12) & 0xf) != 1)) ? 32 : 16;
    let rank = if (val & 0x100) != 0 && (val >> 12) & 0xf != 1 {
        2
    } else {
        1
    };

    for i in 0..rank {
        let ptr = MC_WORK_MODE_RANK0_1 + i * 4;
        let mut val = readl(ptr) & 0xfffff000;

        val |= (para.dram_para2 >> 12) & 0x3; // rank
        val |= ((para.dram_para1 >> (i * 16 + 12)) << 2) & 0x4; // bank - 2
        val |= (((para.dram_para1 >> (i * 16 + 4)) - 1) << 4) & 0xff; // row - 1

        // convert from page size to column addr width - 3
        val |= match (para.dram_para1 >> i * 16) & 0xf {
            8 => 0xa00,
            4 => 0x900,
            2 => 0x800,
            1 => 0x700,
            _ => 0x600,
        };
        writel(ptr, val);
    }

    // set ODTMAP based on number of ranks in use
    let val = match readl(MC_WORK_MODE_RANK0_1) & 0x1 {
        0 => 0x201,
        _ => 0x303,
    };
    unsafe { phy.odtmap.write(val) };

    // set mctl reg 3c4 to zero when using half DQ
    if para.dram_para2 & (1 << 0) > 0 {
        unsafe { phy.datx[1].gcr.write(0) };
    }

    // purpose ??
    if para.dram_tpr4 > 0 {
        let mut val = readl(MC_WORK_MODE_RANK0_1);
        val |= (para.dram_tpr4 << 25) & 0x06000000;
        writel(MC_WORK_MODE_RANK0_1, val);

        let mut val = readl(MC_WORK_MODE_RANK0_2);
        val |= ((para.dram_tpr4 >> 2) << 12) & 0x001ff000;
        writel(MC_WORK_MODE_RANK0_2, val);
    }
}

fn auto_cal_timing(time: u32, freq: u32) -> u32 {
    let t = time * freq;
    let what = if (t % 1000) != 0 { 1 } else { 0 };
    (t / 1000) + what
}

// Main purpose of the auto_set_timing routine seems to be to calculate all
// timing settings for the specific type of sdram used. Read together with
// an sdram datasheet for context on the various variables.
fn auto_set_timing_para(para: &mut dram_parameters, phy: &PHY) {
    let dfreq = para.dram_clk;
    let dtype = para.dram_type;
    let tpr13 = para.dram_tpr13;

    //// println!("type  = {}\n", dtype);
    //// println!("tpr13 = {}\n", tpr13);

    // FIXME: Half of this is unused, wat?!
    let mut tccd: u32 = 0; // 88(sp)
    let mut trrd: u32 = 0; // s7
    let mut trcd: u32 = 0; // s3
    let mut trc: u32 = 0; // s9
    let mut tfaw: u32 = 0; // s10
    let mut tras: u32 = 0; // s11
    let mut trp: u32 = 0; // 0(sp)
    let mut twtr: u32 = 0; // s1
    let mut twr: u32 = 0; // s6
    let mut trtp: u32 = 0; // 64(sp)
    let mut txp: u32 = 0; // a6
    let mut trefi: u32 = 0; // s2
    let mut trfc: u32 = 0; // a5 / 8(sp)

    if para.dram_tpr13 & 0x2 != 0 {
        //dram_tpr0
        tccd = (para.dram_tpr0 >> 21) & 0x7; // [23:21]
        tfaw = (para.dram_tpr0 >> 15) & 0x3f; // [20:15]
        trrd = (para.dram_tpr0 >> 11) & 0xf; // [14:11]
        trcd = (para.dram_tpr0 >> 6) & 0x1f; // [10:6 ]
        trc = (para.dram_tpr0 >> 0) & 0x3f; // [ 5:0 ]

        //dram_tpr1
        txp = (para.dram_tpr1 >> 23) & 0x1f; // [27:23]
        twtr = (para.dram_tpr1 >> 20) & 0x7; // [22:20]
        trtp = (para.dram_tpr1 >> 15) & 0x1f; // [19:15]
        twr = (para.dram_tpr1 >> 11) & 0xf; // [14:11]
        trp = (para.dram_tpr1 >> 6) & 0x1f; // [10:6 ]
        tras = (para.dram_tpr1 >> 0) & 0x3f; // [ 5:0 ]

        //dram_tpr2
        trfc = (para.dram_tpr2 >> 12) & 0x1ff; // [20:12]
        trefi = (para.dram_tpr2 >> 0) & 0xfff; // [11:0 ]
    } else {
        let frq2 = dfreq >> 1; // s0
        match dtype {
            DramType::Ddr3 => {
                // DDR3
                trfc = auto_cal_timing(350, frq2);
                trefi = auto_cal_timing(7800, frq2) / 32 + 1; // XXX
                twr = auto_cal_timing(8, frq2);
                twtr = if twr < 2 { 2 } else { twr + 2 }; // + 2 ? XXX
                trcd = auto_cal_timing(15, frq2);
                twr = if trcd < 2 { 2 } else { trcd };
                if dfreq <= 800 {
                    tfaw = auto_cal_timing(50, frq2);
                    let trrdc = auto_cal_timing(10, frq2);
                    trrd = if trrd < 2 { 2 } else { trrdc };
                    trc = auto_cal_timing(53, frq2);
                    tras = auto_cal_timing(38, frq2);
                    txp = trrd; // 10
                    trp = trcd; // 15
                }
            }
            DramType::Ddr2 => {}   // TODO
            DramType::Lpddr2 => {} // TODO
            DramType::Lpddr3 => {} // TODO
                                    /*
                                    2 => {
                                        // DDR2
                                        tfaw = auto_cal_timing(50, frq2);
                                        trrd = auto_cal_timing(10, frq2);
                                        trcd = auto_cal_timing(20, frq2);
                                        trc = auto_cal_timing(65, frq2);
                                        twtr = auto_cal_timing(8, frq2);
                                        trp = auto_cal_timing(15, frq2);
                                        tras = auto_cal_timing(45, frq2);
                                        trefi = auto_cal_timing(7800, frq2) / 32;
                                        trfc = auto_cal_timing(328, frq2);
                                        txp = 2;
                                        twr = trp; // 15
                                    }
                                    6 => {
                                        // LPDDR2
                                        tfaw = auto_cal_timing(50, frq2);
                                        if tfaw < 4 {
                                            tfaw = 4
                                        };
                                        trrd = auto_cal_timing(10, frq2);
                                        if trrd == 0 {
                                            trrd = 1
                                        };
                                        trcd = auto_cal_timing(24, frq2);
                                        if trcd < 2 {
                                            trcd = 2
                                        };
                                        trc = auto_cal_timing(70, frq2);
                                        txp = auto_cal_timing(8, frq2);
                                        if txp == 0 {
                                            txp = 1;
                                            twtr = 2;
                                        } else {
                                            twtr = txp;
                                            if txp < 2 {
                                                txp = 2;
                                                twtr = 2;
                                            }
                                        }
                                        twr = auto_cal_timing(15, frq2);
                                        if twr < 2 {
                                            twr = 2
                                        };
                                        trp = auto_cal_timing(17, frq2);
                                        tras = auto_cal_timing(42, frq2);
                                        trefi = auto_cal_timing(3900, frq2) / 32;
                                        trfc = auto_cal_timing(210, frq2);
                                    }
                                    7 => {
                                        // LPDDR3
                                        tfaw = auto_cal_timing(50, frq2);
                                        if tfaw < 4 {
                                            tfaw = 4
                                        };
                                        trrd = auto_cal_timing(10, frq2);
                                        if trrd == 0 {
                                            trrd = 1
                                        };
                                        trcd = auto_cal_timing(24, frq2);
                                        if trcd < 2 {
                                            trcd = 2
                                        };
                                        trc = auto_cal_timing(70, frq2);
                                        twtr = auto_cal_timing(8, frq2);
                                        if twtr < 2 {
                                            twtr = 2
                                        };
                                        twr = auto_cal_timing(15, frq2);
                                        if twr < 2 {
                                            twr = 2
                                        };
                                        trp = auto_cal_timing(17, frq2);
                                        tras = auto_cal_timing(42, frq2);
                                        trefi = auto_cal_timing(3900, frq2) / 32;
                                        trfc = auto_cal_timing(210, frq2);
                                        txp = twtr;
                                    }
                                    _ => {
                                        // default
                                        trfc = 128;
                                        trp = 6;
                                        trefi = 98;
                                        txp = 10;
                                        twr = 8;
                                        twtr = 3;
                                        tras = 14;
                                        tfaw = 16;
                                        trc = 20;
                                        trcd = 6;
                                        trrd = 3;
                                    }
                                    */
        }
        //assign the value back to the DRAM structure
        tccd = 2;
        trtp = 4; // not in .S ?
        para.dram_tpr0 = (trc << 0) | (trcd << 6) | (trrd << 11) | (tfaw << 15) | (tccd << 21);
        para.dram_tpr1 =
            (tras << 0) | (trp << 6) | (twr << 11) | (trtp << 15) | (twtr << 20) | (txp << 23);
        para.dram_tpr2 = (trefi << 0) | (trfc << 12);
    }

    let tcksrx: u32; // t1
    let tckesr: u32; // t4;
    let mut trd2wr: u32; // t6
    let trasmax: u32; // t3;
    let twtp: u32; // s6 (was twr!)
    let tcke: u32; // s8
    let tmod: u32; // t0
    let tmrd: u32; // t5
    let tmrw: u32; // a1
    let t_rdata_en: u32; // a4 (was tcwl!)
    let tcl: u32; // a0
    let wr_latency: u32; // a7
    let tcwl: u32; // first a4, then a5
    let mr3: u32; // s0
    let mr2: u32; // t2
    let mr1: u32; // s1
    let mr0: u32; // a3

    //let dmr3: u32; // 72(sp)
    //let trtp:u32;	// 64(sp)
    //let dmr1: u32; // 56(sp)
    let twr2rd: u32; // 48(sp)
    let tdinit3: u32; // 40(sp)
    let tdinit2: u32; // 32(sp)
    let tdinit1: u32; // 24(sp)
    let tdinit0: u32; // 16(sp)

    let dmr1 = para.dram_mr1;
    let dmr3 = para.dram_mr3;

    match dtype {
        /*
        2 =>
        // DDR2
        //	L59:
        {
            trasmax = dfreq / 30;
            if dfreq < 409 {
                tcl = 3;
                t_rdata_en = 1;
                mr0 = 0x06a3;
            } else {
                t_rdata_en = 2;
                tcl = 4;
                mr0 = 0x0e73;
            }
            tmrd = 2;
            twtp = twr as u32 + 5;
            tcksrx = 5;
            tckesr = 4;
            trd2wr = 4;
            tcke = 3;
            tmod = 12;
            wr_latency = 1;
            mr3 = 0;
            mr2 = 0;
            tdinit0 = 200 * dfreq + 1;
            tdinit1 = 100 * dfreq / 1000 + 1;
            tdinit2 = 200 * dfreq + 1;
            tdinit3 = 1 * dfreq + 1;
            tmrw = 0;
            twr2rd = twtr as u32 + 5;
            tcwl = 0;
            mr1 = dmr1;
        }
        */
        // DramType::Ddr2 => {}, // TODO
        DramType::Ddr3 =>
        // DDR3
        //	L57:
        {
            trasmax = dfreq / 30;
            if dfreq <= 800 {
                mr0 = 0x1c70;
                tcl = 6;
                wr_latency = 2;
                tcwl = 4;
                mr2 = 24;
            } else {
                mr0 = 0x1e14;
                tcl = 7;
                wr_latency = 3;
                tcwl = 5;
                mr2 = 32;
            }

            twtp = tcwl + 2 + twtr as u32; // WL+BL/2+tWTR
            trd2wr = tcwl + 2 + twr as u32; // WL+BL/2+tWR
            twr2rd = tcwl + twtr as u32; // WL+tWTR

            tdinit0 = 500 * dfreq + 1; // 500 us
            tdinit1 = 360 * dfreq / 1000 + 1; // 360 ns
            tdinit2 = 200 * dfreq + 1; // 200 us
            tdinit3 = 1 * dfreq + 1; //   1 us

            mr1 = dmr1;
            t_rdata_en = tcwl; // a5 <- a4
            tcksrx = 5;
            tckesr = 4;
            trd2wr = if ((tpr13 >> 2) & 0x03) == 0x01 || dfreq < 912 {
                5
            } else {
                6
            };
            tcke = 3; // not in .S ?
            tmod = 12;
            tmrd = 4;
            tmrw = 0;
            mr3 = 0;
        }
        // DramType::Lpddr2 => {} // TODO
        /*
        6 =>
        // LPDDR2
        //	L61:
        {
            trasmax = dfreq / 60;
            mr3 = dmr3;
            twtp = twr as u32 + 5;
            mr2 = 6;
            //  mr1 = 5; // TODO: this is just overwritten (?!)
            tcksrx = 5;
            tckesr = 5;
            trd2wr = 10;
            tcke = 2;
            tmod = 5;
            tmrd = 5;
            tmrw = 3;
            tcl = 4;
            wr_latency = 1;
            t_rdata_en = 1;
            tdinit0 = 200 * dfreq + 1;
            tdinit1 = 100 * dfreq / 1000 + 1;
            tdinit2 = 11 * dfreq + 1;
            tdinit3 = 1 * dfreq + 1;
            twr2rd = twtr as u32 + 5;
            tcwl = 2;
            mr1 = 195;
            mr0 = 0;
        }

        7 =>
        // LPDDR3
        {
            trasmax = dfreq / 60;
            if dfreq < 800 {
                tcwl = 4;
                wr_latency = 3;
                t_rdata_en = 6;
                mr2 = 12;
            } else {
                tcwl = 3;
                // tcke = 6; // FIXME: This is always overwritten
                wr_latency = 2;
                t_rdata_en = 5;
                mr2 = 10;
            }
            twtp = tcwl + 5;
            tcl = 7;
            mr3 = dmr3;
            tcksrx = 5;
            tckesr = 5;
            trd2wr = 13;
            tcke = 3;
            tmod = 12;
            tdinit0 = 400 * dfreq + 1;
            tdinit1 = 500 * dfreq / 1000 + 1;
            tdinit2 = 11 * dfreq + 1;
            tdinit3 = 1 * dfreq + 1;
            tmrd = 5;
            tmrw = 5;
            twr2rd = tcwl + twtr as u32 + 5;
            mr1 = 195;
            mr0 = 0;
        }

        _ => {}
        */
        // DramType::Lpddr3 => {} // TODO
        _ =>
        //	L84:
        {
            twr2rd = 8; // 48(sp)
            tcksrx = 4; // t1
            tckesr = 3; // t4
            trd2wr = 4; // t6
            trasmax = 27; // t3
            twtp = 12; // s6
            tcke = 2; // s8
            tmod = 6; // t0
            tmrd = 2; // t5
            tmrw = 0; // a1
            tcwl = 3; // a5
            tcl = 3; // a0
            wr_latency = 1; // a7
            t_rdata_en = 1; // a4
            mr3 = 0; // s0
            mr2 = 0; // t2
            mr1 = 0; // s1
            mr0 = 0; // a3
            tdinit3 = 0; // 40(sp)
            tdinit2 = 0; // 32(sp)
            tdinit1 = 0; // 24(sp)
            tdinit0 = 0; // 16(sp)
        }
    }
    // L60:
    /*
    if trtp < tcl - trp + 2 {
        trtp = tcl - trp + 2;
    }
    */
    // FIXME: This always overwrites the above (?!)
    trtp = 4;

    // Update mode block when permitted
    if (para.dram_mr0 & 0xffff0000) == 0 {
        para.dram_mr0 = mr0
    };
    if (para.dram_mr1 & 0xffff0000) == 0 {
        para.dram_mr1 = mr1
    };
    if (para.dram_mr2 & 0xffff0000) == 0 {
        para.dram_mr2 = mr2
    };
    if (para.dram_mr3 & 0xffff0000) == 0 {
        para.dram_mr3 = mr3
    };

    // Set mode registers
    unsafe {
        phy.mr[0].write(para.dram_mr0);
        phy.mr[1].write(para.dram_mr1);
        phy.mr[2].write(para.dram_mr2);
        phy.mr[3].write(para.dram_mr3);
        // should phy.lp3mr11 be named DRAM_ODTX?
        phy.lp3mr11.write((para.dram_odt_en >> 4) & 0x3); // ??
    }

    // Set dram timing DRAMTMG0 - DRAMTMG5
    unsafe {
        phy.dramtmg[0]
            .write((twtp << 24) | (tfaw << 16) as u32 | (trasmax << 8) | (tras << 0) as u32);
        phy.dramtmg[1].write((txp << 16) as u32 | (trtp << 8) as u32 | (trc << 0) as u32);
        phy.dramtmg[2].write((tcwl << 24) | (tcl << 16) as u32 | (trd2wr << 8) | (twr2rd << 0));
        phy.dramtmg[3].write((tmrw << 16) | (tmrd << 12) | (tmod << 0));
        phy.dramtmg[4].write(
            (trcd << 24) as u32 | (tccd << 16) as u32 | (trrd << 8) as u32 | (trp << 0) as u32,
        );
        phy.dramtmg[5].write((tcksrx << 24) | (tcksrx << 16) | (tckesr << 8) | (tcke << 0));
    }

    // Set two rank timing
    unsafe {
        phy.dramtmg[8].modify(|mut val| {
            val &= 0x0fff0000;
            val |= if para.dram_clk < 800 {
                0xf0006600
            } else {
                0xf0007600
            };
            val |= 0x10;
            val
        })
    };

    // Set phy interface time PITMG0, PTR3, PTR4
    unsafe {
        phy.pitmg[0].write((0x2 << 24) | (t_rdata_en << 16) | (0x1 << 8) | (wr_latency << 0));
        phy.ptr[3].write((tdinit0 << 0) | (tdinit1 << 20));
        phy.ptr[4].write((tdinit2 << 0) | (tdinit3 << 20));
    }

    // Set refresh timing and mode
    unsafe {
        phy.rfshtmg.write((trefi << 16) | (trfc << 0));
        phy.rfshctl1.write(0x0fff0000 & (trefi << 15));
    }
}

fn eye_delay_compensation(para: &mut dram_parameters, phy: &PHY) {
    let mut val: u32;

    // DATn0IOCR
    for i in 0..9 {
        unsafe {
            phy.datx[0].iocr[i].modify(|mut val| {
                val |= (para.dram_tpr11 << 9) & 0x1e00;
                val |= (para.dram_tpr12 << 1) & 0x001e;
                val
            });
        }
    }

    // DATn1IOCR
    for i in 0..9 {
        unsafe {
            phy.datx[1].iocr[i].modify(|mut val| {
                val |= ((para.dram_tpr11 >> 4) << 9) & 0x1e00;
                val |= ((para.dram_tpr12 >> 4) << 1) & 0x001e;
                val
            });
        }
    }

    // PGCR0: assert AC loopback FIFO reset
    unsafe { phy.pgcr[0].modify(|val| val & 0xfbffffff) };

    // ??
    val = readl(0x3103334);
    val |= ((para.dram_tpr11 >> 16) << 9) & 0x1e00;
    val |= ((para.dram_tpr12 >> 16) << 1) & 0x001e;
    writel(0x3103334, val);

    val = readl(0x3103338);
    val |= ((para.dram_tpr11 >> 16) << 9) & 0x1e00;
    val |= ((para.dram_tpr12 >> 16) << 1) & 0x001e;
    writel(0x3103338, val);

    val = readl(0x31033b4);
    val |= ((para.dram_tpr11 >> 20) << 9) & 0x1e00;
    val |= ((para.dram_tpr12 >> 20) << 1) & 0x001e;
    writel(0x31033b4, val);

    val = readl(0x31033b8);
    val |= ((para.dram_tpr11 >> 20) << 9) & 0x1e00;
    val |= ((para.dram_tpr12 >> 20) << 1) & 0x001e;
    writel(0x31033b8, val);

    val = readl(0x310333c);
    val |= ((para.dram_tpr11 >> 16) << 25) & 0x1e000000;
    writel(0x310333c, val);

    val = readl(0x31033bc);
    val |= ((para.dram_tpr11 >> 20) << 25) & 0x1e000000;
    writel(0x31033bc, val);

    // PGCR0: release AC loopback FIFO reset
    unsafe { phy.pgcr[0].modify(|val| val | 0x04000000) };

    sdelay(1);

    // TODO: unknown regs
    // NOTE: dram_tpr10 is set to 0x0 for D1
    for i in 0..15 {
        let ptr = 0x3103240 + i * 4;
        val = readl(ptr);
        val |= ((para.dram_tpr10 >> 4) << 8) & 0x0f00;
        writel(ptr, val);
    }

    for i in 0..6 {
        let ptr = 0x3103228 + i * 4;
        val = readl(ptr);
        val |= ((para.dram_tpr10 >> 4) << 8) & 0x0f00;
        writel(ptr, val);
    }

    let val = readl(0x3103218);
    writel(0x3103218, val | (para.dram_tpr10 << 8) & 0x0f00);
    let val = readl(0x310321c);
    writel(0x310321c, val | (para.dram_tpr10 << 8) & 0x0f00);
    let val = readl(0x3103280);
    writel(0x3103280, val | ((para.dram_tpr10 >> 12) << 8) & 0x0f00);
}

// Init the controller channel. The key part is placing commands in the main
// command register (PIR, 0x3103000) and checking command status (PGSR0, 0x3103010).
fn mctl_channel_init(para: &mut dram_parameters, phy: &PHY) -> Result<(), &'static str> {
    let dqs_gating_mode = (para.dram_tpr13 >> 2) & 0x3;
    let mut val;

    // set DDR clock to half of CPU clock
    val = readl(UNKNOWN7) & 0xfffff000;
    val |= (para.dram_clk >> 1) - 1;
    writel(UNKNOWN7, val);

    // MRCTRL0 nibble 3 undocumented
    val = readl(MRCTRL0) & 0xfffff0ff;
    writel(MRCTRL0, val | 0x300);

    unsafe {
        // DX0GCR0
        phy.datx[0].gcr.modify(|val| {
            let mut val = val & 0xffffffcf;
            val |= ((!para.dram_odt_en) << 5) & 0x20;
            if para.dram_clk > 672 {
                val &= 0xffff09f1;
                val |= 0x00000400;
            } else {
                val &= 0xffff0ff1;
            }
            val
        });
        // DX1GCR0
        phy.datx[1].gcr.modify(|val| {
            let mut val = val & 0xffffffcf;
            val |= ((!para.dram_odt_en) << 5) & 0x20;
            if para.dram_clk > 672 {
                val &= 0xffff09f1;
                val |= 0x00000400;
            } else {
                val &= 0xffff0ff1;
            }
            val
        });
    }

    // 0x3103208 undocumented
    unsafe {
        phy.aciocr0.modify(|val| val | 0x2);
    }

    eye_delay_compensation(para, &phy);

    //set PLL SSCG ?
    val = readl(MRCTRL0);
    const PLL_SSCG_X: usize = 0x31030bc;
    match dqs_gating_mode {
        1 => {
            val &= !(0xc0); // FIXME
            writel(MRCTRL0, val);
            let val = readl(PLL_SSCG_X);
            writel(PLL_SSCG_X, val & 0xfffffef8);
        }
        2 => {
            val &= !(0xc0); // FIXME
            val |= 0x80;
            writel(MRCTRL0, val);

            let mut val = readl(PLL_SSCG_X);
            val &= 0xfffffef8;
            val |= ((para.dram_tpr13 >> 16) & 0x1f) - 2;
            val |= 0x100;
            writel(PLL_SSCG_X, val);

            unsafe {
                phy.dxccr.modify(|val| (val & 0x7fffffff) | 0x08000000);
            }
        }
        _ => {
            val &= !(0x40); // FIXME
            writel(MRCTRL0, val);
            sdelay(10);

            let val = readl(MRCTRL0);
            writel(MRCTRL0, val | 0xc0);
        }
    }

    /*
    if para.dram_type == 6 || para.dram_type == 7 {
        let val = readl(DQS_GATING_X);
        if dqs_gating_mode == 1 {
            val &= 0xf7ffff3f;
            val |= 0x80000000;
        } else {
            val &= 0x88ffffff;
            val |= 0x22000000;
        }
        writel(DQS_GATING_X, val);
    }
    */

    unsafe {
        phy.dtcr.modify(|mut val| {
            val &= 0xf0000000;
            val |= if para.dram_para2 & (1 << 12) > 0 {
                0x03000001
            } else {
                0x01000007
            }; // 0x01003087 XXX
            val
        });
    }

    if readl(SOME_STATUS) & (1 << 16) > 0 {
        val = readl(SOME_OTHER);
        writel(SOME_OTHER, val & 0xfffffffd);
        sdelay(10);
    }

    // Set ZQ config
    unsafe {
        phy.zqcr.modify(|mut val| {
            val = val & 0xfc000000;
            val |= para.dram_zq & 0x00ffffff;
            val |= 0x02000000;
            val
        });
    }

    // Initialise DRAM controller
    val = if dqs_gating_mode == 1 {
        unsafe { phy.pir.write(0x52) }; // prep PHY reset + PLL init + z-cal
        unsafe { phy.pir.write(0x53) }; // Go

        while phy.pgsr[0].read() & 0x1 == 0 {} // wait for IDONE
        sdelay(10);

        // 0x520 = prep DQS gating + DRAM init + d-cal
        if para.dram_type == DramType::Ddr3 {
            0x5a0
        }
        // + DRAM reset
        else {
            0x520
        }
    } else {
        if (readl(SOME_STATUS) & (1 << 16)) == 0 {
            // prep DRAM init + PHY reset + d-cal + PLL init + z-cal
            if para.dram_type == DramType::Ddr3 {
                0x1f2
            }
            // + DRAM reset
            else {
                0x172
            }
        } else {
            // prep PHY reset + d-cal + z-cal
            0x62
        }
    };

    unsafe { phy.pir.write(val) }; // Prep
    unsafe { phy.pir.write(val | 1) }; // Go
    sdelay(10);

    while (phy.pgsr[0].read() & 0x1) == 0 {} // wait for IDONE

    if readl(SOME_STATUS) & (1 << 16) > 0 {
        unsafe {
            phy.pgcr[3].modify(|mut val| {
                val &= 0xf9ffffff;
                val |= 0x04000000;
                val
            })
        };
        sdelay(10);

        unsafe { phy.pwrctl.modify(|val| val | 0x1) };
        while (phy.statr.read() & 0x7) != 0x3 {}

        val = readl(SOME_OTHER);
        writel(SOME_OTHER, val & 0xfffffffe);
        sdelay(10);

        unsafe { phy.pwrctl.modify(|val| val & 0xfffffffe) };
        while (phy.statr.read() & 0x7) != 0x1 {}
        sdelay(15);

        if dqs_gating_mode == 1 {
            val = readl(MRCTRL0);
            val &= 0xffffff3f;
            writel(MRCTRL0, val);

            unsafe {
                phy.pgcr[3].modify(|mut val| {
                    val &= 0xf9ffffff;
                    val |= 0x02000000;
                    val
                })
            };

            sdelay(1);
            unsafe { phy.pir.write(0x401) };

            while (phy.pgsr[0].read() & 0x1) == 0 {}
        }
    }

    // Check for training error
    val = phy.pgsr[0].read();
    if ((val >> 20) & 0xff != 0) && (val & 0x100000) != 0 {
        // return Err("DRAM initialisation error : 0"); // TODO
        return Err("ZQ calibration error, check external 240 ohm resistor.");
    }

    // STATR = Zynq STAT? Wait for status 'normal'?
    while (phy.statr.read() & 0x1) == 0 {}

    unsafe { phy.rfshctl0.modify(|val| val | 0x80000000) };
    sdelay(10);
    unsafe { phy.rfshctl0.modify(|val| val & 0x7fffffff) };
    sdelay(10);
    val = readl(UNKNOWN12);
    writel(UNKNOWN12, val | 0x80000000);
    sdelay(10);
    unsafe {
        phy.pgcr[3].modify(|val| val & 0xf9ffffff);
    }

    if dqs_gating_mode == 1 {
        unsafe { phy.dxccr.modify(|val| (val & 0xffffff3f) | 0x00000040) };
    }
    Ok(())
}

// FIXME: Cannot you see that this could be more elegant?
// Perform an init of the controller. This is actually done 3 times. The first
// time to establish the number of ranks and DQ width. The second time to
// establish the actual ram size. The third time is final one, with the final
// settings.
fn mctl_core_init(para: &mut dram_parameters, ccu: &CCU, phy: &PHY) -> Result<(), &'static str> {
    let should_override = para.dram_tpr13 & (1 << 6) != 0;
    let overrided_dram_clk = para.dram_tpr9;
    mctl_sys_init(
        should_override,
        overrided_dram_clk,
        &mut para.dram_clk,
        &ccu,
        &phy,
    );
    mctl_vrefzq_init(para, &phy);
    mctl_com_init(para, &phy);
    unsafe {
        mctl_phy_ac_remapping(para);
    }
    auto_set_timing_para(para, &phy);
    mctl_channel_init(para, &phy)
}

// The below routine reads the dram config registers and extracts
// the number of address bits in each rank available. It then calculates
// total memory size in MB.
fn dramc_get_dram_size() -> u32 {
    // MC_WORK_MODE0 (not MC_WORK_MODE, low word)
    let low = readl(MC_WORK_MODE_RANK0_1);

    let mut temp = (low >> 8) & 0xf; // page size - 3
    temp += (low >> 4) & 0xf; // row width - 1
    temp += (low >> 2) & 0x3; // bank count - 2
    temp -= 14; // 1MB = 20 bits, minus above 6 = 14
    let size0 = 1 << temp;
    // // println!("low {} size0 {}", low, size0);

    temp = low & 0x3; // rank count = 0? -> done
    if temp == 0 {
        return size0;
    }

    // MC_WORK_MODE1 (not MC_WORK_MODE, high word)
    let high = readl(MC_WORK_MODE_RANK0_2);

    temp = high & 0x3;
    if temp == 0 {
        // two identical ranks
        return 2 * size0;
    }

    temp = (high >> 8) & 0xf; // page size - 3
    temp += (high >> 4) & 0xf; // row width - 1
    temp += (high >> 2) & 0x3; // bank number - 2
    temp -= 14; // 1MB = 20 bits, minus above 6 = 14
    let size1 = 1 << temp;
    // // println!("high {} size1 {}", high, size1);

    return size0 + size1; // add size of each rank
}

// The below routine reads the command status register to extract
// DQ width and rank count. This follows the DQS training command in
// channel_init. If error bit 22 is reset, we have two ranks and full DQ.
// If there was an error, figure out whether it was half DQ, single rank,
// or both. Set bit 12 and 0 in dram_para2 with the results.
fn dqs_gate_detect(para: &mut dram_parameters, phy: &PHY) -> Result<&'static str, &'static str> {
    if phy.pgsr[0].read() & (1 << 22) != 0 {
        let dx0 = (phy.datx[0].gsr0.read() >> 24) & 0x3;
        let dx1 = (phy.datx[1].gsr0.read() >> 24) & 0x3;

        if dx0 == 2 {
            let mut rval = para.dram_para2;
            rval &= 0xffff0ff0;
            if dx0 != dx1 {
                rval |= 0x1;
                para.dram_para2 = rval;
                return Ok("[AUTO DEBUG] single rank and half DQ!");
            }
            para.dram_para2 = rval;
            // NOTE: D1 should do this here
            return Ok("single rank, full DQ");
        } else if dx0 == 0 {
            let mut rval = para.dram_para2;
            rval &= 0xfffffff0; // l 7920
            rval |= 0x00001001; // l 7918
            para.dram_para2 = rval;
            return Ok("dual rank, half DQ!");
        } else {
            if para.dram_tpr13 & (1 << 29) != 0 {
                // l 7935
                // // println!("DX0 {}", dx0);
                // // println!("DX1 {}", dx1);
            }
            return Err("dqs gate detect");
        }
    } else {
        let mut rval = para.dram_para2;
        rval &= 0xfffffff0;
        rval |= 0x00001000;
        para.dram_para2 = rval;
        return Ok("dual rank, full DQ");
    }
}

fn dramc_simple_wr_test(mem_mb: u32, len: u32) -> Result<(), &'static str> {
    let offs: usize = (mem_mb as usize >> 1) << 18; // half of memory size
    let patt1: u32 = 0x01234567;
    let patt2: u32 = 0xfedcba98;

    for i in 0..len {
        let addr = RAM_BASE + 4 * i as usize;
        writel(addr, patt1 + i);
        writel(addr + offs, patt2 + i);
    }

    for i in 0..len {
        let addr = RAM_BASE + 4 * i as usize;
        let val = readl(addr);
        let exp = patt1 + i;
        if val != exp {
            // // println!("{:x} != {:x} at address {:x}", val, exp, addr);
            return Err("base");
        }
        let val = readl(addr + offs);
        let exp = patt2 + i;
        if val != exp {
            // // println!("{:x} != {:x} at address {:x}", val, exp, addr + offs);
            return Err("offs");
        }
    }
    Ok(())
}

// Autoscan sizes a dram device by cycling through address lines and figuring
// out if it is connected to a real address line, or if the address is a mirror.
// First the column and bank bit allocations are set to low values (2 and 9 address
// lines. Then a maximum allocation (16 lines) is set for rows and this is tested.
// Next the BA2 line is checked. This seems to be placed above the column, BA0-1 and
// row addresses. Finally, the column address is allocated 13 lines and these are
// tested. The results are placed in dram_para1 and dram_para2.
fn auto_scan_dram_size(
    para: &mut dram_parameters,
    ccu: &CCU,
    phy: &PHY,
) -> Result<(), &'static str> {
    mctl_core_init(para, &ccu, &phy)?;

    // write test pattern
    for i in 0..64 {
        let ptr = RAM_BASE + 4 * i;
        let val = if i & 1 > 0 { ptr } else { !ptr };
        writel(ptr, val as u32);
    }

    let maxrank = if para.dram_para2 & 0xf000 == 0 { 1 } else { 2 };
    let mut mc_work_mode = MC_WORK_MODE_RANK0_1;
    let mut offs = 0;

    // Scan per address line, until address wraps (i.e. see shadow)
    fn scan_for_addr_wrap() -> u32 {
        for i in 11..17 {
            let mut done = true;
            for j in 0..64 {
                let ptr = RAM_BASE + j * 4;
                let chk = ptr + (1 << (i + 11));
                let exp = if j & 1 != 0 { ptr } else { !ptr };
                if readl(chk) != exp as u32 {
                    done = false;
                    break;
                }
            }
            if done {
                return i;
            }
        }
        return 16;
    }

    // Scan per address line, until address wraps (i.e. see shadow)
    fn scan_for_addr_wrap2() -> u32 {
        for i in 9..15 {
            let mut done = true;
            for j in 0..64 {
                let ptr = RAM_BASE + j * 4;
                let chk = ptr + (1 << i);
                let exp = if j & 1 != 0 { ptr } else { !ptr };
                if readl(chk) != exp as u32 {
                    done = false;
                    break;
                }
            }
            if done {
                return i;
            }
        }
        return 13;
    }

    for rank in 0..maxrank {
        // Set row mode
        let mut rval = readl(mc_work_mode);
        rval &= 0xfffff0f3;
        rval |= 0x000006f0;
        writel(mc_work_mode, rval);
        while readl(mc_work_mode) != rval {}
        let i = scan_for_addr_wrap();

        if VERBOSE {
            // println!("rank {} row = {}", rank, i);
        }

        // Store rows in para 1
        let shft = 4 + offs;
        rval = para.dram_para1;
        rval &= !(0xff << shft);
        rval |= i << shft;
        para.dram_para1 = rval;

        if rank == 1 {
            // Set bank mode for rank0
            rval = readl(MC_WORK_MODE_RANK0_1);
            rval &= 0xfffff003;
            rval |= 0x000006a4;
            writel(MC_WORK_MODE_RANK0_1, rval);
        }

        // Set bank mode for current rank
        rval = readl(mc_work_mode);
        rval &= 0xfffff003;
        rval |= 0x000006a4;
        writel(mc_work_mode, rval);
        while readl(mc_work_mode) != rval {}

        // Test if bit A23 is BA2 or mirror XXX A22?
        let mut j = 0;
        for i in 0..63 {
            // where to check
            let chk = RAM_BASE + (1 << 22) + i * 4;
            // pattern
            let ptr = RAM_BASE + i * 4;
            // expected value
            let exp = (if i & 1 != 0 { ptr } else { !ptr }) as u32;
            if readl(chk) != exp {
                j = 1;
                break;
            }
        }
        // let banks = (j + 1) << 2; // 4 or 8
        // if VERBOSE {
        //     // println!("rank {} bank = {}", rank, banks);
        // }

        // Store banks in para 1
        let shft = 12 + offs;
        rval = para.dram_para1;
        rval &= !(0xf << shft);
        rval |= j << shft;
        para.dram_para1 = rval;

        if rank == 1 {
            // Set page mode for rank0
            rval = readl(MC_WORK_MODE_RANK0_1);
            rval &= 0xfffff003;
            rval |= 0x00000aa0;
            writel(MC_WORK_MODE_RANK0_1, rval);
        }

        // Set page mode for current rank
        rval = readl(mc_work_mode);
        rval &= 0xfffff003;
        rval |= 0x00000aa0;
        writel(mc_work_mode, rval);
        while readl(mc_work_mode) != rval {}

        let i = scan_for_addr_wrap2();
        let pgsize = if i == 9 { 0 } else { 1 << (i - 10) };

        if VERBOSE {
            // println!("rank {} page size = {}KB", rank, pgsize);
        }

        // Store page size
        let shft = offs;
        rval = para.dram_para1;
        rval &= !(0xf << shft);
        rval |= pgsize << shft;
        para.dram_para1 = rval;

        // FIXME: should not be here; those loops are pretty messed up
        {
            rval = readl(MC_WORK_MODE_RANK1_1); // MC_WORK_MODE
            rval &= 0xfffff003;
            rval |= 0x000006f0;
            writel(MC_WORK_MODE_RANK1_1, rval);

            rval = readl(MC_WORK_MODE_RANK1_2); // MC_WORK_MODE2
            rval &= 0xfffff003;
            rval |= 0x000006f0;
            writel(MC_WORK_MODE_RANK1_2, rval);
        }

        // Move to next rank
        if rank != maxrank {
            if rank == 1 {
                rval = readl(MC_WORK_MODE_RANK1_1); // MC_WORK_MODE
                rval &= 0xfffff003;
                rval |= 0x000006f0;
                writel(MC_WORK_MODE_RANK1_1, rval);

                rval = readl(MC_WORK_MODE_RANK1_2); // MC_WORK_MODE2
                rval &= 0xfffff003;
                rval |= 0x000006f0;
                writel(MC_WORK_MODE_RANK1_2, rval);
            }
            offs += 16; // store rank1 config in upper half of para1
            mc_work_mode += 4; // move to MC_WORK_MODE2
        }
    }
    /*
    if (maxrank == 2) {
        para->dram_para2 &= 0xfffff0ff;
        // note: rval is equal to para->dram_para1 here
        if ((rval & 0xffff) == ((rval >> 16) & 0xffff)) {
            printf("rank1 config same as rank0\n");
        }
        else {
            para->dram_para2 |= 0x00000100;
            printf("rank1 config different from rank0\n");
        }
    }
    */

    Ok(())
}

// This routine sets up parameters with dqs_gating_mode equal to 1 and two
// ranks enabled. It then configures the core and tests for 1 or 2 ranks and
// full or half DQ width. it then resets the parameters to the original values.
// dram_para2 is updated with the rank & width findings.
fn auto_scan_dram_rank_width(
    para: &mut dram_parameters,
    ccu: &CCU,
    phy: &PHY,
) -> Result<(), &'static str> {
    let s1 = para.dram_tpr13;
    let s2 = para.dram_para1;

    para.dram_para1 = 0x00b000b0;
    para.dram_para2 = (para.dram_para2 & 0xfffffff0) | 0x1000;
    para.dram_tpr13 = (s1 & 0xfffffff7) | 0x5; // set DQS probe mode

    mctl_core_init(para, &ccu, &phy)?;

    if phy.pgsr[0].read() & (1 << 20) != 0 {
        return Err("auto scan rank/width");
    }
    // TODO: print success message
    dqs_gate_detect(para, &phy)?;

    para.dram_tpr13 = s1;
    para.dram_para1 = s2;
    Ok(())
}

/* STEP 2 */
/// This routine determines the SDRAM topology.
///
/// It first establishes the number of ranks and the DQ width. Then it scans the
/// SDRAM address lines to establish the size of each rank. It then updates
/// `dram_tpr13` to reflect that the sizes are now known: a re-init will not
/// repeat the autoscan.
fn auto_scan_dram_config(
    para: &mut dram_parameters,
    ccu: &CCU,
    phy: &PHY,
) -> Result<(), &'static str> {
    if para.dram_tpr13 & (1 << 14) == 0 {
        auto_scan_dram_rank_width(para, &ccu, &phy)?
    }
    if para.dram_tpr13 & (1 << 0) == 0 {
        auto_scan_dram_size(para, &ccu, &phy)?
    }
    if (para.dram_tpr13 & (1 << 15)) == 0 {
        para.dram_tpr13 |= 0x6003;
    }
    Ok(())
}

/// # Safety
///
/// No warranty. Use at own risk. Be lucky to get values from vendor.
pub fn init_dram(para: &mut dram_parameters, ccu: &CCU, phy: &PHY) -> usize {
    // STEP 1: ZQ, gating, calibration and voltage
    // Test ZQ status
    if para.dram_tpr13 & (1 << 16) > 0 {
        if VERBOSE {
            // println!("DRAM only has internal ZQ.");
        }
        writel(RES_CAL_CTRL_REG, readl(RES_CAL_CTRL_REG) | 0x100);
        writel(RES240_CTRL_REG, 0);
        sdelay(10);
    } else {
        writel(ANALOG_SYS_PWROFF_GATING_REG, 0); // 0x7010000 + 0x254; l 9655
        writel(RES_CAL_CTRL_REG, readl(RES_CAL_CTRL_REG) & !0x003);
        sdelay(10);
        writel(RES_CAL_CTRL_REG, readl(RES_CAL_CTRL_REG) & !0x108);
        sdelay(10);
        writel(RES_CAL_CTRL_REG, readl(RES_CAL_CTRL_REG) | 0x001);
        sdelay(20);
        // if VERBOSE {
        //     let zq_val = readl(ZQ_VALUE);
        //     // println!("ZQ: {}", zq_val);
        // }
    }

    // Set voltage
    let rc = get_pmu_exists();
    if VERBOSE {
        // println!("PMU exists? {}", rc);
    }

    if !rc {
        dram_vol_set(para);
    } else {
        if para.dram_type == DramType::Ddr2 {
            set_ddr_voltage(1800);
        } else if para.dram_type == DramType::Ddr3 {
            set_ddr_voltage(1500);
        }
    }

    // STEP 2: CONFIG
    // Set SDRAM controller auto config
    if (para.dram_tpr13 & 0x1) == 0 {
        if let Err(_msg) = auto_scan_dram_config(para, &ccu, &phy) {
            // println!("config fail {}", msg);
            return 0;
        }
    }

    // let dtype = match para.dram_type {
    //     DramType::Ddr2 => "DDR2",
    //     DramType::Ddr3 => "DDR3",
    //     _ => "",
    // };
    // println!("{}@{}MHz", dtype, para.dram_clk);

    if VERBOSE {
        if (para.dram_odt_en & 0x1) == 0 {
            // println!("ODT off");
        } else {
            // println!("ZQ: {}", para.dram_zq);
        }
    }

    if VERBOSE {
        // report ODT
        if (para.dram_mr1 & 0x44) == 0 {
            // println!("ODT off");
        } else {
            // println!("ODT: {}", para.dram_mr1);
        }
    }

    // Init core, final run
    if let Err(msg) = mctl_core_init(para, &ccu, &phy) {
        // println!("init error {}", msg);
        return 0;
    };

    // Get sdram size
    let mut rc: u32 = para.dram_para2;
    if rc != 0 {
        rc = (rc & 0x7fff0000) >> 16;
    } else {
        rc = dramc_get_dram_size();
        para.dram_para2 = (para.dram_para2 & 0xffff) | rc << 16;
    }
    let mem_size = rc;
    if VERBOSE {
        // println!("DRAM: {}M", mem_size);
    }

    // Purpose ??
    // What is Auto SR?
    if para.dram_tpr13 & (1 << 30) != 0 {
        let rc = readl(para.dram_tpr8 as usize);
        unsafe {
            phy.asrtc.write(if rc == 0 { 0x10000200 } else { rc });
            phy.asrc.write(0x40a);
            phy.pwrctl.modify(|val| val | 0x1)
        };
        // // println!("Enable Auto SR");
    } else {
        unsafe {
            phy.asrtc.modify(|val| val & 0xffff0000);
            phy.pwrctl.modify(|val| val & !0x1)
        };
    }

    // Purpose ??

    rc = phy.pgcr[0].read() & !(0xf000);
    if (para.dram_tpr13 & 0x200) == 0 {
        if para.dram_type != DramType::Lpddr2 {
            unsafe { phy.pgcr[0].write(rc) };
        }
    } else {
        unsafe { phy.pgcr[0].write(rc | 0x5000) };
    }

    unsafe {
        phy.zqcr.modify(|val| val | (1 << 31));
    }
    if para.dram_tpr13 & (1 << 8) != 0 {
        writel(0x31030b8, phy.zqcr.read() | 0x300);
    }

    let mut rc = readl(MRCTRL0);
    if para.dram_tpr13 & (1 << 16) != 0 {
        rc &= 0xffffdfff;
    } else {
        rc |= 0x00002000;
    }
    writel(MRCTRL0, rc);

    // Purpose ??
    if para.dram_type == DramType::Lpddr3 {
        let rc = phy.odtcfg.read() & 0xfff0ffff;
        unsafe { phy.odtcfg.write(rc | 0x0001000) }
    }

    dram_enable_all_master();

    let len = 4096; // NOTE: a commented call outside the if uses 64 in C code
    if para.dram_tpr13 & (1 << 28) != 0 {
        rc = readl(SOME_STATUS);
        if rc & (1 << 16) != 0 {
            return 0;
        }
        if let Err(msg) = dramc_simple_wr_test(mem_size, len) {
            // println!("test fail {}", msg);
            return 0;
        }
        // println!("test OK");
    }

    handler_super_standby();

    mem_size as usize
}

pub fn init(ccu: &CCU, phy: &PHY) -> usize {
    // taken from SPL
    #[rustfmt::skip]
    let mut dram_para: dram_parameters = dram_parameters {
        dram_clk:            792,
        dram_type:   DramType::Ddr3,
        dram_zq:     0x007b_7bfb,
        dram_odt_en: 0x0000_0001,
        #[cfg(feature="nezha")]
        dram_para1:  0x0000_10f2,
        #[cfg(feature="lichee")]
        dram_para1:  0x0000_10d2,
        dram_para2:  0x0000_0000,
        dram_mr0:    0x0000_1c70,
        dram_mr1:    0x0000_0042,
        #[cfg(feature="nezha")]
        dram_mr2:    0x0000_0000,
        #[cfg(feature="lichee")]
        dram_mr2:    0x0000_0018,
        dram_mr3:    0x0000_0000,
        dram_tpr0:   0x004a_2195,
        dram_tpr1:   0x0242_3190,
        dram_tpr2:   0x0008_b061,
        dram_tpr3:   0xb478_7896,
        dram_tpr4:   0x0000_0000,
        dram_tpr5:   0x4848_4848,
        dram_tpr6:   0x0000_0048,
        dram_tpr7:   0x1620_121e,
        dram_tpr8:   0x0000_0000,
        dram_tpr9:   0x0000_0000,
        dram_tpr10:  0x0000_0000,
        #[cfg(feature="nezha")]
        dram_tpr11:  0x0076_0000,
        #[cfg(feature="lichee")]
        dram_tpr11:  0x0087_0000,
        #[cfg(feature="nezha")]
        dram_tpr12:  0x0000_0035,
        #[cfg(feature="lichee")]
        dram_tpr12:  0x0000_0024,
        #[cfg(feature="nezha")]
        dram_tpr13:  0x3405_0101,
        #[cfg(feature="lichee")]
        dram_tpr13:  0x3405_0100,
    };

    // // println!("DRAM INIT");
    return init_dram(&mut dram_para, &ccu, &phy);
}
