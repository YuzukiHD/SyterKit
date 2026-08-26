/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file exception.c
 * @brief RISC-V trap decoding, misaligned access emulation, and dispatch.
 *
 * Faulting instructions are fetched through MPRV accessors. Supported
 * byte/halfword/word loads and stores are emulated in software; unsupported
 * traps are redirected to the previous context or reported as fatal errors.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <mmu.h>
#include <timer.h>

#include <csr.h>

#include <common.h>
#include <log.h>

#if defined(CONFIG_BACKTRACE)
#include <backtrace.h>
#endif

#if defined(CONFIG_DRIVER_CLIC)
#include <drivers/intc/clic.h>
#endif

#if defined(__riscv_flen)
#if __riscv_flen >= 32
extern void f32_read(int n, uint32_t *v);
extern void f32_write(int n, uint32_t *v);
#endif
#if __riscv_flen >= 64
extern void f64_read(int n, uint64_t *v);
extern void f64_write(int n, uint64_t *v);
#endif
#endif

#define EXTRACT_FIELD(val, which) (((val) & (which)) / ((which) & ~((which) - 1)))
#define INSERT_FIELD(val, which, fieldval) (((val) & ~(which)) | ((fieldval) * ((which) & ~((which) - 1))))
#define RISCV_CAUSE_INTERRUPT (1UL << (__riscv_xlen - 1))

#ifndef STRINGIFY
#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)
#endif

/**
 * @brief Generate a privileged-memory read helper for trap emulation.
 *
 * The generated inline function temporarily enables MPRV (and optional MXR)
 * in @c mstatus, executes the requested load instruction, then restores the
 * status bits even though the access is performed from machine mode.
 *
 * @param name Generated helper name.
 * @param type C type returned by the helper.
 * @param insn Load instruction mnemonic used by the assembler.
 * @param flags MPRV/MXR status bits enabled around the access.
 */
#define DEFINE_MPRV_READ_FLAGS(name, type, insn, flags)                                        \
	static inline type name(type *p)                                                       \
	{                                                                                      \
		size_t mprv = flags;                                                           \
		type value;                                                                    \
		__asm__ __volatile__("csrs mstatus, %1\n" STRINGIFY(insn) " %0, 0(%2)\n"       \
									  "csrc mstatus, %1\n" \
				     : "=&r"(value)                                            \
				     : "r"(mprv), "r"(p)                                       \
				     : "memory");                                              \
		return value;                                                                  \
	}

/** @brief Generate an MPRV read helper without MXR permission. */
#define DEFINE_MPRV_READ(name, type, insn) DEFINE_MPRV_READ_FLAGS(name, type, insn, 0x00020000)

/** @brief Generate an MPRV read helper that also permits execute-only pages. */
#define DEFINE_MPRV_READ_MXR(name, type, insn) DEFINE_MPRV_READ_FLAGS(name, type, insn, 0x00020000 | 0x00080000)

/**
 * @brief Generate a privileged-memory write helper for trap emulation.
 * @param name Generated helper name.
 * @param type C type accepted by the helper.
 * @param insn Store instruction mnemonic used by the assembler.
 */
#define DEFINE_MPRV_WRITE(name, type, insn)                                                                 \
	static inline void name(type *p, type value)                                                        \
	{                                                                                                   \
		size_t mprv = 0x00020000;                                                                   \
		__asm__ __volatile__("csrs mstatus, %0\n" STRINGIFY(insn) " %1, 0(%2)\n"                    \
									  "csrc mstatus, %0\n" ::"r"(mprv), \
				     "r"(value), "r"(p)                                                     \
				     : "memory");                                                           \
	}

DEFINE_MPRV_READ(mprv_read_u8, uint8_t, lbu)
DEFINE_MPRV_READ(mprv_read_u16, uint16_t, lhu)
DEFINE_MPRV_READ(mprv_read_u32, uint32_t, lwu)
DEFINE_MPRV_READ(mprv_read_u64, uint64_t, ld)
DEFINE_MPRV_READ(mprv_read_long, long, ld)
DEFINE_MPRV_READ(mprv_read_ulong, unsigned long, ld)
DEFINE_MPRV_READ_MXR(mprv_read_mxr_u8, uint8_t, lbu)
DEFINE_MPRV_READ_MXR(mprv_read_mxr_u16, uint16_t, lhu)
DEFINE_MPRV_READ_MXR(mprv_read_mxr_u32, uint32_t, lwu)
DEFINE_MPRV_READ_MXR(mprv_read_mxr_u64, uint64_t, ld)
DEFINE_MPRV_READ_MXR(mprv_read_mxr_long, long, ld)
DEFINE_MPRV_READ_MXR(mprv_read_mxr_ulong, unsigned long, ld)
DEFINE_MPRV_WRITE(mprv_write_u8, uint8_t, sb)
DEFINE_MPRV_WRITE(mprv_write_u16, uint16_t, sh)
DEFINE_MPRV_WRITE(mprv_write_u32, uint32_t, sw)
DEFINE_MPRV_WRITE(mprv_write_u64, uint64_t, sd)
DEFINE_MPRV_WRITE(mprv_write_long, long, sd)
DEFINE_MPRV_WRITE(mprv_write_ulong, unsigned long, sd)

