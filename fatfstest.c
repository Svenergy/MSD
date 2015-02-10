/*
 * fatfstest.c
 *
 *  Created on: Feb 8, 2015
 *      Author: Sven
 */

#include "fatfstest.h"
#include "ff.h"


void testFS() {

	FATFS fatfs[_VOLUMES];
	FIL file[2];
	FRESULT fr;
	DIR dir;
	BYTE Buff[8*512] __attribute__ ((aligned (4)));

	f_mount(&fatfs,"",0);
	f_open(&file[0],"test",FA_CREATE_ALWAYS | FA_WRITE);
	f_puts("I love weiners\n",&file[0]);
	f_close(&file[0]);
	f_mount(NULL,"",0);
}
