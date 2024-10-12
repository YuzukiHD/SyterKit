/* SPDX-License-Identifier: Apache-2.0 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <log.h>
#include <timer.h>
#include <sys-clk.h>

static uint32_t init_timestamp = 0; /**< Timestamp for initialization. */
uint32_t current_hosc_freq = 24; /**< Current frequency of the high-speed oscillator (HOSC) in MHz. */

/**
 * @brief Detect the current high-speed oscillator (HOSC) frequency.
 *
 * This function enables the HOSC frequency detection and reads the
 * detected frequency value. It determines if the frequency is closer
 * to 24 MHz or 40 MHz based on the measured counter value.
 *
 * @return Current HOSC frequency in MHz (either 24 or 40).
 */
static int sunxi_hosc_detect(void) {
    uint32_t counter_val = 0;
    writel(HOSC_FREQ_DET_HOSC_ENABLE_DETECT, CCU_HOSC_FREQ_DET_REG);
    while (!(HOSC_FREQ_DET_HOSC_FREQ_READY_CLEAR_MASK & readl(CCU_HOSC_FREQ_DET_REG)))
        ;
    counter_val = (readl(CCU_HOSC_FREQ_DET_REG) & HOSC_FREQ_DET_HOSC_FREQ_DET_CLEAR_MASK) >> HOSC_FREQ_DET_HOSC_FREQ_DET_OFFSET;
    if (counter_val < ((HOSC_24M_COUNTER + HOSC_40M_COUNTER) / 2)) {
        current_hosc_freq = HOSC_FREQ_24M;
        return HOSC_FREQ_24M;
    } else {
        current_hosc_freq = HOSC_FREQ_40M;
        return HOSC_FREQ_40M;
    }
}

/**
 * @brief Set the timer count based on the detected HOSC frequency.
 *
 * This function calls the frequency detection routine and initializes
 * the timestamp based on the current time in microseconds.
 */
void set_timer_count() {
    sunxi_hosc_detect();
    init_timestamp = (uint32_t) time_us();
}

/**
 * @brief Get the architecture-specific counter value.
 *
 * This function reads a high-resolution timer counter and ensures
 * that the values are read consistently.
 *
 * @return Current counter value as a 64-bit integer.
 */
uint64_t get_arch_counter(void) {
    uint64_t cnt = 0;
    uint32_t upper, lower;
    uint32_t upper_new;

    asm volatile(
            "1:  rdtimeh %[upper]\n"
            "    rdtime %[lower]\n"
            "    rdtimeh %[upper_new]\n"
            "    bne %[upper], %[upper_new], 1b\n"
            : [upper] "=r"(upper),
              [lower] "=r"(lower),
              [upper_new] "=&r"(upper_new)
            :
            : "memory");

    cnt = ((uint64_t) upper << 32) | lower;

    return cnt;
}

/**
 * @brief Get the current time in milliseconds.
 *
 * This function calculates the current time by dividing the architecture
 * counter value by the current HOSC frequency (in MHz) scaled to milliseconds.
 *
 * @return Current time in milliseconds.
 */
uint32_t time_ms(void) {
    return get_arch_counter() / (current_hosc_freq * 1000);
}

/**
 * @brief Get the current time in microseconds.
 *
 * This function calculates the current time by dividing the architecture
 * counter value by the current HOSC frequency (in MHz).
 *
 * @return Current time in microseconds.
 */
uint64_t time_us(void) {
    return get_arch_counter() / (uint64_t) current_hosc_freq;
}

/**
 * @brief Delay execution for a specified number of microseconds.
 *
 * This function uses a busy-wait loop to create a delay for the specified
 * number of microseconds.
 *
 * @param us Number of microseconds to delay.
 */
void udelay(uint64_t us) {
    uint64_t t1, t2;

    t1 = get_arch_counter();
    t2 = t1 + us * current_hosc_freq;
    do {
        t1 = get_arch_counter();
    } while (t2 >= t1);
}

/**
 * @brief Delay execution for a specified number of milliseconds.
 *
 * This function converts milliseconds to microseconds and calls the
 * udelay function to implement the delay.
 *
 * @param ms Number of milliseconds to delay.
 */
void mdelay(uint32_t ms) {
    udelay(ms * 1000);
}

/**
 * @brief Delay execution for a specified number of loops (microseconds).
 *
 * This function directly calls udelay with the specified number of loops,
 * which is treated as microseconds.
 *
 * @param loops Number of microsecond loops to delay.
 */
void sdelay(uint32_t loops) {
    udelay(loops);
}

/**
 * @brief Get the initialization timestamp.
 *
 * This function returns the timestamp that was set during initialization.
 *
 * @return The initialization timestamp in microseconds.
 */
uint32_t get_init_timestamp() {
    return init_timestamp;
}
