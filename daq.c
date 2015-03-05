#include "daq.h"

// Buffer used for string formatted data
RingBuffer *strBuff;

// FatFS file object
FIL dataFile;

// DAQ configuration data
DAQ daq;

// DAQ loop function
void (*daq_loop)(void);

// Time tracking
static volatile uint32_t sampleCount; // Count of samples taken in the current recording
static uint32_t sampleStrfCount; // Count of samples string formatted in the current recording
static volatile uint32_t dwt_lastTime; // Time of the last sample according to the DWT timer, used to measure sampling integral error and jitter
static volatile uint64_t dwt_elapsedTime; // Total sampling elapsed time according to the DWT timer
static uint32_t buttonTime; // Time that the record button was pressed, used for trigger delay

// Flag cleared when the data file is created, set on init
static bool noFile;

// Flag set by SCT0_IRQHandler, accessed by RIT_IRQHandler
static volatile bool adcUsed;

// Vout PWM
// Takes 1190cc (16.5us). At 7200Hz, takes 11.9% of cpu time
void SCT0_IRQHandler(void){
	/* Clear interrupt */
	Chip_SCT_ClearEventFlag(LPC_SCT0, SCT_EVT_0);

	adcUsed = true;
    static int32_t intError;
    int32_t propError, pwmOut;

    uint16_t voutCFG = 	(1 << ADC_CFG ) | // Overwrite config
    					(6 << ADC_INCC) | // Unipolar, referenced to COM
						(3 << ADC_IN) 	| // Channel IN3
						(1 << ADC_BW) 	| // Full bandwidth
						(1 << ADC_REF) 	| // Internal reference output 4.096v
						(0 << ADC_SEQ) 	| // Channel sequencer disabled
						(1 << ADC_RB);	  // Do not read back config
    adc_read(voutCFG);
    adc_read(0);

    // Theoretical mv / LSB = 1000 * ((100+20)/20) * 4.096 / (1 << 16) = 0.375
    propError =  daq.mv_out - (3 * adc_read(0)) / 8; // Units are mv

    intError += propError; // Units are mv * ms * 7.2, or giving a rate of 7.2e6/(v*s)

    // Integral Error Saturation
    intError = clamp(intError, -72000, 72000); // 7.2E5 means saturation at 10[mv*s]

    // Proportional Error Saturation, low to reduce overshoot and decrease integral settling time for large steps
    propError = clamp(propError, -350, 350); // Saturation at +/- 1v

    // Calculate PWM output value out of 10000
    //Theoretical Duty = mv_out * 10 / ((3.3*39 / (39+51.7)) * (3.48E6/182E3 + 1)), = 0.350253
    pwmOut = (daq.mv_out * 35025) / 100000 + intError / 200 + 2 * propError;
    pwmOut = clamp(pwmOut, 0, 9999);

    // Set PWM output
    Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 1, pwmOut);
}

