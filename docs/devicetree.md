# Compile-time device tree

SyterKit uses Linux-style DTS as the board hardware-description source. The
host-side `dt2c` tool preprocesses and validates the selected
`boards/<board>/board.dts`, resolves labels and phandles, checks enabled devices
against bindings and selected drivers, and emits immutable C data. Firmware
does not carry a DTB, run a general DTB parser, or allocate device-tree objects.

This board tree is independent from a Linux payload DTB. Board drivers query
the compiled tree only through the namespaced `<dt2c/dt.h>` backend. The
existing libfdt code remains reserved for inspecting and updating the external
DTB passed to the kernel; it is not used to register SyterKit devices.

The build places board-qualified outputs below
`.obj/boards/<board>/dt2c/`. This prevents one board's generated header from
being reused when an `O=` directory builds several defconfigs. The directory
also contains `devicetree.json`, which records normalized nodes, binding
selection, and driver matching, and a Make depfile covering DTS includes,
bindings, and driver sources.

## Build dependency

The `tools/dt2c` submodule pins the generator and matching `<dt2c/dt.h>`
headers. Initialize it once after cloning:

```sh
git submodule update --init tools/dt2c
```

On Linux x86_64, the build first uses the checked-in musl static executable at
`tools/bin/dt2c`. If that binary cannot run, Make builds the pinned source with
Cargo into the selected `O=` object tree. An external compatible release can
be selected explicitly:

```sh
make tinyvision_defconfig
make DT2C=/opt/dt2c/dt2c \
     DT2C_INCLUDE=/opt/dt2c/include -j$(nproc)
```

CI executes the prebuilt binary, verifies that it has no dynamic dependencies,
and separately exercises the source-build fallback. Keeping the tool and
headers at the same revision matters because the generated X-macro format is
consumed by the dt2c runtime header.

## Runtime model

Drivers include the native `<dt2c/dt.h>` interface in the translation unit that
consumes their fixed properties. Node paths and property names therefore remain
compile-time constants. GCC folds the dt2c lookups into the final configuration
writes, and section garbage collection removes the generated tree storage and
lookup helpers from the firmware image.

There is no generic OF layer, runtime node array, or target-side DTB parser for
the board tree. Each driver owns its device selection, immutable configuration,
and static runtime state. It then registers an ordinary `struct device` with the
flat driver core. Existing board-owned platform devices remain supported.

## UART example

The debug UART is the first migrated device. A board describes it and its pin
configuration in DTS:

```dts
uart0_pins: uart0-pins {
	allwinner,pins =
		<SUNXI_GPIO_PORT_B 9 2>,
		<SUNXI_GPIO_PORT_B 10 2>;
};

uart0: serial@2500000 {
	compatible = "allwinner,sunxi-uart";
	reg = <0x02500000 0x400>;
	current-speed = <115200>;
	clock-frequency = <24000000>;
	allwinner,uart-id = <0>;
	allwinner,clock-gate = <0x0200190c 0>;
	allwinner,reset = <0x0200190c 16>;
	pinctrl-names = "default";
	pinctrl-0 = <&uart0_pins>;
	status = "okay";
};
```

The console is selected with a direct node reference, following common Linux
DTS practice:

```dts
chosen {
	stdout-path = &uart0;
};
```

`drivers/serial/serial.c` declares its compatible with
`DT2C_DRIVER_COMPAT()`. dt2c rejects an enabled UART if no selected driver owns
one of its compatible strings. The always-inline reader in
`dts/include/dt-compatible/serial-dt.h` validates the selected node and its
parent status, resolves the pinctrl phandle, and builds
`sunxi_serial_t uart_dbg`. Because the tree is fixed, this reduces to constant
assignments before the normal serial device probe. Only the selected console is
registered, so a second enabled UART cannot overwrite `uart_dbg`. Secondary
UARTs that applications configure directly can continue using static
`sunxi_serial_t` objects during migration.

## Adding a device

To migrate another device type:

1. Add a binding below `dts/bindings/` and set `dt2c,device: true` when an
   enabled node must have a selected driver.
2. Add `DT2C_DRIVER_COMPAT("vendor,device")` to the driver and include that
   source in the Kconfig-derived `dt2c_driver_sources` selection. Make writes
   dt2c's required manifest into the object tree; there is no source-tree list.
3. Describe each board instance in `boards/<board>/board.dts` with a Linux-style
   compatible, resources, phandles, and status.
4. Put always-inline compatibility readers below `dts/include/dt-compatible/`,
   read fixed properties through `<dt2c/dt.h>`, then register driver-owned
   static configuration and runtime state.

Keep node offsets and property names visible at the dt2c call site. Moving
lookups behind an out-of-line generic property API prevents constant folding
and needlessly retains the compiled tree in SRAM-constrained images.

The current migration intentionally leaves non-UART board resources in
`board.c`; they can move one driver at a time without changing existing
application-facing `sunxi_*` APIs.
