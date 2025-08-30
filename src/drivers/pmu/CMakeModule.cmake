if (CONFIG_CHIP_WITHPMU)
    set(DRIVER_PMU 
        pmu/axp.c
        pmu/axp1530.c
        pmu/axp2101.c
        pmu/axp2202.c
        pmu/axp8191.c
        pmu/axp333.c
    )
endif()