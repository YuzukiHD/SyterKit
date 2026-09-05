/* SPDX-License-Identifier: MIT */
#ifndef DT2C_DT_H
#define DT2C_DT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __CHECKER__
#define DT2C_FDT_FORCE __attribute__((force))
#define DT2C_FDT_BITWISE __attribute__((bitwise))
#else
#define DT2C_FDT_FORCE
#define DT2C_FDT_BITWISE
#endif

typedef uint16_t DT2C_FDT_BITWISE dt2c_fdt16_t;
typedef uint32_t DT2C_FDT_BITWISE dt2c_fdt32_t;
typedef uint64_t DT2C_FDT_BITWISE dt2c_fdt64_t;

struct dt2c_fdt_header {
	dt2c_fdt32_t magic;
	dt2c_fdt32_t totalsize;
	dt2c_fdt32_t off_dt_struct;
	dt2c_fdt32_t off_dt_strings;
	dt2c_fdt32_t off_mem_rsvmap;
	dt2c_fdt32_t version;
	dt2c_fdt32_t last_comp_version;
	dt2c_fdt32_t boot_cpuid_phys;
	dt2c_fdt32_t size_dt_strings;
	dt2c_fdt32_t size_dt_struct;
};

struct dt2c_fdt_reserve_entry {
	dt2c_fdt64_t address;
	dt2c_fdt64_t size;
};

struct dt2c_fdt_node_header {
	dt2c_fdt32_t tag;
	char name[];
};

struct dt2c_fdt_property {
	dt2c_fdt32_t tag;
	dt2c_fdt32_t len;
	dt2c_fdt32_t nameoff;
	char data[];
};

#define DT2C_FDT_MAGIC 0xd00dfeedU
#define DT2C_FDT_TAGSIZE sizeof(dt2c_fdt32_t)
#define DT2C_FDT_BEGIN_NODE 0x1U
#define DT2C_FDT_END_NODE 0x2U
#define DT2C_FDT_PROP 0x3U
#define DT2C_FDT_NOP 0x4U
#define DT2C_FDT_END 0x9U

#define DT2C_FDT_V1_SIZE (7 * sizeof(dt2c_fdt32_t))
#define DT2C_FDT_V2_SIZE (DT2C_FDT_V1_SIZE + sizeof(dt2c_fdt32_t))
#define DT2C_FDT_V3_SIZE (DT2C_FDT_V2_SIZE + sizeof(dt2c_fdt32_t))
#define DT2C_FDT_V16_SIZE DT2C_FDT_V3_SIZE
#define DT2C_FDT_V17_SIZE (DT2C_FDT_V16_SIZE + sizeof(dt2c_fdt32_t))

#define DT2C_FDT_FIRST_SUPPORTED_VERSION 0x02
#define DT2C_FDT_LAST_COMPATIBLE_VERSION 0x10
#define DT2C_FDT_LAST_SUPPORTED_VERSION 0x11

#define DT2C_FDT_ERR_NOTFOUND 1
#define DT2C_FDT_ERR_EXISTS 2
#define DT2C_FDT_ERR_NOSPACE 3
#define DT2C_FDT_ERR_BADOFFSET 4
#define DT2C_FDT_ERR_BADPATH 5
#define DT2C_FDT_ERR_BADPHANDLE 6
#define DT2C_FDT_ERR_BADSTATE 7
#define DT2C_FDT_ERR_TRUNCATED 8
#define DT2C_FDT_ERR_BADMAGIC 9
#define DT2C_FDT_ERR_BADVERSION 10
#define DT2C_FDT_ERR_BADSTRUCTURE 11
#define DT2C_FDT_ERR_BADLAYOUT 12
#define DT2C_FDT_ERR_INTERNAL 13
#define DT2C_FDT_ERR_BADNCELLS 14
#define DT2C_FDT_ERR_BADVALUE 15
#define DT2C_FDT_ERR_BADOVERLAY 16
#define DT2C_FDT_ERR_NOPHANDLES 17
#define DT2C_FDT_ERR_BADFLAGS 18
#define DT2C_FDT_ERR_ALIGNMENT 19
#define DT2C_FDT_ERR_MAX 19

