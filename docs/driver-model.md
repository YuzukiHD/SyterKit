# Driver Architecture

SyterKit keeps a short, visible boot sequence and small hardware
interfaces. It does not use a runtime driver/device registry. Every
application owns its device instances, chooses the SoC driver
that it was built for, and calls the driver directly.

## Three layers

The code is split into three simple layers:

1. `dts/include/dt-compatible/` contains always-inline readers for data that
   belongs in the board description. Chip-specific register bases are not read
   from DTS; the selected SoC driver owns those constants.
2. `drivers/` contains the common protocol code and the selected SoC
   implementation. Hardware differences live in that SoC implementation and
   are exposed through a small `ops` table when a subsystem has more than one
   operation.
3. `boards/<board>/<app>/main.c` is the composition layer. It decides the order
   and error policy for that application; `main()` sequences its own HAL
   calls.

This keeps the call graph obvious and allows two applications on one board to
initialize different peripherals without hidden global state.

## Explicit initialization

A normal application follows this shape:

```c
int main(void)
{
	static sunxi_serial_t uart;
	static sunxi_i2c_t i2c;
	static axp_pmu_t pmu;
	static sunxi_dram_t dram;

	if (sunxi_serial_dt_read_stdout(&uart) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;
	sunxi_serial_init(&uart);

	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    pmu_axp2202_config(&pmu, &i2c) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;

	sunxi_clk_init();
	sunxi_i2c_init(&i2c);
	if (pmu_axp2202_init(&pmu) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;

	/* Read DRAM parameters, then invoke the selected SoC DRAM implementation. */
	return sunxi_dram_dt_read_alias(&dram, "dram0") == DRIVER_OK &&
	       sunxi_dram_init(&dram) != 0U ? 0 : DRIVER_ERROR_INVALID;
}
```

The actual order is application-specific. For example, a remoteproc loader can
read its DT resources, call `sunxi_remoteproc_prepare()`, load firmware, and
then call `sunxi_remoteproc_start()`. There is no implicit startup pass before
`main()` and no retry through unrelated driver implementations.

## SoC operations

Subsystems with materially different chips use an operations table selected by
the build:

```c
typedef struct {
	int (*reset)(sunxi_remoteproc_t *remoteproc);
	int (*prepare)(sunxi_remoteproc_t *remoteproc);
	int (*start)(sunxi_remoteproc_t *remoteproc);
} sunxi_remoteproc_ops_t;

/* Defined by the selected rproc-sun*.c file. */
extern const sunxi_remoteproc_ops_t sunxi_remoteproc_ops;
```

The DT reader only fills generic firmware and register data, then attaches the
single `sunxi_remoteproc_ops` exported by the selected SoC source. DRAM follows
the same boundary: every SoC source owns the parameter layout, while the reader
only copies the fixed 32-word `allwinner,dram-parameters` array.

PMUs are selected by their real chip API (`pmu_axp2202_config()`,
`pmu_axp1530_config()`, and so on). PMU selection is not represented as a
positional role in Kconfig or DTS. The only board data passed to PMU
configuration is the already selected I2C controller; fixed PMU addresses
belong to the PMU implementation.

## Multi-phase initialization

A thin stage wrapper around explicit calls keeps long initializations legible:
`board_early_init()`, `board_init()`, `board_late_init()`, and
`board_final_init()` delegate to chip-specific functions and HAL operations.
An application that needs several phases can use the same local style:

```c
static int board_early_init(void) { /* clocks, pinctrl, console */ }
static int board_init(void)       { /* I2C, PMU, DRAM */ }
static int board_late_init(void)  { /* storage and remote processors */ }
```

These are ordinary functions called by that application's `main()`, not linker
callbacks. A small static `ops` table is appropriate when there are multiple
implementations of one protocol. Linker lists are useful only for
registries that genuinely need enumeration; they should not be used to select
between incompatible SoC drivers.

## Adding a driver

Keep the public data types and operations in `include/drivers/`, put common
protocol code in a shared source file, and put SoC register work in a named
`drivers/<subsystem>/<subsystem>-sun*.c` file. Add a focused DT reader only for
board resources that are truly data. Add a host test for valid and invalid DT
properties, and test the selected ops with a mock where hardware access is not
available.

Continue with [Compile-time device tree](devicetree.md).
