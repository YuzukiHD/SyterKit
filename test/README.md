# SyterKit API tests

The test tree validates hardware-independent APIs against the real SyterKit
sources. Host cases use mocks only at hardware boundaries. Architecture cases
build freestanding images and run them with QEMU.

Each directory below `cases/` is self-contained and contains:

- `Makefile`: case type, source list, and build settings;
- `main.c`: test code;
- `data/`: input vectors and expected results;
- `verify.sh`: case-specific result validation.

Run all tests with:

```sh
make -C test
```

Run only host or QEMU cases with `make -C test host` or
`make -C test qemu`. A single case can be selected by name, for example
`make -C test cli_parse`. Set `O=/path/to/output` to keep generated files
outside the source tree.

Architecture cases run freestanding Linux-ABI ELF images with QEMU user mode.
ARM cases require `qemu-arm` and `arm-none-eabi-gcc`. E907 cases use the
Xuantie-900 ELF Newlib V3.2.0 toolchain and require `qemu-riscv32`. Set
`ARM_CROSS_COMPILE` or `RISCV_CROSS_COMPILE` when a toolchain is not available
on `PATH`.

The CI toolchain archive is downloaded from:

```text
https://occ-oss-prod.oss-cn-hangzhou.aliyuncs.com/resource//1751370399722/Xuantie-900-gcc-elf-newlib-x86_64-V3.2.0-20250627.tar.gz
```

Its expected SHA-256 is
`80c174c6445f7565bc082d328045021862a63beddfad8c393c534e2d9523dc3b`.
The workflow rejects archives or compiler version strings that do not match
this pinned release.

Backtrace coverage is split into independent minimal call-chain,
full symbolized call-chain, optimized-code, ARM/Thumb interworking,
invalid-context, and instruction-decoder cases. Full cases generate their
symbol tables from a first-pass ELF instead of using test-only symbol data.
Each case owns its source, linker script, vectors, expected output, and
verifier.