// Sample timer
void RIT_IRQHandler(void){
	/* Clear interrupt */
	Chip_RIT_ClearIntStatus(LPC_RITIMER);

	uint32_t i, j;
	uint16_t rawVal[MAX_CHAN];

	uint32_t dwt_currentTime = DWT_Get();

	if(daq.sample_rate >= 1000){
		// Set ADC config
		uint16_t sampleCFG = (1 << ADC_CFG ) | // Overwrite config
							 (6 << ADC_INCC) | // Unipolar, referenced to COM
							 ((MAX_CHAN-1) << ADC_IN)   | // Sequence channels 0,1.. (MAX_CHAN-1)
							 (1 << ADC_BW)   | // Full bandwidth
							 (1 << ADC_REF)  | // Internal reference output 4.096v
							 (3 << ADC_SEQ)  | // Channel sequencer enabled
							 (1 << ADC_RB);    // Do not read back config

		NVIC_DisableIRQ(SCT0_IRQn); // Prevent sampling interruptions
		adc_read(sampleCFG);
		adc_read(0); // Buffering read, next read returns sample from channel 0
		// Read all channels, only store data for enabled channels
		uint8_t ch = 0;
		for(i=0;i<MAX_CHAN;i++){
			if(daq.channel[i].enable){
				rawVal[ch++] = adc_read(0);
			} else {
				adc_read(0);
			}
		}
		NVIC_EnableIRQ(SCT0_IRQn);

	} else { // Average a bunch of samples for each channel when recording at slow rates for better noise rejection
		// Read all enabled channels
		uint8_t ch = 0;
		for(i=0;i<MAX_CHAN;i++){
			if(daq.channel[i].enable){
				// Set ADC config
				uint16_t sampleCFG = (1 << ADC_CFG ) | // Overwrite config
									 (6 << ADC_INCC) | // Unipolar, referenced to COM
									 (i << ADC_IN)   | // Read channel i
									 (1 << ADC_BW)   | // Full bandwidth
									 (1 << ADC_REF)  | // Internal reference output 4.096v
									 (0 << ADC_SEQ)  | // Channel sequencer disabled
									 (1 << ADC_RB);    // Do not read back config
				uint16_t count = 10000/daq.sample_rate;
				uint32_t sum = 0;

				adcUsed = false;
				adc_read(sampleCFG);
				adc_read(0); // Buffering read, next read returns sample from channel 0

				for(j=0;j<count;j++){
					uint16_t raw;
					if(adcUsed){
						adcUsed = false;
						adc_read(sampleCFG);
						adc_read(0);
					}
					raw = adc_read(0);
					while(adcUsed){
						adcUsed = false;
						adc_read(sampleCFG);
						adc_read(0);
						raw = adc_read(0);
					}
					sum += raw;
				}
				rawVal[ch++] = sum / count;
			}
		}
	}

	// Read current time with microsecond precision, increment sample counter
	uint64_t us = ((uint64_t)++sampleCount * 1000000 ) / daq.sample_rate;

	// Compare to DWT timer
	dwt_elapsedTime += dwt_currentTime - dwt_lastTime;
	dwt_lastTime = dwt_currentTime;
	int32_t dT = us - dwt_elapsedTime/72;

	// Error if sample timing is off by > 50us
	if(dT > 3600 || dT < -3600){
		error(ERROR_SAMPLE_TIME);
	}

	RingBuffer_writeData(rawBuff, rawVal, daq.channel_count*2); // 16 bit samples = 2bytes/sample
}

// Set up daq
void daq_init(void)
{
	log_string("Acquisition Ready");
	Board_LED_Color(LED_PURPLE);

	// Read config
	daq_configDefault();

	// Limit config values to valid values
	daq_configCheck();

	// Set up ADC
	adc_spi_setup();

	// Set up channel ranges in hardware mux
#ifndef DEBUG
	int i;
	for(i=0;i<3;i++){ // This Kills the UART
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, rsel_pins[i]);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, rsel_pins[i], daq.channel[i].range);
	}
#endif

	// Clear the raw data buffer
	RingBuffer_clear(rawBuff);

	// Initialize the string formatted buffer
	strBuff = RingBuffer_init(BLOCK_SIZE + SAMPLE_STR_SIZE);

	// Set flag to indicate no data file exists
	noFile = true;

	// 0 the sample count
	sampleCount = 0;
	sampleStrfCount = 0;

	// Save button time for trigger delay
	buttonTime = Chip_RTC_GetCount(LPC_RTC);

	// Set loop to wait for trigger delay to expire
	daq_loop = daq_triggerDelay;
}

// Start acquiring data
void daq_record(){
	log_string("Acquisition Start");
	Board_LED_Color(LED_RED);

	// Turn on vout
	daq_voutEnable();

	// Delay 200ms to allow power to stabilize
	DWT_Delay(200000);

	// Start time according to DWT timer
	dwt_lastTime = DWT_Get();
	dwt_elapsedTime = 0;

	// Set up sampling interrupt using RITimer
	Chip_RIT_Init(LPC_RITIMER);

	/* Set timer compare value and periodic mode */
	// Do not use Chip_RIT_SetTimerIntervalHz, for timing critical operations, it has an off by 1 error on the period in clock cycles
	uint64_t cmp_value = Chip_Clock_GetSystemClockRate() / daq.sample_rate - 1;
	Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);
	Chip_RIT_EnableCompClear(LPC_RITIMER);

	Chip_RIT_Enable(LPC_RITIMER);
	Chip_RIT_ClearIntStatus(LPC_RITIMER);

	NVIC_EnableIRQ(RITIMER_IRQn);
	NVIC_SetPriority(RITIMER_IRQn, 0x01); // Set to second highest priority to ensure sample timing accuracy

	// Set loop to write data from buffer to file
	daq_loop = daq_writeData;
}

// Make the data file
void daq_makeDataFile(void){
	time_t t = Chip_RTC_GetCount(LPC_RTC);
	struct tm * tm;
	tm = localtime(&t);
	char fn[40];
	strftime (fn,40,"%Y-%m-%d_%H-%M-%S_data.txt",tm);
	f_open(&dataFile,fn,FA_CREATE_ALWAYS | FA_WRITE);
	noFile = false; // File has been created
}