#define DT2C_FDT_MAX_PHANDLE 0xfffffffeU
#define DT2C_FDT_MAX_NCELLS 4

static inline uint16_t dt2c_fdt16_ld(const dt2c_fdt16_t *value)
{
	const uint8_t *bytes = (const uint8_t *)value;
	return ((uint16_t)bytes[0] << 8) | bytes[1];
}

static inline uint32_t dt2c_fdt32_ld(const dt2c_fdt32_t *value)
{
	const uint8_t *bytes = (const uint8_t *)value;
	return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
	       ((uint32_t)bytes[2] << 8) | bytes[3];
}

static inline uint64_t dt2c_fdt64_ld(const dt2c_fdt64_t *value)
{
	const uint8_t *bytes = (const uint8_t *)value;
	return ((uint64_t)bytes[0] << 56) | ((uint64_t)bytes[1] << 48) |
	       ((uint64_t)bytes[2] << 40) | ((uint64_t)bytes[3] << 32) |
	       ((uint64_t)bytes[4] << 24) | ((uint64_t)bytes[5] << 16) |
	       ((uint64_t)bytes[6] << 8) | bytes[7];
}

static inline void dt2c_fdt32_st(void *property, uint32_t value)
{
	uint8_t *bytes = (uint8_t *)property;
	bytes[0] = (uint8_t)(value >> 24);
	bytes[1] = (uint8_t)(value >> 16);
	bytes[2] = (uint8_t)(value >> 8);
	bytes[3] = (uint8_t)value;
}

