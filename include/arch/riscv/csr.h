#ifndef __RISCV_H__
#define __RISCV_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Machine Status Bit Definitions */
#define MSTATUS_UIE (1 << 0)
#define MSTATUS_SIE (1 << 1)
#define MSTATUS_MIE (1 << 3)
#define MSTATUS_UPIE (1 << 4)
#define MSTATUS_SPIE (1 << 5)
#define MSTATUS_MPIE (1 << 7)
#define MSTATUS_SPP (1 << 8)
#define MSTATUS_MPP (3 << 11)
#define MSTATUS_FS (3 << 13)
#define MSTATUS_XS (3 << 15)
#define MSTATUS_MPRV (1 << 17)
#define MSTATUS_SUM (1 << 18)
#define MSTATUS_MXR (1 << 19)
#define MSTATUS_TVM (1 << 20)
#define MSTATUS_TW (1 << 21)
#define MSTATUS32_SD (1 << 31)
#define MSTATUS_UXL (3ULL << 32)
#define MSTATUS_SXL (3ULL << 34)
#define MSTATUS64_SD (1ULL << 63)

/** Machine Extra Status Bit Definitions */
#define MXSTATUS_MM (1 << 15)
#define MXSTATUS_THEADISAEE (1 << 22)

/** Machine Hardware Config Bit Definitions */
#define MHCR_IE (1 << 0)
#define MHCR_DE (1 << 1)
#define MHCR_WB (1 << 2)
#define MHCR_WA (1 << 3)
#define MHCR_RS (1 << 4)
#define MHCR_BPE (1 << 5)
#define MHCR_BTE (1 << 12)

/** Machine HINT Bit Definitions */
#define MHINT_D_PLD (1 << 2)
#define MHINT_AMR_0 (0b00 << 3)
#define MHINT_AMR_1 (0b01 << 3)
#define MHINT_AMR_2 (0b10 << 3)
#define MHINT_AMR_3 (0b11 << 3)
#define MHINT_IWPE (1 << 10)
#define MHINT_PREF_N_2 (0b00 << 13)
#define MHINT_PREF_N_4 (0b01 << 13)
#define MHINT_PREF_N_8 (0b10 << 13)
#define MHINT_PREF_N_16 (0b11 << 13)
#define MHINT_AEE (1 << 20)
#define MHINT_PCFIFO_FREEZE (1 << 24)

/** Machine Interrupt Pending (mip) Bit Definitions */
#define MIP_USIP (1 << 0)  /**< User Software Interrupt Pending */
#define MIP_SSIP (1 << 1)  /**< Supervisor Software Interrupt Pending */
#define MIP_MSIP (1 << 3)  /**< Machine Software Interrupt Pending */
#define MIP_UTIP (1 << 4)  /**< User Timer Interrupt Pending */
#define MIP_STIP (1 << 5)  /**< Supervisor Timer Interrupt Pending */
#define MIP_MTIP (1 << 7)  /**< Machine Timer Interrupt Pending */
#define MIP_UEIP (1 << 8)  /**< User External Interrupt Pending */
#define MIP_SEIP (1 << 9)  /**< Supervisor External Interrupt Pending */
#define MIP_MEIP (1 << 11) /**< Machine External Interrupt Pending */

/** Machine Interrupt Enable (mie) Bit Definitions */
#define MIE_USIE (1 << 0)  /**< User Software Interrupt Enable */
#define MIE_SSIE (1 << 1)  /**< Supervisor Software Interrupt Enable */
#define MIE_MSIE (1 << 3)  /**< Machine Software Interrupt Enable */
#define MIE_UTIE (1 << 4)  /**< User Timer Interrupt Enable */
#define MIE_STIE (1 << 5)  /**< Supervisor Timer Interrupt Enable */
#define MIE_MTIE (1 << 7)  /**< Machine Timer Interrupt Enable */
#define MIE_UEIE (1 << 8)  /**< User External Interrupt Enable */
#define MIE_SEIE (1 << 9)  /**< Supervisor External Interrupt Enable */
#define MIE_MEIE (1 << 11) /**< Machine External Interrupt Enable */

