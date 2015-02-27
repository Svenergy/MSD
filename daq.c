#include "daq.h"

// DAQ configuration data
DAQ daq;

// FatFS file object
FIL dataFile;

// Buffer used for string formatted data
RingBuffer *strBuff;

// Time tracking
static volatile uint32_t sampleCount; // Count of samples taken in the current recording
static uint32_t sampleStrfCount; // Count of samples string formatted in the current recording
static volatile uint32_t startTime; // Absolute start time in seconds
static volatile uint32_t dwt_lastTime; // Time of the last sample according to the DWT timer, used to measure sampling integral error and jitter
static volatile int64_t dwt_elapsedTime; // Total sampling elapsed time according to the DWT timer

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
#ifdef DEBUG
	uint32_t start_time = DWT_Get();
#endif
    adc_read(voutCFG);
#ifdef DEBUG
	uint32_t elapsed_time = DWT_Get() - start_time;
#endif
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

#ifdef DEBUG
    char b[32];
    sprintf(b,"%d\n", elapsed_time);
    putLineUART(b);
#endif
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

	// Read current time with microsecond precision
	int64_t microseconds = ((int64_t)sampleCount * 1000000 ) / daq.sample_rate;

	// Compare to DWT timer
	dwt_elapsedTime += dwt_currentTime - dwt_lastTime;
	dwt_lastTime = dwt_currentTime;
	int32_t dT = microseconds - dwt_elapsedTime/72;

	// Error if sample timing is off by > 50us
	if(dT > 3600 || dT < -3600){
		error(ERROR_SAMPLE_TIME);
	}

	RingBuffer_writeData(rawBuff, rawVal, daq.channel_count*2); // 16 bit samples = 2bytes/sample

	// Increment sample counter
	sampleCount++;
}

// Start acquiring data
void daq_init(void)
{
	log_string("Acquisition start");

	// Read config
	daq_configDefault();

	// Limit config values to valid values
	daq_configCheck();

	// Set up ADC
	adc_spi_setup();

	putLineUART("here 0\n");

	// Set up channel ranges in hardware mux
#ifndef DEBUG
	int i;
	for(i=0;i<3;i++){ // This Kills the UART
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, rsel_pins[i]);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, rsel_pins[i], daq.channel[i].range);
	}
#endif

	// Enable Vout using ~SHDN
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, VOUT_N_SHDN);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, VOUT_N_SHDN, true);

	putLineUART("here 1\n");

	// Set up Vout PWM
	Chip_SCTPWM_Init(LPC_SCT0);
	Chip_SCTPWM_SetRate(LPC_SCT0, 7200); // 7200Hz, 10000 counts per cycle
	Chip_SWM_MovablePinAssign(SWM_SCT0_OUT0_O, VOUT_PWM); // Assign PWM to output pin
	Chip_SCTPWM_SetOutPin(LPC_SCT0, 1, 0); // Set SCT PWM output 1 to SCT pin output 0
	Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 1, 0); // Set to duty cycle of 0
	Chip_SCTPWM_Start(LPC_SCT0); // Start PWM

	putLineUART("here 2\n");

	// Create SCT0 Vout PWM interrupt
	Chip_SCT_EnableEventInt(LPC_SCT0, SCT_EVT_0);
	NVIC_EnableIRQ(SCT0_IRQn);
	NVIC_SetPriority(SCT0_IRQn, 0x00); // Set to higher priority than sampling

	putLineUART("here 3\n");

	// Delay 200ms to allow system to stabilize
	DWT_Delay(200000);

	// Write data file header
	daq_header();

	// Clear the raw data buffer
	RingBuffer_clear(rawBuff);

	// Initialize the string formatted buffer
	strBuff = RingBuffer_init(BLOCK_SIZE + SAMPLE_STR_SIZE);

	// 0 the sample count
	sampleCount = 0;
	sampleStrfCount = 0;

	// Get actual start time from RTC
	startTime = 0; //Chip_RTC_GetCount(LPC_RTC);

	// Start time according to DWT timer
	dwt_lastTime = DWT_Get();
	dwt_elapsedTime = -(SystemCoreClock / daq.sample_rate);

	// Set up sampling interrupt using RIT
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
}

// Write data file header
void daq_header(void){
	uint8_t i;
	char headerStr[80];
	uint8_t headerStr_size = 0;

	/* Make the data file with time-stamped file name */
	time_t t = Chip_RTC_GetCount(LPC_RTC);
	struct tm * tm;
	tm = localtime(&t);
	char fn[40];
	strftime (fn,40,"%Y-%m-%d_%H-%M-%S_data.txt",tm);
	f_open(&dataFile,fn,FA_CREATE_ALWAYS | FA_WRITE);

	/* User comment */
	// Ex. "User header comment"
#if defined(DEBUG) && defined(PRINT_DATA_UART)
	putLineUART(daq.user_comment);
#endif
	// Write string to file buffer
	f_puts(daq.user_comment,&dataFile);

	/* Channel labels */
	// Ex. "time, ch1, ch2, ch3"
	headerStr_size = sprintf(headerStr, "\ntime");
	for(i=0;i<3;i++){
		if(daq.channel[i].enable){ // Only print enabled channels
			headerStr_size += sprintf(headerStr+headerStr_size, ", ch%d", i+1);
		}
	}
#if defined(DEBUG) && defined(PRINT_DATA_UART)
	putLineUART(headerStr);
#endif
	// Write string to file buffer
	f_puts(headerStr, &dataFile);

	/* Units */
	// Ex. "seconds, volts, volts, volts"
	headerStr_size = sprintf(headerStr, "\nseconds");
	for(i=0;i<3;i++){
		if(daq.channel[i].enable){ // Only print enabled channels
			headerStr_size += sprintf(headerStr+headerStr_size, ", %s", daq.channel[i].unit_name);
		}
	}
	headerStr_size += sprintf(headerStr+headerStr_size, "\n");
#if defined(DEBUG) && defined(PRINT_DATA_UART)
	putLineUART(headerStr);
#endif
	// Write string to file buffer
	f_puts(headerStr, &dataFile);
}

