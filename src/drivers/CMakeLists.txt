if (CONFIG_CHIP_SUN8IW21)
add_subdirectory(sun8iw21)
endif()

add_library(drivers-obj OBJECT
    sys-uart.c
    sys-gpio.c
    sys-rtc.c
    sys-spi-nand.c
    sys-spi.c
    sys-sdcard.c
    sys-sdhci.c
    sys-dma.c
    sys-i2c.c
)