# SyterKit

![SyterKit LOGO](docs/assets/SyterKit%20LOGO_Thin.png)

SyterKit is a bare-metal firmware framework for Allwinner SoCs. It provides
board bring-up, peripheral drivers, image loading, bootloader applications, and
small standalone firmware utilities for ARM and RISC-V platforms.

SyterKit 0.5 uses GNU Make, Kconfig, Kbuild-style object lists, application-owned
initialization, and Linux-style board device trees compiled to static C data by
[dt2c](https://github.com/YuzukiTsuru/dt2c).

## Supported boards

The configuration name in the last column can be passed directly to `make`.
The CPU column describes the core used to run SyterKit, not every processor in
the SoC.

| Board | SoC / platform | SyterKit CPU | Configuration |
| --- | --- | --- | --- |
| [TinyVision](https://github.com/YuzukiHD/TinyVision) | V851se/V851s3 | 1 * Cortex-A7 | `tinyvision_defconfig` |
| [LonganPi 3H](https://wiki.sipeed.com/hardware/zh/longan/H618/lpi3h/1_intro.html) | H618 | 4 * Cortex-A53 | `longanpi-3h_defconfig` |
| Avaota A1 | T527/A527 | 8 * Cortex-A55 | `avaota-a1_defconfig` |
| Radxa Cubie A7A | A733 | 6 * Cortex-A55 + 2* Cortex-A76 | `radxa-cubie-a7a_defconfig` |
| Avaota F1 | V821 | Xuantie E907 + Andes A27L2 | `avaota-f1_defconfig` |
| Avaota F2 | V861 | Xuantie E907 + 2 * Xuantie C907 | `avaota-f2_defconfig` |
| Avaota M1 | A537 | 6 * Cortex-A53 + 2* Cortex-A73 | `avaota-m1_defconfig` |
| YuzukiNeko | F101 | Xuantie C907 | `yuzukineko_rv32_defconfig`<br/>`yuzukineko_rv64_defconfig` |
| TLT153 MiniEVM | T153 | 4 * Cortex-A7 | `tlt153-minievm_defconfig` |
| TLT536 EVM | T536 | 4 * Cortex-A55 | `tlt536-evm_defconfig` |

Run `make list-defconfigs` for the authoritative list available in the current
checkout. Board-specific applications and hardware notes are kept below
[`boards/`](boards/). Retired board sources and defconfigs are preserved under
[`archive/`](archive/).

The archived boards are not included in the active configuration or build
matrix.

## Architecture

![SyterKit architecture](https://github.com/YuzukiHD/SyterKit/assets/12003087/f6ffe47e-6274-43ff-9a74-4a5b7b81083e)

SyterKit is intentionally small and static. The selected board contributes its
applications and DTS, Kconfig selects the architecture and drivers, dt2c turns
the board DTS into compile-time data, and the linker combines the selected
objects into the final firmware image.

The board device tree configures SyterKit itself. A Linux DTB loaded for the
next boot stage remains a separate object and is handled with libfdt. See
[Compile-time device tree](docs/devicetree.md) and
[Driver architecture](docs/driver-model.md) for the full model.

## Building SyterKit

### Host dependencies

A Linux host needs GNU Make, a C compiler, Flex, Bison, pkg-config, ncurses development headers, and a suitable bare-metal cross compiler. For an Ubuntu ARM32 setup:

```sh
sudo apt-get update
sudo apt-get install -y build-essential gcc-arm-none-eabi flex bison \
	libncurses-dev pkg-config doxygen graphviz default-jre
```

RISC-V boards require a compatible RISC-V toolchain. CPU-specific flags are
selected by Kconfig; the compiler must recognize the selected core.

The checked-in Linux x86_64 dt2c distribution includes a musl-static binary
and matching headers, so a normal firmware build needs neither Rust nor the
dt2c submodule. Initialize the submodule only to build or develop dt2c itself:

```sh
git submodule update --init tools/dt2c
```

Other hosts can build the submodule with Cargo, or select an external
compatible binary and headers:

```sh
make DT2C=/opt/dt2c/dt2c DT2C_INCLUDE=/opt/dt2c/include ...
```

### Configure and build

Select a board, then build all applications enabled by that board:

```sh
make tinyvision_defconfig
make -j$(nproc)
```

Useful configuration and build commands:

```sh
make list-defconfigs        # list board configurations
make menuconfig             # edit the active configuration
make list-apps              # list applications for the selected board
make syter_boot             # build one application and its three images
make firmware               # build board companion firmware, when declared
make utilities              # build the standalone BL33 utilities
make artifacts              # build images, companion firmware, and utilities
make test                   # run host and QEMU tests
make docs                   # generate API documentation
```

Out-of-tree builds follow the Linux kernel `O=` convention:

```sh
make O=out tinyvision_defconfig
make O=out -j$(nproc)
```

Override a toolchain prefix in the usual way when it is not in the default
location:

```sh
make CROSS_COMPILE=/opt/toolchains/arm-none-eabi- -j$(nproc)
```

### Build outputs

Each application is written below `build/<board>/<application>/` or below the
selected `O=` directory. It produces:

| File | Purpose |
| --- | --- |
| `<app>_fel.bin` | Load and run from SRAM with a host FEL tool |
| `<app>_card.bin` | Padded and checksummed for SD/eMMC boot |
| `<app>_spi.bin` | Padded and checksummed for SPI NOR/NAND boot |
| `<app>_fel.elf`, `<app>_bin.elf` | Symbols and debugging |
| `<app>_fel.map`, `<app>_bin.map` | Link maps |

## Writing boot media

Writing raw devices can destroy existing data. Confirm the target device before
running any command.

For an MBR-formatted SD card, Allwinner BROM normally looks for an eGON image at
an 8 KiB offset:

```sh
sudo dd if=syter_boot_card.bin of=/dev/sdX bs=1024 seek=8 conv=fsync
```

For GPT media, use the secondary 128 KiB BROM location:

```sh
sudo dd if=syter_boot_card.bin of=/dev/sdX bs=1024 seek=128 conv=fsync
```

For SPI NAND or SPI NOR, a redundant image can be assembled at 64 KiB
intervals:

```sh
dd if=syter_boot_spi.bin of=spi.img bs=2k
dd if=syter_boot_spi.bin of=spi.img bs=2k seek=32
dd if=syter_boot_spi.bin of=spi.img bs=2k seek=64
```

The resulting image can be programmed with
[xfel](https://github.com/xboot/xfel):

```sh
xfel spinand write 0 spi.img
# or
xfel spinor write 0 spi.img
```

Detailed boot-header and media-layout documentation is available in the
[online documentation source](docs/README.md).

## Documentation

- [Getting started and Allwinner boot flow](docs/README.md)
- [Driver architecture](docs/driver-model.md)
- [Compile-time device tree](docs/devicetree.md)
- [Generated API documentation](https://syterkit.yuzukihd.top/api/html/)

## License

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FYuzukiHD%2FSyterKit.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FYuzukiHD%2FSyterKit?ref=badge_large)
