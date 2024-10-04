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
enum Base {
    /// Get/set bootargs for kernel.
    Bootargs,
    /// Rescan TF Card and reload DTB, Kernel zImage.
    Reload,
    /// Print out env config.
    Print,
    /// Boot to Linux.
    Boot,
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
    let (command_buffer, history_buffer) = ([0; 32], [0; 32]);
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
                }
                Ok(())
            }),
        );
    }
}

fn command_bootargs<'a>(cli: &mut CliHandle<'a, Stdout, Infallible>) {
    ufmt::uwrite!(cli.writer(), "TODO Bootargs").ok();
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
