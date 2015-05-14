/*
 * config.h
 *
 *  Created on: Feb 22, 2015
 *      Author: Kyle Smith
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <time.h>
#include <stdlib.h>

#include "board.h"
#include "fixed.h"
#include "ff.h"
#include "daq.h"
#include "system.h"
#include "stdarg.h"

#define LINE_SIZE 100

void configStart(void);

void readConfigDefault(void);

void readConfigFromEEPROM(void);

void writeConfigToEEPROM(void);

void readConfigFromFile(void);

void writeConfigToFile(void);

void writeConverterToFile(void);

void setTime(char *timeStr);

char *getTimeStr(void);

// Wrapper for printf and f_puts that writes the string to the config file
void config_printf(char *fmt, ...);

void getNonBlankLine(char* line, int32_t skipCount);

int32_t countToColon(char* line);

void endAtNewline(char* line);

#endif /* CONFIG_H_ */
