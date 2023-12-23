
if (CONFIG_ARCH_ARM32)
    add_subdirectory(arm32)
endif()

if (CONFIG_ARCH_RISCV64)
    add_subdirectory(riscv64)
endif()