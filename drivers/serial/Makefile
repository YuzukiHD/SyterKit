# SPDX-License-Identifier: GPL-2.0+

include $(srctree)/drivers/log-level.mk
ccflags-y += $(call driver-log-flags,DRIVER_SERIAL)
obj-y += serial.o

ifneq ($(CONFIG_EFEX),y)
obj-y += serial-dt.o
endif
