#ifndef FATTIME_H_

#include "integer.h"
#include "rtc_15xx.h"

typedef struct {
	WORD	year;	/* 1..4095 */
	BYTE	month;	/* 1..12 */
	BYTE	mday;	/* 1.. 31 */
	BYTE	wday;	/* 1..7 */
	BYTE	hour;	/* 0..23 */
	BYTE	min;	/* 0..59 */
	BYTE	sec;	/* 0..59 */
} DATETIME;

extern LPC_RTC_T RTC;
DWORD get_fattime (void);

#endif
