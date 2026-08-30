# SyterKit Documentation

![SyterKit LOGO](assets/SyterKit%20LOGO_Thin.png)

SyterKit is a bare-metal firmware framework and bootloader toolkit for Allwinner
SoCs. It supports ARM and RISC-V targets, board bring-up, peripheral drivers,
image loading, and booting operating systems from SD/eMMC and SPI flash.

This page covers the build workflow and the Allwinner boot image format. For
subsystem details, see [Driver architecture](driver-model.md) and
[Compile-time device tree](devicetree.md). Generated declarations are available
in the [API documentation](https://syterkit.yuzukihd.top/api/html/).

## Getting started

### Install dependencies

The build requires GNU Make, a host C compiler, Flex, Bison, pkg-config,
ncurses development headers, and a bare-metal compiler for the selected target.
The API documentation additionally needs Doxygen, Graphviz, Java, and PlantUML.

For a typical Ubuntu ARM32 environment:

```sh
sudo apt-get update
sudo apt-get install -y build-essential gcc-arm-none-eabi flex bison \
	libncurses-dev pkg-config doxygen graphviz default-jre plantuml
```

Boards running C906 or E907 cores require a RISC-V compiler that supports the
CPU flags selected by Kconfig.

The checked-in Linux x86_64 dt2c distribution includes both the musl-static
binary and matching headers. Initialize the submodule only when dt2c must be
built from source:

```sh
git submodule update --init tools/dt2c
```

Linux x86_64 uses `tools/bin/dt2c` without requiring the submodule. Rust is only
needed when the prebuilt binary cannot run and the tool must be built from the
pinned submodule. A compatible external installation can be selected with
`DT2C` and `DT2C_INCLUDE`.

### Configure a board

List the available board configurations and select one:

```sh
make list-defconfigs
make tinyvision_defconfig
```

The defconfig selects the board, architecture, drivers, and the applications
that are built by default. Use `make menuconfig` to inspect or change those
options. `make list-apps` prints the applications selected for the active board.
Configuration files are grouped by board in `configs/<board>/`; Make translates
those paths into flat targets such as `avaota-a1_defconfig` and
`avaota-a1_efex_defconfig`.

### Build

Build every selected application:

```sh
make -j$(nproc)
```

Build a single application by name:

```sh
make syter_boot
```

Use an output directory without modifying the source tree:

```sh
make O=out tinyvision_defconfig
make O=out -j$(nproc)
```

Cross compiler prefixes can be overridden on the command line:

```sh
make CROSS_COMPILE=/opt/toolchains/arm-none-eabi- -j$(nproc)
```

Additional targets include:

| Target | Result |
| --- | --- |
| `make firmware` | Board-specific companion firmware |
| `make utilities` | Standalone BL33 utility images |
| `make artifacts` | Applications, companion firmware, and utilities |
| `make test` | Host tests and ARM/RISC-V QEMU tests |
| `make docs` | Doxygen API documentation in `docs/api/html/` |
| `make check` | Make/Kconfig source-tree consistency checks |

### Output files

Application outputs are stored in `build/<board>/<application>/`, or under the
chosen `O=` directory:

| Output | Description |
| --- | --- |
| `<app>_fel.bin` | SRAM image intended for host-side FEL loading |
| `<app>_card.bin` | eGON image padded to 512-byte media blocks |
| `<app>_spi.bin` | eGON image padded to 8192-byte flash blocks |
| `<app>_fel.elf` | ELF linked for the FEL SRAM address |
| `<app>_bin.elf` | ELF linked for the boot-media SRAM address |
| `*.map` | Linker map for the corresponding ELF |

## Writing boot media

Raw writes can destroy a partition table or filesystem. Verify the output file
and target device before running these commands.

### SD card and eMMC

Allwinner BROM normally checks sector 16, an 8 KiB offset, for an eGON boot
image. This location leaves the first sectors available for an MBR:

```text
+-----------+------------------+-----------------------------+
| 0 - 8 KiB | SyterKit image   | Remaining media             |
+-----------+------------------+-----------------------------+
| MBR area  | eGON.BT0 header  | Filesystems / payload data  |
+-----------+------------------+-----------------------------+
```

Write a card image to that location with:

```sh
sudo dd if=syter_boot_card.bin of=/dev/sdX bs=1024 seek=8 conv=fsync
```

Many Allwinner BROM implementations also check sector 256, a 128 KiB offset.
Use this location when the first 128 KiB must remain available for GPT data:

```sh
sudo dd if=syter_boot_card.bin of=/dev/sdX bs=1024 seek=128 conv=fsync
```

### SPI NAND

The SPI image is padded and checksummed by `tools/mksunxi`. A simple redundant
layout places copies at offsets 0, 64 KiB, and 128 KiB:

```sh
dd if=syter_boot_spi.bin of=spi.img bs=2k
dd if=syter_boot_spi.bin of=spi.img bs=2k seek=32
dd if=syter_boot_spi.bin of=spi.img bs=2k seek=64
```

Optional payloads can be placed later in the same image when the application
expects those locations:

```sh
dd if=sunxi.dtb of=spi.img bs=2k seek=128
dd if=zImage of=spi.img bs=2k seek=256
```

Program the image with xfel:

```sh
xfel spinand write 0 spi.img
```

### SPI NOR

The same redundant layout can be used for SPI NOR:

```sh
dd if=syter_boot_spi.bin of=spi.img bs=2k
dd if=syter_boot_spi.bin of=spi.img bs=2k seek=32
dd if=syter_boot_spi.bin of=spi.img bs=2k seek=64
xfel spinor write 0 spi.img
```

Raw NAND devices that require an NFTL layer are not supported by SyterKit.

## How an Allwinner device boots

Allwinner SoCs start in immutable BROM. Depending on the boot mode and media,
BROM searches for a valid first-stage image, copies it into SRAM, verifies its
header and checksum, then jumps to its entry instruction. SyterKit boot-media
images use the traditional `eGON.BT0` header expected by this flow.

The exact media search order and supported offsets are SoC-specific. The board
documentation and SoC user manual remain authoritative when they differ from
the common layout above.

### Boot header

Each board provides a header definition in `boards/<board>/head.c`. The linker
places it in `.boot0_head` before the normal startup code. Its important fields
are:

| Field | Purpose |
| --- | --- |
| `jump_instruction` | Architecture instruction that jumps over the header |
| `magic` | Eight-byte `eGON.BT0` signature |
| `check_sum` | Additive checksum written by `mksunxi` |
| `length` | Padded image length written by `mksunxi` |
| `pub_head_size` | Size of the public boot header |
| `pub_head_vsn` | Header version, currently `3000` |
| `ret_addr`, `run_addr` | Link-time SRAM addresses used by the image |
| `boot_cpu`, `platform` | Boot CPU and platform metadata |

The linked ELF initially contains a checksum stamp. When producing
`<app>_card.bin` or `<app>_spi.bin`, Make invokes `tools/mksunxi` with a 512-byte
or 8192-byte alignment. The tool pads the image, updates `length`, replaces the
stamp with the checksum seed, sums the complete padded image as 32-bit words,
and stores the result in `check_sum`.

### ARM jump instruction

On ARM boards, the first word is an unconditional A32 `B` instruction. The
upper byte is `0xea`; the signed 24-bit immediate encodes the word offset from
the architectural PC value to the first instruction after the boot header:

```text
31              28 27       24 23                         0
+-----------------+-----------+----------------------------+
| cond = 1110     | 1010      | signed immediate (24 bits) |
+-----------------+-----------+----------------------------+
```

The board header computes the immediate from `sizeof(boot_file_head_t)`, so the
processor skips the metadata and enters the normal startup sequence.

The ARM board headers express that calculation directly:

```c
#define BROM_FILE_HEAD_SIZE_OFFSET \
	(((sizeof(boot_file_head_t) + sizeof(int)) / sizeof(int)) + 1)
#define JUMP_INSTRUCTION \
	(BROM_FILE_HEAD_SIZE_OFFSET | 0xea000000)
```

### RISC-V jump instruction

RISC-V boards encode a `JAL x0, offset`, conventionally written as `j offset`.
The byte offset is split across the J-type immediate fields:

```text
31          30        21 20 19        12 11      7 6       0
+-------------+----------+--+------------+----------+---------+
| imm[20]     | imm[10:1]|imm[11] imm[19:12] | rd=0 | 1101111 |
+-------------+----------+--+------------+----------+---------+
```

Each RISC-V `head.c` computes those fields from its required header size and
padding. Using `rd = x0` discards the return address because boot never returns
to the metadata word.

The J-type immediate is assembled from the byte offset as follows:

```c
#define HEAD_BIT_10_1  ((header_bytes & 0x0007fe) >> 1)
#define HEAD_BIT_11    ((header_bytes & 0x000800) >> 11)
#define HEAD_BIT_19_12 ((header_bytes & 0x0ff000) >> 12)
#define HEAD_BIT_20    ((header_bytes & 0x100000) >> 20)

#define JUMP_OFFSET ((HEAD_BIT_20 << 31) | (HEAD_BIT_10_1 << 21) | \
		     (HEAD_BIT_11 << 20) | (HEAD_BIT_19_12 << 12))
#define JUMP_INSTRUCTION (JUMP_OFFSET | 0x6f)
```

## Runtime initialization

After the architecture startup code establishes the stack, clears BSS, and
performs required CPU setup, control enters the selected application `main()`.
The application explicitly reads its console, clocks, buses, memory, and other
devices in the order required by that boot flow.

Board hardware used by drivers comes from `boards/<board>/board.dts`. dt2c
validates that tree and emits immutable C data during the host build. This is
separate from a Linux payload DTB: SyterKit uses dt2c for its own static hardware
description and libfdt for a DTB that will be handed to a loaded kernel.

Continue with:

- [Driver architecture](driver-model.md)
- [Compile-time device tree](devicetree.md)
- [SyterKit source repository](https://github.com/YuzukiHD/SyterKit)
