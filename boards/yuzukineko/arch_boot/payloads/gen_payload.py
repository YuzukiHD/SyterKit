#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0+
#
# Turn the linked RV32 payload (arch_boot_payload.bin, exactly 0x80 bytes)
# into a C byte-array include for boards/yuzukineko/head.c.
#
# Usage: gen_payload.py <input.bin> <output.inc>

import sys


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: gen_payload.py <input.bin> <output.inc>")

    src, dst = sys.argv[1], sys.argv[2]
    data = open(src, "rb").read()

    if len(data) != 0x80:
        sys.exit("payload is 0x%x bytes, expected 0x80" % len(data))

    lines = [
        "/* SPDX-License-Identifier: GPL-2.0+ */",
        "/*",
        " * RV32 bootstrap machine code for the C907 RV32->RV64 mode switch.",
        " * GENERATED from boards/yuzukineko/arch_boot/payloads/arch_boot.S by",
        " * the arch_boot payload Makefile -- do not edit by hand.",
        " * Regenerate with:  make -C boards/yuzukineko/arch_boot/payloads",
        " */",
    ]
    for i in range(0, len(data), 16):
        lines.append("    " + ", ".join("0x%02x" % b for b in data[i:i + 16]) + ",")

    with open(dst, "w") as f:
        f.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
