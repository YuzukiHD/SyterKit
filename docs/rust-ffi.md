# Bare-metal Rust

SyterKit remains a C-first bare-metal framework. C owns startup, board initialization, linker scripts, Kconfig, image packaging, and the driver implementations. Rust is an optional consumer of that existing C ABI, organized as a Cargo workspace in `rust/` with four crates: `rust/ffi` holds the bindgen-generated raw declarations, `rust/lib` contains shared no_std helpers, `rust/drivers` has one typed wrapper module per C driver family, and `rust/core` owns the Rust app entry point, the logging path, and the `Core` facade for the root C core interfaces. No project C header is added for Rust; bindgen writes its output into Cargo's generated `OUT_DIR`, and the Rust archive is placed into the firmware image by the existing C linker step.

```text
C core/ + drivers / Kconfig / Kbuild / linker
                    |
                    v
              existing C headers
                    |
                    v
      bindgen -> syterkit-ffi (rust/ffi)
                    |
                    v
    syterkit-lib (rust/lib) + syterkit-drivers (rust/drivers)
                    |
                    v
      syterkit-core staticlib (rust/core, app entry)
                    |
                    v
        linked by the normal C link step
```

## Dependencies

Rust, Cargo, Clang, and libclang are needed only when Rust FFI is enabled. On Ubuntu, install the host-side tools with:

```sh
sudo apt-get install clang libclang-dev
rustup target add riscv32imac-unknown-none-elf \
  riscv64gc-unknown-none-elf armv7a-none-eabi
```

The Rust target is inferred from the Kconfig architecture, and the C cross compiler is still selected through `CROSS_COMPILE`; Cargo compiles the archive for the matching Rust target and the existing C linker places it in the firmware image. The default target pairs are:

| Kconfig architecture | Rust target | Clang target |
| --- | --- | --- |
| `CONFIG_ARCH_RISCV32` | `riscv32imac-unknown-none-elf` | `riscv32-unknown-elf` |
| `CONFIG_ARCH_RISCV64` | `riscv64gc-unknown-none-elf` | `riscv64-unknown-elf` |
| `CONFIG_ARCH_ARM32` | `armv7a-none-eabi` | `arm-none-eabi` |

`RUST_FFI_TARGET` and `RUST_FFI_CLANG_TARGET` override the inferred targets when a board uses a custom target or ABI.

## Enable and build

Select a board configuration, enable `Build Rust FFI and applications` (`CONFIG_RUST_FFI`, which depends on `DRIVER_SERIAL`) in `menuconfig`, then build the library or the normal images:

```sh
make O=out tinyvision_defconfig
make O=out menuconfig
make O=out CROSS_COMPILE=arm-none-eabi- rust-ffi
make O=out CROSS_COMPILE=arm-none-eabi- -j$(nproc)
```

`rust-ffi` builds the Rust core archive and every Rust app declared by the selected board. The core archive is placed at `.obj/rust/<board>/<app_mode>/<rust-target>/release/libsyterkit_core.a`, and each app archive at `.obj/rust/<board>/<app_mode>/apps/<app>/<rust-target>/release/libsyterkit_core.a` in the selected output tree, where `<app_mode>` is `app_sram`, `app_dram`, or `app_efex` depending on the configuration. Cargo runs with `--locked` and receives `-C panic=abort -C opt-level=<z|3>` through `RUSTFLAGS`, using `z` when `CONFIG_OPTIMIZE_FOR_SIZE` is set and `3` otherwise; extra flags can be appended through `RUST_FFI_RUSTFLAGS`.

The Rust archive is linked with `--no-whole-archive`, so a C app does not pull Rust code into its image unless it references a Rust symbol. A Rust app replaces the C app archive for its entry point and supplies the ABI-compatible `main` symbol instead. C apps and Rust apps can coexist in the board's application list, but a name that appears in both lists is a build error, and a declared Rust app without a `main.rs` fails the build before Cargo runs.

## Generated bindings

`rust/ffi/build.rs` is the single generation entry point. It consumes a header manifest (default `rust/ffi/headers.txt`) whose lines select headers project-relative to the source tree. The supported directives are:

- `@all <dir>` — recursively discover every `.h` file below the directory. The default manifest contains `@all include`, which covers all public driver headers; architecture headers are pulled in transitively through the same include paths used by C.
- `@selected-arch` — discover every header below `arch/<SYTERKIT_FFI_ARCH>/include`, where the architecture directory is `arm` or `riscv` as selected by Kconfig.
- A plain path selects one header explicitly, which allows a product build to describe a smaller binding set.

