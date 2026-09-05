/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file common.c
 * @brief Common firmware presentation and board-identification helpers.
 *
 * The routines in this file are deliberately small so every board can share
 * the same startup banner while providing an optional SoC-specific chip
 * identification hook.
 */
#include <config.h>
#include <io.h>
#include <log.h>
#include <timer.h>

/**
 * @brief Display board or SoC identification information.
 *
 * Boards may override this weak hook when they can read a useful chip ID.  The
 * default implementation is intentionally empty so boards without an ID
 * reader do not need a dummy source file.
 */
void __attribute__((weak)) show_chip(void)
{
}

/**
 * @brief Print the SyterKit startup banner and build identity.
 *
 * The banner includes the project version, commit hash, compiler identity,
 * and any board-specific information emitted by `show_chip()`.  It writes to
 * the logging console and has no return value or persistent side effects.
 */
static void show_banner_impl(const char *build_info)
{
	printk(LOG_LEVEL_MUTE, "\n");
	pr_info(" _____     _           _____ _ _   \n");
	pr_info("|   __|_ _| |_ ___ ___|  |  |_| |_ \n");
	pr_info("|__   | | |  _| -_|  _|    -| | _| \n");
	pr_info("|_____|_  |_| |___|_| |__|__|_|_|  \n");
	pr_info("      |___|                        \n");
	pr_info("***********************************\n");
	pr_info(" %s v%s Commit: %s\n", PROJECT_NAME, PROJECT_VERSION, PROJECT_GIT_HASH);
	pr_info(" github.com/YuzukiHD/SyterKit      \n");
	pr_info("***********************************\n");
	pr_info(" Built by: %s\n", build_info);
	pr_info("\n");

	show_chip();
}

void show_banner(void)
{
	static const char build_info[] = PROJECT_C_COMPILER " " PROJECT_C_COMPILER_VERSION;

	show_banner_impl(build_info);
}

void show_banner_with_build_info(const char *build_info)
{
	if (build_info == NULL || build_info[0] == '\0')
		return show_banner();
	show_banner_impl(build_info);
}
