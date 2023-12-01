/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2012, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __FDT_H__
#define __FDT_H__

/* see linux document: ./Documentation/devicetree/booting-without-of.txt */
#define OF_DT_MAGIC 0xd00dfeed

typedef struct
{
	unsigned int magic_number;
	unsigned int total_size;
	unsigned int offset_dt_struct;
	unsigned int offset_dt_strings;
	unsigned int offset_reserve_map;
	unsigned int format_version;
	unsigned int last_compatible_version;

	/* version 2 field */
	unsigned int mach_id;
	/* version 3 field */
	unsigned int dt_strings_len;
	/* version 17 field */
	unsigned int dt_struct_len;
} boot_param_header_t;

unsigned int of_get_magic_number(void *blob);
unsigned int of_get_dt_total_size(void *blob);
int check_dt_blob_valid(void *blob);
int fixup_chosen_node(void *blob, char *bootargs);
int fixup_memory_node(void *blob, unsigned int *mem_bank, unsigned int *mem_size);
#endif /* #ifndef __FDT_H__ */
