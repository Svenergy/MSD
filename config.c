#include "config.h"
#include "config_text.h"

FIL config;

void configStart() {
	// Load the config from EEPROM
	readConfigFromEEPROM();

	// Check for config file on SD card
	FRESULT fr = f_stat("config.txt", NULL);
	switch (fr) {
	case FR_OK:
		// Config file exists
		break;
	case FR_NO_FILE:
		// Config file does not exist, create default
		readConfigDefault();
		writeConfigToFile();
		break;
	default:
		// Unknown file read error
		error(ERROR_READ_CONFIG);
	}

	// Read config file from card
	readConfigFromFile();

	// Write config back to card
	writeConfigToFile();

	// Update the config back to EEPROM
	writeConfigToEEPROM();
}

// Set channel configuration defaults
void readConfigDefault(void){
	int32_t i;

	// Channel_Config defaults
	for(i=0;i<MAX_CHAN;i++){
		daq.channel[i].enable = true;				// enable channel
		daq.channel[i].range = V24;					// 0-5v input range

		daq.channel[i].units_per_volt = floatToDecFloat(1.0); // sensitivity in units/volt
		daq.channel[i].offset_uV = floatToFix(0); 			// zero offset in volts

		strcpy(daq.channel[i].unit_name, "V");	// name of channel units

		daq.channel[i].v5_zero_offset = floatToFix(0.0);		// theoretical value of raw 16-bit sample for 0 input voltage
		daq.channel[i].v5_uV_per_LSB = floatToFix(78.04726);	// theoretical sensitivity of reading in uV / LSB = 1000000 * (4.096 * ( (402+100) / 402 )) / (1 << 16)
		daq.channel[i].v24_zero_offset = floatToFix(32511.13);	// theoretical value of raw 16-bit sample for 0 input voltage = (1 << 16) * ( (1/(1/100+1/402+1/21)) / (1/(1/100+1/402+1/21) + 16.9) )
		daq.channel[i].v24_uV_per_LSB = floatToFix(745.48879);	// theoretical sensitivity of reading in uV / LSB = 1000000 / (((1 << 16)/4.096) * (1/(1/100+1/402+1/21+1/16.9)) / 100)
	}

	// Sample rate in Hz
	daq.sample_rate = 10000;

	// Trigger Delay in seconds
	daq.trigger_delay = 0;

	// Data mode can be READABLE or BINARY
	daq.data_type = BINARY;

	// Vout = 5v
	daq.mv_out = 5000;

	// User comment string
	strcpy(daq.user_comment, "User header comment");
}

// Read config from EEPROM
void readConfigFromEEPROM(void){
	Chip_EEPROM_Read(0x00000000, (uint8_t *)&daq, sizeof(daq));
}

// Write config to EEPROM
void writeConfigToEEPROM(void){
	Chip_EEPROM_Write(0x00000000, (uint8_t *)&daq, sizeof(daq));
}