/** @brief Register frame saved by the RISC-V trap entry assembly. */
struct pt_regs_t {
	unsigned long x[32];
	unsigned long status;
	unsigned long epc;
	unsigned long badvaddr;
	unsigned long cause;
	unsigned long insn;
};

/** @brief Result container used while fetching a faulting instruction. */
struct insn_fetch_t {
	unsigned long error;
	uint32_t insn;
};

/** @brief Byte-addressable storage for an emulated load or store. */
union endian_buf_t {
	uint8_t b[8];
	uint16_t h[4];
	uint32_t w[2];
	uint64_t d[1];
	unsigned long v;
};

/** @brief Decoder metadata for one supported load/store encoding. */
struct instruction_info_t {
	uint32_t opcode;
	uint32_t mask;
	unsigned int reg_shift;
	unsigned int reg_mask;
	unsigned int reg_addition;
	unsigned int is_fp : 1;
	unsigned int is_load : 1;
	unsigned int width : 8;
	unsigned int sign_extend : 1;
};

#if defined(CONFIG_ARCH_CPU_C906)
typedef struct irq_handler {
	void *data;
	void (*func)(void *data);
} irq_handler_t;

static irq_handler_t core_interrupt_handler[8];
#endif

static struct instruction_info_t insn_info[] = {
#if __riscv_xlen == 128
	{ 0x00002000, 0x0000e003, 2, 7, 8, 0, 1, 16, 1 }, /* C.LQ */
#else
	{ 0x00002000, 0x0000e003, 2, 7, 8, 1, 1, 8, 0 }, /*  C.FLD */
#endif
	{ 0x00004000, 0x0000e003, 2, 7, 8, 0, 1, 4, 1 }, /*  C.LW */
#if __riscv_xlen == 32
	{ 0x00006000, 0x0000e003, 2, 7, 8, 1, 1, 4, 0 }, /*  C.FLW */
#else
	{ 0x00006000, 0x0000e003, 2, 7, 8, 0, 1, 8, 1 }, /*  C.LD */
#endif

#if __riscv_xlen == 128
	{ 0x0000a000, 0x0000e003, 2, 7, 8, 0, 0, 16, 0 }, /*  C.SQ */
#else
	{ 0x0000a000, 0x0000e003, 2, 7, 8, 1, 0, 8, 0 }, /*  C.FSD */
#endif
	{ 0x0000c000, 0x0000e003, 2, 7, 8, 0, 0, 4, 0 }, /*  C.SW */
#if __riscv_xlen == 32
	{ 0x0000e000, 0x0000e003, 2, 7, 8, 1, 0, 4, 0 }, /*  C.FSW */
#else
	{ 0x0000e000, 0x0000e003, 2, 7, 8, 0, 0, 8, 0 }, /*  C.SD */
#endif

#if __riscv_xlen == 128
	{ 0x00002002, 0x0000e003, 7, 15, 0, 0, 1, 16, 1 }, /*  C.LQSP */
#else
	{ 0x00002002, 0x0000e003, 7, 15, 0, 1, 1, 8, 0 }, /*  C.FLDSP */
#endif
	{ 0x00004002, 0x0000e003, 7, 15, 0, 0, 1, 4, 1 }, /*  C.LWSP */
#if __riscv_xlen == 32
	{ 0x00006002, 0x0000e003, 7, 15, 0, 1, 1, 4, 0 }, /*  C.FLWSP */
#else
	{ 0x00006002, 0x0000e003, 7, 15, 0, 0, 1, 8, 1 }, /*  C.LDSP */
#endif

#if __riscv_xlen == 128
	{ 0x0000a002, 0x0000e003, 2, 15, 0, 0, 0, 16, 0 }, /*  C.SQSP */
#else
	{ 0x0000a002, 0x0000e003, 2, 15, 0, 1, 0, 8, 0 }, /*  C.FSDSP */
#endif
	{ 0x0000c002, 0x0000e003, 2, 15, 0, 0, 0, 4, 0 }, /*  C.SWSP */
#if __riscv_xlen == 32
	{ 0x0000e002, 0x0000e003, 2, 15, 0, 1, 0, 4, 0 }, /*  C.FSWSP */
#else
	{ 0x0000e002, 0x0000e003, 2, 15, 0, 0, 0, 8, 0 }, /*  C.SDSP */
#endif

