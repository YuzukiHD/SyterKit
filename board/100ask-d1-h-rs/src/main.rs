#![no_std]
#![no_main]

use core::convert::Infallible;

use embedded_cli::{
    cli::{CliBuilder, CliHandle},
    Command,
};
use embedded_io::Read;
use panic_halt as _;
use syterkit::{clock_dump, entry, println, show_banner, Clocks, Peripherals, Stdout};

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

fn parse_value<T: core::str::FromStr + num_traits::Num>(value: &str) -> Option<T> {
    if value.starts_with("0x") {
        T::from_str_radix(value.strip_prefix("0x").unwrap(), 16).ok()
    } else {
        value.parse::<T>().ok()
    }
}
