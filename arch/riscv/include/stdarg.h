/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __STDARG_H__
#define __STDARG_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef __builtin_va_list va_list; /**< Type used for variable argument lists. */

/**
 * @file
 * @brief Standard variable argument handling for C.
 *
 * This header file provides macros for handling functions with a variable
 * number of arguments. It defines the type `va_list` and the associated
 * macros to initialize, access, and clean up variable arguments.
 */

/**
 * @def va_start(v, l)
 * @brief Initialize a `va_list` for variable argument access.
 * 
 * This macro initializes the `va_list` variable `v` to retrieve the variable
 * arguments, starting after the last named parameter `l`.
 * 
 * @param[out] v The `va_list` variable to initialize.
 * @param[in] l The last named parameter before the variable arguments.
 */
#define va_start(v, l) __builtin_va_start(v, l)

/**
 * @def va_arg(v, l)
 * @brief Retrieve the next argument in the variable argument list.
 * 
 * This macro retrieves the next argument from the `va_list` variable `v` and
 * advances `v` to the next argument. The type of the argument to be retrieved
 * is specified by the type `l`.
 * 
 * @param[in,out] v The `va_list` variable from which to retrieve the argument.
 * @param[in] l The type of the argument to retrieve.
 * @return The next argument in the variable argument list.
 */
#define va_arg(v, l) __builtin_va_arg(v, l)

/**
 * @def va_end(v)
 * @brief Clean up the `va_list` variable.
 * 
 * This macro performs any necessary cleanup for the `va_list` variable `v`
 * after all variable arguments have been accessed.
 * 
 * @param[in,out] v The `va_list` variable to clean up.
 */
#define va_end(v) __builtin_va_end(v)

/**
 * @def va_copy(d, s)
 * @brief Copy a `va_list` variable.
 * 
 * This macro copies the variable argument list from `s` to `d`, allowing
 * multiple traversals of the same list.
 * 
 * @param[out] d The destination `va_list` variable.
 * @param[in] s The source `va_list` variable to copy.
 */
#define va_copy(d, s) __builtin_va_copy(d, s)

#ifdef __cplusplus
}
#endif

#endif /* __STDARG_H__ */