	{ 0x00000003, 0x0000707f, 7, 15, 0, 0, 1, 1, 1 }, /*  LB */
	{ 0x00001003, 0x0000707f, 7, 15, 0, 0, 1, 2, 1 }, /*  LH */
	{ 0x00002003, 0x0000707f, 7, 15, 0, 0, 1, 4, 1 }, /*  LW */
#if __riscv_xlen > 32
	{ 0x00003003, 0x0000707f, 7, 15, 0, 0, 1, 8, 1 }, /*  LD */
#endif
	{ 0x00004003, 0x0000707f, 7, 15, 0, 0, 1, 1, 0 }, /*  LBU */
	{ 0x00005003, 0x0000707f, 7, 15, 0, 0, 1, 2, 0 }, /*  LHU */
	{ 0x00006003, 0x0000707f, 7, 15, 0, 0, 1, 4, 0 }, /*  LWU */

	{ 0x00000023, 0x0000707f, 20, 15, 0, 0, 0, 1, 0 }, /*  SB */
	{ 0x00001023, 0x0000707f, 20, 15, 0, 0, 0, 2, 0 }, /*  SH */
	{ 0x00002023, 0x0000707f, 20, 15, 0, 0, 0, 4, 0 }, /*  SW */
#if __riscv_xlen > 32
	{ 0x00003023, 0x0000707f, 20, 15, 0, 0, 0, 8, 0 }, /*  SD */
#endif

#if defined(__riscv_flen)
#if __riscv_flen >= 32
	{ 0x00002007, 0x0000707f, 7, 15, 0, 1, 1, 4, 0 }, /*  FLW */
	{ 0x00003007, 0x0000707f, 7, 15, 0, 1, 1, 8, 0 }, /*  FLD */
#endif

#if __riscv_flen >= 64
	{ 0x00002027, 0x0000707f, 20, 15, 0, 1, 0, 4, 0 }, /*  FSW */
	{ 0x00003027, 0x0000707f, 20, 15, 0, 1, 0, 8, 0 }, /*  FSD */
#endif
#endif
};

static const char *interrupt_names[] = {
	"User software interrupt", "Supervisor software interrupt", "Hypervisor software interrupt", "Machine software interrupt",
	"User timer interrupt",	   "Supervisor timer interrupt",    "Hypervisor timer interrupt",    "Machine timer interrupt",
	"User external interrupt", "Supervisor external interrupt", "Hypervisor external interrupt", "Machine external interrupt",
};

static const char *exception_names[] = {
	"Instruction address misaligned",
	"Instruction access fault",
	"Illegal instruction",
	"Breakpoint",
	"Load address misaligned",
	"Load access fault",
	"Store address misaligned",
	"Store access fault",
	"Environment call from U-mode",
	"Environment call from S-mode",
	"Reserved (10)",
	"Environment call from M-mode",
	"Instruction page fault",
	"Load page fault",
	"Reserved (14)",
	"Store page fault",
};

/**
 * @brief Decode the privilege mode encoded in MSTATUS.MPP.
 * @param[in] ms MSTATUS value.
 * @return Human-readable privilege-mode name.
 */
static const char *mstatus_to_previous_mode(unsigned long ms)
{
	switch ((ms >> 11) & 0x3) {
	case 0x0:
		return "User";
	case 0x1:
		return "Supervisor";
	case 0x2:
		return "Hypervisor";
	case 0x3:
		return "Machine";
	default:
		break;
	}
	return "unknown";
}

/**
 * @brief Print a RISC-V trap frame and optional backtrace.
 * @param[in] regs Saved register frame supplied by trap entry.
 */