// Wait for the trigger time to start
void daq_triggerDelay(void){
	// Wait for the Trigger Delay to expire
	if( (int32_t)(Chip_RTC_GetCount(LPC_RTC) - buttonTime) >= daq.trigger_delay){
		// Start acquiring data
		daq_record();
	}
}

// Enable the output voltage
void daq_voutEnable(void){
	// Enable Vout using ~SHDN
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, VOUT_N_SHDN);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, VOUT_N_SHDN, true);

	// Set up Vout PWM
	Chip_SCTPWM_Init(LPC_SCT0);
	Chip_SCTPWM_SetRate(LPC_SCT0, 7200); // 7200Hz, 10000 counts per cycle
	Chip_SWM_MovablePinAssign(SWM_SCT0_OUT0_O, VOUT_PWM); // Assign PWM to output pin
	Chip_SCTPWM_SetOutPin(LPC_SCT0, 1, 0); // Set SCT PWM output 1 to SCT pin output 0
	Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 1, 0); // Set to duty cycle of 0
	Chip_SCTPWM_Start(LPC_SCT0); // Start PWM

	// Create SCT0 Vout PWM interrupt
	Chip_SCT_EnableEventInt(LPC_SCT0, SCT_EVT_0);
	NVIC_EnableIRQ(SCT0_IRQn);
	NVIC_SetPriority(SCT0_IRQn, 0x00); // Set to higher priority than sampling
}

// Disable the output voltage
void daq_voutDisable(void){
	// Stop Vout interrupt
	NVIC_DisableIRQ(SCT0_IRQn);

	// Disable Vout using ~SHDN
	Chip_GPIO_SetPinState(LPC_GPIO, 0, VOUT_N_SHDN, false);
}

// Write data file header
void daq_header(void){
	uint8_t i;
	char headerStr[100];
	uint8_t headerStr_size = 0;

	/**** User comment ****
	 * Ex.
	 * User header comment
	 */
#if defined(DEBUG) && defined(PRINT_DATA_UART)
	putLineUART(daq.user_comment);
#endif
	RingBuffer_writeStr(strBuff, daq.user_comment);

	/**** Time Stamps ****
	 * Ex.
	 * *
	 * Mon Mar 02 20:02:43 2015
	 * 1425326563
	 * *
	 */
	sprintf(headerStr, "\n*\n%s\n%d\n*\n", getTimeStr(), Chip_RTC_GetCount(LPC_RTC));
#if defined(DEBUG) && defined(PRINT_DATA_UART)
	putLineUART(headerStr);
#endif
	RingBuffer_writeStr(strBuff, headerStr);

	/**** Channel Scaling ****
	 * Ex.
	 * ch1: Scale = 2.3456e+00 g/V, Offset = 0.0000V
	 * ch2: Scale = 2.3456e+00 g/V, Offset = 0.0000V
	 * ch3: Scale = 2.3456e+00 g/V, Offset = 0.0000V
	 */
	for(i=0;i<MAX_CHAN;i++){
		if(daq.channel[i].enable){
			headerStr_size = sprintf(headerStr, "ch%d: Scale = ", i+1);
			headerStr_size += fullDecFloatToStr(headerStr+headerStr_size, &daq.channel[i].units_per_volt, 4);
			headerStr_size += sprintf(headerStr+headerStr_size, " %s/V, Offset = ", daq.channel[i].unit_name);
			headerStr_size += sprintf(headerStr+headerStr_size, "%.4fV\n", fixToFloat(&daq.channel[i].offset_uV)/1000000.0);
#if defined(DEBUG) && defined(PRINT_DATA_UART)
			putLineUART(headerStr);
#endif
			RingBuffer_writeStr(strBuff, headerStr);
		}
	}

	/**** Sample Rate ****
	 * Ex.
	 * *
	 * Sample rate = 1000Hz
	 * Sample period = .001s
	 */
	headerStr_size =sprintf(headerStr, "*\nSample rate = %dHz\n", daq.sample_rate);
	headerStr_size += sprintf(headerStr+headerStr_size, "Sample period = %.4es\n", 1.0/ daq.sample_rate);
#if defined(DEBUG) && defined(PRINT_DATA_UART)
	putLineUART(headerStr);
#endif
	RingBuffer_writeStr(strBuff, headerStr);

	/**** Channel Labels ****
	 * Ex.
	 * *
	 * time[s], ch1[V], ch1[V], ch1[V]
	 */
	headerStr_size = sprintf(headerStr, "*\ntime[s]");
	for(i=0;i<MAX_CHAN;i++){
		if(daq.channel[i].enable){
			headerStr_size += sprintf(headerStr+headerStr_size, ", ch%d[%s]", i+1, daq.channel[i].unit_name);
		}
	}
	headerStr[headerStr_size++] = '\n';
	headerStr[headerStr_size++] = '\0';
#if defined(DEBUG) && defined(PRINT_DATA_UART)
	putLineUART(headerStr);
#endif
	RingBuffer_writeStr(strBuff, headerStr);
}

