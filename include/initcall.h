/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __INITCALL_H__
#define __INITCALL_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Function signature used by boot-time initialization callbacks. */
typedef int (*initcall_t)(void);

/**
 * @brief Execute an inclusive-exclusive range of initialization callbacks.
 * @param[in] start First callback entry to execute.
 * @param[in] end Entry immediately after the last callback.
 * @return The first non-zero callback result, or zero when all callbacks pass.
 *
 * Every callback is executed even when an earlier callback reports an error.
 */
int do_initcall_range(const initcall_t *start, const initcall_t *end);

/**
 * @brief Execute all linker-registered initialization callbacks once.
 * @return The first non-zero callback result, or zero when all callbacks pass.
 */
int do_initcalls(void);

#define __initcall_stringify_1(value) #value
#define __initcall_stringify(value) __initcall_stringify_1(value)
#define __initcall_section(level)                                        \
	".initcall" __initcall_stringify(level) ".init"

#define __initcall_symbol_1(fn, id) __initcall_##fn##_##id
#define __initcall_symbol(fn, id) __initcall_symbol_1(fn, id)

#define ___define_initcall(fn, level, id)                                \
	static initcall_t const __initcall_symbol(fn, id)                  \
			__attribute__((used, section(__initcall_section(level)), \
					aligned(sizeof(initcall_t)))) = (fn)

/* __COUNTER__ permits repeated registration of the same callback. */
#define __define_initcall(fn, level)                                     \
	___define_initcall(fn, level, __COUNTER__)

/* Four levels are enough for the flat SyterKit boot and driver model. */
#define early_initcall(fn) __define_initcall(fn, early)
#define core_initcall(fn) __define_initcall(fn, 1)
#define device_initcall(fn) __define_initcall(fn, 6)
#define late_initcall(fn) __define_initcall(fn, 7)

/* Match the kernel's default of placing plain initcalls at device level. */
#define initcall(fn) device_initcall(fn)

#ifdef __cplusplus
}
#endif

#endif /* __INITCALL_H__ */
