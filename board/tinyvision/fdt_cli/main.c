/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <config.h>
#include <log.h>

#include <mmu.h>
#include <common.h>
#include <jmp.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

#include "sys-dram.h"
#include "sys-sdcard.h"
#include "sys-sid.h"

#include "fdt_wrapper.h"
#include "ff.h"
#include "libfdt.h"

#define CONFIG_DTB_FILENAME "sunxi.dtb"
#define CONFIG_DTB_LOADADDR (0x41008000)

#define MAX_LEVEL 32	/* how deeply nested we will go */
#define SCRATCHPAD 1024 /* bytes of scratchpad memory */
#define CMD_FDT_MAX_DUMP 64

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

extern sunxi_serial_t uart_dbg;

extern dram_para_t dram_para;

extern sdhci_t sdhci0;

#define FILENAME_MAX_LEN 64
typedef struct {
	unsigned int offset;
	unsigned int length;
	unsigned char *dest;

	char filename[FILENAME_MAX_LEN];
} image_info_t;

image_info_t image;

#define CHUNK_SIZE 0x20000

static int fatfs_loadimage(char *filename, BYTE *dest) {
	FIL file;
	UINT byte_to_read = CHUNK_SIZE;
	UINT byte_read;
	UINT total_read = 0;
	FRESULT fret;
	int ret;
	uint32_t start, time;

	fret = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
	if (fret != FR_OK) {
		printk_error("FATFS: open, filename: [%s]: error %d\n", filename, fret);
		ret = -1;
		goto open_fail;
	}

	start = time_ms();

	do {
		byte_read = 0;
		fret = f_read(&file, (void *) (dest), byte_to_read, &byte_read);
		dest += byte_to_read;
		total_read += byte_read;
	} while (byte_read >= byte_to_read && fret == FR_OK);

	time = time_ms() - start + 1;

	if (fret != FR_OK) {
		printk_error("FATFS: read: error %d\n", fret);
		ret = -1;
		goto read_fail;
	}
	ret = 0;

read_fail:
	fret = f_close(&file);

	printk_debug("FATFS: read in %ums at %.2fMB/S\n", time, (f32) (total_read / time) / 1024.0f);

open_fail:
	return ret;
}

