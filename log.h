/*
 * log.h
 *
 *  Created on: Feb 22, 2015
 *      Author: Kyle
 */

#ifndef LOG_H_
#define LOG_H_

#include "board.h"
#include "config.h"
#include "ff.h"

// FatFS volume
extern FATFS fatfs[_VOLUMES];

// Append the string to the log file with a time stamp, print to UART if debug enabled
void log_string(char *logString);

#endif /* LOG_H_ */