static inline void dt2c_fdt64_st(void *property, uint64_t value)
{
	uint8_t *bytes = (uint8_t *)property;
	bytes[0] = (uint8_t)(value >> 56);
	bytes[1] = (uint8_t)(value >> 48);
	bytes[2] = (uint8_t)(value >> 40);
	bytes[3] = (uint8_t)(value >> 32);
	bytes[4] = (uint8_t)(value >> 24);
	bytes[5] = (uint8_t)(value >> 16);
	bytes[6] = (uint8_t)(value >> 8);
	bytes[7] = (uint8_t)value;
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __DT2C_CONST_FDT32(value)                                          \
	((DT2C_FDT_FORCE dt2c_fdt32_t)__builtin_bswap32((uint32_t)(value)))

static inline uint16_t dt2c_fdt16_to_cpu(dt2c_fdt16_t value)
{
	return __builtin_bswap16((DT2C_FDT_FORCE uint16_t)value);
}

static inline dt2c_fdt16_t dt2c_cpu_to_fdt16(uint16_t value)
{
	return (DT2C_FDT_FORCE dt2c_fdt16_t)__builtin_bswap16(value);
}

static inline uint32_t dt2c_fdt32_to_cpu(dt2c_fdt32_t value)
{
	return __builtin_bswap32((DT2C_FDT_FORCE uint32_t)value);
}

static inline dt2c_fdt32_t dt2c_cpu_to_fdt32(uint32_t value)
{
	return (DT2C_FDT_FORCE dt2c_fdt32_t)__builtin_bswap32(value);
}

static inline uint64_t dt2c_fdt64_to_cpu(dt2c_fdt64_t value)
{
	return __builtin_bswap64((DT2C_FDT_FORCE uint64_t)value);
}

static inline dt2c_fdt64_t dt2c_cpu_to_fdt64(uint64_t value)
{
	return (DT2C_FDT_FORCE dt2c_fdt64_t)__builtin_bswap64(value);
}
#else
#define __DT2C_CONST_FDT32(value) ((DT2C_FDT_FORCE dt2c_fdt32_t)(uint32_t)(value))

static inline uint16_t dt2c_fdt16_to_cpu(dt2c_fdt16_t value)
{
	return (DT2C_FDT_FORCE uint16_t)value;
}

static inline dt2c_fdt16_t dt2c_cpu_to_fdt16(uint16_t value)
{
	return (DT2C_FDT_FORCE dt2c_fdt16_t)value;
}

static inline uint32_t dt2c_fdt32_to_cpu(dt2c_fdt32_t value)
{
	return (DT2C_FDT_FORCE uint32_t)value;
}

static inline dt2c_fdt32_t dt2c_cpu_to_fdt32(uint32_t value)
{
	return (DT2C_FDT_FORCE dt2c_fdt32_t)value;
}

static inline uint64_t dt2c_fdt64_to_cpu(dt2c_fdt64_t value)
{
	return (DT2C_FDT_FORCE uint64_t)value;
}

static inline dt2c_fdt64_t dt2c_cpu_to_fdt64(uint64_t value)
{
	return (DT2C_FDT_FORCE dt2c_fdt64_t)value;
}
#endif

/* The token is never dereferenced and therefore does not retain a DTB. */
#define DT2C_FDT_COMPILED_TREE ((const void *)(uintptr_t)1)

#include <generated/fdt_generated.h>
#undef __DT2C_CONST_FDT32
#include <dt2c/dt_runtime.h>

#define dt2c_fdt_get_header(fdt, field) dt2c_fdt_##field(fdt)
#define dt2c_fdt_magic(fdt) ((void)(fdt), DT2C_FDT_MAGIC)
#define dt2c_fdt_totalsize(fdt) ((void)(fdt), __DT2C_FDT_TOTAL_SIZE)
#define dt2c_fdt_off_mem_rsvmap(fdt) ((void)(fdt), 40U)
#define dt2c_fdt_off_dt_struct(fdt)                                       \
	((void)(fdt),                                                       \
	 40U + (uint32_t)(dt2c_fdt_num_mem_rsv(                            \
			    DT2C_FDT_COMPILED_TREE) +                         \
			    1) *                                                \
		 16U)
#define dt2c_fdt_off_dt_strings(fdt)                                      \
	((void)(fdt), dt2c_fdt_off_dt_struct(fdt) + __DT2C_FDT_STRUCT_SIZE)
#define dt2c_fdt_version(fdt) ((void)(fdt), DT2C_FDT_LAST_SUPPORTED_VERSION)
#define dt2c_fdt_last_comp_version(fdt)                                   \
	((void)(fdt), DT2C_FDT_LAST_COMPATIBLE_VERSION)
#define dt2c_fdt_boot_cpuid_phys(fdt) ((void)(fdt), 0U)
#define dt2c_fdt_size_dt_strings(fdt)                                     \
	((void)(fdt), __DT2C_FDT_STRINGS_SIZE)
#define dt2c_fdt_size_dt_struct(fdt) ((void)(fdt), __DT2C_FDT_STRUCT_SIZE)

static inline size_t dt2c_fdt_header_size_(uint32_t version)
{
	if (version <= 1)
		return 7 * sizeof(dt2c_fdt32_t);
	if (version == 2)
		return 8 * sizeof(dt2c_fdt32_t);
	if (version <= 16)
		return 9 * sizeof(dt2c_fdt32_t);
	return sizeof(struct dt2c_fdt_header);
}

static inline size_t dt2c_fdt_header_size(const void *fdt)
{
	(void)fdt;
	return sizeof(struct dt2c_fdt_header);
}

/* Host-side dt2c scans this marker; it emits no target data. */
#define DT2C_DRIVER_COMPAT(compatible)

#define dt2c_fdt_for_each_subnode(node, fdt, parent)                       \
	for ((node) = dt2c_fdt_first_subnode((fdt), (parent)); (node) >= 0;  \
	     (node) = dt2c_fdt_next_subnode((fdt), (node)))

#define dt2c_fdt_for_each_property_offset(property, fdt, node)             \
	for ((property) = dt2c_fdt_first_property_offset((fdt), (node));     \
	     (property) >= 0;                                                \
	     (property) = dt2c_fdt_next_property_offset((fdt), (property)))

static inline uint32_t dt2c_fdt_get_max_phandle(const void *fdt)
{
	uint32_t phandle;
	int error = dt2c_fdt_find_max_phandle(fdt, &phandle);

	return error < 0 ? (uint32_t)-1 : phandle;
}

#ifdef __cplusplus
}
#endif

#endif /* DT2C_DT_H */
