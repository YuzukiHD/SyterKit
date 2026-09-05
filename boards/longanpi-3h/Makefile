# SPDX-License-Identifier: GPL-2.0+
include $(srctree)/boards/Makefile.common
apps-sram-y := cli_test hello_world load_bl31 smhc_test syter_boot tinymaix
apps-efex-y := init_dram
apps-dram-y := hello_world
tinymaix-y := tm_layers.o tm_layers_fp8.o tm_layers_O1.o tm_model.o
obj-y += payloads/init_dram_bin.o
