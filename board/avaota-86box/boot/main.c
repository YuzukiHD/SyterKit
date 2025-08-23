/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>
#include <timer.h>

#include <common.h>
#include <jmp.h>
#include <mmu.h>
#include <smalloc.h>
#include <sstdlib.h>
#include <string.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

#include <image_loader.h>

#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-sdcard.h>
#include <sys-sid.h>
#include <sys-spi.h>
#include <sys-uart.h>

#include "fdt_wrapper.h"
#include "ff.h"
#include "libfdt.h"
#include "uart.h"

#define CONFIG_KERNEL_FILENAME "zImage"
#define CONFIG_DTB_FILENAME "sunxi.dtb"
#define CONFIG_CMDLINE                                  \
	"earlyprintk=uart8250,mmio32,0x02500000 "           \
	"console=ttyS0,115200 loglevel=8 initcall_debug=0 " \
	"root=/dev/mmcblk1p2 init=/init rdinit=/rdinit"     \
	"partitions=boot@mmcblk0p1:rootfs@mmcblk0p2:rootfs_data@mmcblk0p3:UDISK@mmcblk0p4"

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

#define CONFIG_DTB_LOAD_ADDR (0x41008000)
#define CONFIG_KERNEL_LOAD_ADDR (0x41800000)
#define CONFIG_CONFIG_LOAD_ADDR (0x40008000)
#define CONFIG_HEAP_BASE (0x40800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

#define CONFIG_DEFAULT_BOOTDELAY 0

#define FILENAME_MAX_LEN 16
typedef struct {
	uint8_t *dest;
	uint8_t *of_dest;
	char filename[FILENAME_MAX_LEN];
	char of_filename[FILENAME_MAX_LEN];
} image_info_t;

extern sunxi_serial_t uart_dbg;

extern sunxi_spi_t sunxi_spi0;

#if defined(CONFIG_CHIP_MMC_V2)
extern sunxi_sdhci_t sdhci0;
extern sunxi_sdhci_t sdhci2;
#else
extern sdhci_t sdhci0;
extern sdhci_t sdhci2;
#endif

extern dram_para_t dram_para;

image_info_t image;

#define CHUNK_SIZE 0x20000

static int fatfs_loadimage(char *filename, BYTE *dest) {
	FIL file;
	UINT byte_to_read = CHUNK_SIZE;
	UINT byte_read;
	UINT total_read = 0;
	FRESULT fret;
	int ret = 1;
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

	printk_info("FATFS: read in %ums at %.2fMB/S\n", time, (f32) (total_read / time) / 1024.0f);

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

	/* load DTB */
	printk_info("FATFS: read %s addr=%x\n", image->of_filename, (uint32_t) image->of_dest);
	ret = fatfs_loadimage(image->of_filename, image->of_dest);
	if (ret)
		return ret;

	/* load Kernel */
	printk_info("FATFS: read %s addr=%x\n", image->filename, (uint32_t) image->dest);
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
	printk_info("FATFS: done in %ums\n", time_ms() - start);

	return 0;
}

static int fdt_pack_reg(const void *fdt, void *buf, uint64_t address, uint64_t size) {
	int i;
	int address_cells = fdt_address_cells(fdt, 0);
	int size_cells = fdt_size_cells(fdt, 0);
	char *p = buf;

	if (address_cells == 2)
		*(fdt64_t *) p = cpu_to_fdt64(address);
	else
		*(fdt32_t *) p = cpu_to_fdt32(address);
	p += 4 * address_cells;

	if (size_cells == 2)
		*(fdt64_t *) p = cpu_to_fdt64(size);
	else
		*(fdt32_t *) p = cpu_to_fdt32(size);
	p += 4 * size_cells;

	return p - (char *) buf;
}

static char *skip_spaces(char *str) {
	while (*str == ' ') str++;
	return str;
}

