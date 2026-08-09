# SPDX-License-Identifier: GPL-2.0+
obj-y += core.o sdhci.o sdcard.o

mmc-host-socs := $(CONFIG_SOC_SUN20IW1) $(CONFIG_SOC_SUN300IW1) \
	$(CONFIG_SOC_SUN50IW9) $(CONFIG_SOC_SUN50IW10) \
	$(CONFIG_SOC_SUN55IW3) $(CONFIG_SOC_SUN55IW6) \
	$(CONFIG_SOC_SUN60IW2) $(CONFIG_SOC_SUN65IW1) \
	$(CONFIG_SOC_SUN8IW20) $(CONFIG_SOC_SUN8IW21) \
	$(CONFIG_SOC_SUN8IW22)
ifneq ($(filter y,$(mmc-host-socs)),)
obj-y += host/
endif
