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

void config_fromFile(void);

void setTime(char *timeStr);

char *getTimeStr(void);

#endif /* CONFIG_H_ */
