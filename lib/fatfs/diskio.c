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
/* we can consume up to CONFIG_FATFS_CACHE_SIZE of SDRAM starting at SDRAM_BASE */
#define FATFS_CACHE_CHUNK_SIZE (32 * 1024)
#define FATFS_CACHE_SECTORS (CONFIG_FATFS_CACHE_SIZE / FF_MIN_SS)
#define FATFS_CACHE_SECTORS_PER_BIT (FATFS_CACHE_CHUNK_SIZE / FF_MIN_SS)
#define FATFS_CACHE_CHUNKS (FATFS_CACHE_SECTORS / FATFS_CACHE_SECTORS_PER_BIT)

static uint8_t *const cache_data = (uint8_t *) SDRAM_BASE; /* in SDRAM */
static uint8_t cache_bitmap[FATFS_CACHE_CHUNKS / 8];  /* in SRAM */
static BYTE cache_pdrv = -1;

#define CACHE_SECTOR_TO_OFFSET(ss) (((ss) / FATFS_CACHE_SECTORS_PER_BIT) / 8)
#define CACHE_SECTOR_TO_BIT(ss) (((ss) / FATFS_CACHE_SECTORS_PER_BIT) % 8)

#define CACHE_IS_VALID(ss) ({ __typeof(ss) _ss = (ss); cache_bitmap[CACHE_SECTOR_TO_OFFSET(_ss)] & (1 << CACHE_SECTOR_TO_BIT(_ss)); })
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
)
{
	if (pdrv)
		return STA_NOINIT;

    return Stat;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS
disk_initialize(BYTE pdrv /* Physical drive nmuber to identify the drive */
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

DRESULT disk_read(BYTE pdrv,    /* Physical drive nmuber to identify the drive */
                  BYTE *buff,   /* Data buffer to store read data */
                  LBA_t sector, /* Start sector in LBA */
                  UINT count    /* Number of sectors to read */
) {
    if (pdrv || !count)
        return RES_PARERR;
    if (Stat & STA_NOINIT)
        return RES_NOTRDY;

    printk(LOG_LEVEL_TRACE, "FATFS: read %u sectors at %llu\r\n", count, sector);

#ifdef CONFIG_FATFS_CACHE_SIZE
    if (pdrv != cache_pdrv) {
        printk(LOG_LEVEL_DEBUG, "FATFS: cache: %u bytes in %u chunks\r\n", CONFIG_FATFS_CACHE_SIZE, FATFS_CACHE_CHUNKS);
        if (cache_pdrv != -1)
            memset(cache_bitmap, 0, sizeof(cache_bitmap));
        cache_pdrv = pdrv;
    }

    while (count) {
        if (sector >= FATFS_CACHE_SECTORS) {
            printk(LOG_LEVEL_TRACE, "FATFS: beyond cache %llu count %u\r\n", sector, count);
            /* beyond end of cache, read remaining */
            if (sdmmc_blk_read(&card0, buff, sector, count) != count) {
                printk(LOG_LEVEL_WARNING, "FATFS: read failed %llu count %u\r\n", sector, count);
                return RES_ERROR;
            }
            return RES_OK;
        }

        if (!CACHE_IS_VALID(sector)) {
            LBA_t chunk = sector & ~(FATFS_CACHE_SECTORS_PER_BIT - 1);
            printk(LOG_LEVEL_TRACE, "FATFS: cache miss %llu, loading %llu count %u\r\n", sector, chunk, FATFS_CACHE_SECTORS_PER_BIT);
            if (sdmmc_blk_read(&card0, &cache_data[chunk * FF_MIN_SS], chunk, FATFS_CACHE_SECTORS_PER_BIT) != FATFS_CACHE_SECTORS_PER_BIT) {
                printk(LOG_LEVEL_WARNING, "FATFS: read failed %llu count %u\r\n", sector, FATFS_CACHE_SECTORS_PER_BIT);
                return RES_ERROR;
            }
            CACHE_SET_VALID(sector);
        } else {
            printk(LOG_LEVEL_TRACE, "FATFS: cache hit %llu\r\n", sector);
        }
        memcpy(buff, &cache_data[sector * FF_MIN_SS], FF_MIN_SS);

        sector++;
        buff += FF_MIN_SS;
        count--;
    }
    return RES_OK;
#else
    return (sdmmc_blk_read(&card0, buff, sector, count) == count ? RES_OK : RES_ERROR);
#endif
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv,        /* Physical drive nmuber to identify the drive */
                   const BYTE *buff, /* Data to be written */
                   LBA_t sector,     /* Start sector in LBA */
                   UINT count        /* Number of sectors to write */
) {
    return RES_ERROR;
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