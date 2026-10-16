# SPDX-License-Identifier: GPL-2.0+

include $(srctree)/drivers/log-level.mk
ccflags-y += $(call driver-log-flags,DRIVER_DISPLAY)
obj-y += display.o display-clk.o
obj-y += tcon/ de/ panel/ backlight/ platform/
obj-$(CONFIG_DRIVER_DISPLAY_DSI) += dsi/
obj-$(CONFIG_DRIVER_DISPLAY_DPHY) += dphy/
