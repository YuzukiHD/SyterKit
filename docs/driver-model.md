# Driver model and initcalls

SyterKit uses a small static driver model inspired by Linux. It keeps the useful
parts for early firmware, compatible matching, probing, and ordered boot-time
initialization, without buses, classes, modules, reference counting, or dynamic
device allocation.

The model has two runtime objects:

- `struct device` represents one hardware or logical instance.
- `struct driver` supplies matching, probe, and optional remove callbacks.

Both descriptors and their referenced data must remain valid while registered.
They are normally static objects retained for the lifetime of the firmware.

## Registration and matching

Registration order does not matter. Registering a driver probes every existing
unbound device; registering a device probes the existing drivers in registration
order. The first successful match owns that device.

The default matching rules are:

1. Use the driver's custom `match` callback when one is provided.
2. Otherwise compare `compatible` strings when both objects provide one.
3. Fall back to descriptor names only when compatible matching is unavailable.

A failed probe does not remove either descriptor. The device remains registered
and unbound, so another matching driver can be tried and `device_probe()` can be
called again later. Probe failures clear `driver_data` before the next attempt.

Descriptor names must be non-empty and unique within their registry. Registering
the same descriptor twice, or reusing a registered name, returns
`DRIVER_ERROR_EXISTS`.

## One driver, multiple devices

A driver descriptor is shared. Every controller or peripheral instance has its
own device descriptor and platform data:

```c
#include <stdint.h>
#include <driver.h>

struct example_config {
	uintptr_t base;
	unsigned int id;
};

static int example_probe(struct device *device)
{
	struct example_config *config = device_get_platform_data(device);

	if (config == NULL || config->base == 0)
		return DRIVER_ERROR_INVALID;
	example_hw_init(config);
	device_set_driver_data(device, config);
	return DRIVER_OK;
}

static struct driver example_driver = {
	.name = "example",
	.compatible = "vendor,example-controller",
	.probe = example_probe,
};

static struct example_config example0_config = {
	.base = 0x02000000,
	.id = 0,
};

static struct example_config example1_config = {
	.base = 0x02001000,
	.id = 1,
};

static struct device example0 = {
	.name = "example0",
	.compatible = "vendor,example-controller",
	.platform_data = &example0_config,
};

static struct device example1 = {
	.name = "example1",
	.compatible = "vendor,example-controller",
	.platform_data = &example1_config,
};

builtin_driver(example_driver);
builtin_device(example0);
builtin_device(example1);
```

The driver must not keep all instance state in one global object. Put immutable
board configuration in `platform_data`; put state created by the driver in
`driver_data` through `device_set_driver_data()`. This keeps two devices bound to
the same driver independent.

Many SyterKit drivers derive the instance configuration from the board DTS
instead of writing the objects by hand. The ownership rule remains the same:
each enabled instance gets distinct storage, while all compatible instances use
the same driver implementation.

## Removing descriptors

`device_unregister()` calls the bound driver's `remove` callback, detaches the
device, and clears its internal links and driver data.

`driver_unregister()` removes the driver, calls `remove` for every device bound
to it, then immediately probes those devices against the remaining registered
drivers. This makes replacement drivers possible without rebuilding the device
list.

## Initcall levels

Built-in descriptors are registered through linker-collected initcalls. Startup
runs these callbacks after the stack, BSS, timer, and architecture-specific CPU
setup are ready, but before the selected application enters `main()`.

| API | Section | Intended use |
| --- | --- | --- |
| `early_initcall()` | `.initcallearly.init` | Console and prerequisites for later diagnostics |
| `core_initcall()` | `.initcall1.init` | Framework-wide core services |
| `device_initcall()` | `.initcall6.init` | Normal devices and drivers |
| `late_initcall()` | `.initcall7.init` | Work that depends on normal devices |

Plain `initcall()` uses the device level. `builtin_driver()` and
`builtin_device()` generate device-level registration callbacks;
`early_builtin_driver()` and `early_builtin_device()` provide the corresponding
early registration path.

Each macro uses `__COUNTER__` to create a unique symbol, so the same callback can
be registered more than once. The counter does not assign priority. Callbacks in
one level execute in link order, which follows the Kbuild object order.

`do_initcalls()` executes all entries once and caches the result. A callback
failure records the first non-zero return value but does not stop later
callbacks. A second call returns the cached result without running the table
again.

## Console example

The Sunxi UART shows the complete path:

1. `stdout-path` in `/chosen` selects a single UART node.
2. The always-inline DTS reader fills the static `uart_dbg` configuration.
3. An early initcall registers the `stdout` device.
4. The early built-in serial driver matches `allwinner,sunxi-uart` and probes it.
5. Logging can then be used by later initcalls and application code.

Secondary UARTs are separate devices or application-owned configurations. They
do not overwrite the console instance.

## Adding a built-in driver

For a new driver:

1. Keep its public hardware API in `include/drivers/` and its implementation in
   `drivers/`.
2. Give each device instance unique platform data and a unique device name.
3. Make `probe` validate all required resources before touching hardware.
4. Store per-instance runtime state in `driver_data`.
5. Add `DT2C_DRIVER_COMPAT()` in the driver source when the device can be
   instantiated from DTS.
6. Use the normal device initcall level unless the hardware is genuinely needed
   before core initialization.

Continue with [Compile-time device tree](devicetree.md) for DTS bindings,
instance selection, and constant folding.
