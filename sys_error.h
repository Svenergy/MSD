#ifndef __SYS_ERROR_
#define __SYS_ERROR_

#include "board.h"
#include "delay.h"
#include "system.h"
#include "log.h"
#include "ff.h"
#include "sd_spi.h"

typedef enum {
	ERROR_UNKNOWN		= 0,
	ERROR_F_WRITE		= 1,
	ERROR_BUF_OVF		= 2,
	ERROR_MSC_SD_READ	= 3,
	ERROR_MSC_SD_WRITE	= 4,
	ERROR_MSC_INIT		= 5,
	ERROR_SD_INIT		= 6,
	ERROR_SAMPLE_TIME	= 7,
} ERROR_CODE;

extern FATFS fatfs[_VOLUMES];

extern SD_CardInfo cardinfo;

void error(ERROR_CODE code);

#endif /* __ERROR_ */
