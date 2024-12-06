if (CONFIG_CHIP_GPIO_V1)
set(DRIVER_GPIO gpio/sys-gpio-v1.c)
elseif(CONFIG_CHIP_GPIO_V2)
set(DRIVER_GPIO gpio/sys-gpio-v2.c)
elseif(CONFIG_CHIP_GPIO_V3)
set(DRIVER_GPIO gpio/sys-gpio-v3.c)
else()
set(DRIVER_GPIO gpio/sys-gpio-v2.c)
endif()