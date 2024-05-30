/* SPDX-License-Identifier: GPL-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <mmu.h>

#include <common.h>

#define rand_ul() rand32()
#define UL_ONEBITS 0xffffffff
#define UL_LEN 32
#define CHECKERBOARD1 0x55555555
#define CHECKERBOARD2 0xaaaaaaaa
#define UL_BYTE(x) ((x | x << 8 | x << 16 | x << 24))

typedef unsigned int ul;
typedef unsigned long long ull;
typedef unsigned int volatile ulv;
typedef unsigned char volatile u8v;
typedef unsigned short volatile u16v;

struct test {
    char *name;
    int (*fp)(ulv *bufa, ulv *bufb, size_t count);
};

union {
    unsigned char bytes[UL_LEN / 8];
    ul val;
} mword8;

union {
    unsigned short u16s[UL_LEN / 16];
    ul val;
} mword16;

uint32_t rand32() {
    return time_ms();
}

char progress[] = "-\\|/";
#define PROGRESSLEN 4
#define PROGRESSOFTEN 2500
#define ONE 0x00000001L

/* Function definitions. */

int compare_regions(ulv *bufa, ulv *bufb, size_t count) {
    int r = 0;
    size_t i;
    ulv *p1 = bufa;
    ulv *p2 = bufb;

    for (i = 0; i < count; i++, p1++, p2++) {
        if (*p1 != *p2) {
            printk(LOG_LEVEL_MUTE,
                   "FAILURE: 0x%x != 0x%x at physical address "
                   "0x%x 0x%x.\n",
                   *p1, *p2, p1, p2);
            r = -1;
            break;
        }
    }
    return r;
}

