if (CONFIG_CHIP_SUN8IW21)
    add_subdirectory(sun8iw21)
elseif(CONFIG_CHIP_SUN8IW20)
    add_subdirectory(sun8iw20)
elseif(CONFIG_CHIP_SUN50IW9)
    add_subdirectory(sun50iw9)
endif()

set(DRIVER_PMU "")

if (CONFIG_CHIP_WITHPMU)
    set(DRIVER_PMU 
        pmu/axp1530.c
    )
endif()

add_library(drivers-obj OBJECT
    sys-uart.c
    sys-gpio.c
    sys-rtc.c
    sys-spi-nand.c
    sys-spi.c
    sys-sdcard.c
    sys-dma.c
    sys-i2c.c
    ${DRIVER_PMU}
)