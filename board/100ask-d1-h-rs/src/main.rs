#![no_std]
#![no_main]

use core::convert::Infallible;

use embedded_cli::{
    cli::{CliBuilder, CliHandle},
    Command,
};
use embedded_io::Read;
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

#[entry] // This macro would initialize system clocks.
fn main(p: Peripherals, _c: Clocks) {
    // Display the bootloader banner.
    show_banner();

    // Initialize the DRAM.
    let dram_size = syterkit::mctl::init(&p.ccu, &p.phy);
    println!("DRAM size: {}M 🐏", dram_size);

    // Dump information about the system clocks.
    clock_dump(&p.ccu);

    // TODO: SD/MMC init.
    let _sdmmc_pins = {
        let d1 = p.gpio.pf0.into_function::<2>();
        let d0 = p.gpio.pf1.into_function::<2>();
        let clk = p.gpio.pf2.into_function::<2>();
        let cmd = p.gpio.pf3.into_function::<2>();
        let d3 = p.gpio.pf4.into_function::<2>();
        let d2 = p.gpio.pf5.into_function::<2>();
        (clk, cmd, d0, d1, d2, d3)
    };
    /*
    p.ccu.smhc_bgr.modify(|val| val.gate_pass::<0>().deassert_reset::<0>()); // 0 == sdhci->id
    p.ccu.smhc0_clk.modify(|val| val.unmask_clock());
    // let sdhci = p.smhc0.smhc(sdmmc_pins)
    // sdhci.detect_sdcard() // TODO or: detect_mmc()
    p.smhc0.global_control.modify(|val| val.set_dma_reset().set_fifo_reset().set_software_reset());
    // TODO sdhci_set_clock(hci, MMC_CLK_400K)
    // TODO sdhci_set_width(hci, MMC_BUS_WIDTH_1)
    // TODO go_idle_state(hci)
    // delay_ms(2)
    // let sdcard = sdhci.try_detect_sdcard()?;
    // // TODO volumn, directory, file, etc
    // let dtb = dir.open_file_in_dir()?;
    // // file read loop
    // ddr_mem[..size].copy_from_slice(&buf);
    // let kernel = ...
    // let config = ...
     */

    // Start boot command line.
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
                    Base::Reload => command_reload(cli),
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

fn command_reload<'a>(cli: &mut CliHandle<'a, Stdout, Infallible>) {
    ufmt::uwrite!(cli.writer(), "TODO Reload").ok();
}

fn command_boot<'a>(cli: &mut CliHandle<'a, Stdout, Infallible>) {
    ufmt::uwrite!(cli.writer(), "TODO Boot").ok();
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
