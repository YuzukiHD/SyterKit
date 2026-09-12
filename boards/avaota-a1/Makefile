# SPDX-License-Identifier: GPL-2.0+
include $(srctree)/boards/Makefile.common

board-soc := sun55iw3

apps-sram-y := extlinux_boot hello_world load_app_dram load_e906 mem_speed smhc2_test smhc_test
apps-sram-y += spi_lcd syter_boot syter_boot_bl33 syter_boot_uboot
apps-dram-y := hello_world
mem_speed-y := bench.o

board-libs-y := $(soc-lib-dir)/libdram.a

board-fel-lds-y := link_elf.ld
board-bin-lds-y := link_bin.ld
