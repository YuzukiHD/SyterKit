/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_DT_COMMON_H__
#define __DT_COMPATIBLE_DT_COMMON_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <dt2c/dt.h>

static inline __attribute__((always_inline)) bool
syterkit_dt_string_equal(const char *value, int length,
			 const char *expected, size_t expected_length) {
	return value != NULL && length == (int) expected_length + 1 &&
	       value[expected_length] == '\0' &&
	       __builtin_memcmp(value, expected, expected_length) == 0;
}

static inline __attribute__((always_inline)) const dt2c_fdt32_t *
syterkit_dt_cells(int node, const char *name, size_t count) {
	const dt2c_fdt32_t *cells;
	int length;

	cells = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, name, &length);
	if (cells == NULL || count > (size_t) 0x7fffffffU / sizeof(*cells) ||
	    length != (int) (count * sizeof(*cells)))
		return NULL;
	return cells;
}

static inline __attribute__((always_inline)) bool
syterkit_dt_node_available(int node) {
	while (node >= 0) {
		const char *status;
		int length;

		status = (const char *) dt2c_fdt_getprop(
				DT2C_FDT_COMPILED_TREE, node, "status", &length);
		if (status != NULL &&
		    !syterkit_dt_string_equal(status, length, "okay", 4) &&
		    !syterkit_dt_string_equal(status, length, "ok", 2))
			return false;
		node = dt2c_fdt_parent_offset(DT2C_FDT_COMPILED_TREE, node);
	}

	return node == -DT2C_FDT_ERR_NOTFOUND;
}

static inline __attribute__((always_inline)) int
syterkit_dt_alias_node(const char *alias, const char *compatible) {
	int node;

	node = dt2c_fdt_alias_node_offset(DT2C_FDT_COMPILED_TREE, alias);
	if (node < 0)
		return node;
	if (!syterkit_dt_node_available(node) ||
	    (compatible != NULL &&
	     dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
					    compatible) != 0))
		return -DT2C_FDT_ERR_BADVALUE;
	return node;
}

static inline __attribute__((always_inline)) int
syterkit_dt_phandle_node(int node, const char *name, const char *compatible) {
	const dt2c_fdt32_t *phandle;
	int referenced;

	phandle = syterkit_dt_cells(node, name, 1);
	if (phandle == NULL)
		return -DT2C_FDT_ERR_BADVALUE;
	referenced = dt2c_fdt_node_offset_by_phandle(
			DT2C_FDT_COMPILED_TREE, dt2c_fdt32_to_cpu(phandle[0]));
	if (referenced < 0)
		return referenced;
	if (!syterkit_dt_node_available(referenced) ||
	    (compatible != NULL &&
	     dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, referenced,
					    compatible) != 0))
		return -DT2C_FDT_ERR_BADVALUE;
	return referenced;
}

#endif /* __DT_COMPATIBLE_DT_COMMON_H__ */
