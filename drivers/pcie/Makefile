# SPDX-License-Identifier: GPL-2.0+

include $(srctree)/drivers/log-level.mk

ccflags-y += $(call driver-log-flags,DRIVER_PCIE)
obj-y += pcie-core.o
obj-y += controller/
obj-y += phy/
obj-$(CONFIG_DRIVER_PCIE_RC) += rc/
obj-$(CONFIG_DRIVER_PCIE_EP) += ep/
