# Driver model and initcalls

SyterKit uses a small, static driver core inspired by the Linux device model.
It intentionally has only two runtime objects: `struct device` describes one
board-owned instance, and `struct driver` provides matching, probe, and remove
callbacks. There are no bus, class, module, reference-counting, or dynamic
allocation layers.

Both objects must remain alive while registered. Registration order does not
matter: registering a device probes existing drivers, while registering a
driver probes existing unbound devices. The default matcher compares
`compatible` strings and falls back to descriptor names when no compatible
string is available. A driver can provide a custom `match` callback when exact
string matching is not sufficient. A descriptor remains registered if one of
these probe attempts fails, so it can bind when another driver appears or after
the device is probed again.

## Declaring a driver

The driver keeps its existing hardware API and adds a small probe adapter:

```c
#include <driver.h>

static int example_probe(struct device *device)
{
	struct example_config *config = device_get_platform_data(device);

	return example_hw_init(config);
}

static struct driver example_driver = {
	.name = "example",
	.compatible = "vendor,example",
	.probe = example_probe,
};
builtin_driver(example_driver);
```

Board code owns the configuration and device descriptor:

```c
static struct example_config example0_config = {
	.base = EXAMPLE0_BASE,
};

static struct device example0 = {
	.name = "example0",
	.compatible = "vendor,example",
	.platform_data = &example0_config,
};
builtin_device(example0);
```

`driver_data` is reserved for state produced by the driver. Use
`device_set_driver_data()` and `device_get_driver_data()` rather than changing
board-owned platform data. The core clears `driver_data` after failed probes and
when a device is detached, so an unbound device never exposes stale state from a
previous driver.

## Initcall levels

Static registration is implemented with linker-collected initcalls. Startup
runs the table after the stack, BSS, timer, and architecture-specific cache or
floating-point setup are ready, and before application `main()`.

As in Linux, each declaration gets an automatically increasing `__COUNTER__`
ID and is emitted into an initcall section. This allows the same callback to be
registered more than once and lets new callbacks extend the table without a
central list change. Plain `initcall()` is an alias for `device_initcall()`.

| Level | Intended use |
| --- | --- |
| `early_initcall()` | Console and other services needed by later diagnostics |
| `core_initcall()` | Framework-wide core services |
| `device_initcall()` | Normal devices and drivers; the default built-in level |
| `late_initcall()` | Optional work that depends on normal devices |

Each callback returns zero on success. A non-zero result is recorded as the
overall result, but does not prevent later callbacks from running.
`do_initcalls()` is idempotent and executes the linker table only once.
Startup continues into `main()` even when a callback fails; calling
`do_initcalls()` again returns the cached result without running the table
again.

Callbacks in the same level still run in link order, matching the Linux
initcall model; the counter provides identity rather than priority. Dependencies
within a level must therefore be reflected in Kbuild object order. The
Sun300iw1 clock object precedes the serial object so its pre-clock callback runs
before the early serial driver and board device registrations.

Use `early_builtin_driver()` and `early_builtin_device()` for descriptors that
must bind at the early level. The Sunxi debug UART uses this path, so board
applications no longer initialize `uart_dbg` individually. Direct `sunxi_*`
initialization APIs remain available for secondary or application-controlled
devices while other drivers migrate incrementally.

The linker scripts keep all four input sections explicitly. Adding a new level
requires updating both architecture linker scripts, the Avaota A1 board linker
scripts, and the initcall API together; arbitrary section names are not part of
the interface.