static void show_regs(struct pt_regs_t *regs)
{
	unsigned long cause;

	if (regs->cause & RISCV_CAUSE_INTERRUPT) {
		cause = regs->cause & ~RISCV_CAUSE_INTERRUPT;
		if (cause < ARRAY_SIZE(interrupt_names))
			printk_error("Interrupt:          %s\r\n", interrupt_names[cause]);
		else
			printk_error("Trap:               Unknown cause %p\r\n", (void *)regs->cause);
	} else {
		cause = regs->cause & 0xfff;
		if (cause < ARRAY_SIZE(exception_names))
			printk_error("Exception:          %s\r\n", exception_names[cause]);
		else
			printk_error("Trap:               Unknown cause %p\r\n", (void *)regs->cause);
	}
	printk_error("Previous mode:      %s%s\r\n", mstatus_to_previous_mode(csr_read(mstatus)), (regs->status & (1 << 17)) ? " (MPRV)" : "");
	printk_error("Bad instruction pc: %p\r\n", (void *)regs->epc);
	printk_error("Bad address:        %p\r\n", (void *)regs->badvaddr);
	printk_error("Stored ra:          %p\r\n", (void *)regs->x[1]);
	printk_error("Stored sp:          %p\r\n", (void *)regs->x[2]);
#if defined(CONFIG_BACKTRACE)
	printk_error("========== backtrace ==========\n");
	dump_stack();
#if defined(CONFIG_ARCH_CPU_C906)
	{
		const struct backtrace_context context = {
			.pc = regs->epc,
			.sp = regs->x[2],
			.fp = regs->x[8],
			.lr = regs->x[1],
		};

		backtrace_from_context(&context);
	}
#else
	backtrace((char *)regs->epc, (long *)regs->x[2], (char *)regs->x[1]);
#endif
#endif
}

/**
 * @brief Find decoder metadata matching an instruction word.
 * @param[in] insn Instruction bits, either compressed or full width.
 * @return Matching metadata, or NULL when the instruction is unsupported.
 */
static struct instruction_info_t *match_instruction(unsigned long insn)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(insn_info); i++)
		if ((insn_info[i].mask & insn) == insn_info[i].opcode)
			return &(insn_info[i]);
	return NULL;
}

/**
 * @brief Fetch and validate a compressed 16-bit instruction under MPRV.
 * @param[in] vaddr Faulting instruction address.
 * @param[out] insn Receives the instruction when valid.
 * @return Zero for a compressed instruction, or -1 when the encoding is not
 *         16-bit.
 */
static int fetch_16bit_instruction(unsigned long vaddr, unsigned long *insn)
{
	uint16_t ins = mprv_read_mxr_u16((uint16_t *)vaddr);
	if (EXTRACT_FIELD(ins, 0x3) != 3) {
		*insn = ins;
		return 0;
	}
	return -1;
}

/**
 * @brief Fetch and validate a 32-bit instruction under MPRV.
 * @param[in] vaddr Faulting instruction address.
 * @param[out] insn Receives the instruction when valid.
 * @return Zero for a supported 32-bit encoding, or -1 otherwise.
 */
static int fetch_32bit_instruction(unsigned long vaddr, unsigned long *insn)
{
	uint32_t l = (uint32_t)mprv_read_mxr_u16((uint16_t *)vaddr + 0);
	uint32_t h = (uint32_t)mprv_read_mxr_u16((uint16_t *)vaddr + 2);
	uint32_t ins = (h << 16) | l;
	if ((EXTRACT_FIELD(ins, 0x3) == 3) && (EXTRACT_FIELD(ins, 0x1c) != 0x7)) {
		*insn = ins;
		return 0;
	}
	return -1;
}

/**
 * @brief Forward an unsupported trap to the previous execution context.
 * @param[in,out] regs Trap frame whose return PC and status may be rewritten.
 */
static void redirect_trap(struct pt_regs_t *regs)
{
#if defined(CONFIG_DRIVER_CLIC)
	show_regs(regs);
	abort();
#else
	unsigned long status = regs->status;
	unsigned long mpp = EXTRACT_FIELD(status, 0x00001800);

	csr_write(stval, regs->badvaddr);
	csr_write(sepc, regs->epc);
	csr_write(scause, regs->cause);
	regs->epc = csr_read(stvec);
	status = INSERT_FIELD(status, 0x00001800, 1);
	status = INSERT_FIELD(status, 0x00000100, mpp & 1);
	regs->status = status;
#endif
}

/**
 * @brief Emulate a supported misaligned load or store instruction.
 * @param[in,out] regs Trap frame containing the faulting address and register
 *                     state; the PC advances after successful emulation.
 */