int test_stuck_address(ulv *bufa, size_t count) {
    ulv *p1 = bufa;
    unsigned int j;
    size_t i;

    printk(LOG_LEVEL_MUTE, "           ");
    for (j = 0; j < 16; j++) {
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        printk(LOG_LEVEL_MUTE, "setting %3u", j);
        for (i = 0; i < count; i++) {
            *p1 = ((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1);
            *p1++;
        }
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        printk(LOG_LEVEL_MUTE, "testing %3u", j);
        p1 = (ulv *) bufa;
        for (i = 0; i < count; i++, p1++) {
            if (*p1 != (((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1))) {
                printk(LOG_LEVEL_MUTE,
                       "FAILURE: possible bad address line at physical "
                       "address 0x%x.\n",
                       p1);

                printk(LOG_LEVEL_MUTE, "address 0x%x value is 0x%x, should be 0x%x\n", p1, *p1, (((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1)));

                printk(LOG_LEVEL_MUTE, "Skipping to next test...\n");
                return -1;
            }
        }
    }
    printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_random_value(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    ul j = 0;
    size_t i;

    uart_putchar(' ');
    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = rand_ul();
        if (!(i % PROGRESSOFTEN)) {
            uart_putchar('\b');
            uart_putchar(progress[++j % PROGRESSLEN]);
        }
    }
    printk(LOG_LEVEL_MUTE, "\b \b");
    return compare_regions(bufa, bufb, count);
}

int test_xor_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ ^= q;
        *p2++ ^= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_sub_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ -= q;
        *p2++ -= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_mul_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ *= q;
        *p2++ *= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_div_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        if (!q) {
            q++;
        }
        *p1++ /= q;
        *p2++ /= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_or_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ |= q;
        *p2++ |= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_and_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ &= q;
        *p2++ &= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_seqinc_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = (i + q);
    }
    return compare_regions(bufa, bufb, count);
}

int test_solidbits_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    size_t i;

    printk(LOG_LEVEL_MUTE, "           ");
    for (j = 0; j < 64; j++) {
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        q = (j % 2) == 0 ? UL_ONEBITS : 0;
        printk(LOG_LEVEL_MUTE, "setting %3u", j);
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        }
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        printk(LOG_LEVEL_MUTE, "testing %3u", j);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_checkerboard_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    size_t i;

    printk(LOG_LEVEL_MUTE, "           ");
    for (j = 0; j < 64; j++) {
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        q = (j % 2) == 0 ? CHECKERBOARD1 : CHECKERBOARD2;
        printk(LOG_LEVEL_MUTE, "setting %3u", j);
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        }
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        printk(LOG_LEVEL_MUTE, "testing %3u", j);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_blockseq_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printk(LOG_LEVEL_MUTE, "           ");
    for (j = 0; j < 256; j++) {
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        printk(LOG_LEVEL_MUTE, "setting %3u", j);
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (ul) UL_BYTE(j);
        }
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        printk(LOG_LEVEL_MUTE, "testing %3u", j);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_walkbits0_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printk(LOG_LEVEL_MUTE, "           ");
    for (j = 0; j < UL_LEN * 2; j++) {
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        printk(LOG_LEVEL_MUTE, "setting %3u", j);
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = ONE << j;
            } else { /* Walk it back down. */
                *p1++ = *p2++ = ONE << (UL_LEN * 2 - j - 1);
            }
        }
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        printk(LOG_LEVEL_MUTE, "testing %3u", j);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_walkbits1_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printk(LOG_LEVEL_MUTE, "           ");
    for (j = 0; j < UL_LEN * 2; j++) {
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        printk(LOG_LEVEL_MUTE, "setting %3u", j);
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = UL_ONEBITS ^ (ONE << j);
            } else { /* Walk it back down. */
                *p1++ = *p2++ = UL_ONEBITS ^ (ONE << (UL_LEN * 2 - j - 1));
            }
        }
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        printk(LOG_LEVEL_MUTE, "testing %3u", j);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_bitspread_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printk(LOG_LEVEL_MUTE, "           ");
    for (j = 0; j < UL_LEN * 2; j++) {
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        printk(LOG_LEVEL_MUTE, "setting %3u", j);
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = (i % 2 == 0)
                                        ? (ONE << j) | (ONE << (j + 2))
                                        : UL_ONEBITS ^ ((ONE << j) | (ONE << (j + 2)));
            } else { /* Walk it back down. */
                *p1++ = *p2++ = (i % 2 == 0)
                                        ? (ONE << (UL_LEN * 2 - 1 - j)) | (ONE << (UL_LEN * 2 + 1 - j))
                                        : UL_ONEBITS ^ (ONE << (UL_LEN * 2 - 1 - j) | (ONE << (UL_LEN * 2 + 1 - j)));
            }
        }
        printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
        printk(LOG_LEVEL_MUTE, "testing %3u", j);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_bitflip_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j, k;
    ul q;
    size_t i;

    printk(LOG_LEVEL_MUTE, "           ");
    for (k = 0; k < UL_LEN; k++) {
        q = ONE << k;
        for (j = 0; j < 8; j++) {
            printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
            q = ~q;
            printk(LOG_LEVEL_MUTE, "setting %3u", k * 8 + j);
            p1 = (ulv *) bufa;
            p2 = (ulv *) bufb;
            for (i = 0; i < count; i++) {
                *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
            }
            printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b");
            printk(LOG_LEVEL_MUTE, "testing %3u", k * 8 + j);
            if (compare_regions(bufa, bufb, count)) {
                return -1;
            }
        }
    }
    printk(LOG_LEVEL_MUTE, "\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int test_8bit_wide_random(ulv *bufa, ulv *bufb, size_t count) {
    u8v *p1, *t;
    ulv *p2;
    int attempt;
    unsigned int b, j = 0;
    size_t i;

    uart_putchar(' ');
    for (attempt = 0; attempt < 2; attempt++) {
        if (attempt & 1) {
            p1 = (u8v *) bufa;
            p2 = bufb;
        } else {
            p1 = (u8v *) bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword8.bytes;
            *p2++ = mword8.val = rand_ul();
            for (b = 0; b < UL_LEN / 8; b++) {
                *p1++ = *t++;
            }
            if (!(i % PROGRESSOFTEN)) {
                uart_putchar('\b');
                uart_putchar(progress[++j % PROGRESSLEN]);
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printk(LOG_LEVEL_MUTE, "\b \b");
    return 0;
}

int test_16bit_wide_random(ulv *bufa, ulv *bufb, size_t count) {
    u16v *p1, *t;
    ulv *p2;
    int attempt;
    unsigned int b, j = 0;
    size_t i;

    uart_putchar(' ');
    for (attempt = 0; attempt < 2; attempt++) {
        if (attempt & 1) {
            p1 = (u16v *) bufa;
            p2 = bufb;
        } else {
            p1 = (u16v *) bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword16.u16s;
            *p2++ = mword16.val = rand_ul();
            for (b = 0; b < UL_LEN / 16; b++) {
                *p1++ = *t++;
            }
            if (!(i % PROGRESSOFTEN)) {
                uart_putchar('\b');
                uart_putchar(progress[++j % PROGRESSLEN]);
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printk(LOG_LEVEL_MUTE, "\b \b");
    return 0;
}

static struct test tests[] = {
        {"Random Value", test_random_value},
        {"Compare XOR", test_xor_comparison},
        {"Compare SUB", test_sub_comparison},
        {"Compare MUL", test_mul_comparison},
        {"Compare DIV", test_div_comparison},
        {"Compare OR", test_or_comparison},
        {"Compare AND", test_and_comparison},
        {"Sequential Increment", test_seqinc_comparison},
        {"Solid Bits", test_solidbits_comparison},
        {"Block Sequential", test_blockseq_comparison},
        {"Checkerboard", test_checkerboard_comparison},
        {"Bit Spread", test_bitspread_comparison},
        {"Bit Flip", test_bitflip_comparison},
        {"Walking Ones", test_walkbits1_comparison},
        {"Walking Zeroes", test_walkbits0_comparison},
        {"8-bit Writes", test_8bit_wide_random},
        {"16-bit Writes", test_16bit_wide_random},
        {NULL, NULL}};

/* Function declarations */
static int do_memtester(uint64_t start_addr, uint64_t dram_size, uint64_t test_size, uint32_t loops) {
    ul loop, i;
    uint64_t bufsize, wantbytes, wantmb, halflen, count;
    char *memsuffix;
    int memshift;
    ulv *bufa, *bufb;

    wantbytes = test_size;
    wantmb = (wantbytes >> 20);
    halflen = wantbytes / 2;
    count = halflen / sizeof(ul);
    bufa = (uint32_t *) start_addr;
    bufb = (ulv *) ((size_t) bufa + test_size);

    printk(LOG_LEVEL_MUTE, "Memtester Want %dMB (%llu bytes)\n", wantmb, wantbytes);
    printk(LOG_LEVEL_MUTE, "bufa 0x%x, bufb 0x%x, loops %d, count %d\n", bufa, bufb, loops, count);
    printk(LOG_LEVEL_MUTE, "Loop %lu", loops);
    printk(LOG_LEVEL_MUTE, ":\n");
    printk(LOG_LEVEL_MUTE, "  %-20s: ", "Stuck Address");
    if (!test_stuck_address(bufa, wantbytes / sizeof(ul))) {
        printk(LOG_LEVEL_MUTE, "ok\n");
    } else {
        printk(LOG_LEVEL_MUTE, "bad\n");
    }
    for (i = 0;; i++) {
        if (!tests[i].name) break;

        printk(LOG_LEVEL_MUTE, "  %-20s: ", tests[i].name);

        if (!tests[i].fp(bufa, bufb, count)) {
            printk(LOG_LEVEL_MUTE, "ok\n");
        } else {
            printk(LOG_LEVEL_MUTE, "bad\n");
        }
    }
    printk(LOG_LEVEL_MUTE, "\n");

    printk(LOG_LEVEL_MUTE, "Done.\n");
    return 0;
}