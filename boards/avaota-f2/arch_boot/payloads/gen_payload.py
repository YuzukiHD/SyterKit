#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0+

import sys


# c907_boot0 computes the C907 run address as payload + 0x100. The generated
# array is padded to that fixed offset so the SyterKit entry follows it.
PAYLOAD_SIZE = 0x100


def main():
    if len(sys.argv) != 4:
        sys.exit("usage: gen_payload.py <rv32.bin> <rv64.bin> <arch_boot.c>")

    rv32_src, rv64_src, dst = sys.argv[1:]
    payloads = {
        "RV32": open(rv32_src, "rb").read(),
        "RV64": open(rv64_src, "rb").read(),
    }
    for mode, data in payloads.items():
        if len(data) > PAYLOAD_SIZE:
            sys.exit("%s payload is 0x%x bytes, maximum is 0x%x" % (mode, len(data), PAYLOAD_SIZE))

    lines = [
        "/* SPDX-License-Identifier: GPL-2.0+ */",
        "",
        "/*",
        " * Generated from arch_boot/payloads/c907_boot0.S with the standalone",
        " * E907 toolchain. Regenerate with: make -C boards/avaota-f2/arch_boot/payloads",
        " */",
        "const __attribute__((section(\".boot0_head\"), aligned(64), used)) unsigned char arch_boot_payload[] = {",
        "#ifdef CONFIG_ARCH_RISCV64",
    ]
    payloads["RV64"] = payloads["RV64"].ljust(PAYLOAD_SIZE, b"\0")
    payloads["RV32"] = payloads["RV32"].ljust(PAYLOAD_SIZE, b"\0")
    for offset in range(0, PAYLOAD_SIZE, 16):
        chunk = payloads["RV64"][offset:offset + 16]
        lines.append("\t" + ", ".join("0x%02x" % byte for byte in chunk) + ",")
    lines.append("#else")
    for offset in range(0, PAYLOAD_SIZE, 16):
        chunk = payloads["RV32"][offset:offset + 16]
        lines.append("\t" + ", ".join("0x%02x" % byte for byte in chunk) + ",")
    lines.extend(["#endif", "};"])

    with open(dst, "w") as output:
        output.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
