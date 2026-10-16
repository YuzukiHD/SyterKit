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

Port for SyterKit.
*/
#include "coremark.h"
#include "core_portme.h"

#include <timer.h>
#include <uart.h>

#if MULTITHREAD > 1
#include <smp.h>
#endif

#if VALIDATION_RUN
volatile ee_s32 seed1_volatile = 0x3415;
volatile ee_s32 seed2_volatile = 0x3415;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PERFORMANCE_RUN
volatile ee_s32 seed1_volatile = 0x0;
volatile ee_s32 seed2_volatile = 0x0;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PROFILE_RUN
volatile ee_s32 seed1_volatile = 0x8;
volatile ee_s32 seed2_volatile = 0x8;
volatile ee_s32 seed3_volatile = 0x8;
#endif
volatile ee_s32 seed4_volatile = ITERATIONS;
volatile ee_s32 seed5_volatile = 0;

#define EE_TICKS_PER_SEC 1000000U

static CORETIMETYPE start_time_val, stop_time_val;

void start_time(void)
{
    start_time_val = (CORETIMETYPE)time_us();
}

void stop_time(void)
{
    stop_time_val = (CORETIMETYPE)time_us();
}

CORE_TICKS get_time(void)
{
    return (CORE_TICKS)(stop_time_val - start_time_val);
}

secs_ret time_in_secs(CORE_TICKS ticks)
{
    return ((secs_ret)ticks) / (secs_ret)EE_TICKS_PER_SEC;
}

ee_u32 default_num_contexts = MULTITHREAD;

/* Output: ee_printf.c sends its characters here. */
void uart_send_char(char c)
{
    if (c == '\n')
        uart_putchar('\r');
    uart_putchar(c);
}

/*
 * The only allocations are the data blocks of the contexts, handed out once
 * per run. Every block starts on its own cache line so two harts never share one.
 */
#define POOL_BLOCK 2048U
static ee_u8 pool[MULTITHREAD][POOL_BLOCK] __attribute__((aligned(64)));
static ee_u32 pool_next;

void *portable_malloc(ee_size_t size)
{
    if (size > POOL_BLOCK || pool_next >= MULTITHREAD)
        return NULL;
    return pool[pool_next++];
}

void portable_free(void *p)
{
    (void)p;
    pool_next = 0;
}

#if MULTITHREAD > 1
/*
 * Context 0 runs on the calling hart, the others on the secondary harts.
 * The hart is started when its context is requested; the calling hart then
 * runs its own copy inside core_stop_parallel(), after all are started.
 */
static core_results *volatile hart_results[MULTITHREAD];
static volatile ee_u32 hart_done[MULTITHREAD] __attribute__((aligned(64)));
static ee_u32 next_context;

static void hart_entry(unsigned long ctx)
{
    iterate(hart_results[ctx]);
    __atomic_store_n(&hart_done[ctx], 1, __ATOMIC_RELEASE);
    for (;;)
        __asm__ __volatile__("wfi");
}

ee_u8 core_start_parallel(core_results *res)
{
    ee_u32 ctx = next_context++;

    hart_results[ctx] = res;
    hart_done[ctx] = 0;
    if (ctx == 0)
        return 0;
    if (smp_hart_start(ctx, hart_entry, ctx, 1000) != 0) {
        ee_printf("ERROR! cannot start hart %u\n", (unsigned int)ctx);
        res->err = 1;
    }
    return 0;
}

ee_u8 core_stop_parallel(core_results *res)
{
    ee_u32 ctx;

    for (ctx = 0; ctx < MULTITHREAD; ctx++)
        if (hart_results[ctx] == res)
            break;
    if (ctx == 0) {
        iterate(res);
        __atomic_store_n(&hart_done[0], 1, __ATOMIC_RELEASE);
    } else {
        while (!__atomic_load_n(&hart_done[ctx], __ATOMIC_ACQUIRE))
            ;
    }
    return 0;
}
#endif

void portable_init(core_portable *p, int *argc, char *argv[])
{
    (void)argc;
    (void)argv;
    if (sizeof(ee_ptr_int) != sizeof(ee_u8 *))
        ee_printf("ERROR! Please define ee_ptr_int to a type that holds a pointer!\n");
    if (sizeof(ee_u32) != 4)
        ee_printf("ERROR! Please define ee_u32 to a 32b unsigned type!\n");
    p->portable_id = 1;
}

void portable_fini(core_portable *p)
{
    p->portable_id = 0;
}

/* The one libm function the number formatter (cvt.c) uses. */
double modf(double x, double *iptr)
{
    double whole;

    if (x >= 2147483648.0 * 2147483648.0 || x <= -2147483648.0 * 2147483648.0) {
        *iptr = x;
        return 0.0;
    }
    whole = (double)(long long)x;
    *iptr = whole;
    return x - whole;
}
