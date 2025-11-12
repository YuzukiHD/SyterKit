use byteorder::{LittleEndian, ReadBytesExt, WriteBytesExt};
use clap::{Args, Parser, Subcommand};
use clap_verbosity_flag::Verbosity;
use log::{error, info, trace};
use std::env;
use std::fs::File;
use std::io::{ErrorKind, Seek, SeekFrom};
use std::path::{Path, PathBuf};
use std::process::{self, Command, Stdio};

#[derive(Parser)]
#[clap(name = "xtask")]
#[clap(about = "Program that help you build and debug d1 flash test project", long_about = None)]
struct Cli {
    #[clap(subcommand)]
    command: Commands,
    #[clap(flatten)]
    env: Env,
    #[clap(flatten)]
    verbose: Verbosity,
}

#[derive(Subcommand)]
enum Commands {
    /// Make ELF and binary for this project
    Make(Make),
    /// Build flash and burn into FEL mode board
    Flash(Flash),
}

#[derive(Args)]
struct Make {
    #[clap(help = "Specify the binary name")]
    bin: Option<String>,
}

#[derive(Args)]
struct Flash {
    #[clap(help = "Specify the binary name")]
    bin: Option<String>,
}

#[derive(clap::Args)]
struct Env {
    #[clap(
        long = "release",
        global = true,
        help = "Build in release mode",
        long_help = None,
    )]
    release: bool,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum FlashType {
    Nand,
    Nor,
}

fn main() {
    let args = Cli::parse();
    env_logger::Builder::new()
        .filter_level(args.verbose.log_level_filter())
        .init();
    match &args.command {
        Commands::Make(make) => {
            info!("make D1 flash binary");
            let binutils_prefix = find_binutils_prefix_or_fail();
            xtask_build_d1_flash_bt0(&args.env, &make.bin);
            xtask_binary_d1_flash_bt0(binutils_prefix, &args.env, &make.bin);
            xtask_finialize_d1_flash_bt0(&args.env, &make.bin);
        }
        Commands::Flash(flash) => {
            info!("build D1 binary and burn");
            let xfel = find_xfel();
            xfel_find_connected_device(xfel);
            let binutils_prefix = find_binutils_prefix_or_fail();
            xtask_build_d1_flash_bt0(&args.env, &flash.bin);
            xtask_binary_d1_flash_bt0(binutils_prefix, &args.env, &flash.bin);
            xtask_finialize_d1_flash_bt0(&args.env, &flash.bin);
            let flash_type = xtask_detect_flash_type(xfel);
            xtask_burn_d1_flash_bt0(xfel, flash_type, &args.env, &flash.bin);
        }
    }
}

const DEFAULT_TARGET: &'static str = "riscv64imac-unknown-none-elf";

fn xtask_build_d1_flash_bt0(env: &Env, bin: &Option<String>) {
    trace!("build D1 flash bt0");
    let cargo = env::var("CARGO").unwrap_or_else(|_| "cargo".to_string());
    trace!("found cargo at {}", cargo);
    let mut command = Command::new(cargo);
    command.current_dir(project_root());
    trace!("cargo current dir: {:?}", project_root());
    command.arg("build");
    command.arg("-p");
    command.arg("syterkit-100ask-d1-h");
    if env.release {
        command.arg("--release");
    }
    if let Some(bin) = bin {
        command.arg("--bin");
        command.arg(&bin);
    }
    trace!("cargo command: {:?}", command);
    let status = command.status().unwrap();
    trace!("cargo returned {}", status);
    if !status.success() {
        error!("cargo build failed with {}", status);
        process::exit(1);
    }
}

fn xtask_binary_d1_flash_bt0(prefix: &str, env: &Env, bin: &Option<String>) {
    trace!("objcopy binary, prefix: '{}'", prefix);
    let mut command = Command::new(format!("{}objcopy", prefix));
    trace!("objcopy dist dir: {:?}", dist_dir(env));
    command.current_dir(dist_dir(env));
    command.arg("--binary-architecture=riscv64");
    command.arg("--strip-all");
    if let Some(bin) = bin {
        command.arg(&bin);
        command.args(&["-O", "binary", &format!("{}.bin", bin)]);
    } else {
        command.arg("syterkit-100ask-d1-h");
        command.args(&["-O", "binary", "syterkit-100ask-d1-h.bin"]);
    }
    trace!("objcopy command: {:?}", command);

    let status = command.status().unwrap();
    trace!("objcopy returned {}", status);

    if !status.success() {
        error!("objcopy failed with {}", status);
        process::exit(1);
    }
}

const EGON_HEADER_LENGTH: u64 = 0x60;