static int load_sdcard(image_info_t *image) {
	FATFS fs;
	FRESULT fret;
	int ret;
	uint32_t start;

	uint32_t test_time;
	start = time_ms();
	sdmmc_blk_read(&card0, (uint8_t *) (SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

	start = time_ms();

	fret = f_mount(&fs, "", 1);
	if (fret != FR_OK) {
		printk_error("FATFS: mount error: %d\n", fret);
		return -1;
	} else {
		printk_debug("FATFS: mount OK\n");
	}

	printk_info("FATFS: read %s addr=%x\n", image->filename, (unsigned int) image->dest);
	ret = fatfs_loadimage(image->filename, image->dest);
	if (ret)
		return ret;

	/* umount fs */
	fret = f_mount(0, "", 0);
	if (fret != FR_OK) {
		printk_error("FATFS: unmount error %d\n", fret);
		return -1;
	} else {
		printk_debug("FATFS: unmount OK\n");
	}
	printk_debug("FATFS: done in %ums\n", time_ms() - start);

	return 0;
}


msh_declare_command(fdt);
msh_define_help(fdt, "flattened device tree utility commands",
				"fdt print  <path> [<prop>]          - Recursive print starting at <path>\n"
				"fdt list   <path> [<prop>]          - Print one level starting at <path>\n"
				"fdt set    <path> <prop> [<val>]    - Set <property> [to <val>]\n"
				"fdt mknode <path> <node>            - Create a new node after <path>\n"
				"fdt rm     <path> [<prop>]          - Delete the node or <property>\n"
				"fdt header                          - Display header info\n"
				"fdt rsvmem print                    - Show current mem reserves\n"
				"fdt rsvmem add <addr> <size>        - Add a mem reserve\n"
				"fdt rsvmem delete <index>           - Delete a mem reserves\n"
				"NOTE: Dereference aliases by omitting the leading '/', "
				"e.g. fdt print ethernet0.\n\n");
int cmd_fdt(int argc, const char **argv) {
	if (argc < 2) {
		uart_puts(cmd_fdt_usage);
		return 0;
	}
	if (strncmp(argv[1], "mk", 2) == 0) {
		char *pathp;	/* path */
		char *nodep;	/* new node to add */
		int nodeoffset; /* node offset from libfdt */
		int err;

		/*
		 * Parameters: Node path, new node to be appended to the path.
		 */
		if (argc < 4) {
			uart_puts(cmd_fdt_usage);
			return 0;
		}

		pathp = argv[2];
		nodep = argv[3];

		nodeoffset = fdt_path_offset(image.dest, pathp);
		if (nodeoffset < 0) {
			/*
			 * Not found or something else bad happened.
			 */
			printk(LOG_LEVEL_MUTE, "libfdt fdt_path_offset() returned %s\n", fdt_strerror(nodeoffset));
			return 1;
		}
		err = fdt_add_subnode(image.dest, nodeoffset, nodep);
		if (err < 0) {
			printk(LOG_LEVEL_MUTE, "libfdt fdt_add_subnode(): %s\n", fdt_strerror(err));
			return 1;
		}
	} else if (strncmp(argv[1], "set", 3) == 0) {
		char *pathp;							   /* path */
		char *prop;								   /* property */
		int nodeoffset;							   /* node offset from libfdt */
		static char data[SCRATCHPAD] __aligned(4); /* property storage */
		const void *ptmp;
		int len; /* new length of the property */
		int ret; /* return value */

		/*
		 * Parameters: Node path, property, optional value.
		 */
		if (argc < 4) {
			uart_puts(cmd_fdt_usage);
			return 0;
		}

		pathp = argv[2];
		prop = argv[3];

		nodeoffset = fdt_path_offset(image.dest, pathp);
		if (nodeoffset < 0) {
			/*
			 * Not found or something else bad happened.
			 */
			printk(LOG_LEVEL_MUTE, "libfdt fdt_path_offset() returned %s\n", fdt_strerror(nodeoffset));
			return 1;
		}

		if (argc == 4) {
			len = 0;
		} else {
			ptmp = fdt_getprop(image.dest, nodeoffset, prop, &len);
			if (len > SCRATCHPAD) {
				printk(LOG_LEVEL_MUTE, "prop (%d) doesn't fit in scratchpad!\n", len);
				return 1;
			}
			if (ptmp != NULL)
				memcpy(data, ptmp, len);

			ret = fdt_parse_prop(&argv[4], argc - 4, data, &len);
			if (ret != 0)
				return ret;
		}

		ret = fdt_setprop(image.dest, nodeoffset, prop, data, len);
		if (ret < 0) {
			printk(LOG_LEVEL_MUTE, "libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
			return 1;
		}
	} else if ((argv[1][0] == 'p') || (argv[1][0] == 'l')) {
		int depth = MAX_LEVEL; /* how deep to print */
		char *pathp;		   /* path */
		char *prop;			   /* property */
		int ret;			   /* return value */
		static char root[2] = "/";

		/*
		 * list is an alias for print, but limited to 1 level
		 */
		if (argv[1][0] == 'l') {
			depth = 1;
		}

		/*
		 * Get the starting path.  The root node is an oddball,
		 * the offset is zero and has no name.
		 */
		if (argc == 2)
			pathp = root;
		else
			pathp = argv[2];
		if (argc > 3)
			prop = argv[3];
		else
			prop = NULL;

		fdt_print(image.dest, pathp, prop, depth);
	} else if (strncmp(argv[1], "rm", 2) == 0) {
		int nodeoffset; /* node offset from libfdt */
		int err;

		/*
		 * Get the path.  The root node is an oddball, the offset
		 * is zero and has no name.
		 */
		nodeoffset = fdt_path_offset(image.dest, argv[2]);
		if (nodeoffset < 0) {
			/*
			 * Not found or something else bad happened.
			 */
			printk(LOG_LEVEL_MUTE, "libfdt fdt_path_offset() returned %s\n", fdt_strerror(nodeoffset));
			return 1;
		}
		/*
		 * Do the delete.  A fourth parameter means delete a property,
		 * otherwise delete the node.
		 */
		if (argc > 3) {
			err = fdt_delprop(image.dest, nodeoffset, argv[3]);
			if (err < 0) {
				printk(LOG_LEVEL_MUTE, "libfdt fdt_delprop():  %s\n", fdt_strerror(err));
				return 0;
			}
		} else {
			err = fdt_del_node(image.dest, nodeoffset);
			if (err < 0) {
				printk(LOG_LEVEL_MUTE, "libfdt fdt_del_node():  %s\n", fdt_strerror(err));
				return 0;
			}
		}
	} else if (argv[1][0] == 'h') {
		u32 version = fdt_version(image.dest);
		printk(LOG_LEVEL_MUTE, "magic:\t\t\t0x%x\n", fdt_magic(image.dest));
		printk(LOG_LEVEL_MUTE, "totalsize:\t\t0x%x (%d)\n", fdt_totalsize(image.dest), fdt_totalsize(image.dest));
		printk(LOG_LEVEL_MUTE, "off_dt_struct:\t\t0x%x\n", fdt_off_dt_struct(image.dest));
		printk(LOG_LEVEL_MUTE, "off_dt_strings:\t\t0x%x\n", fdt_off_dt_strings(image.dest));
		printk(LOG_LEVEL_MUTE, "off_mem_rsvmap:\t\t0x%x\n", fdt_off_mem_rsvmap(image.dest));
		printk(LOG_LEVEL_MUTE, "version:\t\t%d\n", version);
		printk(LOG_LEVEL_MUTE, "last_comp_version:\t%d\n", fdt_last_comp_version(image.dest));
		if (version >= 2)
			printk(LOG_LEVEL_MUTE, "boot_cpuid_phys:\t0x%x\n", fdt_boot_cpuid_phys(image.dest));
		if (version >= 3)
			printk(LOG_LEVEL_MUTE, "size_dt_strings:\t0x%x\n", fdt_size_dt_strings(image.dest));
		if (version >= 17)
			printk(LOG_LEVEL_MUTE, "size_dt_struct:\t\t0x%x\n", fdt_size_dt_struct(image.dest));
		printk(LOG_LEVEL_MUTE, "number mem_rsv:\t\t0x%x\n", fdt_num_mem_rsv(image.dest));
		printk(LOG_LEVEL_MUTE, "\n");
	} else if (strncmp(argv[1], "rs", 2) == 0) {
		if (argv[2][0] == 'p') {
			uint64_t addr, size;
			int total = fdt_num_mem_rsv(image.dest);
			int j, err;
			printk(LOG_LEVEL_MUTE, "index\t\t   start\t\t    size\n");
			printk(LOG_LEVEL_MUTE, "-------------------------------"
								   "-----------------\n");
			for (j = 0; j < total; j++) {
				err = fdt_get_mem_rsv(image.dest, j, &addr, &size);
				if (err < 0) {
					printk(LOG_LEVEL_MUTE, "libfdt fdt_get_mem_rsv():  %s\n", fdt_strerror(err));
					return 0;
				}
				printk(LOG_LEVEL_MUTE, "    %x\t%08x%08x\t%08x%08x\n", j, (u32) (addr >> 32), (u32) (addr & 0xffffffff), (u32) (size >> 32), (u32) (size & 0xffffffff));
			}
		} else if (argv[2][0] == 'a') {
			uint64_t addr, size;
			int err;
			addr = simple_strtoull(argv[3], NULL, 16);
			size = simple_strtoull(argv[4], NULL, 16);
			err = fdt_add_mem_rsv(image.dest, addr, size);

			if (err < 0) {
				printk(LOG_LEVEL_MUTE, "libfdt fdt_add_mem_rsv():  %s\n", fdt_strerror(err));
				return 0;
			}
		} else if (argv[2][0] == 'd') {
			unsigned long idx = simple_strtoul(argv[3], NULL, 16);
			int err = fdt_del_mem_rsv(image.dest, idx);

			if (err < 0) {
				printk(LOG_LEVEL_MUTE, "libfdt fdt_del_mem_rsv():  %s\n", fdt_strerror(err));
				return 0;
			}
		} else {
			uart_puts(cmd_fdt_usage);
			return 0;
		}
	} else {
		uart_puts(cmd_fdt_usage);
		return 0;
	}
	return 0;
}

msh_declare_command(reload);
msh_define_help(reload, "rescan TF Card and reload DTB", "Usage: reload\n");
int cmd_reload(int argc, const char **argv) {
	if (sdmmc_init(&card0, &sdhci0) != 0) {
		printk_error("SMHC: init failed\n");
		return 0;
	}

	if (load_sdcard(&image) != 0) {
		printk_error("SMHC: loading failed\n");
		return 0;
	}
	return 0;
}

const msh_command_entry commands[] = {
		msh_define_command(fdt),
		msh_define_command(reload),
		msh_command_end,
};

int main(void) {
	/* Initialize UART debug interface */
	sunxi_serial_init(&uart_dbg);

	/* Print boot screen */
	show_banner();

	/* Initialize clock */
	sunxi_clk_init();

	/* Initialize DRAM */
	sunxi_dram_init(&dram_para);

	/* Print clock information */
	sunxi_clk_dump();

	/* Clear image structure */
	memset(&image, 0, sizeof(image_info_t));

	/* Set the target address of image to DTB load address */
	image.dest = (uint8_t *) CONFIG_DTB_LOADADDR;

	/* Copy the DTB filename to the image structure */
	strcpy(image.filename, CONFIG_DTB_FILENAME);

	/* Initialize SD card controller */
	if (sunxi_sdhci_init(&sdhci0) != 0) {
		printk_error("SMHC: %s controller init failed\n", sdhci0.name);
		goto _shell;
	} else {
		printk_info("SMHC: %s controller v%x initialized\n", sdhci0.name, sdhci0.reg->vers);
	}

	/* Initialize SD card */
	if (sdmmc_init(&card0, &sdhci0) != 0) {
		printk_error("SMHC: init failed\n");
		goto _shell;
	}

	/* Load DTB file from SD card */
	if (load_sdcard(&image) != 0) {
		printk_error("SMHC: loading failed\n");
		goto _shell;
	}

	/* Force image.dest to be a pointer to fdt_header structure */
	struct fdt_header *dtb_header = (struct fdt_header *) image.dest;

	int err = 0;

	/* Check if DTB header is valid */
	if ((err = fdt_check_header(dtb_header)) != 0) {
		printk_error("Invalid device tree blob: %s\n", fdt_strerror(err));
		goto _shell;
	}

	/* Get the total size of DTB */
	uint32_t size = fdt_totalsize(image.dest);
	printk_info("DTB FDT Size = 0x%x\n", size);

_shell:
	syterkit_shell_attach(commands);

	return 0;
}