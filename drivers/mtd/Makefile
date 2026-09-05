# SPDX-License-Identifier: GPL-2.0+

include $(srctree)/drivers/log-level.mk
ccflags-y += $(call driver-log-flags,DRIVER_MTD)

ifeq ($(CONFIG_DRIVER_MTD),y)
mtd_flash_drivers := $(filter y,$(CONFIG_DRIVER_MTD_SPI_NOR) \
	$(CONFIG_DRIVER_MTD_SPIF_NOR) $(CONFIG_DRIVER_MTD_SPI_NAND))
ifneq ($(words $(mtd_flash_drivers)),1)
$(error CONFIG_DRIVER_MTD requires exactly one flash driver)
endif
endif

ifeq ($(CONFIG_DRIVER_MTD),y)
ifeq ($(CONFIG_DRIVER_MTD_SPI_NOR),y)
obj-y += spi-nor/
else ifeq ($(CONFIG_DRIVER_MTD_SPIF_NOR),y)
obj-y += spif-nor/
else ifeq ($(CONFIG_DRIVER_MTD_SPI_NAND),y)
obj-y += nand/
endif
endif
