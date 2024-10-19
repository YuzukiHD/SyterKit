# RISC-V 64 ARCH
if (CONFIG_ARCH_RISCV)
if (CONFIG_ARCH_RISCV_CORE_C906)
    add_subdirectory(riscv64_c906)
endif()
endif()

# RISC-V 32 ARCH
if (CONFIG_ARCH_RISCV)
if (CONFIG_ARCH_RISCV_CORE_E907)
    add_subdirectory(riscv32_e907)
endif()
endif()