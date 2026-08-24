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
void show_banner(void)
{
	printk(LOG_LEVEL_MUTE, "\n");
	printk_info(" _____     _           _____ _ _   \n");
	printk_info("|   __|_ _| |_ ___ ___|  |  |_| |_ \n");
	printk_info("|__   | | |  _| -_|  _|    -| | _| \n");
	printk_info("|_____|_  |_| |___|_| |__|__|_|_|  \n");
	printk_info("      |___|                        \n");
	printk_info("***********************************\n");
	printk_info(" %s v%s Commit: %s\n", PROJECT_NAME, PROJECT_VERSION, PROJECT_GIT_HASH);
	printk_info(" github.com/YuzukiHD/SyterKit      \n");
	printk_info("***********************************\n");
	printk_info(" Built by: %s %s\n", PROJECT_C_COMPILER, PROJECT_C_COMPILER_VERSION);
	printk_info("\n");

	show_chip();
}
