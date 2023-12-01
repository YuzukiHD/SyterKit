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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>

#include <log.h>

#include "fdt.h"

#define OF_DT_TOKEN_NODE_BEGIN 0x00000001 /* Start node */
#define OF_DT_TOKEN_NODE_END 0x00000002	  /* End node */
#define OF_DT_TOKEN_PROP 0x00000003		  /* Property */
#define OF_DT_TOKEN_NOP 0x00000004
#define OF_DT_END 0x00000009

inline unsigned int of_get_magic_number(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->magic_number);
}

static inline unsigned int of_get_format_version(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->format_version);
}

static inline unsigned int of_get_offset_dt_strings(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->offset_dt_strings);
}

static inline void of_set_offset_dt_strings(void *blob, unsigned int offset)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	header->offset_dt_strings = swap_uint32(offset);
}

static inline char *of_get_string_by_offset(void *blob, unsigned int offset)
{
	return (char *)((unsigned int)blob + of_get_offset_dt_strings(blob) + offset);
}

static inline unsigned int of_get_offset_dt_struct(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->offset_dt_struct);
}

static inline unsigned int of_dt_struct_offset(void *blob, unsigned int offset)
{
	return (unsigned int)blob + of_get_offset_dt_struct(blob) + offset;
}

unsigned int of_get_dt_total_size(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->total_size);
}

static inline void of_set_dt_total_size(void *blob, unsigned int size)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	header->total_size = swap_uint32(size);
}

static inline unsigned int of_get_dt_strings_len(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->dt_strings_len);
}

static inline void of_set_dt_strings_len(void *blob, unsigned int len)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	header->dt_strings_len = swap_uint32(len);
}

static inline unsigned int of_get_dt_struct_len(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->dt_struct_len);
}

static inline void of_set_dt_struct_len(void *blob, unsigned int len)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	header->dt_struct_len = swap_uint32(len);
}

static inline int of_blob_data_size(void *blob)
{
	return (unsigned int)of_get_offset_dt_strings(blob) + of_get_dt_strings_len(blob);
}
