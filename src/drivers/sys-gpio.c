#if defined(CONFIG_CHIP_GPIO_V1)
    #include "gpio/sys-gpio-v1.c"
#else
    #include "gpio/sys-gpio-v2.c"
#endif