static int update_dtb_for_linux(uint32_t dram_size) {
	int ret = 0;

	/* Force image.of_dest to be a pointer to fdt_header structure */
	struct fdt_header *dtb_header = (struct fdt_header *) image.of_dest;

	/* Check if DTB header is valid */
	if ((ret = fdt_check_header(dtb_header)) != 0) {
		printk_error("Invalid device tree blob: %s\n", fdt_strerror(ret));
		goto _error;
	}

	/* Get the total size of DTB */
	uint32_t size = fdt_totalsize(image.of_dest);

	printk_debug("FDT dtb size = %d\n", size);

	if ((ret = fdt_increase_size(image.of_dest, 512)) != 0) {
		printk_error("FDT: device tree increase error: %s\n", fdt_strerror(ret));
		goto _error;
	}

	int memory_node = fdt_find_or_add_subnode(image.of_dest, 0, "memory");

	if ((ret = fdt_setprop_string(image.of_dest, memory_node, "device_type", "memory")) != 0) {
		printk_error("Can't change memory size node: %s\n", fdt_strerror(ret));
		goto _error;
	}

	uint8_t *tmp_buf = (uint8_t *) smalloc(16 * sizeof(uint8_t));

	/* fix up memory region */
	int len = fdt_pack_reg(image.of_dest, tmp_buf, SDRAM_BASE, (dram_size * 1024 * 1024));

	if ((ret = fdt_setprop(image.of_dest, memory_node, "reg", tmp_buf, len)) != 0) {
		printk_error("Can't change memory base node: %s\n", fdt_strerror(ret));
		sfree(tmp_buf);
		goto _error;
	}

	/* Get the offset of "/chosen" node */
	int chosen_node = fdt_find_or_add_subnode(image.of_dest, 0, "chosen");

	len = 0;
	/* Get bootargs string */
	char *bootargs_str = (void *) fdt_getprop(image.of_dest, chosen_node, "bootargs", &len);
	if (bootargs_str == NULL) {
		bootargs_str = (char *) smalloc(strlen(CONFIG_CMDLINE) + 1);
		bootargs_str[0] = '\0';
	} else {
		strcat(bootargs_str, " ");
	}

	strcat(bootargs_str, CONFIG_CMDLINE);

	printk_info("Kernel cmdline = [%s]\n", bootargs_str);

_add_dts_size:
	/* Modify bootargs string */
	ret = fdt_setprop_string(image.of_dest, chosen_node, "bootargs", skip_spaces(bootargs_str));
	if (ret == -FDT_ERR_NOSPACE) {
		printk_debug("FDT: FDT_ERR_NOSPACE, Size = %d, Increase Size = %d\n", size, 512);
		ret = fdt_increase_size(image.of_dest, 512);
		if (!ret) {
			goto _add_dts_size;
		} else {
			printk_error("DTB: Can't increase blob size: %s\n", fdt_strerror(ret));
			goto _error;
		}
	} else if (ret < 0) {
		printk_error("Can't change bootargs node: %s\n", fdt_strerror(ret));
		goto _error;
	}

	/* Get the total size of DTB */
	printk_debug("Modify FDT Size = %d\n", fdt_totalsize(image.of_dest));

	if (ret < 0) {
		printk_error("libfdt fdt_setprop() error: %s\n", fdt_strerror(ret));
		goto _error;
	}

	return 0;

_error:
	return -1;
}

static int abortboot_single_key(int bootdelay) {
	int abort = 0;
	unsigned long ts;

	printk_info("Hit any key to stop autoboot: %2d ", bootdelay);

	/* Check if key already pressed */
	if (tstc()) {		/* we got a key press */
		uart_getchar(); /* consume input */
		printk(LOG_LEVEL_MUTE, "\b\b\b%2d", bootdelay);
		abort = 0; /* auto boot */
	}

	while ((bootdelay > 0) && (!abort)) {
		--bootdelay;
		/* delay 1000 ms */
		ts = time_ms();
		do {
			if (tstc()) {  /* we got a key press */
				abort = 1; /* don't auto boot */
				break;
			}
			udelay(10000);
		} while (!abort && time_ms() - ts < 1000);
		printk(LOG_LEVEL_MUTE, "\b\b\b%2d ", bootdelay);
	}
	uart_putchar('\n');
	return abort;
}

