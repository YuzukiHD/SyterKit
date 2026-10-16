# SPDX-License-Identifier: GPL-2.0+
# Everything that differs between SoCs, one directory per SoC. Only the SoC the board selects is built.
obj-$(CONFIG_SOC_SUN252IW1) += sun252iw1/
obj-$(CONFIG_SOC_SUN252IW2) += sun252iw2/
ifneq ($(CONFIG_SOC_SUN252IW1)$(CONFIG_SOC_SUN252IW2),)
obj-y += sunxi-pll.o
endif
