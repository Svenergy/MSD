/*
 * ff_glue.c
 *
 *  Created on: Feb 24, 2015
 *      Author: Kyle
 */

#include "ff.h"

#include <time.h>

/* Glue functions to bind FatFS to rest of the system. */

/* Get time from RTC and pack in uint32_t */
uint32_t get_fattime (void)
{
	time_t t = Chip_RTC_GetCount(LPC_RTC);
	struct tm tm;
	tm = *localtime(&t);
	// Years: Fattime epoch 1980, ctime epoch 1900
	// Months: Fattime 1-12, ctime 0-11
	// Seconds: Fattime uses seconds/2

	uint32_t fatTime = ((tm.tm_year - 80) << 25) |
					   ((tm.tm_mon + 1) << 21) |
					   (tm.tm_mday << 16) |
					   (tm.tm_hour << 11) |
					   (tm.tm_min << 5) |
					   (tm.tm_sec >> 1);

	return  fatTime;
}
