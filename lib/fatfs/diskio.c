/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/
#include "ff.h" /* Obtains integer types */
#include "diskio.h"

#include <sys-dma.h>
#include <sys-dram.h>
#include <sys-sdcard.h>

static DSTATUS Stat = STA_NOINIT; /* Disk status */
#ifdef CONFIG_FATFS_CACHE_SIZE
static u8 *const cache		= (u8 *)SDRAM_BASE;
static const u32 cache_size = (CONFIG_FATFS_CACHE_SIZE);
static u32		 cache_first, cache_last;
#endif

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
	if (pdrv)
		return STA_NOINIT;

#ifdef CONFIG_FATFS_CACHE_SIZE
	cache_first = 0xFFFFFFFF - cache_size; // Set to a big sector for a proper init
	cache_last	= 0xFFFFFFFF;
#endif

	return Stat;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
	if (pdrv)
		return STA_NOINIT;

	Stat &= ~STA_NOINIT;

	return Stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE	pdrv, /* Physical drive nmuber to identify the drive */
				  BYTE *buff, /* Data buffer to store read data */
				  LBA_t sector, /* Start sector in LBA */
				  UINT	count /* Number of sectors to read */
)
{
	u32 blkread, read_pos, first, last, chunk, bytes;

	if (pdrv || !count)
		return RES_PARERR;
	if (Stat & STA_NOINIT)
		return RES_NOTRDY;

	first = sector;
	last  = sector + count;
	bytes = count * FF_MIN_SS;

	printk(LOG_LEVEL_TRACE, "FATFS: read %u sectors at %u\r\n", count, first);

#ifdef CONFIG_FATFS_CACHE_SIZE
	// Read starts in cache but overflows
	if (first >= cache_first && first < cache_last && last > cache_last) {
		chunk = (cache_last - first) * FF_MIN_SS;
		memcpy(buff, cache + (first - cache_first) * FF_MIN_SS, chunk);
		buff += chunk;
		first += (cache_last - first);
		printk(LOG_LEVEL_TRACE, "FATFS: chunk %u first %u\r\n", chunk, first);
	}

	// Read is NOT in the cache
	if (last > cache_last || first < cache_first) {
		printk(LOG_LEVEL_TRACE, "FATFS: if %u > %u || %u < %u\r\n", last, cache_last, first, cache_first);

		read_pos	= (first / cache_size) * cache_size; // TODO: check with card max capacity
		cache_first = read_pos;
		cache_last	= read_pos + cache_size;
		blkread		= sdmmc_blk_read(&card0, cache, read_pos, cache_size);

		if (blkread != cache_size) {
			printk(LOG_LEVEL_WARNING, "FATFS: MMC read %u/%u blocks\r\n", blkread, cache_size);
			return RES_ERROR;
		}
		printk(LOG_LEVEL_TRACE, "FATFS: cached %u sectors (%uKB) at %u/[%u-%u]\r\n", blkread, (blkread * FF_MIN_SS) / 1024, first,
			  read_pos, read_pos + cache_size);
	}

	// Copy from read cache to output buffer
	printk(LOG_LEVEL_TRACE, "FATFS: copy %u from 0x%x to 0x%x\r\n", bytes, (cache + ((first - cache_first) * FF_MIN_SS)), buff);
	memcpy(buff, (cache + ((first - cache_first) * FF_MIN_SS)), bytes);

	return RES_OK;
#else
	return (sdmmc_blk_read(&card0, buff, sector, count) == count ? RES_OK : RES_ERROR);
#endif
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE		   pdrv, /* Physical drive nmuber to identify the drive */
				   const BYTE *buff, /* Data to be written */
				   LBA_t	   sector, /* Start sector in LBA */
				   UINT		   count /* Number of sectors to write */
)
{
	return RES_ERROR;
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE	 pdrv, /* Physical drive nmuber (0..) */
				   BYTE	 cmd, /* Control code */
				   void *buff /* Buffer to send/receive control data */
)
{
	return RES_PARERR;
}
