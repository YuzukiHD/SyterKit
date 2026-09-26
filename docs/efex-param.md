# eFEX runtime parameters

Returning eFEX applications (`soc/<soc>/app_efex/<app>/`) keep their hardware
settings as ordinary C driver structures. A host can override those settings
for a single run without rebuilding, and reads the results back afterwards,
through one fixed 4 KiB parameter area linked into every eFEX image. There is
no device tree involved: the area is a flat table of 32-bit key/value pairs.

The ABI is defined in `include/efex.h`; the host-side helper is
`tools/efex-param.py`.

## Locating the area

The eGON head of an eFEX image is 0x30 bytes and the entry point sits at
+0x40. The 16 bytes in between hold the parameter descriptor:

| Offset | Size | Field | Value |
| --- | --- | --- | --- |
| 0x30 | 4 | `magic` | `"SKEP"` |
| 0x34 | 4 | `addr` | SRAM address of the parameter area |
| 0x38 | 4 | `size` | 4096 |
| 0x3c | 4 | `version` | 1 |

The head's `ret_addr` (+0x1c) holds the same address, so tools that only read
the Boot ROM-visible result pointer find the area as well. The file offset is
`addr - run_addr` (`run_addr` at +0x20).

## Area layout

All fields are little-endian.

```text
0x000  u32 magic      "SKPA" (0x41504b53)
0x004  u16 version    1
0x006  u16 hdr_size   32
0x008  u16 count      entries in use
0x00a  u16 capacity   508
0x00c  u32 checksum   32-bit sum of key+value over all used entries, 0 = unchecked
0x010  u32 status     written by the target: 0 idle, "DONE", "FAIL", "BADP"
0x014  i32 ret        written by the target: main() return value
0x018  u32 reserved[2]
0x020  {u32 key, u32 value} entries[508]
```

The linked image contains a valid header with `count = 0`, so a run without
host changes uses the compiled defaults.

If the header or checksum does not validate, the target resets the table to
empty, uses its defaults for everything, still records its results, and sets
`status` to `BADP`.

## Keys

```text
 31        30       29..24   23..8   7..0
APPLIED  OUTPUT     group     id     index
```

- `APPLIED` is set by `efex_param_load()` on every host entry it applied.
  Entries the application has no target for (for example a TWI key in an app
  without a PMU) stay unmarked, so the host can see what was ignored.
- `OUTPUT` is set by the target on every entry it wrote. An output entry is
  never read back as an override.
- The host writes keys with both flag bits clear; lookups ignore them.

| Group | Name | id | index |
| --- | --- | --- | --- |
| 0x01 | `uart` | bus field | 0 |
| 0x02 | `twi` (PMU I²C) | bus field | 0 |
| 0x03 | `rail` | PMU number | rail number |
| 0x04 | `dram` | memory field | parameter word |
| 0x05 | `psram` | memory field | parameter word |
| 0x3f | `app` | key[23:0] = value number (`app.<n>`) | — |

Bus fields (`uart`, `twi`):

| id | Field | Notes |
| --- | --- | --- |
| 0x00 | `base` | controller base address |
| 0x01 | `id` | controller number |
| 0x02 | `rate` | UART baud rate, I²C speed in Hz |
| 0x03 / 0x05 | `pin0` / `pin1` | TX/RX or SCL/SDA, `GPIO_PIN(port, n)` = `port << 5 \| n` |
| 0x04 / 0x06 | `mux0` / `mux1` | pin function |
| 0x07 | `gpio_base` | pin controller base |
| 0x08 | `gpio_bank0` | first port of that controller (`GPIO_PORTL` for R_PIO); bank = port − `gpio_bank0` |
| 0x09..0x0c | `gate_reg`, `gate_bit`, `rst_reg`, `rst_bit` | bus clock gate and reset |
| 0x0d | `parent_clk` | module clock in Hz |
| 0x0e..0x10 | `parity`, `stop`, `dlen` | UART only, driver enum values |

When only a pin number changes, the bank is recomputed against the controller
of the compiled default pin, so moving TX from PB9 to PH9 needs just `pin0`
(and usually `mux0`).

Memory fields (`dram`, `psram`):

| id | Field | Direction |
| --- | --- | --- |
| 0x00 | `para[index]` | in: override word; out: value after init/training |
| 0x01 | `para_count` | in |
| 0x02 | `base` | in: CPU-visible base |
| 0x03 | `size` | in: CPU-visible window size |
| 0x10 | `size_mb` | out: detected size, 0 on failure |
| 0x11 | `init_ok` | out: 1 on success |

PMU rails index the application's `rail_mv[pmu][rail]` default table (up to
`EFEX_PARAM_RAIL_MAX` = 16 rails per PMU); each `main.c` names the rails next
to the table. The value is the voltage in mV.

Key space is not the limit: 57 groups (0x06–0x3e) are free and `app` has a
24-bit number. The limit is the 508 entries shared by host overrides and target
results; a DRAM run reports its parameter words plus two status entries.

## Host flow

1. Read the descriptor at +0x30 of `<app>_efex.bin` to get `addr`.
2. Either patch the image file before download:
   `tools/efex-param.py set init_dram_efex.bin uart.baud=1500000 rail.0.1=940`,
   or download the unmodified image and FEL-write a 4 KiB area to `addr`
   before exec: `tools/efex-param.py set init_dram_efex.bin -o area.bin ...`
   leaves the image alone and writes only the area.
3. Exec the image. Before returning, the target writes `status`, `ret`, its
   output entries and a fresh checksum, then flushes the data cache.
4. Read 4 KiB back from `addr` and decode it with
   `tools/efex-param.py dump readback.bin`.

eFEX images are FEL payloads, so patching the area does not require updating
the eGON checksum.

## Target API

Applications keep their defaults in C and apply the whole table with one call
at the start of `main()`, before any driver init. Nothing is looked up later:

```c
/* PMU rail defaults in mV as [pmu][rail]; the host may override them. */
static int rail_mv[][EFEX_PARAM_RAIL_MAX] = {
	{ 1100, 920 }, /* AXP2202: dcdc1 dcdc2 */
};

int main(void)
{
	uart_dbg = console;
	efex_param_load(&(const struct efex_param_targets){
		.uart = &uart_dbg,
		.i2c = &i2c,
		.dram = &dram,
		.rail_mv = rail_mv,
		.pmu_count = ARRAY_SIZE(rail_mv),
	});
	sunxi_serial_init(&uart_dbg);
	...
	pmu_axp2202_set_vol(&axp2202, "dcdc1", rail_mv[0][0], 1);
	...
	uint32_t dram_size = sunxi_dram_init(&dram);
	efex_param_report_dram(&dram, dram_size);
	return 0;
}
```

Defaults computed at run time (for example rail voltages selected from eFuse)
must be written into the target structures before `efex_param_load()` so the
host values win. `.psram` and `.app`/`.app_count` (an array indexed by
`app.<n>`) work the same way; unused members stay NULL. UART and TWI fields are
applied as a whole after the scan, so their order in the table does not matter.

`efex_param_put()` and `efex_param_put_array()` report extra results. The eFEX
entry calls `efex_param_finish()` with the return value of `main()`.

`test/cases/efex_param` is a host test for the table logic and
`efex_param_load()`; it also checks that `tools/efex-param.py` encodes keys the same way as
`include/efex.h`.
