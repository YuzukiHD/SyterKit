/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

typedef struct boot_file_head {
    uint32_t jump_instruction; /* one intruction jumping to real code */
    uint8_t magic[8];          /* ="eGON.BT0" */
    uint32_t check_sum;        /* generated by PC */
    uint32_t *length;          /* generated by LD */
    uint32_t pub_head_size;    /* the size of boot_file_head_t */
    uint8_t pub_head_vsn[4];   /* the version of boot_file_head_t */
    uint32_t *ret_addr;        /* the return value */
    uint32_t *run_addr;        /* run addr */
    uint32_t boot_cpu;         /* eGON version */
    uint8_t platform[8];       /* platform information */
} boot_file_head_t;

/*
* This code is used to calculate some values related to the size of a file header
* and combine these values to form a jump instruction:
*
* * BROM_FILE_HEAD_SIZE: Defines a mask for the size of the file header
*   and uses the bitwise & operator with 0x00FFFFF to truncate and obtain the size of the file header.
*
* * BROM_FILE_HEAD_BIT_10_1, BROM_FILE_HEAD_BIT_11, BROM_FILE_HEAD_BIT_19_12, BROM_FILE_HEAD_BIT_20:
*   These macros define the values of various bit segments of the file header size.
*
* * BROM_FILE_HEAD_SIZE_OFFSET: This macro combines the bit segments of the file header size
*   calculated above to form an offset.
*
* * JUMP_INSTRUCTION: This macro uses the calculated offset from above and performs a bitwise
*   OR operation with 0x6f to create a jump instruction.
*
* This code calculates an offset corresponding to the size of the file header through a series of
* bitwise operations and utilizes it to construct a jump instruction. 
*/

#define BROM_FILE_HEAD_PADDING (0x10)
#define BROM_FILE_HEAD_SIZE ((sizeof(boot_file_head_t) + BROM_FILE_HEAD_PADDING) & 0x00FFFFF)
#define BROM_FILE_HEAD_BIT_10_1 ((BROM_FILE_HEAD_SIZE & 0x7FE) >> 1)
#define BROM_FILE_HEAD_BIT_11 ((BROM_FILE_HEAD_SIZE & 0x800) >> 11)
#define BROM_FILE_HEAD_BIT_19_12 ((BROM_FILE_HEAD_SIZE & 0xFF000) >> 12)
#define BROM_FILE_HEAD_BIT_20 ((BROM_FILE_HEAD_SIZE & 0x100000) >> 20)

#define BROM_FILE_HEAD_SIZE_OFFSET ((BROM_FILE_HEAD_BIT_20 << 31) |   \
                                    (BROM_FILE_HEAD_BIT_10_1 << 21) | \
                                    (BROM_FILE_HEAD_BIT_11 << 20) |   \
                                    (BROM_FILE_HEAD_BIT_19_12 << 12))

#define JUMP_INSTRUCTION (BROM_FILE_HEAD_SIZE_OFFSET | 0x6f)

#define BOOT0_MAGIC "eGON.BT0"
#define STAMP_VALUE (0x12345678)
#define BOOT_PUB_HEAD_VERSION "3000"

extern uint32_t __spl_size[];
extern uint32_t __code_start_address[];

const __attribute__((section(".boot0_head"))) boot_file_head_t boot_head = {
        .jump_instruction = JUMP_INSTRUCTION,
        .magic = BOOT0_MAGIC,
        .check_sum = STAMP_VALUE,
        .length = __spl_size,
        .pub_head_size = sizeof(boot_file_head_t),
        .pub_head_vsn = BOOT_PUB_HEAD_VERSION,
        .ret_addr = __code_start_address,
        .run_addr = __code_start_address,
        .boot_cpu = 0,
        .platform = {0, 0, '3', '.', '0', '.', '0', 0},
};
