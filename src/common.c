/* SPDX-License-Identifier: Apache-2.0 */

#include <config.h>
#include <log.h>

void show_banner(void) {
    uint32_t id[4];

    printk(LOG_LEVEL_MUTE, "\n");
    printk(LOG_LEVEL_INFO, " _____     _           _____ _ _   \n");
    printk(LOG_LEVEL_INFO, "|   __|_ _| |_ ___ ___|  |  |_| |_ \n");
    printk(LOG_LEVEL_INFO, "|__   | | |  _| -_|  _|    -| | _| \n");
    printk(LOG_LEVEL_INFO, "|_____|_  |_| |___|_| |__|__|_|_|  \n");
    printk(LOG_LEVEL_INFO, "      |___|                        \n");
    printk(LOG_LEVEL_INFO, "***********************************\n");
    printk(LOG_LEVEL_INFO, " %s V0.1.1 Commit: %s\n", PROJECT_NAME, PROJECT_GIT_HASH);
    printk(LOG_LEVEL_INFO, " Built by: %s\n", PROJECT_C_COMPILER);
    printk(LOG_LEVEL_INFO, "***********************************\n");
    printk(LOG_LEVEL_INFO, "\n");

    id[0] = read32(0x03006200 + 0x0);
    id[1] = read32(0x03006200 + 0x4);
    id[2] = read32(0x03006200 + 0x8);
    id[3] = read32(0x03006200 + 0xc);

    printk(LOG_LEVEL_INFO, "Chip ID is: %08x%08x%08x%08x\n", id[0], id[1], id[2], id[3]);
}