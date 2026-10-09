# SPDX-License-Identifier: GPL-2.0+
include $(srctree)/boards/Makefile.common
apps-sram-y := app

ifeq ($(CONFIG_ARCH_RISCV32)$(CONFIG_DRIVER_PSRAM),yy)
apps-sram-y += spinor-boot
apps-sram-y += spinor-boot-rv64i
apps-sram-y += usb-boot
apps-sram-y += flash-boot
apps-sram-y += xip-boot
endif

rust-apps-sram-$(CONFIG_RUST_FFI) += app-rs
apps-sram-$(CONFIG_DRIVER_PLIC) += intc_test
apps-sram-$(CONFIG_DRIVER_USB) += usb_test
# the display test keeps its frame buffer and stack in the PSRAM
ifeq ($(CONFIG_DRIVER_DISPLAY)$(CONFIG_DRIVER_PSRAM),yy)
apps-sram-y += disp_test
endif
apps-dram-$(CONFIG_DRIVER_DISPLAY) += disp_test

obj-$(CONFIG_ARCH_BOOT) += arch_boot/arch_boot.o
