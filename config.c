#include "config.h"
#include "config_text.h"
#include "fixed.h"

FIL config;

void configStart() {
	// First check for config file
	if (f_open(&config, "config.txt", FA_OPEN_EXISTING | FA_READ)) {
		// No config file found, create default.
		f_close(&config);
		writeConfigToFile();
	} else {
		// Config file found, read it.
		readConfigFromFile();
	}
	f_close(&config);
}

// Write config object to file, this should only happen if no config is found.
void writeConfigToFile() {
	// Set default daq config.
	daq_configDefault();
	char* string = "";
	int i = 0;
	fix64_t *v_per_LSB;
	fix64_t *off_LSB;

	// If config.txt doesn't exist, create it.
	if (f_open(&config, "config.txt", FA_CREATE_ALWAYS | FA_WRITE)) {
		error(ERROR_READ_CONFIG);
	}
	putsAndNewLines(fileStrings[0], 2);
	putsAndNewLines(fileStrings[1], 1);
	putsAndNewLines(fileStrings[23], 1);
	putsAndNewLines(fileStrings[2], 1);
	putsAndNewLines(getTimeStr(), 3);
	putsAndNewLines(fileStrings[3], 1);
	putsAndNewLines(fileStrings[23], 1);
	putsAndNewLines(fileStrings[4], 1);
	putsAndNewLines(daq.user_comment, 2);
	putsAndNewLines(fileStrings[5], 1);
	sprintf(string,"%f",daq.mv_out/1000);
	putsAndNewLines(string, 1);
	putsAndNewLines(fileStrings[6], 1);
	sprintf(string,"%d",daq.sample_rate);
	putsAndNewLines(string, 1);
	putsAndNewLines(fileStrings[7], 1);
	sprintf(string,"%d",daq.trigger_delay);
	putsAndNewLines(string, 1);
	putsAndNewLines(fileStrings[8], 1);
	switch (daq.data_type){
		case READABLE:
			putsAndNewLines("R", 2);
			break;
		case HEX:
			putsAndNewLines("H", 2);
			break;
		case BINARY:
			putsAndNewLines("B", 2);
			break;
	}
	for (i = 0; i < MAX_CHAN; i++) {
		putsAndNewLines(fileStrings[9+i], 1);
		putsAndNewLines(fileStrings[12], 0);
		if (daq.channel[i].enable) {
			putsAndNewLines(fileStrings[22], 1);
		} else {
			putsAndNewLines(fileStrings[23], 1);
		}
		putsAndNewLines(fileStrings[13], 0);
		switch (daq.channel[i].range) {
		case V5:
			putsAndNewLines(fileStrings[24], 1);
			break;
		case V24:
			putsAndNewLines(fileStrings[25], 1);
			break;
		}
		putsAndNewLines(fileStrings[14], 0);
		putsAndNewLines(daq.channel[i].unit_name, 1);
		putsAndNewLines(fileStrings[15], 0);
		decFloatToStr(string, &daq.channel[i].units_per_volt, 6);
		putsAndNewLines(string, 1);
		putsAndNewLines(fileStrings[16], 0);
		fixToStr(string, &daq.channel[i].offset_uV, 6, -6);
		putsAndNewLines(string, 2);
	}

	putsAndNewLines(fileStrings[17], 1);
	putsAndNewLines(fileStrings[23], 2);

	for (i = 0; i < MAX_CHAN; i++){
		putsAndNewLines(fileStrings[9+i], 1);
		putsAndNewLines(fileStrings[18], 0);
		off_LSB =	&daq.channel[i].v5_zero_offset;
		fixToStr(string, off_LSB, 6, 0);
		putsAndNewLines(string, 1);
		putsAndNewLines(fileStrings[19], 0);
		v_per_LSB = &daq.channel[i].v5_uV_per_LSB;
		fixToStr(string, v_per_LSB, 6, -6);
		putsAndNewLines(string, 1);
		putsAndNewLines(fileStrings[20], 0);
		off_LSB =	&daq.channel[i].v24_zero_offset;
		fixToStr(string, off_LSB, 6, 0);
		putsAndNewLines(string, 1);
		putsAndNewLines(fileStrings[21], 0);
		v_per_LSB = &daq.channel[i].v24_uV_per_LSB;
		fixToStr(string, v_per_LSB, 6, -6);
		putsAndNewLines(string, 2);
	}
}