static void handle_misaligned(struct pt_regs_t *regs)
{
	struct instruction_info_t *match;
	unsigned long insn = 0;
	union endian_buf_t buff;
	uint8_t *addr;
	unsigned int insn_len;
	int done, n;

	/* Try to fetch 16 / 32 bits instruction */
	if (!fetch_16bit_instruction(regs->epc, &insn)) {
		insn_len = 2;
	} else if (!fetch_32bit_instruction(regs->epc, &insn)) {
		insn_len = 4;
	} else {
		redirect_trap(regs);
		return;
	}

	/* Matching instruction */
	match = match_instruction(insn);
	if (!match) {
		redirect_trap(regs);
		return;
	}

	n = ((insn >> match->reg_shift) & match->reg_mask);
	n = n + match->reg_addition;
	buff.v = 0;
	if (match->is_load) {
		/* Load operation */
		/* Reading from memory by bytes prevents misaligned memory access */
		for (int i = 0; i < match->width; i++) {
			uint8_t *addr = (uint8_t *)(regs->badvaddr + i);
			buff.b[i] = mprv_read_u8(addr);
		}

		/* Sign extend for signed integer loading */
		if (match->sign_extend && match->width < sizeof(buff.v)) {
			unsigned int bits = 8 * match->width;

			if (buff.v & (1UL << (bits - 1)))
				buff.v |= ~0UL << bits;
		}

		/* Write to register */
		if (match->is_fp) {
			int done = 0;
#if defined(__riscv_flen)
#if __riscv_flen >= 32
			/* Single-precision floating-point */
			if (match->width == 4) {
				f32_write(n, buff.w);
				done = 1;
			}
#endif
#if __riscv_flen >= 64
			/* Double-precision floating-point */
			if (match->width == 8) {
				f64_write(n, buff.d);
				done = 1;
			}
#endif
#endif
			if (!done) {
				redirect_trap(regs);
				return;
			}
		} else {
			if (n != 0)
				regs->x[n] = buff.v;
		}
	} else {
		/* Store operation */
		/* Reading from register */
		if (match->is_fp) {
			done = 0;
#if defined(__riscv_flen)
#if __riscv_flen >= 32
			if (match->width == 4) {
				f32_read(n, buff.w);
				done = 1;
			}
#endif
#if __riscv_flen >= 64
			if (match->width == 8) {
				f64_read(n, buff.d);
				done = 1;
			}
#endif
#endif
			if (!done) {
				redirect_trap(regs);
				return;
			}
		} else {
			buff.v = n == 0 ? 0 : regs->x[n];
		}

		/* Writing to memory by bytes prevents misaligned memory access */
		for (int i = 0; i < match->width; i++) {
			addr = (uint8_t *)(regs->badvaddr + i);
			mprv_write_u8(addr, buff.b[i]);
		}
	}

	regs->epc += insn_len;
}

/**
 * @brief Dispatch an RV32 machine trap.
 * @param[in,out] regs Saved trap frame from the assembly entry path.
 */
void riscv_handle_exception(struct pt_regs_t *regs)
{
	csr_write(mscratch, regs);
	if (regs->cause & RISCV_CAUSE_INTERRUPT) {
		unsigned long cause = regs->cause & ~RISCV_CAUSE_INTERRUPT;
#if defined(CONFIG_DRIVER_CLIC)
		do_irq(cause & 0xfff);
#else
		unsigned long pending = csr_read(mip) & (1UL << cause);

		switch (cause) {
		case 0: /* User software interrupt */
		case 1: /* Supervisor software interrupt */
		case 2: /* Hypervisor software interrupt */
		case 3: /* Machine software interrupt */
		case 4: /* User timer interrupt */
		case 5: /* Supervisor timer interrupt */
		case 6: /* Hypervisor timer interrupt */
		case 7: /* Machine timer interrupt */
			csr_clear(mip, pending);
			if (core_interrupt_handler[cause].func)
				core_interrupt_handler[cause].func(core_interrupt_handler[cause].data);
			break;
		case 8: /* User external interrupt */
		case 9: /* Supervisor external interrupt */
		case 10: /* Hypervisor external interrupt */
		case 11: /* Machine external interrupt */
			csr_clear(mip, pending);
			break;
		default:
			show_regs(regs);
			abort();
		}
#endif
	} else {
		switch (regs->cause) {
		case 0x0: /* Misaligned fetch */
		case 0x1: /* Fetch access */
		case 0x2: /* Illegal instruction */
		case 0x3: /* Breakpoint */
			show_regs(regs);
			abort();
		case 0x4: /* Misaligned load */
			handle_misaligned(regs);
			break;
		case 0x5: /* Load acces */
			show_regs(regs);
			abort();
		case 0x6: /* Misaligned store */
			handle_misaligned(regs);
			break;
		case 0x7: /* Store accesss */
		case 0x8: /* User ecall */
		case 0x9: /* Supervisor ecall */
		case 0xa: /* Hypervisor ecall */
		case 0xb: /* Machine ecall */
			show_regs(regs);
			abort();
		default:
			show_regs(regs);
			abort();
		}
	}
}
