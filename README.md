<div align="center">

![SyterKit LOGO](docs/assets/SyterKit%20LOGO_Thin.png)

# SyterKit

**A bare-metal firmware framework and bootloader toolkit for Allwinner SoCs**

[![Build](https://github.com/YuzukiHD/SyterKit/actions/workflows/build.yml/badge.svg)](https://github.com/YuzukiHD/SyterKit/actions/workflows/build.yml)
[![Test](https://github.com/YuzukiHD/SyterKit/actions/workflows/test.yml/badge.svg)](https://github.com/YuzukiHD/SyterKit/actions/workflows/test.yml)
[![Release](https://img.shields.io/github/v/release/YuzukiHD/SyterKit)](https://github.com/YuzukiHD/SyterKit/releases)
[![License: GPL-2.0](https://img.shields.io/badge/License-GPL--2.0-blue.svg)](LICENSE)

[Documentation](docs/README.md) ·
[API Reference](https://syterkit.yuzukihd.top/api/html/) ·
[Report Bug](https://github.com/YuzukiHD/SyterKit/issues) ·
[Request Feature](https://github.com/YuzukiHD/SyterKit/issues/new)

</div>

## Overview

SyterKit is a small, static bare-metal firmware framework for Allwinner SoCs. It provides board bring-up, peripheral drivers, image loading, bootloader applications, and standalone firmware utilities for both ARM and RISC-V platforms.

SyterKit uses GNU Make, Kconfig, Kbuild-style object lists, application-owned initialization, and Linux-style board device trees compiled to immutable C data by [dt2c](https://github.com/YuzukiTsuru/dt2c).

## Features

- Board bring-up and peripheral drivers for Allwinner ARM (Cortex-A) and RISC-V (C906/C907/E907) SoCs
- Bootloader applications (`syter_boot`) with image loading for SD/eMMC and SPI NOR/NAND boot
- `eGON.BT0` boot image generation, padding, and checksums via `tools/mksunxi`
- Compile-time device trees: board DTS validated and converted to static C data by [dt2c](https://github.com/YuzukiTsuru/dt2c); payload DTBs for loaded kernels handled with libfdt
- Linux kernel-style build system: Kconfig, Kbuild object lists, GNU Make, and out-of-tree `O=` builds
- Companion firmware and standalone BL33 utilities
- Host and QEMU-based tests, Doxygen API documentation

## Supported Boards

Run `make list-defconfigs` for the authoritative list in your checkout. Board-specific applications and hardware notes live in [`boards/`](boards/). Archived board sources are preserved under [`archive/`](archive/).

| Board | SoC / Platform | SyterKit CPU | Configuration |
| --- | --- | --- | --- |
| [TinyVision](https://github.com/YuzukiHD/TinyVision) | V851se/V851s3 | 1 × Cortex-A7 | `tinyvision_defconfig` |
| [LonganPi 3H](https://wiki.sipeed.com/hardware/zh/longan/H618/lpi3h/1_intro.html) | H618 | 4 × Cortex-A53 | `longanpi-3h_defconfig` |
| Avaota A1 | T527/A527 | 8 × Cortex-A55 | `avaota-a1_defconfig` |
| Radxa Cubie A7A | A733 | 6 × Cortex-A55 + 2 × Cortex-A76 | `radxa-cubie-a7a_defconfig` |
| Avaota F1 | V821 | Xuantie E907 + Andes A27L2 | `avaota-f1_defconfig` |
| Avaota F2 | V861 | Xuantie E907 + 2 × Xuantie C907 | `avaota-f2_defconfig` |
| Avaota M1 | A537 | 6 × Cortex-A53 + 2 × Cortex-A73 | `avaota-m1_defconfig` |
| YuzukiNeko | F101 | Xuantie C907 | `yuzukineko_rv32_defconfig`, `yuzukineko_rv32_usb_defconfig`, `yuzukineko_rv64_defconfig` |
| TLT153 MiniEVM | T153 | 4 × Cortex-A7 | `tlt153-minievm_defconfig` |
| TLT536 EVM | T536 | 4 × Cortex-A55 | `tlt536-evm_defconfig` |

Active configurations are grouped under `configs/<board>/`. Most boards provide `sram_defconfig`, `efex_defconfig`, and `dram_defconfig` variants; the public Make targets remain flat, e.g. `make avaota-a1_efex_defconfig`.

## Architecture

![SyterKit architecture](https://github.com/YuzukiHD/SyterKit/assets/12003087/f6ffe47e-6274-43ff-9a74-4a5b7b81083e)

SyterKit is intentionally small and static. The selected board contributes its applications and DTS, Kconfig selects the architecture and drivers, dt2c turns the board DTS into compile-time data, and the linker combines the selected objects into the final firmware image.

See [Compile-time device tree](docs/devicetree.md) and [Driver architecture](docs/driver-model.md) for the full model.

## Getting Started

### Prerequisites

A Linux host needs GNU Make, a host C compiler, Flex, Bison, pkg-config, ncurses development headers, and a bare-metal cross compiler. On Ubuntu (ARM32 targets):

```sh
sudo apt-get update
sudo apt-get install -y build-essential gcc-arm-none-eabi flex bison \
	libncurses-dev pkg-config doxygen graphviz default-jre
```

RISC-V boards require a compatible RISC-V toolchain whose flags match the CPU selected by Kconfig.

The checked-in Linux x86_64 dt2c distribution (`tools/bin/dt2c`) includes a musl-static binary and matching headers, so neither Rust nor the submodule is needed for a normal build. Initialize the submodule only to develop dt2c itself:

```sh
git submodule update --init tools/dt2c
```

An external dt2c installation can be selected instead:

```sh
make DT2C=/opt/dt2c/dt2c DT2C_INCLUDE=/opt/dt2c/include ...
```

### Configure and Build

List the available boards, select one, and build:

```sh
make list-defconfigs
make tinyvision_defconfig
make -j$(nproc)
```

Out-of-tree builds follow the Linux kernel `O=` convention:

```sh
make O=out tinyvision_defconfig
make O=out -j$(nproc)
```

Useful targets:

| Target | Description |
| --- | --- |
| `make menuconfig` | Edit the active configuration interactively |
| `make list-apps` | List applications selected for the active board |
| `make syter_boot` | Build a single application and its three images |
| `make firmware` | Build board companion firmware, when declared |
| `make utilities` | Build the standalone BL33 utilities |
| `make artifacts` | Build images, companion firmware, and utilities |
| `make test` | Run host and QEMU tests |
| `make docs` | Generate the Doxygen API documentation |
| `make check` | Make/Kconfig source-tree consistency checks |

Override the toolchain prefix when it is not in the default location:

```sh
make CROSS_COMPILE=/opt/toolchains/arm-none-eabi- -j$(nproc)
```

### Build Outputs

Each application is written below `build/<board>/<application>/` (or the chosen `O=` directory):

| File | Purpose |
| --- | --- |
| `<app>_fel.bin` | Load and run from SRAM with a host FEL tool |
| `<app>_card.bin` | Padded and checksummed for SD/eMMC boot |
| `<app>_spi.bin` | Padded and checksummed for SPI NOR/NAND boot |
| `<app>_fel.elf`, `<app>_bin.elf` | Symbols and debugging |
| `<app>_fel.map`, `<app>_bin.map` | Link maps |

## Writing Boot Media

> **Warning:** Writing raw devices can destroy existing data. Confirm the target device before running any command.

**SD/eMMC, MBR-formatted media** (BROM searches at an 8 KiB offset):

```sh
sudo dd if=syter_boot_card.bin of=/dev/sdX bs=1024 seek=8 conv=fsync
```

**SD/eMMC, GPT media** (secondary 128 KiB BROM location):

```sh
sudo dd if=syter_boot_card.bin of=/dev/sdX bs=1024 seek=128 conv=fsync
```

**SPI NAND/NOR** (redundant copies at 64 KiB intervals, programmed with [xfel](https://github.com/xboot/xfel)):

```sh
dd if=syter_boot_spi.bin of=spi.img bs=2k
dd if=syter_boot_spi.bin of=spi.img bs=2k seek=32
dd if=syter_boot_spi.bin of=spi.img bs=2k seek=64

xfel spinand write 0 spi.img   # or: xfel spinor write 0 spi.img
```

Detailed boot-header and media-layout documentation is available in the [documentation source](docs/README.md).

## Documentation

- [Getting started and Allwinner boot flow](docs/README.md)
- [Driver architecture](docs/driver-model.md)
- [Compile-time device tree](docs/devicetree.md)
- [Generated API documentation](https://syterkit.yuzukihd.top/api/html/)

## Contributing

Contributions are welcome! To contribute:

1. Fork the repository and create a feature branch.
2. Follow the existing code style (`.clang-format` is provided).
3. Run `make check` and `make test` before submitting.
4. Open a pull request with a clear description of the change.

Bug reports and feature requests are tracked in the [issue tracker](https://github.com/YuzukiHD/SyterKit/issues).

## Code of Conduct

This project follows the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to the community leaders listed in the document.

## License

SyterKit is released under the **GNU General Public License v2.0**. See [LICENSE](LICENSE) for the full text.

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FYuzukiHD%2FSyterKit.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FYuzukiHD%2FSyterKit?ref=badge_large)
