/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __ARM32_ATOMIC_H__
#define __ARM32_ATOMIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <barrier.h>
#include <irqflags.h>
#include <types.h>

#if (__ARM32_ARCH__ >= 6)
static inline void atomic_add(atomic_t *a, int v) {
    unsigned int tmp;
    int result;

    __asm__ __volatile__("1:	ldrex %0, [%3]\n"
                         "	add	%0, %0, %4\n"
                         "	strex %1, %0, [%3]\n"
                         "	teq %1, #0\n"
                         "	bne 1b"
                         : "=&r"(result), "=&r"(tmp), "+Qo"(a->counter)
                         : "r"(&a->counter), "Ir"(v)
                         : "cc");
}

static inline int atomic_add_return(atomic_t *a, int v) {
    unsigned int tmp;
    int result;

    __asm__ __volatile__("1:	ldrex %0, [%3]\n"
                         "	add	%0, %0, %4\n"
                         "	strex %1, %0, [%3]\n"
                         "	teq	%1, #0\n"
                         "	bne	1b"
                         : "=&r"(result), "=&r"(tmp), "+Qo"(a->counter)
                         : "r"(&a->counter), "Ir"(v)
                         : "cc");

    return result;
}

static inline void atomic_sub(atomic_t *a, int v) {
    unsigned int tmp;
    int result;

    __asm__ __volatile__("1:	ldrex %0, [%3]\n"
                         "	sub	%0, %0, %4\n"
                         "	strex %1, %0, [%3]\n"
                         "	teq	%1, #0\n"
                         "	bne	1b"
                         : "=&r"(result), "=&r"(tmp), "+Qo"(a->counter)
                         : "r"(&a->counter), "Ir"(v)
                         : "cc");
}

static inline int atomic_sub_return(atomic_t *a, int v) {
    unsigned int tmp;
    int result;

    __asm__ __volatile__("1:	ldrex %0, [%3]\n"
                         "	sub	%0, %0, %4\n"
                         "	strex %1, %0, [%3]\n"
                         "	teq	%1, #0\n"
                         "	bne	1b"
                         : "=&r"(result), "=&r"(tmp), "+Qo"(a->counter)
                         : "r"(&a->counter), "Ir"(v)
                         : "cc");

    return result;
}

static inline int atomic_cmp_exchange(atomic_t *a, int o, int n) {
    int pre, res;

    do {
        __asm__ __volatile__("	ldrex %1, [%3]\n"
                             "	mov %0, #0\n"
                             "	teq %1, %4\n"
                             "	strexeq %0, %5, [%3]\n"
                             : "=&r"(res), "=&r"(pre), "+Qo"(a->counter)
                             : "r"(&a->counter), "Ir"(o), "r"(n)
                             : "cc");
    } while (res);

    return pre;
}
#else
static inline void atomic_add(atomic_t *a, int v) {
    mb();
    a->counter += v;
    mb();
}

static inline int atomic_add_return(atomic_t *a, int v) {
    mb();
    a->counter += v;
    mb();
    return a->counter;
}

static inline void atomic_sub(atomic_t *a, int v) {
    mb();
    a->counter -= v;
    mb();
}

static inline int atomic_sub_return(atomic_t *a, int v) {
    mb();
    a->counter -= v;
    mb();
    return a->counter;
}

static inline int atomic_cmp_exchange(atomic_t *a, int o, int n) {
    volatile int v;

    mb();
    v = a->counter;
    if (v == o)
        a->counter = n;
    mb();
    return v;
}
#endif

#define atomic_set(a, v)      \
    do {                      \
        ((a)->counter) = (v); \
        smp_wmb();            \
    } while (0)
#define atomic_get(a)       \
    ({                      \
        int __v;            \
        __v = (a)->counter; \
        smp_rmb();          \
        __v;                \
    })
#define atomic_inc(a) (atomic_add(a, 1))
#define atomic_dec(a) (atomic_sub(a, 1))
#define atomic_inc_return(a) (atomic_add_return(a, 1))
#define atomic_dec_return(a) (atomic_sub_return(a, 1))
#define atomic_inc_and_test(a) (atomic_add_return(a, 1) == 0)
#define atomic_dec_and_test(a) (atomic_sub_return(a, 1) == 0)
#define atomic_add_negative(a, v) (atomic_add_return(a, v) < 0)
#define atomic_sub_and_test(a, v) (atomic_sub_return(a, v) == 0)
#define atomic_cmpxchg(a, o, n) (atomic_cmp_exchange(a, o, n))

#ifdef __cplusplus
}
#endif

#endif /* __ARM32_ATOMIC_H__ */