// Set channel configuration from the config file on the SD card
void readConfigFromFile(){
	char line[LINE_SIZE+1]; /* Line Buffer */
	int32_t i = 0;
	int32_t iVal = 0;
	char cVal = 0;
	float fVal = 0;

	/* Attempt to open config file, error if the file does not exist */
	if(f_open(&config, "config.txt", FA_OPEN_EXISTING | FA_READ) != FR_OK){
		error(ERROR_READ_CONFIG);
	}

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
		endAtNewline(line);
		memcpy(daq.user_comment, line, 101);
		getNonBlankLine(line,1);
		/* Line is now Output Voltage */
		sscanf(line, " %f", &fVal);
		daq.mv_out = (int32_t)(fVal * 1000);
		getNonBlankLine(line,1);
		/* Line is now Sample Rate */
		sscanf(line, " %d", &iVal);
		daq.sample_rate = (int32_t)iVal;
		getNonBlankLine(line,1);
		/* Line is now trigger delay */
		sscanf(line, " %d", &iVal);
		daq.trigger_delay = iVal;
		getNonBlankLine(line,1);
		/* Line is now data mode */
		if (line[0] == 'R' || line[0] == 'r') {
			daq.data_type = READABLE;
		} else if (line[0] == 'B' || line[0] == 'b') {
			daq.data_type = BINARY;
		} else {
			error(ERROR_READ_CONFIG);
		}
		for (i = 0; i<MAX_CHAN; i++) {
			getNonBlankLine(line,1);
			/* Channel Config */
			/* CH Enabled */
			sscanf(line + countToColon(line), " %c", &cVal);
			if (cVal == 'Y' || cVal == 'y') {
				daq.channel[i].enable = true;
			} else if (cVal == 'N' || cVal == 'n') {
				daq.channel[i].enable = false;
			} else {
				error(ERROR_READ_CONFIG);
			}
			getNonBlankLine(line,0);
			/* CH Range */
			sscanf(line + countToColon(line), " %c", &cVal);
			if (cVal == '5'){
				daq.channel[i].range = V5;
			} else if (cVal == '2') {
				daq.channel[i].range = V24;
			} else {
				error(ERROR_READ_CONFIG);
			}
			getNonBlankLine(line, 0);
			/* CH Units */
			endAtNewline(line);
			memcpy(daq.channel[i].unit_name, line + countToColon(line)+1, 9);
			getNonBlankLine(line, 0);

			/* CH Units/Volt */
			sscanf(line + countToColon(line), " %f", &fVal);
			daq.channel[i].units_per_volt = floatToDecFloat(fVal);
			getNonBlankLine(line,0);

			/* CH Zero Offset */
			sscanf(line + countToColon(line), " %f", &fVal);
			daq.channel[i].offset_uV = floatToFix(fVal);
		}

		/* Read lines to get to update calibration */
		getNonBlankLine(line,1);

	} else {
		/* Move to next section if no update config */
		getNonBlankLine(line,29);
	}
	if (line[0] == 'Y' || line[0] == 'y') {
		/* Update Calibration - 18 Lines (Maybe) */
		for (i = 0; i<MAX_CHAN;i++) {
			getNonBlankLine(line,0);

			/* 5V Zero Offset */
			sscanf(line + countToColon(line), " %f", &fVal);
			daq.channel[i].v5_zero_offset = floatToFix(fVal);
			getNonBlankLine(line,0);

			/* 5V LSB/Volt */
			sscanf(line + countToColon(line), " %f", &fVal);
			daq.channel[i].v5_uV_per_LSB = floatToFix(fVal);
			getNonBlankLine(line,0);

			/* 24V Zero Offset */
			sscanf(line + countToColon(line), " %f", &fVal);
			daq.channel[i].v24_zero_offset = floatToFix(fVal);
			getNonBlankLine(line,0);

			/* 24V LSB/Volt */
			sscanf(line + countToColon(line), " %f", &fVal);
			daq.channel[i].v24_uV_per_LSB = floatToFix(fVal);
		}
	}
	/* Close config file */
	f_close(&config);

	/* Run ConfigCheck After Reading */
	daq_configCheck();
}

// Write config object to file
void writeConfigToFile() {
	char string[LINE_SIZE+1];
	int i = 0;
	fix64_t *v_per_LSB;
	fix64_t *off_LSB;

	// If config.txt doesn't exist, create it.
	if (f_open(&config, "config.txt", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
		error(ERROR_WRITE_CONFIG);
	}
	putsAndNewLines(fileStrings[0], 2);
	putsAndNewLines(fileStrings[1], 1);
	putsAndNewLines(fileStrings[23], 1);
	putsAndNewLines(fileStrings[2], 1);

	time_t t = Chip_RTC_GetCount(LPC_RTC);
	struct tm * tm;
	tm = localtime(&t);
	strftime(string,LINE_SIZE,"%Y-%m-%d %H:%M:%S",tm);
	putsAndNewLines(string, 3);

	putsAndNewLines(fileStrings[3], 1);
	putsAndNewLines(fileStrings[23], 1);
	putsAndNewLines(fileStrings[4], 1);
	putsAndNewLines(daq.user_comment, 2);
	putsAndNewLines(fileStrings[5], 1);
	sprintf(string,"%f",daq.mv_out/1000.0);
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

	/* Close config file */
	f_close(&config);
}

// Set the current time given a time string in the format
// 2015-02-21 05:57:00
void setTime(char *timeStr){
	log_string("Time Set");
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

// Scans the string until reaching a '\0' or '\n'
// Returns true if line only contains spaces and tabs
static bool isBlank(char* line){
	int32_t i = 0;
	while (line[i] != '\0' && line[i] != '\n' && line[i] != '\r'){
		if(line[i] != '\t' && line[i] != ' '){
			return false;
		}
		i++;
	}
	return true;
}

void getNonBlankLine(char* line, int32_t skipCount){
	int32_t i;
	for(i=0;i<=skipCount;i++){
		f_gets(line, LINE_SIZE, &config);
		while (isBlank(line)) {
			if(f_gets(line, LINE_SIZE, &config) == NULL){ // Break if f_gets fails
				break;
			}
		}
	}
}

int32_t countToColon(char* line) {
	int32_t count = 0;
	while ((line[count] != ':')) {
		if(line[count] == '\0'){
			return count;
		}
		count++;
	}
	return count+1;
}

void endAtNewline(char* line) {
	int32_t i = 0;
	while(line[i] != '\n' && line[i] != '\0'){
		i++;
	}
	line[i] = '\0';
}
