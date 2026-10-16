/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file board_smp.c
 * @brief Power on the second C907 hart of the V861 (sun252iw1).
 *
 * The CPU configuration block holds, per hart, the register width and the
 * reset entry; the per hart power controller (PMC) then ungates the core.
 */

#include <stdint.h>

#include <common.h>
#include <io.h>
#include <smp.h>

#define CCU_BASE		0x02001000U
#define CCU_RISCV_CFG_BGR	(CCU_BASE + 0x50cU)
#define CCU_RISCV_CFG_BGR_EN	(BIT(16) | BIT(0))

#define RISCV_CFG_BASE		0x08008000U
#define RESET_ENTRY_LO(n)	(RISCV_CFG_BASE + 0x100U + 0x80U * (n))
#define RESET_ENTRY_HI(n)	(RISCV_CFG_BASE + 0x104U + 0x80U * (n))
#define CORE_CFG(n)		(RISCV_CFG_BASE + 0x108U + 0x80U * (n))
#define CORE_CFG_MODE_MASK	(0x3U << 8)
#define CORE_CFG_MODE_32BIT	(0x1U << 8)
#define CORE_CFG_MODE_64BIT	(0x2U << 8)

#define PMC_BASE(n)		(0x08000000U + 0x1000U * (n))
#define PMC_SW_MODE_CTRL(n, m)	(PMC_BASE(n) + 0x100U + 0x4U * (m))
#define PMC_MODE_MANUAL_ON	(0x3U << 3)
#define PMC_POWER_ON		0x11U

int smp_soc_hart_start(unsigned int hartid, unsigned long entry_addr)
{
	uint32_t cfg;

	if (hartid == 0 || hartid > 1)
		return -1;

	writel(CCU_RISCV_CFG_BGR_EN | readl(CCU_RISCV_CFG_BGR), CCU_RISCV_CFG_BGR);

	cfg = readl(CORE_CFG(hartid)) & ~CORE_CFG_MODE_MASK;
#ifdef CONFIG_ARCH_RISCV64
	cfg |= CORE_CFG_MODE_64BIT;
#else
	cfg |= CORE_CFG_MODE_32BIT;
#endif
	writel(cfg, CORE_CFG(hartid));
	writel((uint32_t)entry_addr, RESET_ENTRY_LO(hartid));
#ifdef CONFIG_ARCH_RISCV64
	writel((uint32_t)((uint64_t)entry_addr >> 32), RESET_ENTRY_HI(hartid));
#else
	writel(0, RESET_ENTRY_HI(hartid));
#endif
	asm volatile("fence ow, ow" ::: "memory");

	writel(PMC_MODE_MANUAL_ON, PMC_SW_MODE_CTRL(hartid, 0));
	writel(PMC_POWER_ON, PMC_SW_MODE_CTRL(hartid, 1));
	return 0;
}
