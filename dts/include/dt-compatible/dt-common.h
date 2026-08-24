/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_DT_COMMON_H__
#define __DT_COMPATIBLE_DT_COMMON_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <dt2c/dt.h>

#ifdef TRACE_MODE
#include <log.h>
#endif

#if defined(TRACE_MODE) && LOG_LEVEL_DEFAULT >= LOG_LEVEL_TRACE
static inline __attribute__((always_inline)) bool syterkit_dt_trace_is_string(const void *data, int length)
{
	const uint8_t *bytes = (const uint8_t *)data;
	int index;

	if (bytes == NULL || length < 2 || bytes[length - 1] != '\0')
		return false;
	for (index = 0; index < length - 1; ++index) {
		if (bytes[index] < ' ' || bytes[index] > '~')
			return false;
	}
	return true;
}

static inline __attribute__((always_inline)) void syterkit_dt_trace_bytes(const char *prefix, const void *data, size_t size)
{
	const uint8_t *bytes = (const uint8_t *)data;
	size_t index;

	uart_printf("%s", prefix);
	for (index = 0; index < size; ++index) {
		if (index != 0U && index % 16U == 0U)
			uart_printf("\n                 ");
		uart_printf(" %02x", bytes[index]);
	}
	uart_printf("\n");
}

static inline __attribute__((always_inline)) void syterkit_dt_trace_node(const char *driver, int node)
{
	const struct dt2c_fdt_property *property;
	const char *name;
	const char *property_name;
	const void *value;
	int length;
	int property_offset;

	name = dt2c_fdt_get_name(DT2C_FDT_COMPILED_TREE, node, NULL);
	printk_trace("DT: %s node=%d name=%s\n", driver, node, name != NULL ? name : "<unknown>");
	dt2c_fdt_for_each_property_offset(property_offset, DT2C_FDT_COMPILED_TREE, node)
	{
		property = dt2c_fdt_get_property_by_offset(DT2C_FDT_COMPILED_TREE, property_offset, &length);
		if (property == NULL || length < 0)
			continue;
		property_name = dt2c_fdt_string(DT2C_FDT_COMPILED_TREE, (int)dt2c_fdt32_to_cpu(property->nameoff));
		value = property->data;
		if (property_name == NULL)
			property_name = "<unknown>";
		if (syterkit_dt_trace_is_string(value, length)) {
			printk_trace("DT:   %s.%s = \"%s\"\n", driver, property_name, (const char *)value);
			continue;
		}
		printk_trace("DT:   %s.%s (%d bytes) =", driver, property_name, length);
		syterkit_dt_trace_bytes("", value, (size_t)length);
	}
}

#define SYTERKIT_DT_TRACE_NODE(driver, node) syterkit_dt_trace_node((driver), (node))
#define SYTERKIT_DT_TRACE(fmt, ...) printk_trace("DT: " fmt, ##__VA_ARGS__)
#else
#define SYTERKIT_DT_TRACE_NODE(driver, node) ((void)0)
#define SYTERKIT_DT_TRACE(fmt, ...) ((void)0)
#endif

static inline __attribute__((always_inline)) bool syterkit_dt_string_equal(const char *value, int length, const char *expected, size_t expected_length)
{
	return value != NULL && length == (int)expected_length + 1 && value[expected_length] == '\0' && __builtin_memcmp(value, expected, expected_length) == 0;
}

static inline __attribute__((always_inline)) const dt2c_fdt32_t *syterkit_dt_cells(int node, const char *name, size_t count)
{
	const dt2c_fdt32_t *cells;
	int length;

	cells = (const dt2c_fdt32_t *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, name, &length);
	if (cells == NULL || count > (size_t)0x7fffffffU / sizeof(*cells) || length != (int)(count * sizeof(*cells)))
		return NULL;
	return cells;
}

static inline __attribute__((always_inline)) bool syterkit_dt_node_available(int node)
{
	return dt2c_fdt_node_is_available(DT2C_FDT_COMPILED_TREE, node) > 0;
}

static inline __attribute__((always_inline)) int syterkit_dt_alias_node(const char *alias, const char *compatible)
{
	int node;

	if (alias == NULL)
		return -DT2C_FDT_ERR_BADVALUE;
	node = dt2c_fdt_alias_node_offset(DT2C_FDT_COMPILED_TREE, alias);
	if (node < 0)
		return node;
	if (!syterkit_dt_node_available(node) || (compatible != NULL && dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, compatible) != 0))
		return -DT2C_FDT_ERR_BADVALUE;
	return node;
}

static inline __attribute__((always_inline)) int syterkit_dt_read_reg_alias(const char *alias, uintptr_t *base, size_t *size)
{
	const dt2c_fdt32_t *reg;
	uintptr_t value;
	int node;

	if (base == NULL || size == NULL)
		return -DT2C_FDT_ERR_BADVALUE;
	node = syterkit_dt_alias_node(alias, NULL);
	if (node < 0)
		return node;
	reg = syterkit_dt_cells(node, "reg", 2);
	if (reg == NULL)
		return -DT2C_FDT_ERR_BADVALUE;
	value = (uintptr_t)dt2c_fdt32_to_cpu(reg[0]);
	*size = (size_t)dt2c_fdt32_to_cpu(reg[1]);
	if (value == 0U || *size == 0U || value + *size < value)
		return -DT2C_FDT_ERR_BADVALUE;
	*base = value;
	return 0;
}

static inline __attribute__((always_inline)) int syterkit_dt_phandle_node(int node, const char *name, const char *compatible)
{
	const dt2c_fdt32_t *phandle;
	int referenced;

	phandle = syterkit_dt_cells(node, name, 1);
	if (phandle == NULL)
		return -DT2C_FDT_ERR_BADVALUE;
	referenced = dt2c_fdt_node_offset_by_phandle(DT2C_FDT_COMPILED_TREE, dt2c_fdt32_to_cpu(phandle[0]));
	if (referenced < 0)
		return referenced;
	if (!syterkit_dt_node_available(referenced) || (compatible != NULL && dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, referenced, compatible) != 0))
		return -DT2C_FDT_ERR_BADVALUE;
	return referenced;
}

#endif /* __DT_COMPATIBLE_DT_COMMON_H__ */
