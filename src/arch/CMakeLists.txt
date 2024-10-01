
if (CONFIG_ARCH_ARM32)
    add_subdirectory(arm32)
endif()

if (CONFIG_ARCH_RISCV64)
    add_subdirectory(riscv)
endif()

if (CONFIG_ARCH_RISCV32)
    add_subdirectory(riscv)
endif()