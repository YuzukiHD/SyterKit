/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file eabi_compat.c
 * @brief Minimal ARM EABI runtime compatibility shims for the Yuzuki Neko.
 */

/**
 * @brief Halt the system in an infinite loop.
 */
void abort(void)
{
	while (1)
		;
}

/**
 * @brief Handle a raise signal.
 *
 * @param[in] signum Signal number, ignored by this stub.
 * @return Always 0.
 */
int raise(int signum)
{
	return 0;
}

/**
 * @brief Suppress unwind table generation for C code.
 *
 * Provides the ARM EABI unwind routine so the linker does not pull in extra
 * runtime support code.
 */
/* Dummy function to avoid linker complaints */
void __aeabi_unwind_cpp_pr0(void)
{
}