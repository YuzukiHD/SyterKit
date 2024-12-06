set(DRIVER_PMU "")

if (CONFIG_CHIP_GPIO_V1)
set(DRIVER_GPIO gpio/sys-gpio-v1.c)
elseif(CONFIG_CHIP_GPIO_V2)
set(DRIVER_GPIO gpio/sys-gpio-v2.c)
elseif(CONFIG_CHIP_GPIO_V3)
set(DRIVER_GPIO gpio/sys-gpio-v3.c)
else()
set(DRIVER_GPIO gpio/sys-gpio-v2.c)
endif()

set(COMMON_DRIVER
    sys-uart.c
    ${DRIVER_GPIO}
)

set(USB_DRIVER "")
if (CONFIG_CHIP_USB)
set(USB_DRIVER
    usb/usb.c
    usb/usb_controller.c
    usb/usb_device.c
    usb/usb_dma.c

    usb/module/usb_detect.c
    usb/module/usb_mass.c
)
endif()

set(GIC_DRIVER "")
if (CONFIG_CHIP_GIC)
set(GIC_DRIVER
    intc/sys-gic.c
)
endif()

if (CONFIG_CHIP_WITHPMU)
    set(DRIVER_PMU 
        pmu/axp.c
        pmu/axp1530.c
        pmu/axp2101.c
        pmu/axp2202.c
    )
endif()

if (CONFIG_CHIP_MMC_V2)
    set(MMC_DRIVER
        mmc/sys-mmc.c
        mmc/sys-sdhci.c
        mmc/sys-sdcard.c
    )
else()
    set(MMC_DRIVER
        sdhci/sys-sdcard.c
        sdhci/sys-sdhci.c
    )
endif()

set(MTD_DRIVER
    mtd/sys-spi-nand.c
    mtd/sys-spi-nor.c
)

if (NOT CONFIG_CHIP_MINSYS)
    set(COMMON_DRIVER
        ${COMMON_DRIVER}
        sys-clk.c
        sys-dram.c
        sys-rtc.c
        sys-spi.c
        sys-dma.c
        sys-i2c.c

        ${USB_DRIVER}
        ${GIC_DRIVER}
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