// Stop acquiring data
void daq_stop(void){
	// Stop RIT interrupt
	Chip_RIT_DeInit(LPC_RITIMER);
	NVIC_DisableIRQ(RITIMER_IRQn);

	// Stop Vout interrupt
	NVIC_DisableIRQ(SCT0_IRQn);

	// Disable Vout using ~SHDN
	Chip_GPIO_SetPinState(LPC_GPIO, 0, VOUT_N_SHDN, false);

	log_string("Acquisition stop");

	// flush ring buffer to disk
	daq_writeData();
	daq_writeBlock(); // Write any partial block remaining

	// Destroy the string formatted buffer
	RingBuffer_destroy(strBuff);

	// Write all buffered data to disk
	f_close(&dataFile);
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

// Write data from raw buffer to file, formatting to string  buffer as an intermediate step
// Stop when the raw buffer is empty
void daq_writeData(void){
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
				daq_stringFormat(rawData, sampleStr);
				RingBuffer_writeStr(strBuff, sampleStr);
#if defined(DEBUG) && defined(PRINT_DATA_UART)
				putLineUART(sampleStr);
#endif
			}
		}
		daq_writeBlock();
	}
}

// Convert rawData into a formatted output string
void daq_stringFormat(uint16_t *rawData, char *sampleStr){
	// sampleStr Ex. 9999.123400, 1.23456E+01, 1.23456E+01, 1.23456E+01
	int8_t sampleStr_size = 0;

	float scaledVal[MAX_CHAN];

	// Calculate sample time with microsecond precision
	int64_t microseconds = ((int64_t)sampleStrfCount++ * 1000000 ) / daq.sample_rate;

	uint32_t seconds = startTime + microseconds / 1000000;
	microseconds = microseconds % 1000000;

	/* Scale samples , takes 1260cc (17.5us) */
	uint8_t ch = 0;
	int8_t i;
	for(i=0;i<MAX_CHAN;i++){
		if(daq.channel[i].enable){ // Only scale enabled channels
			// Calculate value scaled to volts
			if(daq.channel[i].range == V5){
				scaledVal[ch] = (rawData[ch] - daq.channel[i].v5_zero_offset) / daq.channel[i].v5_LSB_per_volt;
				scaledVal[ch] = clamp(scaledVal[ch], 0.0, 5.0);
			} else {
				scaledVal[ch] = (rawData[ch] - daq.channel[i].v24_zero_offset) / daq.channel[i].v24_LSB_per_volt;
				scaledVal[ch] = clamp(scaledVal[ch], -24.0, 24.0);
			}
			// Scale volts to [units]
			scaledVal[ch] *= daq.channel[i].units_per_volt;
			ch++;
		}
	}

    /* String formatting takes 45000cc (625us) */
	// Format time in output string
	sampleStr_size += sprintf(sampleStr+sampleStr_size, "%u.%04u", seconds, (uint32_t)microseconds/100); // sprintf does not work with 64-bit ints

	// Format samples in output string
	for(i=0;i<daq.channel_count;i++){
		// Format and append sample string to output string
		sampleStr_size += sprintf(sampleStr+sampleStr_size, ", %.5E", scaledVal[i]);
	}
	sampleStr_size += sprintf(sampleStr+sampleStr_size, "\n");
}

// Limit configuration values to valid ranges
void daq_configCheck(void){
	daq.sample_rate = clamp(daq.sample_rate, 1, 10000);

	// Force sample rate to be in the set [1,2,5]*10^k
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

	//TODO: read daq config from sd card
}

// Set channel configuration defaults
void daq_configDefault(void){
	int i;

	// Channel_Config defaults
	for(i=0;i<MAX_CHAN;i++){
		daq.channel[i].enable = false;				// enable channel
		daq.channel[i].range = V24;					// 0-5v input range
		daq.channel[i].units_per_volt = 1.0;		// output in volts
		strcpy(daq.channel[i].unit_name, "Volts");	// name of channel units

		daq.channel[i].v5_zero_offset = 0.0;		// theoretical value of raw 16-bit sample for 0 input voltage
		daq.channel[i].v5_LSB_per_volt = 12812.75;	// theoretical sensitivity of reading in LSB / volt = (1 << 16) / (4.096 * ( (402+100) / 402 ))
		daq.channel[i].v24_zero_offset = 32511.13;	// theoretical value of raw 16-bit sample for 0 input voltage = (1 << 16) * ( (1/(1/100+1/402+1/21)) / (1/(1/100+1/402+1/21) + 16.9) )
		daq.channel[i].v24_LSB_per_volt = 1341.402;	// theoretical sensitivity of reading in LSB / volt = ((1 << 16)/4.096) * (1/(1/100+1/402+1/21+1/16.9)) / 100
	}
	daq.channel[2].enable = true;

	// Count enabled channels
	daq.channel_count = 0;
	for(i=0;i<MAX_CHAN;i++){
		if (daq.channel[i].enable){
			daq.channel_count++;
		}
	}

	// Sample rate in Hz
	daq.sample_rate = 10;

	// Vout = 5v
	daq.mv_out = 5000;

	// User comment string
	strcpy(daq.user_comment, "User header comment");
}
