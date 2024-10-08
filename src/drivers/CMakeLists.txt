set(DRIVER_PMU "")

set(COMMON_DRIVER
    sys-uart.c
    sys-gpio.c
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
    sys-gic.c
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
    set(DRIVER_MMC
        mmc/sys-mmc.c
        mmc/sys-sdhci.c
        mmc/sys-sdcard.c
    )
else()
    set(DRIVER_MMC
        sdhci/sys-sdcard.c
        sdhci/sys-sdhci.c
    )
endif()

if (NOT CONFIG_CHIP_CLK_V1)
    set(COMMON_DRIVER
        ${COMMON_DRIVER}
        sys-clk.c
        sys-dram.c
        sys-rtc.c
        sys-spi-nand.c
        sys-spi.c
        sys-dma.c
        sys-i2c.c

        ${USB_DRIVER}
        ${GIC_DRIVER}
        ${DRIVER_MMC}
    )
endif()

# chip implement
add_subdirectory(chips)

add_library(drivers-obj OBJECT
    ${COMMON_DRIVER}
    ${DRIVER_PMU}
)