// Stop acquiring data
void daq_stop(void){
	if(daq_loop == daq_writeData){ // If data has been written
		// Stop RIT interrupt
		Chip_RIT_DeInit(LPC_RITIMER);
		NVIC_DisableIRQ(RITIMER_IRQn);

		// Turn off output voltage
		daq_voutDisable();

		log_string("Acquisition Stop");

		// flush ring buffer to disk
		daq_writeData();
		daq_writeBlock(); // Write any partial block remaining

		// Destroy the string formatted buffer
		RingBuffer_destroy(strBuff);

		// Write all buffered data to disk
		f_close(&dataFile);
	}
}

// Write data from raw buffer to file, formatting to string  buffer as an intermediate step
// Stop when the raw buffer is empty
void daq_writeData(void){
	if(noFile){
		daq_makeDataFile();
		daq_header(); // Write data file header to string buffer
	}
	while(true){
		// Format raw data into the string buffer until a block is ready
		while(RingBuffer_getSize(strBuff) < BLOCK_SIZE){
			int32_t br;
			uint16_t rawData[MAX_CHAN];
			br = RingBuffer_read(rawBuff, rawData, daq.channel_count*2);
			if(br < daq.channel_count*2){
				return; // No more raw data, finished processing
			} else {
				// Format data into string
				char sampleStr[SAMPLE_STR_SIZE];
				switch (daq.data_mode){
				case READABLE:
					daq_readableFormat(rawData, sampleStr);
					RingBuffer_writeStr(strBuff, sampleStr);
					break;
				case HEX:
					daq_hexFormat(rawData, sampleStr);
					RingBuffer_writeStr(strBuff, sampleStr);
					break;
				case BINARY:
					RingBuffer_writeData(strBuff, rawData, daq.channel_count*2);
					sampleStr[0] = '\0';
					break;
				}
#if defined(DEBUG) && defined(PRINT_DATA_UART)
				putLineUART(sampleStr);
#endif
			}
		}
		daq_writeBlock();
	}
}

// Write a single block to the data file from the string buffer
void daq_writeBlock(void){
	int32_t br;
	UINT bw;
	FRESULT errorCode;
	char data[BLOCK_SIZE];
	br = RingBuffer_read(strBuff, data, BLOCK_SIZE);
	if((errorCode = f_write(&dataFile, data, br, &bw)) != FR_OK){
		error(ERROR_F_WRITE);
	}
}

// Convert rawData into a hex formatted output string
void daq_hexFormat(uint16_t *rawData, char *sampleStr){
	// sampleStr Ex. 18b39ce2b94f
	int32_t sampleStr_size = 0;
	int32_t i;
	for(i=0;i<daq.channel_count;i++){
		int32_t j;
		uint32_t val = rawData[i];
		for(j=0;j<4;j++){
			char digit = ((val & 0x0000F000) >> 12);
			if(digit > 9){
				sampleStr[sampleStr_size++] = digit - 10 + 'A';
			}else{
				sampleStr[sampleStr_size++] = digit + '0';
			}
			val = val << 4;
		}
	}
	sampleStr[sampleStr_size++] = '\n';
	sampleStr[sampleStr_size++] = '\0';
}