Lines starting with `#` are comments. The assembly-only `asm/trap.h` header is skipped during discovery, and when FatFs or libfdt headers are selected they are placed first in the generated wrapper so their environment definitions precede the rest. `SYTERKIT_FFI_HEADERS` can override manifest discovery entirely with a whitespace-separated list of headers.

Bindgen runs with `use_core`, no layout tests, and comments preserved, and it tracks transitive includes through its Cargo callback, so changing a driver header regenerates the bindings. The clang invocation receives `--target=<clang target>`, the normal `KBUILD_CPPFLAGS` (include paths and `-include autoconf.h`), and any user-supplied arguments, so the generated bindings see the active Kconfig macros; the build script also watches the Kconfig output files and regenerates when the configuration changes. An optional allowlist reduces the generated source and ABI surface without changing the Rust crate; each rule has the form `function`, `type`, or `var` followed by a regular expression.

The inputs are configurable from the Make command line without editing Rust source:

```sh
make O=out rust-ffi \
  RUST_FFI_HEADER_MANIFEST=/path/to/headers.txt \
  RUST_FFI_ALLOWLIST=/path/to/allowlist.txt \
  RUST_FFI_CLANG_ARGS='-DPRODUCT_VARIANT=1'
```

The generated declarations are exposed as the `syterkit_ffi::raw` module, with `syterkit_ffi::bindings` kept as a compatibility alias. The full set of Make-level knobs is:

| Variable | Purpose |
| --- | --- |
| `RUST_FFI_HEADER_MANIFEST` | Header manifest file (default `rust/ffi/headers.txt`). |
| `RUST_FFI_HEADERS` | Whitespace-separated override of manifest discovery. |
| `RUST_FFI_ALLOWLIST` | Optional bindgen allowlist file. |
| `RUST_FFI_CLANG_ARGS` | Extra arguments passed to clang during generation. |
| `RUST_FFI_RUSTFLAGS` | Extra `RUSTFLAGS` for all Rust compilation. |
| `RUST_FFI_TARGET` | Override the inferred Rust target. |
| `RUST_FFI_CLANG_TARGET` | Override the inferred clang target. |
| `RUST_FFI_LINKER_FLAGS` | Extra flags added to Rust-enabled link steps. |
| `RUST_FFI_CARGO` | Cargo binary to use (default `cargo`, or `CARGO` if set). |

## Crate layout

The workspace (`Cargo.toml`, resolver 2, `panic = "abort"` in the release profile) contains:

- `syterkit-ffi` (`rust/ffi`) — includes the generated `bindings.rs` into the `raw` module. It is an ABI description, not a driver model.
- `syterkit-lib` (`rust/lib`) — shared no_std helpers: `DriverResult<T>` (the `Result<T, i32>` wrapper around C status codes), `INVALID_ARGUMENT`, `c_name` for NUL-terminated byte strings, `status` for the common zero-is-success convention, the `BlockDevice` trait and its buffer/block-count checking helpers, and `Disk`, the adapter over the existing FatFs disk layer.
- `syterkit-drivers` (`rust/drivers`) — one wrapper module per public C driver family; re-exported below.
- `syterkit-core` (`rust/core`) — built as a `staticlib` (`libsyterkit_core.a`). It re-exports the driver and lib layers, owns the Rust logging path and the `Core` facade, installs the panic handler, and — when building a Rust app — includes the board's `main.rs` and exports the C-visible `main` symbol.

## Calling the generated API

`syterkit_core` re-exports the driver APIs for Rust apps. The driver layer currently includes `serial::{StdoutUart, SerialPort}`, `gpio::{Gpio, GpioPin, GpioPull, GPIO}`, `i2c::I2cBus`, `spi::{SpiBus, SpiIoMode}`, `spif::{Spif, SpifConfig}`, `pwm::{Pwm, PwmConfig, PwmMode, PwmPolarity}`, `rtc::Rtc`, `sid::Sid`, `dma::{DmaController, DmaChannel}`, `mmc::{Sdhci, SdMmc, MmcTuning, SdMmcMedia}`, `mtd::{SpiNor, SpiNand, SpifNor}`, `pmu::{Pmu, PmuModel}`, `dram::Dram`, `psram::Psram`, `clk::{ClockTree, CLOCK_TREE}`, `soc::{Soc, SOC}`, `timer::{Timer, SoftwareTimer, TimerCallback, TIMER}`, `intc::InterruptController`, `gic::Gic`, `plic::Plic`, `clic::Clic`, `pcie::{Pcie, PcieController, PciePhy, PcieMode, AtuType}`, `ufs::{SunxiUfs, UfsHost, UfsDevice, UfsScsi}`, `remoteproc::RemoteProcessor`, and `usb::{UsbManager, UsbPlatform, UsbController, UsbDevice, UsbDma, UsbDmaChannel, USB}`.

