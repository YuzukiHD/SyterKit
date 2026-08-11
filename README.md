# SyterKit

![SyterKit LOGO](docs/assets/SyterKit%20LOGO_Thin.png)

SyterKit is a bare-metal firmware framework for Allwinner SoCs. It provides
board bring-up, peripheral drivers, image loading, bootloader applications, and
small standalone firmware utilities for ARM and RISC-V platforms.

SyterKit 0.5 uses GNU Make, Kconfig, Kbuild-style object lists, linker-collected
initcalls, and Linux-style board device trees compiled to static C data by
[dt2c](https://github.com/YuzukiTsuru/dt2c).

## Supported boards

The configuration name in the last column can be passed directly to `make`.
The CPU column describes the core used to run SyterKit, not every processor in
the SoC.

| Board | SoC / platform | SyterKit CPU | Configuration |
| --- | --- | --- | --- |
| [Yuzuki Lizard](https://github.com/YuzukiHD/Yuzukilizard) | V851s (`sun8iw21`) | Cortex-A7 | `yuzukilizard_defconfig` |
| [TinyVision](https://github.com/YuzukiHD/TinyVision) | V851se/V851s3 (`sun8iw21`) | Cortex-A7 | `tinyvision_defconfig` |
| 100ASK T113-S3 | T113-S3 (`sun8iw20`) | Dual Cortex-A7 | `100ask-t113s3_defconfig` |
| 100ASK T113-I | T113-I (`sun8iw20`) | Dual Cortex-A7 | `100ask-t113i_defconfig` |
| 100ASK D1-H | D1-H (`sun20iw1`) | XuanTie C906 RV64 | `100ask-d1-h_defconfig` |
| DongshanPI AICT | V853 (`sun8iw21`) | Cortex-A7 | `dongshanpi-aict_defconfig` |
| Project Yosemite | V853 (`sun8iw21`) | Cortex-A7 | `project-yosemite_defconfig` |
| Avaota 86Box | `sun8iw20` | Cortex-A7 | `avaota-86box_defconfig` |
| MCore R818 | R818 (`sun50iw10`) | ARMv8 core | `mcore-r818_defconfig` |
| [LonganPi 3H](https://wiki.sipeed.com/hardware/zh/longan/H618/lpi3h/1_intro.html) | H618 (`sun50iw9`) | Cortex-A53 | `longanpi-3h_defconfig` |
| Avaota A1 | T527/A527 (`sun55iw3`) | Cortex-A55 | `avaota-a1_defconfig` |
| Radxa Cubie A7A | A733 (`sun60iw2`) | ARMv8.2 core | `radxa-cubie-a7a_defconfig` |
| Avaota F1 | `sun300iw1` | E907 RV32 | `avaota-f1_defconfig` |
| Avaota F2 | `sun252iw1` | E907 RV32 | `avaota-f2_defconfig` |
| Avaota M1 | `sun65iw1` | ARMv8 core | `avaota-m1_defconfig` |
| TLT153 MiniEVM | `sun8iw22` | Cortex-A7 | `tlt153-minievm_defconfig` |
| TLT536 EVM | T536 (`sun55iw6`) | Cortex-A55 | `tlt536-evm_defconfig` |
| Yuzuki HomeKit | T113-M4020DC0 (`sun8iw20`) | Dual Cortex-A7 | `yuzukihomekit_defconfig` |

Run `make list-defconfigs` for the authoritative list available in the current
checkout. Board-specific applications and hardware notes are kept below
[`boards/`](boards/).

## Architecture

![SyterKit architecture](https://github.com/YuzukiHD/SyterKit/assets/12003087/f6ffe47e-6274-43ff-9a74-4a5b7b81083e)

SyterKit is intentionally small and static. The selected board contributes its
applications and DTS, Kconfig selects the architecture and drivers, dt2c turns
the board DTS into compile-time data, and the linker collects built-in drivers,
devices, and initcalls into the final firmware image.

The board device tree configures SyterKit itself. A Linux DTB loaded for the
next boot stage remains a separate object and is handled with libfdt. See
[Compile-time device tree](docs/devicetree.md) and
[Driver model and initcalls](docs/driver-model.md) for the full model.

## Building SyterKit

### Host dependencies

A Linux host needs GNU Make, a C compiler, Flex, Bison, pkg-config, ncurses
development headers, and a suitable bare-metal cross compiler. For an Ubuntu
ARM32 setup:

```sh
sudo apt-get update
sudo apt-get install -y build-essential gcc-arm-none-eabi flex bison \
	libncurses-dev pkg-config doxygen graphviz default-jre
```

RISC-V boards require a compatible RISC-V toolchain. CPU-specific flags are
selected by Kconfig; the compiler must recognize the selected core.

Initialize the pinned dt2c source and headers after cloning:

```sh
git submodule update --init tools/dt2c
```

Linux x86_64 builds use the checked-in musl-static `tools/bin/dt2c`, so Rust is
not required for the normal build. Other hosts can build the submodule with
Cargo, or select an external compatible binary and headers:

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
- [Driver model and initcalls](docs/driver-model.md)
- [Compile-time device tree](docs/devicetree.md)
- [Generated API documentation](https://syterkit.yuzukihd.top/api/html/)

## License

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FYuzukiHD%2FSyterKit.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FYuzukiHD%2FSyterKit?ref=badge_large)