// Convert rawData into a readable scaled and formatted output string
// Total calculation time for 3 channels with time and sample precision 4:
// 4650cc (65us)
void daq_readableFormat(uint16_t *rawData, char *sampleStr){
	// sampleStr Ex. 9999.1234,1.2345e+01,1.2345e+01,1.2345e+01
	int8_t sampleStr_size = 0;

	dec_float_t scaledVal[MAX_CHAN];

	// Calculate sample time with microsecond precision
	int64_t us = ((int64_t)sampleStrfCount++ * 1000000 ) / daq.sample_rate;

	/* Scale samples , takes 566cc/sample (7.9us) */
	uint8_t ch = 0;
	int8_t i;
	for(i=0;i<MAX_CHAN;i++){
		if(daq.channel[i].enable){ // Only scale enabled channels
			// Calculate value scaled to uV
			intToFix(scaledVal+ch, rawData[ch]);
			if(daq.channel[i].range == V5){
				fix_sub(scaledVal+ch, &daq.channel[i].v5_zero_offset);
				fix_mult(scaledVal+ch, &daq.channel[i].v5_uV_per_LSB);
			} else {
				fix_sub(scaledVal+ch, &daq.channel[i].v24_zero_offset);
				fix_mult(scaledVal+ch, &daq.channel[i].v24_uV_per_LSB);
			}
			// Scale uV to [units] * 1000000, ignoring user scale exponent
			fix_sub(scaledVal+ch, &daq.channel[i].offset_uV);
			fix_mult(scaledVal+ch, &daq.channel[i].units_per_volt);
			scaledVal[ch].exp = daq.channel[i].units_per_volt.exp - 6; // account for uV to V conversion
			ch++;
		}
	}

	// Format time
	// 424cc + 77cc / seconds digit (5.9us + 1us / digit) for precision 4
	sampleStr_size += usToStr(sampleStr+sampleStr_size, us, daq.time_res);

	// Fast formatting from fixed-point samples
	for(i=0;i<daq.channel_count;i++){
		/* Format and append sample string */
		sampleStr[sampleStr_size++] = ',';
		// Takes 600cc (8.3us) for precision 4
		sampleStr_size += decFloatToStr(sampleStr+sampleStr_size, scaledVal+i, 4); // Precision = 4
	}
	// 14cc each
	sampleStr[sampleStr_size++] = '\n';
	sampleStr[sampleStr_size++] = '\0';
}

// Limit configuration values to valid ranges
void daq_configCheck(void){
	// Count enabled channels
	int i;
	daq.channel_count = 0;
	for(i=0;i<MAX_CHAN;i++){
		if (daq.channel[i].enable){
			daq.channel_count++;
		}
	}

	// Force sample rate to be in the set [1,2,5]*10^k
	daq.sample_rate = clamp(daq.sample_rate, 1, 10000);
	uint32_t mag = 1;
	while(mag < daq.sample_rate){
		if(daq.sample_rate <= mag * 2){
			daq.sample_rate = mag * 2;
		}else if(daq.sample_rate <= mag * 5){
			daq.sample_rate = mag * 5;
		}else if(daq.sample_rate <= mag * 10){
			daq.sample_rate = mag * 10;
		}
		mag *= 10;
	}

	// Determine time resolution required
	daq.time_res = 0;
	mag = 1;
	while(mag < daq.sample_rate){
		daq.time_res++;
		mag *= 10;
	}

	// Limit output voltage to the range 5-24v
	daq.mv_out = clamp(daq.mv_out, 5000, 24000);
}

// Set channel configuration from the config file on the SD card
void daq_configFromFile(void){
	/* Read SD card config file
	 * Parse Data
	 * Set Channel_Config
	 * Set sample_rate
	 * Set mv_out
	 */

	FIL config;		/* File Object */
	char line[100]; /* Line Buffer */
	char units[8];	/* Unit String */
	FRESULT fr;
	int i = 0;
	int iVal = 0;
	float fVal = 0;

	fr = f_open(&config, "config.txt", FA_READ);
	if (fr) {
		/* Something went wrong opening file, throw errors */
	}
	/* Read 4 lines to get to update Date/Time */
	for (i = 0; i < 4; i++) {
		f_gets(line, sizeof(line), &config);
	}

	if (line[0] == 'Y') {
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
	if (line[0] == 'Y') {
		/* Update Config - 32 Lines (Maybe) */
		f_gets(line, sizeof(line), &config);
		f_gets(line, sizeof(line), &config);
		/* Line is now the user header */
		daq.user_comment = line;
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
		/* Set Delay? */
		f_gets(line, sizeof(line), &config);
		f_gets(line, sizeof(line), &config);
		/* Line is now data mode */
		if (line[0] == 'R') {
			daq.data_mode = READABLE;
		} else if (line[0] == 'H') {
			daq.data_mode = HEX;
		} else if (line[0] == 'B') {
			daq.data_mode = BINARY;
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
			sscanf(line, "UNITS [up to 8 chars]: %s", units);
			daq.channel[i].unit_name = units;
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

// Set channel configuration defaults
void daq_configDefault(void){
	int i;

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
	daq.sample_rate = 1000;

	// Trigger Delay in seconds
	daq.trigger_delay = 0;

	// Data mode can be READABLE, HEX, or BINARY
	daq.data_mode = READABLE;

	// Vout = 5v
	daq.mv_out = 5000;

	// User comment string
	strcpy(daq.user_comment, "User header comment");
}
