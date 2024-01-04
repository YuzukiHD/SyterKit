if (CONFIG_CHIP_SUN8IW21)
    add_subdirectory(sun8iw21)
elseif(CONFIG_CHIP_SUN8IW20)
    add_subdirectory(sun8iw20)
elseif(CONFIG_CHIP_SUN50IW9)
    add_subdirectory(sun50iw9)
elseif(CONFIG_CHIP_SUN55IW3)
    add_subdirectory(sun55iw3)
elseif(CONFIG_CHIP_SUN8IW8)
    add_subdirectory(sun8iw8)
endif()

set(DRIVER_PMU "")

set(COMMON_DRIVER
    sys-uart.c
    sys-gpio.c
)

if (CONFIG_CHIP_WITHPMU)
    set(DRIVER_PMU 
        pmu/axp1530.c
        pmu/axp2202.c
    )
endif()

if (NOT CONFIG_CHIP_CLK_V1)
    set(COMMON_DRIVER
        ${COMMON_DRIVER}
        sys-rtc.c
        sys-spi-nand.c
        sys-spi.c
        sys-sdcard.c
        sys-dma.c
        sys-i2c.c
    )
endif()

add_library(drivers-obj OBJECT
    ${COMMON_DRIVER}
    ${DRIVER_PMU}
)