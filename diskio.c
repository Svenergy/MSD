/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include "sd_spi.h"
#include "board.h"
#include "delay.h"
#include "ffconf.h"

/* Define this to enable SD block write timing debug prints */
//#define SD_WRITE_DEBUG

/* Definitions of physical drive number for each drive */
//#define SD		0	/* Example: Map SD card to drive number 0 */

SD_CardInfo cardinfo;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive number to identify the drive */
)
{
	if (sd_state == SD_READY) {
		return RES_OK;
	} else {
		return RES_ERROR;
	}
}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive number to identify the drive */
)
{
	/* Currently initializes SD in main */
	return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive number to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
	if (count == 1) {
		if (sd_read_block(sector,buff) == SD_OK) {
			return RES_OK;
		} else {
			return RES_ERROR;
		}
	} else if (count > 1) {
		if (sd_read_multiple_blocks(sector,count,buff) == SD_OK) {
			return RES_OK;
		} else {
			return RES_ERROR;
		}
	} else {
		return RES_PARERR;
	}
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
#ifdef SD_WRITE_DEBUG
uint32_t writeTimeMax; // Used for profiling
#endif

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive number to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count			/* Number of sectors to write */
)
{
#ifdef SD_WRITE_DEBUG
	uint32_t start_time = DWT_Get();
	uint32_t bc = 0;
#endif
	if (count == 1) {
		if (sd_write_block(sector,buff) == SD_OK) {
#ifdef SD_WRITE_DEBUG
			bc++;
			uint32_t elapsed_time = DWT_Get() - start_time;
			char b[20];
			//if(elapsed_time > 250000){ //only print blocks that take longer than normal
				sprintf(b, "\n%d, %d\n", bc, elapsed_time);
				putLineUART(b);
			//}

#endif
			return RES_OK;
		} else {
			return RES_ERROR;
		}
	} else if (count > 1) {
		UINT i;
		for(i=0;i<count;i++){
			if(sd_write_block(sector,buff) == SD_OK) {
#ifdef SD_WRITE_DEBUG
				bc++;
#endif
				sector++;
				buff += _MAX_SS;
			} else {
				return RES_ERROR;

			}
		}
#ifdef SD_WRITE_DEBUG
		bc++;
		uint32_t elapsed_time = DWT_Get() - start_time;
		char b[20];
		//if(elapsed_time > 250000){ //only print blocks that take longer than normal
			sprintf(b, "\n%d, %d\n", bc, elapsed_time);
			putLineUART(b);
		//}
#endif
		return RES_OK;
		/* Write Multiple Block Function */
		/* if (sd_write_multiple_blocks(sector,count,buff) == SD_OK) {
			return RES_OK;
		} else {
			return RES_ERROR;
		} */
	} else {
		return RES_PARERR;
	}
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive number (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	/* Currently not needed */
	return 0;
}
#endif
