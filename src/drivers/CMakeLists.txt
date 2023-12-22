if (CONFIG_CHIP_SUN8IW21)
add_subdirectory(sun8iw21)
endif()

add_library(drivers-obj OBJECT
    sys-uart.c
    sys-gpio.c
)