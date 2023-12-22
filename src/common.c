/* SPDX-License-Identifier: Apache-2.0 */
#include <config.h>
#include <io.h>
#include <log.h>
#include <timer.h>

void show_banner(void) {
    printk(LOG_LEVEL_MUTE, "\n");
    printk(LOG_LEVEL_INFO, " _____     _           _____ _ _   \n");
    printk(LOG_LEVEL_INFO, "|   __|_ _| |_ ___ ___|  |  |_| |_ \n");
    printk(LOG_LEVEL_INFO, "|__   | | |  _| -_|  _|    -| | _| \n");
    printk(LOG_LEVEL_INFO, "|_____|_  |_| |___|_| |__|__|_|_|  \n");
    printk(LOG_LEVEL_INFO, "      |___|                        \n");
    printk(LOG_LEVEL_INFO, "***********************************\n");
    printk(LOG_LEVEL_INFO, " %s V0.1.2 Commit: %s\n", PROJECT_NAME, PROJECT_GIT_HASH);
    printk(LOG_LEVEL_INFO, " Built by: %s\n", PROJECT_C_COMPILER);
    printk(LOG_LEVEL_INFO, "***********************************\n");
    printk(LOG_LEVEL_INFO, "\n");
}
