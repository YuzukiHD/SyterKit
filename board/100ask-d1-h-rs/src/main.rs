#![no_std]
#![no_main]

use allwinner_hal::smhc::{RegisterBlock, SdCard, Smhc};
use core::{convert::Infallible, ptr::addr_of, slice::from_raw_parts_mut};
use embedded_cli::{
    cli::{CliBuilder, CliHandle},
    Command,
};
use embedded_io::Read;
use panic_halt as _;
use syterkit::{
    clock_dump, entry, load_from_sdcard, prelude::*, print, println, show_banner, Clocks, Config,
    DynamicInfo, Peripherals, SdCardError, Stdout,
};

#[derive(Command)]
enum Base<'a> {
    /// Get/set bootargs for kernel.
    Bootargs,
    /// Rescan TF Card and reload DTB, Kernel Image.
    Reload,
    /// Print out env config.
    Print,
    /// Boot to Linux.
    Boot,
    // TODO: parse command name into 'read32', other than 'read-32'
    /// Read 32-bit value from device register or memory.
    Read32 {
        /// The physical address this command would read.
        address: &'a str,
    },
    /// Write 32-bit value to device register or memory.
    Write32 {
        /// The physical address this command would write into.
        address: &'a str,
        /// The 32-bit data this command would use.
        data: &'a str,
    },
    /// Dumps memory region in hex.
    Hexdump {
        /// The physical address this command would start to dump.
        address: &'a str,
        /// Number of dumped bytes.
        length: &'a str,
    },
}

struct Device<S: AsRef<RegisterBlock>, P> {
    smhc: Smhc<S, P>,
}

#[entry] // This macro would initialize system clocks.
fn main(p: Peripherals, c: Clocks) {
    // Display the bootloader banner.
    show_banner();

    // Initialize the DRAM.
    let dram_size = syterkit::mctl::init(&p.ccu, &p.phy);
    println!("DRAM size: {}M 🐏", dram_size);

    // Dump information about the system clocks.
    clock_dump(&p.ccu);

    // SD/MMC init.
    println!("initialize sdmmc pins...");
    let pads = {
        let sdc0_d1 = p.gpio.pf0.into_function::<2>();
        let sdc0_d0 = p.gpio.pf1.into_function::<2>();
        let sdc0_clk = p.gpio.pf2.into_function::<2>();
        let sdc0_cmd = p.gpio.pf3.into_function::<2>();
        let sdc0_d3 = p.gpio.pf4.into_function::<2>();
        let sdc0_d2 = p.gpio.pf5.into_function::<2>();
        (sdc0_d1, sdc0_d0, sdc0_clk, sdc0_cmd, sdc0_d3, sdc0_d2)
    };

    println!("initialize SMHC device...");
    let smhc = Smhc::new::<0>(&p.smhc0, pads, &c, &p.ccu);
    let mut d = Device { smhc };
    let mut config = Config::default();

    println!("initializing SD card...");
    let sdcard = match SdCard::new(&mut d.smhc) {
        Ok(val) => val,
        Err(e) => {
            println!("SD card init failed with error {:?}", e);
            run_cli(&mut d, &mut config);
        }
    };
    let size_gb = sdcard.get_size_kb() / 1024.0 / 1024.0;
    println!("SD card initialized, size: {:.2}GB", size_gb);

    let opaque_dst = unsafe { from_raw_parts_mut(0x4100_8000 as *mut u8, 64 * 1024) };
    let firmware_dst = unsafe { from_raw_parts_mut(0x4100_0000 as *mut u8, 32 * 1024) };
    let next_stage_dst = unsafe { from_raw_parts_mut(0x4180_0000 as *mut u8, 512 * 1024 * 1024) };
    let ans = load_from_sdcard(
        sdcard,
        syterkit::time_source(),
        &mut config,
        opaque_dst,
        firmware_dst,
        next_stage_dst,
    );

    match ans {
        Ok(_) => {}
        Err(SdCardError::OpenVolume(e)) => {
            println!("Failed to initialize SD card: {:?}", e);
            run_cli(&mut d, &mut config);
        }
        Err(SdCardError::OpenRootDir(e)) => {
            println!("Failed to initialize SD card: {:?}", e);
            run_cli(&mut d, &mut config);
        }
        Err(SdCardError::CloseRootDir(e)) => {
            println!("Failed to initialize SD card: {:?}", e);
            run_cli(&mut d, &mut config);
        }
        Err(SdCardError::ParseConfig(e)) => {
            println!("Failed to initialize SD card: {:?}", e);
            run_cli(&mut d, &mut config);
        }
        Err(SdCardError::LoadFile(e)) => {
            println!("Failed to initialize SD card: {:?}", e);
            run_cli(&mut d, &mut config);
        }
    };

    // Run payload.
    run_payload(&config);
}

static mut DYNAMIC_INFO: DynamicInfo = DynamicInfo::new();

/// Executes the loaded payload
fn run_payload(config: &Config) -> ! {
    const IMAGE_ADDRESS: usize = 0x4180_0000; // Load address of Linux Image
    const DTB_ADDRESS: usize = 0x4100_8000; // Address of the device tree blob
    const HART_ID: usize = 0; // Hartid of the current core
    unsafe {
        DYNAMIC_INFO = DYNAMIC_INFO
            .with_next_stage(config.next_stage.mode, IMAGE_ADDRESS)
            .with_boot_hart(HART_ID)
    };

    type KernelEntry =
        unsafe extern "C" fn(hart_id: usize, dtb_addr: usize, dynamic_info: *const DynamicInfo);
    let kernel_entry: KernelEntry = unsafe { core::mem::transmute(IMAGE_ADDRESS) };
    let opaque_address = config.opaque.as_ref().map_or(0, |_| DTB_ADDRESS);

    unsafe {
        kernel_entry(HART_ID, opaque_address, addr_of!(DYNAMIC_INFO));
    }

    loop {}
}