// This function does:
// 1. fill in binary length
// 2. calculate checksum of bt0 image; old checksum value must be filled as stamp value
fn xtask_finialize_d1_flash_bt0(env: &Env, bin: &Option<String>) {
    let path = dist_dir(env);
    let bin = match bin {
        Some(bin) => &format!("{}.bin", bin),
        None => "syterkit-100ask-d1-h.bin",
    };
    trace!("finialize binary, bin: {}", bin);
    let mut file = File::options()
        .read(true)
        .write(true)
        .open(path.join(bin))
        .expect("open output binary file");
    trace!("finialize binary, opened binary file {:?}", file);
    let total_length = file.metadata().unwrap().len();
    if total_length < EGON_HEADER_LENGTH {
        error!(
            "objcopy binary size less than eGON header length, expected >= {} but is {}",
            EGON_HEADER_LENGTH, total_length
        );
    }
    let new_len = align_up_to(total_length, 16 * 1024); // align up to 16KB
    file.set_len(new_len).unwrap();
    file.seek(SeekFrom::Start(0x10)).unwrap();
    file.write_u32::<LittleEndian>(new_len as u32).unwrap();
    file.seek(SeekFrom::Start(0x0C)).unwrap();
    let mut checksum: u32 = 0;
    file.seek(SeekFrom::Start(0)).unwrap();
    loop {
        match file.read_u32::<LittleEndian>() {
            Ok(val) => checksum = checksum.wrapping_add(val),
            Err(e) if e.kind() == ErrorKind::UnexpectedEof => break,
            Err(e) => error!("io error while calculating checksum: {:?}", e),
        }
    }
    file.seek(SeekFrom::Start(0x0C)).unwrap();
    file.write_u32::<LittleEndian>(checksum).unwrap();
    trace!("finialize binary finished, saving file");
    file.sync_all().unwrap(); // save file before automatic closing
} // for C developers: files are automatically closed when they're out of scope

fn align_up_to(len: u64, target_align: u64) -> u64 {
    let (div, rem) = (len / target_align, len % target_align);
    if rem != 0 {
        (div + 1) * target_align
    } else {
        len
    }
}

const FAIL_NOR_FLASH: &[u8] = b"Can't detect any spi nor flash";
const FAIL_NAND_FLASH: &[u8] = b"Can't detect any spi nand flash";

fn xtask_detect_flash_type(xfel: &str) -> FlashType {
    trace!("detect flash type with xfel '{}'", xfel);
    let mut command = Command::new(xfel);
    command.arg("spinor");
    let output = command.output().unwrap();
    trace!("xfel spinor returned output: {:?}", output);
    if output.status.success() && !output.stdout.trim_ascii_end().ends_with(FAIL_NOR_FLASH) {
        return FlashType::Nor;
    }
    let mut command = Command::new(xfel);
    command.arg("spinand");
    let output = command.output().unwrap();
    trace!("xfel spinand returned output: {:?}", output);
    if output.status.success() && !output.stdout.trim_ascii_end().ends_with(FAIL_NAND_FLASH) {
        return FlashType::Nand;
    }
    error!("cannot detect any flash");
    process::exit(1);
}

fn xtask_burn_d1_flash_bt0(xfel: &str, flash: FlashType, env: &Env, bin: &Option<String>) {
    trace!("burn flash with xfel '{}'", xfel);
    let mut command = Command::new(xfel);
    command.current_dir(dist_dir(env));
    match flash {
        FlashType::Nand => command.arg("spinand"),
        FlashType::Nor => command.arg("spinor"),
    };
    command.args(["write", "0"]);
    if let Some(bin) = bin {
        command.arg(format!("{}.bin", bin));
    } else {
        command.arg("syterkit-100ask-d1-h.bin");
    }
    let status = command.status().unwrap();
    trace!("xfel returned {}", status);
    if !status.success() {
        error!("xfel failed with {}", status);
        process::exit(1);
    }
}

fn find_xfel() -> &'static str {
    let mut command = Command::new("xfel");
    command.stdout(Stdio::null());
    match command.status() {
        Ok(status) if status.success() => return "xfel",
        Ok(status) => match status.code() {
            Some(code) => {
                error!("xfel command failed with code {}", code);
                process::exit(code)
            }
            None => error!("xfel command terminated by signal"),
        },
        Err(e) if e.kind() == ErrorKind::NotFound => error!(
            "xfel not found
    install xfel from: https://github.com/xboot/xfel"
        ),
        Err(e) => error!(
            "I/O error occurred when detecting xfel: {}.
    Please check your xfel program and try again.",
            e
        ),
    }
    process::exit(1)
}

fn xfel_find_connected_device(xfel: &str) {
    let mut command = Command::new(xfel);
    command.arg("version");
    let output = command.output().unwrap();
    if !output.status.success() {
        error!("xfel failed with code {}", output.status);
        error!("Is your device in FEL mode?");
        process::exit(1);
    }
    info!("Found {}", String::from_utf8_lossy(&output.stdout).trim());
}

fn find_binutils_prefix() -> Option<&'static str> {
    for prefix in ["rust-", "riscv64-unknown-elf-", "riscv64-linux-gnu-"] {
        let mut command = Command::new(format!("{}objcopy", prefix));
        command.arg("--version");
        command.stdout(Stdio::null());
        let status = command.status();
        if let Ok(status) = status {
            if status.success() {
                return Some(prefix);
            }
        }
    }
    None
}

fn find_binutils_prefix_or_fail() -> &'static str {
    trace!("find binutils");
    if let Some(ans) = find_binutils_prefix() {
        trace!("found binutils, prefix is '{}'", ans);
        return ans;
    }
    error!(
        "no binutils found, try install using:
    rustup component add llvm-tools-preview
    cargo install cargo-binutils"
    );
    process::exit(1)
}

fn project_root() -> PathBuf {
    Path::new(&env!("CARGO_MANIFEST_DIR"))
        .ancestors()
        .nth(2)
        .unwrap()
        .to_path_buf()
}

fn dist_dir(env: &Env) -> PathBuf {
    let mut path_buf = project_root().join("target").join(DEFAULT_TARGET);
    path_buf = match env.release {
        false => path_buf.join("debug"),
        true => path_buf.join("release"),
    };
    path_buf
}
