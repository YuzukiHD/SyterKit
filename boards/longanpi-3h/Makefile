# SPDX-License-Identifier: GPL-2.0+
include $(srctree)/boards/Makefile.common
apps-y := cli_test hello_world init_dram load_bl31 smhc_test syter_boot tinymaix
tinymaix-y := tm_layers.o tm_layers_fp8.o tm_layers_O1.o tm_model.o
obj-y += payloads/init_dram_bin.o
