/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/
#include <lib/fatfs/ff.h> /* Obtains integer types */

#include <lib/fatfs/diskio.h>

#include <drivers/dma/dma.h>
#include <drivers/dram/dram.h>
#include <drivers/mmc/sdcard.h>
#include <string.h>

static sdmmc_pdata_t *disk_devices[FF_VOLUMES];

#ifdef CONFIG_FATFS_CACHE_SIZE
/* we can consume up to CONFIG_FATFS_CACHE_SIZE of SDRAM starting at CONFIG_FATFS_CACHE_ADDR */
#define FATFS_CACHE_CHUNK_SIZE (32 * 1024)
#define FATFS_CACHE_SECTORS (CONFIG_FATFS_CACHE_SIZE / FF_MIN_SS)
#define FATFS_CACHE_SECTORS_PER_BIT (FATFS_CACHE_CHUNK_SIZE / FF_MIN_SS)
#define FATFS_CACHE_CHUNKS (FATFS_CACHE_SECTORS / FATFS_CACHE_SECTORS_PER_BIT)

static uint8_t *const cache_data = (uint8_t *) CONFIG_FATFS_CACHE_ADDR; /* in CONFIG_FATFS_CACHE_ADDR */
static uint8_t cache_bitmap[FATFS_CACHE_CHUNKS / 8];					/* in SRAM */
static BYTE cache_pdrv = -1;
static int current_cache_sdhci_id = -1;

#define CACHE_SECTOR_TO_OFFSET(ss) (((ss) / FATFS_CACHE_SECTORS_PER_BIT) / 8)
#define CACHE_SECTOR_TO_BIT(ss) (((ss) / FATFS_CACHE_SECTORS_PER_BIT) % 8)

#define CACHE_IS_VALID(ss)                                                           \
	({                                                                               \
		__typeof(ss) _ss = (ss);                                                     \
		cache_bitmap[CACHE_SECTOR_TO_OFFSET(_ss)] & (1 << CACHE_SECTOR_TO_BIT(_ss)); \
	})
#define CACHE_SET_VALID(ss)                                                           \
	do {                                                                              \
		__typeof(ss) _ss = (ss);                                                      \
		cache_bitmap[CACHE_SECTOR_TO_OFFSET(_ss)] |= (1 << CACHE_SECTOR_TO_BIT(_ss)); \
	} while (0)
#endif

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv /* Physical drive nmuber to identify the drive */
) {
	if (pdrv >= FF_VOLUMES || disk_devices[pdrv] == NULL ||
	    !disk_devices[pdrv]->online)
		return STA_NOINIT;

	return 0;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS
disk_initialize(BYTE pdrv /* Physical drive nmuber to identify the drive */
) {
	return disk_status(pdrv);
}

DRESULT disk_set_device(BYTE pdrv, struct sdmmc_pdata *device) {
	if (pdrv >= FF_VOLUMES)
		return RES_PARERR;

	disk_devices[pdrv] = device;
#ifdef CONFIG_FATFS_CACHE_SIZE
	if (cache_pdrv == pdrv) {
		memset(cache_bitmap, 0, sizeof(cache_bitmap));
		cache_pdrv = (BYTE) -1;
		current_cache_sdhci_id = -1;
	}
#endif
	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv,	/* Physical drive nmuber to identify the drive */
				  BYTE *buff,	/* Data buffer to store read data */
				  LBA_t sector, /* Start sector in LBA */
				  UINT count	/* Number of sectors to read */
) {
	sdmmc_pdata_t *device;

	if (pdrv >= FF_VOLUMES || !count)
		return RES_PARERR;
	device = disk_devices[pdrv];
	if (device == NULL || !device->online)
		return RES_NOTRDY;

	printk_trace("FATFS: read %u sectors at %u\r\n", count, (uint32_t) sector);

#ifdef CONFIG_FATFS_CACHE_SIZE
	if (pdrv != cache_pdrv || current_cache_sdhci_id != device->hci->id) {
		printk_debug("FATFS: cache: %u bytes in %u chunks\r\n", CONFIG_FATFS_CACHE_SIZE, FATFS_CACHE_CHUNKS);
		if (cache_pdrv != -1)
			memset(cache_bitmap, 0, sizeof(cache_bitmap));
		cache_pdrv = pdrv;
	}

	current_cache_sdhci_id = device->hci->id;

	while (count) {
		if (sector >= FATFS_CACHE_SECTORS) {
			printk_trace("FATFS: beyond cache %u count %u\r\n", (uint32_t) sector, count);
			/* beyond end of cache, read remaining */
			if (sdmmc_blk_read(device, buff, sector, count) != count) {
				printk_warning("FATFS: read failed %u count %u\r\n", (uint32_t) sector, count);
				return RES_ERROR;
			}
			return RES_OK;
		}

		if (!CACHE_IS_VALID(sector)) {
			LBA_t chunk = sector & ~(FATFS_CACHE_SECTORS_PER_BIT - 1);
			printk_trace("FATFS: cache miss %u, loading %u count %u\r\n", (uint32_t) sector, chunk, FATFS_CACHE_SECTORS_PER_BIT);
			if (sdmmc_blk_read(device, &cache_data[chunk * FF_MIN_SS], chunk, FATFS_CACHE_SECTORS_PER_BIT) != FATFS_CACHE_SECTORS_PER_BIT) {
				printk_warning("FATFS: read failed %u count %u\r\n", (uint32_t) sector, FATFS_CACHE_SECTORS_PER_BIT);
				return RES_ERROR;
			}
			CACHE_SET_VALID(sector);
		} else {
			printk_trace("FATFS: cache hit %u\r\n", (uint32_t) sector);
		}
		memcpy(buff, &cache_data[sector * FF_MIN_SS], FF_MIN_SS);

		sector++;
		buff += FF_MIN_SS;
		count--;
	}
	return RES_OK;
#else
	return (sdmmc_blk_read(device, buff, sector, count) == count ? RES_OK : RES_ERROR);
#endif
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv,		 /* Physical drive nmuber to identify the drive */
				   const BYTE *buff, /* Data to be written */
				   LBA_t sector,	 /* Start sector in LBA */
				   UINT count		 /* Number of sectors to write */
) {
	sdmmc_pdata_t *device;

	if (pdrv >= FF_VOLUMES || !count)
		return RES_PARERR;
	device = disk_devices[pdrv];
	if (device == NULL || !device->online)
		return RES_NOTRDY;

	printk_trace("FATFS: write %u sectors at %llu\r\n", count, sector);

	return (sdmmc_blk_write(device, (uint8_t *) buff, sector, count) == count ? RES_OK : RES_ERROR);
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, /* Physical drive nmuber (0..) */
				   BYTE cmd,  /* Control code */
				   void *buff /* Buffer to send/receive control data */
) {
	return RES_PARERR;
}
