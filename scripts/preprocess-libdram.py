#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0+

import argparse
import os
import subprocess
import tempfile
from pathlib import Path


DATA_SPLIT = 0xE404
DATA_SIZE = 0x217E2
MEMBER_NAME = "libdram.o"


def run(command, cwd=None, capture_output=False):
    return subprocess.run(
        command,
        cwd=cwd,
        check=True,
        capture_output=capture_output,
        text=capture_output,
    )


def archive_members(ar, archive):
    result = run([ar, "t", str(archive)], capture_output=True)
    return [line for line in result.stdout.splitlines() if line]


def parse_args():
    parser = argparse.ArgumentParser(
        description="Keep one DRAM firmware variant in a libdram archive."
    )
    parser.add_argument("input", type=Path, help="input libdram.a")
    parser.add_argument("output", type=Path, help="output DRAM-specific archive")
    parser.add_argument(
        "dram_type", choices=("lpddr4", "lpddr5"), help="firmware to keep"
    )
    parser.add_argument("--ar", default=os.environ.get("AR", "arm-none-eabi-ar"))
    parser.add_argument(
        "--objcopy", default=os.environ.get("OBJCOPY", "arm-none-eabi-objcopy")
    )
    parser.add_argument(
        "--ranlib", default=os.environ.get("RANLIB", "arm-none-eabi-ranlib")
    )
    return parser.parse_args()


def main():
    args = parse_args()
    members = archive_members(args.ar, args.input)
    if members != [MEMBER_NAME]:
        raise SystemExit(
            f"{args.input} must contain exactly {MEMBER_NAME}; found {members!r}"
        )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix=".syterkit-libdram-", dir=args.output.parent
    ) as temp:
        workdir = Path(temp)
        run([args.ar, "x", str(args.input.resolve())], cwd=workdir)

        object_file = workdir / MEMBER_NAME
        data_file = workdir / "data.bin"
        selected_data = workdir / "data.selected.bin"
        run(
            [args.objcopy, "--dump-section", f".data={data_file}", str(object_file)]
        )

        data = data_file.read_bytes()
        if len(data) != DATA_SIZE:
            raise SystemExit(
                f"{object_file} has .data size 0x{len(data):x}; "
                f"expected 0x{DATA_SIZE:x}"
            )

        if args.dram_type == "lpddr4":
            selected_data.write_bytes(data[:DATA_SPLIT])
            update_section = [
                args.objcopy,
                "--update-section",
                f".data={selected_data}",
                str(object_file),
                str(workdir / "libdram.preprocessed.o"),
            ]
        else:
            selected_data.write_bytes(data[DATA_SPLIT:])
            update_section = [
                args.objcopy,
                "--update-section",
                f".data={selected_data}",
                "--change-section-vma",
                f".data=-0x{DATA_SPLIT:x}",
                str(object_file),
                str(workdir / "libdram.preprocessed.o"),
            ]

        run(update_section)
        preprocessed_object = workdir / "libdram.preprocessed.o"
        preprocessed_object.replace(object_file)

        archive = workdir / "libdram.preprocessed.a"
        run([args.ar, "crD", str(archive), MEMBER_NAME], cwd=workdir)
        run([args.ranlib, str(archive)])
        archive.replace(args.output)


if __name__ == "__main__":
    main()
