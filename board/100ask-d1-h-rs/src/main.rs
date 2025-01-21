#![no_std]
#![no_main]

use core::convert::Infallible;

use allwinner_hal::smhc::{RegisterBlock, SdCard, SdCardError, Smhc};
use embedded_cli::{
    cli::{CliBuilder, CliHandle},
    Command,
};
use embedded_io::Read;
use embedded_sdmmc::VolumeManager;
use panic_halt as _;
use syterkit::{clock_dump, entry, print, println, show_banner, Clocks, Peripherals, Stdout};

#[derive(Command)]
enum Base<'a> {
    /// Get/set bootargs for kernel.
    Bootargs,
    /// Rescan TF Card and reload DTB, Kernel zImage.
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

struct MyTimeSource {}

impl embedded_sdmmc::TimeSource for MyTimeSource {
    fn get_timestamp(&self) -> embedded_sdmmc::Timestamp {
        // TODO
        embedded_sdmmc::Timestamp::from_calendar(2023, 1, 1, 0, 0, 0).unwrap()
    }
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

    println!("initialize smhc...");
    let mut smhc = Smhc::new::<0>(&p.smhc0, pads, &c, &p.ccu);

    println!("initializing SD card...");
    match load_from_sdcard(&mut smhc) {
        Ok(_) => {}
        Err(LoadError::SdCard(e)) => {
            println!("Failed to initialize SD card: {:?}", e);
            run_cli(&mut smhc);
        }
        Err(_) => {
            println!("Failed to load from SD card");
            run_cli(&mut smhc);
        }
    }

    // Run payload.
    run_payload();
}

/// Executes the loaded payload
fn run_payload() -> ! {
    const ZIMAGE_ADDRESS: usize = 0x4180_0000; // Load address of Linux zImage
    const DTB_ADDRESS: usize = 0x4100_8000; // Address of the device tree blob
    const HART_ID: usize = 0; // Hartid of the current core

    type KernelEntry = unsafe extern "C" fn(hart_id: usize, dtb_addr: usize);

    let kernel_entry: KernelEntry = unsafe { core::mem::transmute(ZIMAGE_ADDRESS) };
    unsafe {
        kernel_entry(HART_ID, DTB_ADDRESS);
    }

    loop {}
}

enum LoadError {
    SdCard(SdCardError),
    OpenVolume,
    LoadToMemory,
    FileSize,
}

impl From<SdCardError> for LoadError {
    #[inline]
    fn from(value: SdCardError) -> Self {
        LoadError::SdCard(value)
    }
}

fn load_from_sdcard<S: AsRef<RegisterBlock>, P>(smhc: &mut Smhc<S, P>) -> Result<(), LoadError> {
    let sdcard = SdCard::new(smhc)?;
    let size_gb = sdcard.get_size_kb() / 1024.0 / 1024.0;
    println!("SD card initialized, size: {:.2}GB", size_gb);

    let time_source = MyTimeSource {};
    let mut volume_mgr = VolumeManager::new(sdcard, time_source);
    let volume_res = volume_mgr.open_raw_volume(embedded_sdmmc::VolumeIdx(0));
    if let Err(e) = volume_res {
        println!("Failed to open volume: {:?}", e);
        return Err(LoadError::OpenVolume);
    }
    let volume0 = volume_res.unwrap();
    let root_dir = volume_mgr.open_root_dir(volume0).unwrap();

    // Load `snuxi.dtb` and `zImage`
    for (filename, addr, size) in [
        ("SUNXI.DTB", 0x4100_8000, 64 * 1024),
        ("ZIMAGE", 0x4180_0000, 512 * 1024 * 1024),
    ] {
        match unsafe { load_file_into_memory(&mut volume_mgr, root_dir, filename, addr, size) } {
            Ok(bytes) => {
                println!("load {} success, size = {} bytes", filename, bytes);
            }
            Err(LoadFileIntoMemoryError::Sdmmc(_e)) => {
                println!("error: cannot load file `{}`.", filename);
                return Err(LoadError::LoadToMemory);
            }
            Err(LoadFileIntoMemoryError::FileSize) => {
                println!(
                    "error: cannot load file `{}`, file size too large.",
                    filename
                );
                return Err(LoadError::FileSize);
            }
        }
    }

    volume_mgr.close_dir(root_dir).unwrap();

    Ok(())
}

enum LoadFileIntoMemoryError<T: core::fmt::Debug> {
    Sdmmc(embedded_sdmmc::Error<T>),
    FileSize,
}

impl<T: core::fmt::Debug> From<embedded_sdmmc::Error<T>> for LoadFileIntoMemoryError<T> {
    #[inline]
    fn from(value: embedded_sdmmc::Error<T>) -> Self {
        LoadFileIntoMemoryError::Sdmmc(value)
    }
}

/// Loads a file from SD card into specified memory address
unsafe fn load_file_into_memory<T: embedded_sdmmc::BlockDevice>(
    volume_mgr: &mut VolumeManager<T, MyTimeSource>,
    dir: embedded_sdmmc::RawDirectory,
    file_name: &str,
    addr: usize,
    max_size: u32,
) -> Result<usize, LoadFileIntoMemoryError<T::Error>> {
    // Find and open the file
    volume_mgr.find_directory_entry(dir, file_name)?;

    let file = volume_mgr.open_file_in_dir(dir, file_name, embedded_sdmmc::Mode::ReadOnly)?;

    // Check file size
    let file_size = volume_mgr.file_length(file)?;
    if file_size > max_size {
        return Err(LoadFileIntoMemoryError::FileSize);
    }

    // Read file content into memory
    let target = unsafe { core::slice::from_raw_parts_mut(addr as *mut u8, file_size as usize) };
    let size = volume_mgr.read(file, target)?;
    volume_mgr.close_file(file).ok();

    Ok(size)
}

fn run_cli<S: AsRef<RegisterBlock>, P>(smhc: &mut Smhc<S, P>) -> ! {
    let (command_buffer, history_buffer) = ([0; 128], [0; 128]);
    let mut cli = CliBuilder::default()
        .writer(syterkit::stdout())
        .command_buffer(command_buffer)
        .history_buffer(history_buffer)
        .prompt("SyterKit> ")
        .build()
        .unwrap();
    let mut stdin = syterkit::stdin();
    loop {
        let mut slice = [0];
        stdin.read(&mut slice).ok();
        let _ = cli.process_byte::<Base, _>(
            slice[0],
            &mut Base::processor(|cli, command| {
                match command {
                    Base::Bootargs => command_bootargs(cli),
                    Base::Reload => command_reload(cli, smhc),
                    Base::Boot => command_boot(cli),
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
    smhc: &mut Smhc<S, P>,
) {
    let _ = load_from_sdcard(smhc);
}

fn command_boot<'a>(_cli: &mut CliHandle<'a, Stdout, Infallible>) {
    run_payload();
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
    if value.starts_with("0x") {
        T::from_str_radix(value.strip_prefix("0x").unwrap(), 16).ok()
    } else {
        value.parse::<T>().ok()
    }
}
