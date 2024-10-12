
if (CONFIG_ARCH_ARM32)
    add_subdirectory(arm32)
endif()

# RISC-V 64 ARCH
if (CONFIG_ARCH_RISCV64)
    add_subdirectory(riscv)
endif()

# RISC-V 32 ARCH
if (CONFIG_ARCH_RISCV32)
    add_subdirectory(riscv)
endif()