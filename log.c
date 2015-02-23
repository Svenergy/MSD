/*
 * log.c
 *
 *  Created on: Feb 22, 2015
 *      Author: Kyle Smith
 */

#include "log.h"

// FatFS file object
FIL logFile;

// Append the string to the log file with a time stamp, print to UART if debug enabled
void log_string(char *logString)
{
	char lineBuf[100];
	sprintf(lineBuf,"%s <%s>\n",getTimeStr(), logString);
#ifdef DEBUG
	putLineUART(lineBuf);
#endif

	// Write string to log file
	f_open(&logFile,"log.txt",FA_OPEN_ALWAYS | FA_WRITE);
	f_lseek(&logFile,f_size(&logFile));
	f_puts(lineBuf, &logFile);
	f_close(&logFile);
}
