# Compile-time device tree

SyterKit uses Linux-style DTS to describe the hardware needed by its own
firmware. The host-side [dt2c](https://github.com/YuzukiTsuru/dt2c) tool parses,
preprocesses, validates, and converts the selected board tree into immutable C
data during the build.

This is not the Linux payload DTB:

| Tree | Consumer | Interface | Lifetime |
| --- | --- | --- | --- |
| `boards/<board>/board.dts` | SyterKit drivers | dt2c and `<dt2c/dt.h>` | Compiled into firmware, then optimized where possible |
| Linux `.dtb` payload | A loaded kernel and SyterKit image code | libfdt | External binary passed to the next boot stage |

Both paths are current and supported. dt2c handles fixed SyterKit board data;
libfdt handles a runtime flattened tree that must remain in DTB form.

## Build flow

After a defconfig selects a board and its drivers, Make performs this flow:

```text
boards/<board>/board.dts
          + dts/include files
          + dts/bindings
          + compatible declarations from selected drivers/
                              |
                              v
                            dt2c
                              |
          +-------------------+-------------------+
          |                   |                   |
 generated/fdt_generated.h  devicetree.json   devicetree.d
          |
          v
  always-inline readers in dts/include/dt-compatible/
          |
          v
  static driver configuration and device instances
```

Generated files are board-qualified below
`.obj/boards/<board>/dt2c/`, including:

- `include/generated/fdt_generated.h`, the compiled tree consumed by
  `<dt2c/dt.h>`;
- `devicetree.json`, a report of normalized nodes, binding selection, and driver
  matching;
- `devicetree.d`, dependencies on DTS includes, bindings, and selected driver
  sources;
- `selected-drivers`, the source manifest produced from the enabled `drivers/`
  Kbuild graph.

The board-qualified path prevents stale generated data from one defconfig being
reused for another board in the same `O=` output tree.

## Obtaining dt2c

The Linux x86_64 distribution in `tools/bin/` contains the musl-static binary
and its matching public headers. It works without Rust or an initialized
submodule. Initialize the submodule only when building dt2c from source:

```sh
git submodule update --init tools/dt2c
```

On Linux x86_64, Make prefers `tools/bin/dt2c` and its headers under
`tools/bin/include`. If the binary cannot run, Make builds the submodule with
Cargo into `.obj/tools/dt2c/`. Other compatible installations can be selected
explicitly:

```sh
make tinyvision_defconfig
make DT2C=/opt/dt2c/dt2c \
	DT2C_INCLUDE=/opt/dt2c/include -j$(nproc)
```

The binary and `<dt2c/dt.h>` headers must come from compatible revisions.

## DTS structure

A board tree uses normal DTS nodes, labels, phandles, aliases, `status`, and
compatible strings. The exact required properties are defined in
`dts/bindings/`.

The console is a representative example:

```dts
/ {
	aliases {
		serial0 = &uart0;
	};

	chosen {
		stdout-path = &uart0;
	};

	pio: pinctrl@2000000 {
		compatible = "allwinner,sunxi-pinctrl";
		reg = <0x02000000 0x800>;
		status = "okay";

		uart0_pins: uart0-pins {
			allwinner,pins =
				<SUNXI_GPIO_PORT_B 9 2>,
				<SUNXI_GPIO_PORT_B 10 2>;
		};
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
		data-bits = <8>;
		stop-bits = <1>;
		parity = "none";
		status = "okay";
	};
};
```

dt2c resolves `stdout-path = &uart0` into the selected node path. The serial
reader validates that node, follows its pinctrl phandle, and fills `uart_dbg`.
Only that node becomes the early console. Additional UARTs must be instantiated
separately and cannot overwrite the console configuration.

## Static readers and optimization

Compatibility readers live under `dts/include/dt-compatible/`. They are
`static inline __attribute__((always_inline))` functions that call the native
dt2c API with visible node offsets and property-name constants.

This shape is deliberate. Because the board tree and queried names are fixed,
the compiler can fold property reads into direct configuration assignments.
With function and data sections enabled, linker garbage collection can then
discard unused compiled-tree data and lookup paths.

Do not hide fixed lookups behind an out-of-line generic property layer. That
would obscure constants from the optimizer and retain parser-like code and tree
storage in SRAM-constrained images.

## Driver compatibility declarations

Every DTS-backed driver declares the compatible strings it owns in its source:

```c
DT2C_DRIVER_COMPAT("allwinner,sunxi-i2c");
```

Make recursively follows the selected `drivers/` Kbuild objects and generates
the dt2c driver manifest. There is no hand-maintained `drivers.list` or explicit
Make variable listing every driver source. Files outside `drivers/`, such as
board-specific LCD or OLED applications, are not treated as device drivers and
do not participate in this scan.

Bindings marked with `dt2c,device: true` cause dt2c to reject an enabled node
when none of the selected drivers owns a compatible string. This catches
configuration errors before target code is linked.

## One driver, multiple instances

Compatible matching describes a driver type, not a singleton. A board may
enable several I2C, SPI, MMC, PWM, or other controller nodes using the same
compatible. Each selected node is read into its own static configuration and
device state.

Relationships should be represented in DTS rather than recovered through a
runtime alias scan. For example, a PMIC is a child of the I2C controller that
transports it:

```dts
i2c0: i2c@2502000 {
	compatible = "allwinner,sunxi-i2c";
	reg = <0x02502000 0x400>;
	status = "okay";

	pmic@36 {
		compatible = "x-powers,axp1530";
		reg = <0x36>;
		status = "okay";
	};
};
```

The PMIC reader follows its parent node directly. The application chooses the
specific resulting instance it needs; the driver does not walk every alias at
runtime or publish artificial alias symbols.

Aliases remain useful when they express a stable board-level identity, such as
`mmc0`, but they are not a substitute for parent, child, and phandle
relationships already present in the tree.

## Adding or migrating a device

1. Add or update the schema below `dts/bindings/`, including required resources
   and phandle relationships.
2. Add `DT2C_DRIVER_COMPAT("vendor,device")` at the end of the owning driver
   source under `drivers/`.
3. Add the board instances to `boards/<board>/board.dts` with Linux-style
   resources, pinctrl, relationships, and `status`.
4. Add an always-inline reader below `dts/include/dt-compatible/` that validates
   the node before publishing configuration.
5. Give every enabled instance separate static storage. One driver may bind all
   compatible devices, but their platform and runtime state must not be shared.
6. Add focused dt2c tests for valid properties, missing required properties,
   disabled parents, phandle resolution, and multiple instances.
7. Compare image size and symbols to confirm constant folding removed unused
   tree data and helper paths.

SoC-global startup registers, handoff code, and silicon workarounds that are not
device instances may remain platform-defined. Device resources consumed by
drivers should come from the compiled board tree.

Continue with [Driver model and initcalls](driver-model.md) for registration,
binding, and initialization order.
