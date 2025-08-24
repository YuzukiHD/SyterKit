/* SPDX-License-Identifier: GPL-2.0+ */
#include <config.h>
#include <io.h>
#include <log.h>
#include <timer.h>

void __attribute__((weak)) show_chip(void) {
}

void show_banner(void) {
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
