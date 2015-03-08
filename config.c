#include "config.h"


// Set channel configuration from the config file on the SD card
void config_fromFile(void){
	FIL config;		/* FatFS File Object */
	char line[101]; /* Line Buffer */
	int i = 0;
	int iVal = 0;
	float fVal = 0;

	if (f_open(&config, "config.txt", FA_READ)) {
		error(ERROR_READ_CONFIG);
	}
	/* Read 4 lines to get to update Date/Time */
	for (i = 0; i < 4; i++) {
		f_gets(line, sizeof(line), &config);
	}

	if (line[0] == 'Y' || line[0] == 'y') {
		/* Update Date and Time - 4 Lines */
		f_gets(line, sizeof(line), &config);
		f_gets(line, sizeof(line), &config);
		/* Line is now the date/time string in YYYY-MM-DD HH:MM:SS */
		setTime(line);
	}
	/* Read 4 lines to get to update config */
	for (i = 0; i < 4; i++) {
		f_gets(line, sizeof(line), &config);
	}
	if (line[0] == 'Y' || line[0] == 'y') {
		/* Update Config - 32 Lines (Maybe) */
		f_gets(line, sizeof(line), &config);
		f_gets(line, sizeof(line), &config);
		/* Line is now the user header */
		memcpy(daq.user_comment, line, 101);
		f_gets(line, sizeof(line), &config);
		f_gets(line, sizeof(line), &config);
		f_gets(line, sizeof(line), &config);
		/* Line is now Output Voltage */
		sscanf(line, "%f", fVal);
		daq.mv_out = (int32_t)(fVal * 1000);
		f_gets(line, sizeof(line), &config);
		f_gets(line, sizeof(line), &config);
		/* Line is now Sample Rate */
		sscanf(line, "%d", iVal);
		daq.sample_rate = (int32_t)iVal;
		f_gets(line, sizeof(line), &config);
		f_gets(line, sizeof(line), &config);
		/* Line is now trigger delay */
		sscanf(line, "%d", iVal);
		daq.trigger_delay = iVal;
		f_gets(line, sizeof(line), &config);
		f_gets(line, sizeof(line), &config);
		/* Line is now data mode */
		if (line[0] == 'R') {
			daq.data_type = READABLE;
		} else if (line[0] == 'H') {
			daq.data_type = HEX;
		} else if (line[0] == 'B') {
			daq.data_type = BINARY;
		}
		for (i = 0; i<MAX_CHAN; i++) {
			f_gets(line, sizeof(line), &config);
			f_gets(line, sizeof(line), &config);
			f_gets(line, sizeof(line), &config);

			/* Channel Config */
			/* CH Enabled */
			if (line[23] == 'Y') {
				daq.channel[i].enable = true;
			} else if (line[23] == 'N') {
				daq.channel[i].enable = false;
			}
			f_gets(line, sizeof(line), &config);
			/* CH Range */
			if (line[23] == '5'){
				daq.channel[i].range = V5;
			} else if ((line[23] == '2') && (line[24] == '4')) {
				daq.channel[i].range = V24;
			}
			f_gets(line, sizeof(line), &config);

			/* CH Units */
			memcpy(daq.channel[i].unit_name, line+23, 9);
			f_gets(line, sizeof(line), &config);

			/* CH Units/Volt */
			sscanf(line, "UNITS PER VOLT   [fp]: %f", fVal);
			daq.channel[i].units_per_volt = floatToDecFloat(fVal);
			f_gets(line, sizeof(line), &config);

			/* CH Zero Offset */
			sscanf(line, "ZERO OFFSET      [fp]: %f", fVal);
			daq.channel[i].offset_uV = floatToFix(fVal);
		}
	}
	/* Read 4 lines to get to update calibration */
	for (i = 0; i < 4; i++) {
		f_gets(line, sizeof(line), &config);
	}
	if (line[0] == 'Y') {
		/* Update Calibration - 18 Lines (Maybe) */
		for (i = 0; i<MAX_CHAN;i++) {
			f_gets(line, sizeof(line), &config);
			f_gets(line, sizeof(line), &config);
			f_gets(line, sizeof(line), &config);

			/* 5V Zero Offset */
			sscanf(line, "5V ZERO OFFSET  [fp]: %f", fVal);
			daq.channel[i].v5_zero_offset = floatToFix(fVal);
			f_gets(line, sizeof(line), &config);

			/* 5V LSB/Volt */
			sscanf(line, "5V LSB / VOLT   [fp]: %f", fVal);
			daq.channel[i].v5_uV_per_LSB = floatToFix(fVal);
			f_gets(line, sizeof(line), &config);

			/* 24V Zero Offset */
			sscanf(line, "24V ZERO OFFSET [fp]: %f", fVal);
			daq.channel[i].v24_zero_offset = floatToFix(fVal);
			f_gets(line, sizeof(line), &config);

			/* 24V LSB/Volt */
			sscanf(line, "24V LSB / VOLT  [fp]: %f", fVal);
			daq.channel[i].v24_uV_per_LSB = floatToFix(fVal);
		}
	}
	/* Close File */
	f_close(&config);
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