GPIO, storage, bus, and controller descriptors are constructed from board-owned C state through an explicit unsafe boundary (typically `from_raw` over a caller-owned descriptor); operations after construction use typed Rust methods and preserve the C driver's status codes as `DriverResult`. Owned handles such as DMA channels and USB controller/DMA handles release their C resources from `Drop`. Stateless entry points are exposed through const globals (`CORE`, `CLOCK_TREE`, `GPIO`, `SOC`, `TIMER`, `USB`).

`ClockTree` provides the lifecycle calls (`preinitialize`, `initialize`, `reset`, `dump`) plus the common `get_hosc_rate` and `set_cpu_pll` operations with frequencies expressed in MHz; they use the existing SoC C implementations, and the older SoC-named methods remain as compatibility aliases. `InterruptController` wraps the architecture-selected controller lifecycle and the common `irq_install_handler`/`irq_free_handler`/`irq_enable`/`irq_disable` entry points shared by the GIC, PLIC, and CLIC wrappers. `Timer` wraps the C time and delay services (`milliseconds`, `microseconds`, `delay_us`, `delay_ms`, `spin`, the raw counter) and the software timer list with a caller-owned `timer_t` storage.

Some driver surfaces are configuration-gated: the drivers build script maps `CONFIG_DRIVER_MMC_TUNING` and `CONFIG_DRIVER_GPIO_V2_POW` to `cfg(syterkit_config_driver_mmc_tuning)` and `cfg(syterkit_config_driver_gpio_v2_pow)`, and the core build script maps `CONFIG_DRIVER_PSRAM` to `cfg(syterkit_config_driver_psram)`, reading the active Kconfig output or the clang define list, so Rust code can follow the same configuration the C build sees.

`syterkit_core::Core` (also available as the `CORE` constant) wraps the existing root-core interfaces: the startup banner (`show_banner`, and `show_banner_with_build_info` with an application-selected toolchain string), board cleanup (`clean_data`), heap initialization (`malloc_init`), memory dumping (`dump_hex`), the default shell lifecycle (`attach_shell`, `run_shell`), and the root UART console (`putc`, `write_bytes`, `getc`, `try_getc`, `console_ready`). Board power, LDO, and architecture hooks remain in their C board or architecture owners; they are available in the raw FFI for explicit board code.

`syterkit_lib::BlockDevice` is the common component-level contract for read/write block storage: it derives the transfer count from the Rust slice, checks block-size multiplication for overflow, rejects zero-sized requests, and reports the number of transferred blocks as `DriverResult<usize>`. `Disk` implements it for the existing FatFs disk layer; `Sdhci`, `SdMmc`, `UfsDevice`, and `UfsScsi` implement it for their native block layers. The legacy `u32` block methods remain available for code that follows the C convention, but invalid buffers are rejected before entering C. `SdMmcMedia` wraps the older global `sdmmc_initialize` and `sdmmc_block_read` API; its read buffer must contain at least `blocks * 512` bytes and it has no write operation because the C header does not expose one.

The wrappers deliberately do not recreate register definitions or a second hardware model. C remains responsible for device-tree population, register access, SoC quirks, and the actual driver implementation. Rust owns API ergonomics, buffer lifetime checks, enum conversion, status-code conversion, and resource lifetimes. When a C API contains a platform-specific descriptor or callback table, the wrapper accepts that descriptor by reference; this keeps new SoC variants compatible without regenerating Rust source by hand.

### Logging

Logging is formatted by the Rust component and then sent through the existing C `printk` path, so an app does not write UART characters directly. The non-variadic `printk_string(level, message)` declaration lives in the existing `include/log.h`; its C implementation forwards to `printk(level, "%s", message)`, preserving the normal timestamp, severity prefix, early-console buffering, and UART sink.

Formatting happens in a bounded 256-byte stack buffer (`PRINTK_BUFFER_SIZE`) because the bare-metal component layer has no allocator, and a message that does not fit is rejected with `INVALID_ARGUMENT` rather than truncated. The `LogLevel` enum matches the C log severities (mute, error, warning, info, debug, trace, backtrace), and the `printk!`, `print!`, `println!`, `eprint!`, and `eprintln!` macros mirror the standard Rust print API.

```rust
use crate::{println, StdoutUart};

pub fn main() -> i32 {
    let _uart = match StdoutUart::init() {
        Ok(uart) => uart,
        Err(error) => return error,
    };

    println!("Rust printk output");
    0
}
```

