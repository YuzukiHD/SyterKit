/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __RISCV_BARRIER_H__
#define __RISCV_BARRIER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RISCV_FENCE(p, s) asm volatile("fence " #p "," #s \
									   :                  \
									   :                  \
									   : "memory")

#define mb() RISCV_FENCE(iorw, iorw)

#define rmb() RISCV_FENCE(ir, ir)

#define wmb() RISCV_FENCE(ow, ow)

#define __smp_mb() RISCV_FENCE(rw, rw)

#define __smp_rmb() RISCV_FENCE(r, r)

#define __smp_wmb() RISCV_FENCE(w, w)

#ifdef __cplusplus
}
#endif

#endif /* __RISCV_BARRIER_H__ */