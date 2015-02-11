/*
 * fatfstest.c
 *
 *  Created on: Feb 8, 2015
 *      Author: Sven
 */

#include "fatfstest.h"

void testFS() {

	FATFS fatfs[_VOLUMES];
	FIL file[2];
	BYTE Buff[8*512] __attribute__ ((aligned (4)));

	// Test FatFS
	f_mount(&fatfs,"",0);
	f_open(&file[0],"test",FA_CREATE_ALWAYS | FA_WRITE);
	f_puts("I love wieners\n",&file[0]);

	// Test RTC
	/*
	int count = Chip_RTC_GetCount(&RTC);
	time_t currentTime = count;
	struct tm * localTime = localtime(&currentTime);
	f_puts(asctime(localTime),&file[0]);
	*/
	f_close(&file[0]);
	f_mount(NULL,"",0);
}
