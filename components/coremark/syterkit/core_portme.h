/*
Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Port for SyterKit (derived from the barebones port).
*/
#ifndef CORE_PORTME_H
#define CORE_PORTME_H

#include <types.h>

#define HAS_FLOAT  1
#define HAS_TIME_H 0
#define USE_CLOCK  0
#define HAS_STDIO  0
#define HAS_PRINTF 0

#ifndef COMPILER_VERSION
#ifdef __GNUC__
#define COMPILER_VERSION "GCC"__VERSION__
#else
#define COMPILER_VERSION "unknown"
#endif
#endif

#ifndef COMPILER_FLAGS
#define COMPILER_FLAGS "-O2"
#endif

#define MEM_LOCATION "SRAM, static pool"

typedef signed short   ee_s16;
typedef unsigned short ee_u16;
typedef signed int     ee_s32;
typedef double         ee_f32;
typedef unsigned char  ee_u8;
typedef unsigned int   ee_u32;
typedef ee_u32         ee_ptr_int;
typedef size_t         ee_size_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define align_mem(x) (void *)(4 + (((ee_ptr_int)(x)-1) & ~3))

/* time_us() truncated to 32 bits: a run of at most 100 s never wraps the difference. */
#define CORETIMETYPE ee_u32
typedef ee_u32 CORE_TICKS;

#define SEED_METHOD SEED_VOLATILE

/* One block per context from a static pool; the stack of a hart is too small for them. */
#define MEM_METHOD MEM_MALLOC

#ifndef MULTITHREAD
#define MULTITHREAD 1
#endif
#if MULTITHREAD > 1 && !defined(CONFIG_ARCH_SMP)
#error "MULTITHREAD > 1 needs CONFIG_ARCH_SMP"
#endif
#define USE_PTHREAD 0
#define USE_FORK    0
#define USE_SOCKET  0

#if MULTITHREAD > 1
#define PARALLEL_METHOD "SyterKit harts"
#endif

#define MAIN_HAS_NOARGC   1
#define MAIN_HAS_NORETURN 0

extern ee_u32 default_num_contexts;

typedef struct CORE_PORTABLE_S {
    ee_u8 portable_id;
} core_portable;

void portable_init(core_portable *p, int *argc, char *argv[]);
void portable_fini(core_portable *p);

#if !defined(PROFILE_RUN) && !defined(PERFORMANCE_RUN) && !defined(VALIDATION_RUN)
#if (TOTAL_DATA_SIZE == 1200)
#define PROFILE_RUN 1
#elif (TOTAL_DATA_SIZE == 2000)
#define PERFORMANCE_RUN 1
#else
#define VALIDATION_RUN 1
#endif
#endif

int ee_printf(const char *fmt, ...);

#endif /* CORE_PORTME_H */
