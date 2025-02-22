add_syterkit_driver(gpio)
add_syterkit_driver(intc)
add_syterkit_driver(pmu)
add_syterkit_driver(mtd)
add_syterkit_driver(usb)
add_syterkit_driver(ufs)

if (CONFIG_CHIP_MMC_V2)
    add_syterkit_driver(mmc)
else()
    add_syterkit_driver(sdhci)
endif()

set(COMMON_DRIVER
    sys-uart.c
    sys-clk.c
    ${DRIVER_GPIO}
)

if (NOT CONFIG_CHIP_MINSYS)
    set(COMMON_DRIVER
        ${COMMON_DRIVER}
        sys-dram.c
        sys-rtc.c
        sys-spi.c
        sys-dma.c
        sys-i2c.c
        sys-pwm.c
        sys-trng.c

        ${USB_DRIVER}
        ${UFS_DRIVER}
        ${INTC_DRIVER}
        ${MMC_DRIVER}
        ${MTD_DRIVER}
    )
endif()


# chip implement
add_subdirectory(chips)

add_library(drivers-obj OBJECT
    ${COMMON_DRIVER}
    ${DRIVER_PMU}
)