`StdoutUart::init()` only initializes the device-tree-selected console. The returned handle implements the blocking `embedded_io::Read` and `embedded_io::Write` traits; `SerialPort` wraps a caller-owned UART descriptor instead.

## Rust applications

A board declares Rust apps in its Makefile with the same mode-specific naming used by C apps:

```make
rust-apps-sram-$(CONFIG_RUST_FFI) += app-rs
```

`rust-apps-dram-y` and `rust-apps-efex-y` are the DRAM and efex equivalents. The source is then stored at:

```text
boards/<board>/app_sram/app-rs/main.rs
```

The app supplies `pub fn main() -> i32` and uses the root-core facade plus driver re-exports through `crate`. The build passes the app source to the core crate through `SYTERKIT_RUST_APP_SOURCE`; the core build script sets `cfg(syterkit_rust_app)`, the crate includes the file directly, and a `#[no_mangle] extern "C" fn main` wrapper forwards to it. The C startup therefore calls an ABI-compatible `main` symbol: for a Rust app the C app archive is omitted and the Rust app archive supplies that symbol. Each Rust app gets an independent Cargo target directory, preventing app entry points and generated bindings from colliding.

An optional `main.c` in the same directory is compiled as an application-owned C helper unit and linked alongside the Rust archive. This is useful when a C header exposes a `static inline` device-tree parser: the helper provides a linkable forwarding symbol, while `main.rs` declares the symbol in an app-local `extern "C"` block, performs the raw binding call, and then hands the descriptor to a typed Rust driver wrapper. The helper belongs to the application, not to the generic driver implementation; `boards/yuzukineko/app_sram/app-rs` follows exactly this pattern for PSRAM, SPIF, and SPI NOR device-tree aliases.

Rust app startup can pass the build-time Rust toolchain string to `Core::show_banner_with_build_info`; the core build script captures `rustc --version` plus the Cargo target into the `SYTERKIT_RUST_TOOLCHAIN` compile-time environment variable for this purpose. C apps continue to use the C compiler identity from the normal banner:

```rust
use crate::{println, StdoutUart, CORE};

const RUST_TOOLCHAIN: &[u8] = concat!(env!("SYTERKIT_RUST_TOOLCHAIN"), "\0").as_bytes();

pub fn main() -> i32 {
    let _stdout = match StdoutUart::init() {
        Ok(stdout) => stdout,
        Err(error) => return error,
    };

    let _ = CORE.show_banner_with_build_info(RUST_TOOLCHAIN);
    println!("Hello from Rust");
    0
}
```

Build the app like any other selected app:

```sh
make O=out yuzukineko_rv32_defconfig
make O=out menuconfig                 # enable Rust FFI
make O=out CROSS_COMPILE=riscv64-unknown-elf- app-rs
```

## Tests

Run the host-side binding and driver tests with:

```sh
make -C test rust_ffi_bindings
```

This test regenerates bindings from the public header tree for the host architecture (with the ARM architecture headers and a couple of driver configuration defines enabled) and runs `cargo test` across all four crates: `syterkit-ffi`, `syterkit-drivers`, `syterkit-core`, and `syterkit-lib`. It checks generated function signatures and constants across the driver families, including the legacy FatFs disk and `sdmmc` media entry points, and exercises each Rust driver wrapper against in-process ABI mocks. The wrapper tests live next to each wrapper in `syterkit-drivers` (and in `syterkit-core`/`syterkit-lib` for their layers); the integration test in `rust/ffi/tests` checks the raw ABI independently of the wrapper implementation.

## ABI rules

The generated layer is an ABI description, not a second driver model. Use C types already owned by SyterKit, keep pointers and buffer lengths explicit, and do not expose C variadic functions, packed hardware registers, or Rust references across the boundary. `armv7a-none-eabi` uses the soft-float Rust target; integer-only interfaces are compatible with the current ARM builds, but floating-point APIs require a target-specific ABI review before being added to the allowlist. ARM hard-float configurations add the linker's `--no-warn-mismatch` only to Rust-enabled links because stable Rust does not ship an `armv7a-none-eabihf` target. Keep all Rust/C boundary values integer or pointer types in that mode, or provide a matching custom target before exposing floating-point arguments or results. The default RISC-V 64-bit target is `riscv64gc-unknown-none-elf` and matches the LP64D configuration; the integer-only LP64 configuration (`CONFIG_ARCH_RISCV64_ABI_LP64`) must provide a matching Rust target through `RUST_FFI_TARGET` and target-specific `RUST_FFI_RUSTFLAGS`, and the build fails early when Rust FFI is enabled without one.
