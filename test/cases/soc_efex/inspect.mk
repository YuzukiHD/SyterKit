# SPDX-License-Identifier: GPL-2.0+

ifneq ($(CONFIG_EFEX),y)
$(error the selected configuration must enable eFEX)
endif
ifneq ($(CONFIG_SYS_BOARD)$(board),)
$(error eFEX must not select a board)
endif
ifneq ($(dt2c_header)$(board_dts),)
$(error eFEX must not generate a device tree)
endif
ifneq ($(filter boards/%,$(build_dirs)),)
$(error eFEX must not link board objects)
endif
ifneq ($(findstring /boards/,$(platform_libs)),)
$(error eFEX must not link board libraries)
endif

.PHONY: inspect-efex
inspect-efex:
	@test '$(image_rel_root)' = 'build/soc/$(soc)/app_efex'
	@test -n '$(apps)'
	@for app in $(apps); do test -f '$(srctree)/soc/$(soc)/app_efex/'$$app/main.c; done
	@for library in $(platform_libs); do test -f $$library; done
