/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __STDARG_H__
#define __STDARG_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef __builtin_va_list va_list; /**< Type used for variable argument lists. */

/**
 * @file stdarg.h
 * @brief Standard variable argument handling for C.
 *
 * This header file provides macros for handling functions with a variable
 * number of arguments. It defines the type `va_list` and the associated
 * macros to initialize, access, and clean up variable arguments.
 */

/**
 * @macro va_start
 * @brief Initialize a `va_list` for variable argument access.
 * 
 * This macro initializes the `va_list` variable `v` to retrieve the variable
 * arguments, starting after the last named parameter `l`.
 * 
 * @param v The `va_list` variable to be initialized.
 * @param l The last known fixed argument before the variable arguments.
 */
#define va_start(v, l) __builtin_va_start(v, l)

/**
 * @macro va_arg
 * @brief Retrieve the next argument in the variable argument list.
 * 
 * This macro retrieves the next argument from the `va_list` variable `v` and
 * advances `v` to the next argument. The type of the argument to be retrieved
 * is specified by the type `l`.
 * 
 * @param v The `va_list` variable from which to retrieve the argument.
 * @param l The type of the argument to be retrieved.
 * @return The next argument in the variable argument list.
 */
#define va_arg(v, l) __builtin_va_arg(v, l)

/**
 * @macro va_end
 * @brief Clean up the `va_list` variable.
 * 
 * This macro performs any necessary cleanup for the `va_list` variable `v`
 * after all variable arguments have been accessed.
 * 
 * @param v The `va_list` variable to be cleaned up.
 */
#define va_end(v) __builtin_va_end(v)

/**
 * @macro va_copy
 * @brief Copy a `va_list` variable.
 * 
 * This macro copies the variable argument list from `s` to `d`, allowing
 * multiple traversals of the same list.
 * 
 * @param d The destination `va_list` variable.
 * @param s The source `va_list` variable to be copied.
 */
#define va_copy(d, s) __builtin_va_copy(d, s)

#ifdef __cplusplus
}
#endif

#endif /* __STDARG_H__ */