fn run_cli<S: AsRef<RegisterBlock>, P>(d: &mut Device<S, P>, config: &mut Config) -> ! {
    let (command_buffer, history_buffer) = ([0; 128], [0; 128]);
    let Ok(mut cli) = CliBuilder::default()
        .writer(syterkit::stdout())
        .command_buffer(command_buffer)
        .history_buffer(history_buffer)
        .prompt("SyterKit> ")
        .build();
    let mut stdin = syterkit::stdin();
    loop {
        let mut slice = [0];
        stdin.read(&mut slice).ok();
        let _ = cli.process_byte::<Base, _>(
            slice[0],
            &mut Base::processor(|cli, command| {
                match command {
                    Base::Bootargs => command_bootargs(cli),
                    Base::Reload => command_reload(cli, d, config),
                    Base::Boot => command_boot(cli, config),
                    Base::Print => command_print(cli),
                    Base::Read32 { address } => command_read32(cli, address),
                    Base::Write32 { address, data } => command_write32(cli, address, data),
                    Base::Hexdump { address, length } => command_hexdump(cli, address, length),
                }
                Ok(())
            }),
        );
    }
}

fn command_bootargs<'a>(_cli: &mut CliHandle<'a, Stdout, Infallible>) {
    println!("TODO Bootargs");
}

fn command_reload<'a, S: AsRef<RegisterBlock>, P>(
    _cli: &mut CliHandle<'a, Stdout, Infallible>,
    d: &mut Device<S, P>,
    config: &mut Config,
) {
    let sdcard = match SdCard::new(&mut d.smhc) {
        Ok(val) => val,
        Err(e) => {
            println!("SD card init failed with error {:?}", e);
            return;
        }
    };

    let opaque_dst = unsafe { from_raw_parts_mut(0x4100_8000 as *mut u8, 64 * 1024) };
    let firmware_dst = unsafe { from_raw_parts_mut(0x4100_0000 as *mut u8, 32 * 1024) };
    let next_stage_dst = unsafe { from_raw_parts_mut(0x4180_0000 as *mut u8, 512 * 1024 * 1024) };
    let _ = load_from_sdcard(
        sdcard,
        syterkit::time_source(),
        config,
        opaque_dst,
        firmware_dst,
        next_stage_dst,
    );

    println!("SD card reload succeeded");
}

fn command_boot<'a>(_cli: &mut CliHandle<'a, Stdout, Infallible>, config: &mut Config) {
    run_payload(config);
}

fn command_print<'a>(cli: &mut CliHandle<'a, Stdout, Infallible>) {
    ufmt::uwrite!(cli.writer(), "TODO Print").ok();
}

fn command_read32<'a, 'b>(_cli: &mut CliHandle<'a, Stdout, Infallible>, address: &'b str) {
    let address: usize = match parse_value(address.trim()) {
        Some(address) => address,
        None => {
            println!("error: invalid address, shoule be hexadecimal like 0x40000000, or decimal like 1073741824");
            return;
        }
    };
    let result = unsafe { core::ptr::read_volatile(address as *const u32) };
    println!("Value at address 0x{:x}: 0x{:08x}.", address, result);
}

fn command_write32<'a, 'b>(
    _cli: &mut CliHandle<'a, Stdout, Infallible>,
    address: &'b str,
    data: &'b str,
) {
    let address: usize = match parse_value(address.trim()) {
        Some(address) => address,
        None => {
            println!("error: invalid address, shoule be hexadecimal like 0x40000000, or decimal like 1073741824");
            return;
        }
    };
    let data: u32 = match parse_value(data.trim()) {
        Some(address) => address,
        None => {
            println!("error: invalid data, shoule be hexadecimal like 0x40000000, or decimal like 1073741824");
            return;
        }
    };
    unsafe { core::ptr::write_volatile(address as *mut u32, data) };
    println!("Wrote 0x{:08x} to address 0x{:x}.", data, address);
}

fn command_hexdump<'a, 'b>(
    _cli: &mut CliHandle<'a, Stdout, Infallible>,
    address: &'b str,
    length: &'b str,
) {
    let address: usize = match parse_value(address.trim()) {
        Some(address) => address,
        None => {
            println!("error: invalid address, shoule be hexadecimal like 0x40000000, or decimal like 1073741824");
            return;
        }
    };
    let length: usize = match parse_value(length.trim()) {
        Some(address) => address,
        None => {
            println!("error: invalid data, shoule be hexadecimal like 0x40000000, or decimal like 1073741824");
            return;
        }
    };
    for i in (address..address + length).step_by(16) {
        print!("0x{:x}:\t", i);
        let mut data = [0u8; 16];
        for j in (0..16).step_by(4) {
            let address = i + j;
            let word: u32 = unsafe { core::ptr::read_volatile(address as *const u32) };
            data[j..j + 4].copy_from_slice(&word.to_le_bytes());
            print!("{:08x} ", word);
        }
        for byte in data {
            if byte.is_ascii_graphic() || byte == b' ' {
                print!("{}", byte as char);
            } else {
                print!(".");
            }
        }
        println!()
    }
}

fn parse_value<T: core::str::FromStr + num_traits::Num>(value: &str) -> Option<T> {
    if let Some(zero_x_prefix_stripped) = value.strip_prefix("0x") {
        T::from_str_radix(zero_x_prefix_stripped, 16).ok()
    } else {
        value.parse::<T>().ok()
    }
}