// Set channel configuration from the config file on the SD card
void readConfigFromFile(){
	char line[101]; /* Line Buffer */
	int i = 0;
	int iVal = 0;
	char cVal = 0;
	float fVal = 0;

	/* Read lines to get to update Date/Time */
	getNonBlankLine(line,2);

	if (line[0] == 'Y' || line[0] == 'y') {
		/* Update Date and Time - 4 Lines */
		getNonBlankLine(line,1);
		/* Line is now the date/time string in YYYY-MM-DD HH:MM:SS */
		setTime(line);

		/* Read lines to get to update config */
		getNonBlankLine(line,1);

	} else {
		getNonBlankLine(line,3);
	}

	if (line[0] == 'Y' || line[0] == 'y') {
		/* Update Config - */
		getNonBlankLine(line,1);
		/* Line is now the user header */
		memcpy(daq.user_comment, line, 101);
		getNonBlankLine(line,1);
		/* Line is now Output Voltage */
		sscanf(line, " %f", fVal);
		daq.mv_out = (int32_t)(fVal * 1000);
		getNonBlankLine(line,1);
		/* Line is now Sample Rate */
		sscanf(line, " %d", iVal);
		daq.sample_rate = (int32_t)iVal;
		getNonBlankLine(line,1);
		/* Line is now trigger delay */
		sscanf(line, " %d", iVal);
		daq.trigger_delay = iVal;
		getNonBlankLine(line,1);
		/* Line is now data mode */
		if (line[0] == 'R' || line[0] == 'r') {
			daq.data_type = READABLE;
		} else if (line[0] == 'H' || line[0] == 'h') {
			daq.data_type = HEX;
		} else if (line[0] == 'B' || line[0] == 'b') {
			daq.data_type = BINARY;
		} else {
			error(ERROR_READ_CONFIG);
		}
		for (i = 0; i<MAX_CHAN; i++) {
			getNonBlankLine(line,0);

			/* Channel Config */
			/* CH Enabled */
			sscanf(line + countToColon(line), " %c", cVal);
			if (cVal == 'Y' || cVal == 'y') {
				daq.channel[i].enable = true;
			} else if (cVal == 'N' || cVal == 'n') {
				daq.channel[i].enable = false;
			} else {
				error(ERROR_READ_CONFIG);
			}
			getNonBlankLine(line,0);
			/* CH Range */
			sscanf(line + countToColon(line), " %c", cVal);
			if (cVal == '5'){
				daq.channel[i].range = V5;
			} else if (cVal == '2') {
				daq.channel[i].range = V24;
			} else {
				error(ERROR_READ_CONFIG);
			}
			getNonBlankLine(line, 0);

			/* CH Units */
			memcpy(daq.channel[i].unit_name, line+23, 9);
			getNonBlankLine(line, 0);

			/* CH Units/Volt */
			sscanf(line + countToColon(line), " %f", fVal);
			daq.channel[i].units_per_volt = floatToDecFloat(fVal);
			getNonBlankLine(line,0);

			/* CH Zero Offset */
			sscanf(line + countToColon(line), " %f", fVal);
			daq.channel[i].offset_uV = floatToFix(fVal);

			/* Read lines to get to update calibration */
			getNonBlankLine(line,1);
		}
	} else {
		/* Move to next section if no update config */
		getNonBlankLine(line,29);
	}
	if (line[0] == 'Y' || line[0] == 'y') {
		/* Update Calibration - 18 Lines (Maybe) */
		for (i = 0; i<MAX_CHAN;i++) {
			getNonBlankLine(line,0);

			/* 5V Zero Offset */
			sscanf(line + countToColon(line), " %f", fVal);
			daq.channel[i].v5_zero_offset = floatToFix(fVal);
			getNonBlankLine(line,0);

			/* 5V LSB/Volt */
			sscanf(line + countToColon(line), " %f", fVal);
			daq.channel[i].v5_uV_per_LSB = floatToFix(fVal);
			getNonBlankLine(line,0);

			/* 24V Zero Offset */
			sscanf(line + countToColon(line), " %f", fVal);
			daq.channel[i].v24_zero_offset = floatToFix(fVal);
			getNonBlankLine(line,0);

			/* 24V LSB/Volt */
			sscanf(line + countToColon(line), " %f", fVal);
			daq.channel[i].v24_uV_per_LSB = floatToFix(fVal);
		}
	}
	/* Run ConfigCheck After Reading */
	daq_configCheck();
}

// Set the current time given a time string in the format
// 2015-02-21 05:57:00
void setTime(char *timeStr){
	// Enable and configure Real-Time Clock
	Chip_Clock_EnableRTCOsc();
	Chip_RTC_Init(LPC_RTC);

	// Set time in human readable form
	struct tm tm;
	time_t t = 0;
	tm = *localtime(&t);
	sscanf(timeStr, "%u-%u-%u %u:%u:%u",
		&(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday), &(tm.tm_hour), &(tm.tm_min), &(tm.tm_sec));
	tm.tm_year -= 1900;
	tm.tm_mon -= 1;
	tm.tm_isdst = 0; // No DST accounting
	t = mktime(&tm);

	Chip_RTC_Reset(LPC_RTC); // Set then clear SWRESET bit
	Chip_RTC_SetCount(LPC_RTC, t); // Set the time
	Chip_RTC_Enable(LPC_RTC); // Start counting
}

char *getTimeStr(){
	time_t rawtime = Chip_RTC_GetCount(LPC_RTC);
	struct tm * timeinfo;
	timeinfo = localtime(&rawtime);
	return asctime(timeinfo);
}

void putsAndNewLines(const char* string, int32_t lineCount) {
	f_puts(string,&config);
	while (lineCount > 0) {
		f_puts("\n",&config);
		lineCount--;
	}
}

void getNonBlankLine(char* line, int32_t skipCount){
	f_gets(line, sizeof(line), &config);
	int32_t i;
	for(i=0;i<=skipCount;i++){
		while (line[0] == '\0') {
			f_gets(line, sizeof(line), &config);
		}
	}
}

int32_t countToColon(char* line) {
	int32_t count = 0;
	while ((line[count] != ':') && (line[count] != '\0')) {
		count++;
	}
	return count;
}
