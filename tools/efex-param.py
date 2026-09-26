#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0+
"""Edit and decode the SyterKit eFEX parameter area (ABI v1, include/efex.h).

FILE is either an eFEX image (*_efex.bin, located through the "SKEP"
descriptor at +0x30) or a raw 4 KiB parameter area, e.g. one read back from
SRAM after exec.

  efex-param.py addr IMAGE                 print the SRAM address to FEL-write
  efex-param.py set FILE KEY=VALUE...      add/replace entries in place
  efex-param.py set FILE -o AREA KEY=...   write only the 4 KiB area to AREA
  efex-param.py dump FILE                  decode header and entries

Keys:
  uart.<field>  twi.<field>        field: base id rate pin0 mux0 pin1 mux1
                                   gpio_base gpio_bank0 gate_reg gate_bit
                                   rst_reg rst_bit parent_clk
                                   (uart only: parity stop dlen)
                                   pins accept "PB9"-style names
  rail.<pmu>.<rail>                voltage in mV, the application's rail_mv[pmu][rail]
  dram.<field>  psram.<field>      field: para[N] para_count base size
                                   (outputs: size_mb init_ok)
  app.<n>                          application-defined, 24-bit n
  0xKKKKKKKK                       raw key
"""

import argparse
import re
import struct
import sys

DESC_OFFSET = 0x30
RUN_ADDR_OFFSET = 0x20
DESC_MAGIC = b"SKEP"
MAGIC = 0x41504B53
VERSION = 1
AREA_SIZE = 4096
HDR = struct.Struct("<IHHHHIIi8x")
ENT = struct.Struct("<II")
CAPACITY = (AREA_SIZE - HDR.size) // ENT.size

APPLIED = 1 << 31
OUTPUT = 1 << 30
FLAGS = APPLIED | OUTPUT

STATUS = {0: "idle", 0x454E4F44: "DONE", 0x4C494146: "FAIL", 0x50444142: "BADP"}

# Must match include/efex.h; test/cases/efex_param checks this.
GROUPS = {"uart": 0x01, "twi": 0x02, "rail": 0x03, "dram": 0x04, "psram": 0x05, "app": 0x3F}
BUS_FIELDS = {
    "base": 0x00, "id": 0x01, "rate": 0x02, "pin0": 0x03, "mux0": 0x04,
    "pin1": 0x05, "mux1": 0x06, "gpio_base": 0x07, "gpio_bank0": 0x08,
    "gate_reg": 0x09, "gate_bit": 0x0A, "rst_reg": 0x0B, "rst_bit": 0x0C,
    "parent_clk": 0x0D,
}
UART_FIELDS = dict(BUS_FIELDS, parity=0x0E, stop=0x0F, dlen=0x10)
MEM_FIELDS = {"para": 0x00, "para_count": 0x01, "base": 0x02, "size": 0x03, "size_mb": 0x10, "init_ok": 0x11}
FIELDS = {"uart": UART_FIELDS, "twi": BUS_FIELDS, "dram": MEM_FIELDS, "psram": MEM_FIELDS}
ALIASES = {"baud": "rate", "speed": "rate", "tx": "pin0", "tx_mux": "mux0", "rx": "pin1",
           "rx_mux": "mux1", "scl": "pin0", "scl_mux": "mux0", "sda": "pin1", "sda_mux": "mux1"}


def make_key(group, ident, index):
    return (group & 0x3F) << 24 | (ident & 0xFFFF) << 8 | (index & 0xFF)


def parse_key(name):
    if re.fullmatch(r"0x[0-9a-fA-F]+", name):
        return int(name, 16) & ~FLAGS
    parts = name.split(".")
    group = parts[0]
    if group not in GROUPS:
        raise ValueError(f"unknown group in {name!r}")
    if group == "app":
        if len(parts) != 2:
            raise ValueError(f"{name!r}: expected app.<n>")
        return GROUPS[group] << 24 | int(parts[1], 0) & 0xFFFFFF
    if group == "rail":
        if len(parts) != 3:
            raise ValueError(f"{name!r}: expected rail.<pmu>.<rail>")
        return make_key(GROUPS[group], int(parts[1], 0), int(parts[2], 0))
    if len(parts) != 2:
        raise ValueError(f"{name!r}: expected {group}.<field>")
    m = re.fullmatch(r"(\w+?)(?:\[(\w+)\])?", parts[1])
    field = ALIASES.get(m.group(1), m.group(1))
    if field not in FIELDS[group]:
        raise ValueError(f"unknown field in {name!r}")
    index = int(m.group(2), 0) if m.group(2) else 0
    return make_key(GROUPS[group], FIELDS[group][field], index)