msh_declare_command(boot);
msh_define_help(boot, "boot to linux", "Usage: boot\n");
int cmd_boot(int argc, const char **argv) {
	/* Initialize variables for kernel entry point and SD card access. */
	uint32_t entry_point = 0;
	void (*kernel_entry)(int zero, int arch, uint32_t params);

	/* Set up boot parameters for the kernel. */
	if (zImage_loader((uint8_t *) image.dest, &entry_point)) {
		printk_error("boot setup failed\n");
		abort();
	}

	/* Disable MMU, data cache, instruction cache, interrupts */
	clean_syterkit_data();

	enable_kernel_smp();
	printk_info("enable kernel smp ok...\n");

	/* Debug message to indicate the kernel address that the system is jumping to. */
	printk_info("jump to kernel address: 0x%x\n\n", image.dest);

	/* Jump to the kernel entry point. */
	kernel_entry = (void (*)(int, int, uint32_t)) entry_point;
	kernel_entry(0, ~0, (uint32_t) image.of_dest);

	// if kernel boot not success, jump to fel.
	jmp_to_fel();
	return 0;
}

const msh_command_entry commands[] = {
		msh_define_command(boot),
		msh_command_end,
};

/* 
 * main function for the bootloader. Initializes and sets up the system, loads the kernel and device tree binary from
 * an SD card, sets boot arguments, and boots the kernel. If the kernel fails to boot, the function jumps to FEL mode.
 */
int main(void) {
	/* Initialize the debug serial interface. */
	sunxi_serial_init(&uart_dbg);

	/* Display the bootloader banner. */
	show_banner();

	/* Initialize the system clock. */
	sunxi_clk_init();

	/* Initialize the DRAM and enable memory management unit (MMU). */
	uint32_t dram_size = sunxi_dram_init(&dram_para);
	arm32_mmu_enable(SDRAM_BASE, dram_size);

	/* Initialize the small memory allocator. */
	smalloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

	/* Dump information about the system clocks. */
	sunxi_clk_dump();

	/* Clear the image_info_t struct. */
	memset(&image, 0, sizeof(image_info_t));

	/* Set the destination address for the device tree binary (DTB), kernel image, and configuration data. */
	image.of_dest = (uint8_t *) CONFIG_DTB_LOAD_ADDR;
	image.dest = (uint8_t *) CONFIG_KERNEL_LOAD_ADDR;

	/* Copy the filenames for the DTB, kernel image, and configuration data. */
	strcpy(image.filename, CONFIG_KERNEL_FILENAME);
	strcpy(image.of_filename, CONFIG_DTB_FILENAME);

	/* Initialize the SD host controller. */
	if (sunxi_sdhci_init(&sdhci0) != 0) {
		printk_error("SMHC: %s controller init failed\n", sdhci0.name);
		goto _shell;
	} else {
		printk_info("SMHC: %s controller v%x initialized\n", sdhci0.name);
	}

	/* Initialize the SD card and check if initialization is successful. */
	if (sdmmc_init(&card0, &sdhci0) != 0) {
		printk_warning("SMHC: init failed, retry...\n");
		if (sdmmc_init(&card0, &sdhci0) != 0) {
			goto _shell;
		}
	}

	/* Load the DTB, kernel image, and configuration data from the SD card. */
	if (load_sdcard(&image) != 0) {
		printk_warning("SMHC: loading failed\n");
		goto _shell;
	}

	/* Update boot arguments based on configuration file. */
	if (update_dtb_for_linux(dram_size)) {
		goto _shell;
	}

	int bootdelay = CONFIG_DEFAULT_BOOTDELAY;

	/* Showing boot delays */
	if (abortboot_single_key(bootdelay)) {
		goto _shell;
	}

	cmd_boot(0, NULL);

_shell:
	syterkit_shell_attach(commands);

_fel:
	/* If the kernel boot fails, jump to FEL mode. */
	jmp_to_fel();

	/* Return 0 to indicate successful execution. */
	return 0;
}
