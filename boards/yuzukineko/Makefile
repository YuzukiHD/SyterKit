# SPDX-License-Identifier: GPL-2.0+
include $(srctree)/boards/Makefile.common
apps-sram-y := app
rust-apps-sram-$(CONFIG_RUST_FFI) += app-rs
apps-sram-$(CONFIG_DRIVER_PLIC) += intc_test
apps-sram-$(CONFIG_DRIVER_USB) += usb_test
apps-efex-y := hello_world

board-libs-$(CONFIG_DRIVER_PSRAM) := lib/libpsram.a

obj-$(CONFIG_ARCH_BOOT) += arch_boot/arch_boot.o