def key_name(key):
    group, ident, index = key >> 24 & 0x3F, key >> 8 & 0xFFFF, key & 0xFF
    for gname, gid in GROUPS.items():
        if gid != group:
            continue
        if gname == "app":
            return f"app.{key & 0xFFFFFF}"
        if gname == "rail":
            return f"rail.{ident}.{index}"
        for fname, fid in FIELDS[gname].items():
            if fid == ident:
                return f"{gname}.{fname}[{index}]" if fname == "para" else f"{gname}.{fname}"
    return f"0x{key & ~FLAGS:08x}"


def parse_value(text):
    m = re.fullmatch(r"P([A-N])(\d+)", text, re.IGNORECASE)
    if m:  # GPIO_PIN(port, n)
        return (ord(m.group(1).upper()) - ord("A")) << 5 | int(m.group(2))
    return int(text, 0) & 0xFFFFFFFF


def locate(data):
    """Return the offset of the parameter area inside data."""
    if data[DESC_OFFSET:DESC_OFFSET + 4] == DESC_MAGIC:
        addr, size, version = struct.unpack_from("<III", data, DESC_OFFSET + 4)
        run_addr, = struct.unpack_from("<I", data, RUN_ADDR_OFFSET)
        if size != AREA_SIZE or version != VERSION:
            raise ValueError(f"unsupported descriptor: size={size} version={version}")
        return addr - run_addr
    if struct.unpack_from("<I", data, 0)[0] == MAGIC:
        return 0
    raise ValueError("neither an eFEX image nor a parameter area")


def read_area(data, offset):
    magic, version, hdr_size, count, capacity, checksum, status, ret = HDR.unpack_from(data, offset)
    if magic != MAGIC or version != VERSION or hdr_size != HDR.size or capacity != CAPACITY or count > CAPACITY:
        raise ValueError("invalid parameter area header")
    entries = [ENT.unpack_from(data, offset + HDR.size + i * ENT.size) for i in range(count)]
    return {"checksum": checksum, "status": status, "ret": ret, "entries": entries}


def checksum(entries):
    return sum(k + v for k, v in entries) & 0xFFFFFFFF


def write_area(data, offset, area):
    entries = area["entries"]
    HDR.pack_into(data, offset, MAGIC, VERSION, HDR.size, len(entries), CAPACITY,
                  checksum(entries), area["status"], area["ret"])
    for i in range(CAPACITY):
        ENT.pack_into(data, offset + HDR.size + i * ENT.size, *(entries[i] if i < len(entries) else (0, 0)))


def cmd_addr(args):
    data = open(args.file, "rb").read()
    if data[DESC_OFFSET:DESC_OFFSET + 4] != DESC_MAGIC:
        sys.exit("no SKEP descriptor at +0x30")
    addr, size, version = struct.unpack_from("<III", data, DESC_OFFSET + 4)
    print(f"0x{addr:08x} {size} v{version}")


def cmd_set(args):
    data = bytearray(open(args.file, "rb").read())
    offset = locate(data)
    area = read_area(data, offset)
    entries = area["entries"]
    for item in args.items:
        name, _, text = item.partition("=")
        key, value = parse_key(name), parse_value(text)
        for i, (k, _) in enumerate(entries):
            if k & ~FLAGS == key:
                entries[i] = (key, value)
                break
        else:
            if len(entries) >= CAPACITY:
                sys.exit("parameter area is full")
            entries.append((key, value))
    area["status"], area["ret"] = 0, 0
    write_area(data, offset, area)
    if args.output:
        open(args.output, "wb").write(data[offset:offset + AREA_SIZE])
    else:
        open(args.file, "wb").write(data)


def cmd_dump(args):
    data = open(args.file, "rb").read()
    area = read_area(data, locate(data))
    entries = area["entries"]
    ok = area["checksum"] in (0, checksum(entries))
    print(f"status {STATUS.get(area['status'], hex(area['status']))}  ret {area['ret']}  "
          f"entries {len(entries)}/{CAPACITY}  checksum {'ok' if ok else 'BAD'}")
    for key, value in entries:
        flags = ("A" if key & APPLIED else "-") + ("O" if key & OUTPUT else "-")
        print(f"  {flags} 0x{key & ~FLAGS:08x} {key_name(key):<24} 0x{value:08x} ({value})")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("addr")
    p.add_argument("file")
    p.set_defaults(func=cmd_addr)
    p = sub.add_parser("set")
    p.add_argument("file")
    p.add_argument("-o", "--output", help="write only the 4 KiB area here")
    p.add_argument("items", nargs="+", metavar="KEY=VALUE")
    p.set_defaults(func=cmd_set)
    p = sub.add_parser("dump")
    p.add_argument("file")
    p.set_defaults(func=cmd_dump)
    args = parser.parse_args()
    try:
        args.func(args)
    except ValueError as err:
        sys.exit(f"efex-param: {err}")


if __name__ == "__main__":
    main()
