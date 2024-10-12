# RISC-V 64 ARCH
if (CONFIG_ARCH_RISCV64)
if (CONFIG_ARCH_RISCV64_CORE_C906)
    add_subdirectory(riscv64_c906)
endif()
endif()

# RISC-V 32 ARCH
if (CONFIG_ARCH_RISCV32)
if (CONFIG_ARCH_RISCV32_CORE_E907)
    add_subdirectory(riscv32_e907)
endif()
endif()