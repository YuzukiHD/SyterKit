if (CONFIG_CHIP_GIC)
set(INTC_DRIVER
    intc/sys-gic.c
)
endif()