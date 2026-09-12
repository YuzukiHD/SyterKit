# SoC eFEX applications

Returning eFEX applications run on the Boot ROM stack and return to their caller.
They select a chip independently of `boards/`. Hardware settings are ordinary C
driver structures in `soc/<soc>/app_efex/<app>/main.c`: UART pins and clocks,
I²C buses, PMUs, SID registers, and DRAM timing parameters. These builds do not
load a DTS, run dt2c, or link board support.

```sh
make O=out sun8iw21_efex_defconfig
make O=out -j8
```

The image is written to
`out/build/soc/sun8iw21/app_efex/init_dram/init_dram_efex.bin`.
Use an ARM bare-metal toolchain for ARM chips and a Xuantie toolchain supporting
the selected E907/C907 core for RISC-V chips. `CROSS_COMPILE` can specify its path.

| SoC | Configuration | Application |
| --- | --- | --- |
| sun8iw21 | `sun8iw21_efex_defconfig` | `init_dram` |
| sun8iw22 | `sun8iw22_efex_defconfig` | `init_dram` |
| sun50iw9 | `sun50iw9_efex_defconfig` | `init_dram` |
| sun55iw3 | `sun55iw3_efex_defconfig` | `init_dram` |
| sun55iw6 | `sun55iw6_efex_defconfig` | `hello_world` |
| sun60iw2 | `sun60iw2_lpddr4_efex_defconfig`, `sun60iw2_lpddr5_efex_defconfig` | `init_dram` |
| sun65iw1 | `sun65iw1_efex_defconfig` | `init_dram` |
| sun300iw1 | `sun300iw1_efex_defconfig` | `init_dram` |
| sun252iw1 | `sun252iw1_efex_defconfig` | `init_dram` |
| sun252iw2 | `sun252iw2_rv32_efex_defconfig` | `hello_world` |

The old board-named eFEX targets have been removed. SRAM and DRAM board targets
continue to use `boards/<board>/configs/` and their board device trees.

Architecture directories contain the eFEX return entry assembly and linker
scripts. The shared DRAM result implementation lives in `core/`, with the
public interface in `include/efex.h`. Chip directories own their
applications, Kconfig, defconfigs, and required DRAM libraries or payloads.
Board applications share those chip libraries where needed.

DRAM and PMU defaults preserve the existing initialization profiles. Adjust the
structures for the desired memory and power configuration. sun60iw2 selects
both its timing parameters and matching DRAM library through the LPDDR4/LPDDR5
Kconfig choice. Its two profiles share an output path; use separate `O=` trees
when retaining both images.
