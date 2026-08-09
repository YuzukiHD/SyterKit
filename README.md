# SyterKit 0.5

SyterKit 0.5 is a bare-metal firmware framework for Allwinner SoCs. The tree uses
GNU Make, recursive Kbuild-style `obj-y` lists, and a Kconfig implementation built
from the C, Flex, and Bison sources in `scripts/kconfig`.

## Configure and build

Install a suitable cross compiler plus the host build dependencies (`make`, a C
compiler, Flex, Bison, pkg-config, and ncurses development headers). Initialize
the pinned [dt2c](https://github.com/YuzukiTsuru/dt2c) source and headers after
cloning:

```sh
git submodule update --init tools/dt2c
```

Linux x86_64 builds use the checked-in, musl-linked `tools/bin/dt2c` without a
Rust toolchain. Other hosts build dt2c from the submodule with Cargo. An
external release can be selected with `DT2C=/path/to/dt2c` and, when needed,
`DT2C_INCLUDE=/path/to/include`.

Select a board configuration and build:

```sh
make tinyvision_defconfig
make -j$(nproc)
```

Use `make list-defconfigs` to list the board configurations, or `make menuconfig`
to select one interactively. A board configuration builds all applications listed
by that board. Cross compiler prefixes follow the kernel convention and can be
overridden on the command line:

```sh
make CROSS_COMPILE=/opt/toolchains/arm-none-eabi-
make O=out tinyvision_defconfig
make O=out -j$(nproc)
```

Images are written to `build/<board>/<application>/`, or below `O` when an output
directory is specified. After selecting a board, `make <application>` builds only
that application's FEL, SD/eMMC, and SPI images. `make firmware` builds companion
firmware declared by the selected board, when present. `make utilities` builds the
standalone BL33 images, while `make artifacts` builds every output class together.

## Source layout

- `arch/`: architecture code and linker scripts
- `boards/`: board startup, hardware descriptions, assets, and application `main.c`
- `configs/`: one maintainable defconfig per board
- `core/`: boot framework, logging, CLI, and image loading
- `drivers/`: driver subsystems; each driver directory owns its Kconfig and Makefile
- `dts/`: dt2c bindings, DTS includes, and compile-time compatibility readers
- `include/`: common public interfaces
- `lib/`: freestanding C and imported library routines
- `scripts/kconfig/`: source-built Kconfig front ends
- `tools/`: host tools, the pinned dt2c source, and its prebuilt Linux binary
- `utils/`: standalone target-side utility firmware and build checks

The common driver runtime is deliberately flat: static devices match static
drivers and are initialized through four boot-time initcall levels. The full
contract and migration example are documented in
[Driver model and initcalls](docs/driver-model.md). Board hardware is compiled
from Linux-style DTS as described in [Compile-time device tree](docs/devicetree.md).
