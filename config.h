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

#define LINE_SIZE 100

void configStart(void);

void readConfigDefault(void);

void readConfigFromEEPROM(void);

void writeConfigToEEPROM(void);

void readConfigFromFile(void);

void writeConfigToFile(void);

void setTime(char *timeStr);

char *getTimeStr(void);

void putsAndNewLines(const char* string, int32_t lineCount);

void getNonBlankLine(char* line, int32_t skipCount);

int32_t countToColon(char* line);

void endAtNewline(char* line);

#endif /* CONFIG_H_ */