/* RISC-V Privilege */
#define PRV_U (0)
#define PRV_S (1)
#define PRV_H (2)
#define PRV_M (3)

/**
 * @brief Swap the value of a CSR with a new value.
 *
 * This macro writes the new value to the specified CSR and returns the old value.
 *
 * @param csr The CSR to swap with.
 * @param val The new value to write to the CSR.
 * @return The old value of the CSR.
 */
#define csr_swap(csr, val)                            \
	({                                                \
		unsigned long __v = (unsigned long) (val);    \
		__asm__ __volatile__("csrrw %0, " #csr ", %1" \
							 : "=r"(__v)              \
							 : "rK"(__v)              \
							 : "memory");             \
		__v;                                          \
	})

/**
 * @brief Read the value of a CSR.
 *
 * This macro reads the current value of the specified CSR.
 *
 * @param csr The CSR to read.
 * @return The current value of the CSR.
 */
#define csr_read(csr)                         \
	({                                        \
		register unsigned long __v;           \
		__asm__ __volatile__("csrr %0, " #csr \
							 : "=r"(__v)      \
							 :                \
							 : "memory");     \
		__v;                                  \
	})

/**
 * @brief Write a new value to a CSR.
 *
 * This macro writes a new value to the specified CSR.
 *
 * @param csr The CSR to write to.
 * @param val The value to write to the CSR.
 */
#define csr_write(csr, val)                        \
	({                                             \
		unsigned long __v = (unsigned long) (val); \
		__asm__ __volatile__("csrw " #csr ", %0"   \
							 :                     \
							 : "rK"(__v)           \
							 : "memory");          \
	})

/**
 * @brief Read and set bits in a CSR.
 *
 * This macro reads the current value of the specified CSR and sets the specified bits.
 *
 * @param csr The CSR to read and set bits in.
 * @param val The bits to set in the CSR.
 * @return The previous value of the CSR.
 */
#define csr_read_set(csr, val)                        \
	({                                                \
		unsigned long __v = (unsigned long) (val);    \
		__asm__ __volatile__("csrrs %0, " #csr ", %1" \
							 : "=r"(__v)              \
							 : "rK"(__v)              \
							 : "memory");             \
		__v;                                          \
	})

/**
 * @brief Set bits in a CSR.
 *
 * This macro sets the specified bits in the given CSR.
 *
 * @param csr The CSR to set bits in.
 * @param val The bits to set in the CSR.
 */
#define csr_set(csr, val)                          \
	({                                             \
		unsigned long __v = (unsigned long) (val); \
		__asm__ __volatile__("csrs " #csr ", %0"   \
							 :                     \
							 : "rK"(__v)           \
							 : "memory");          \
	})

/**
 * @brief Read a CSR and clear specified bits.
 *
 * This macro reads the current value of the specified CSR and clears the specified bits.
 *
 * @param csr The CSR to read and clear bits in.
 * @param val The bits to clear in the CSR.
 * @return The previous value of the CSR.
 */
#define csr_read_clear(csr, val)                      \
	({                                                \
		unsigned long __v = (unsigned long) (val);    \
		__asm__ __volatile__("csrrc %0, " #csr ", %1" \
							 : "=r"(__v)              \
							 : "rK"(__v)              \
							 : "memory");             \
		__v;                                          \
	})

/**
 * @brief Clear specified bits in a CSR.
 *
 * This macro clears the specified bits in the given CSR.
 *
 * @param csr The CSR to clear bits in.
 * @param val The bits to clear in the CSR.
 */
#define csr_clear(csr, val)                        \
	({                                             \
		unsigned long __v = (unsigned long) (val); \
		__asm__ __volatile__("csrc " #csr ", %0"   \
							 :                     \
							 : "rK"(__v)           \
							 : "memory");          \
	})

#ifdef __cplusplus
}
#endif

#endif /* __RISCV_H__ */