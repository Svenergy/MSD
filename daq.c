#include "daq.h"

// Data type strings
static const char* const dataType[] = {
	"READABLE",
	"BINARY"
};

// Buffer used for string formatted data
RingBuffer *strBuff;

// FatFS file object
FIL dataFile;

// DAQ configuration data
DAQ daq;

// DAQ loop function
void (*daq_loop)(void);

// Time tracking
static volatile uint32_t sampleCount; // Count of samples taken in the current recording, used for timing verification
static uint32_t sampleStrfCount; // Count of samples string formatted in the current recording
static volatile uint32_t dwt_lastTime; // Time of the last sample according to the DWT timer, used to measure sampling integral error and jitter
static volatile uint64_t dwt_elapsedTime; // Total sampling elapsed time according to the DWT timer
static uint32_t buttonTime; // Time that the record button was pressed, used for trigger delay

// Sampling
static uint32_t rawValSum[MAX_CHAN]; // Raw sample values, summed over the number of over-samples
static uint32_t MRTCount; // Count of runs of the MRT1 timer interrupt
static uint32_t subSampleCount; // Count of over samples

// Vout raw value read from ADC
static volatile uint16_t rawVout;

// Flag set when data recording starts
static volatile bool recordData;

// Vout PWM
// Takes 195cc (2.7us). At 10000Hz, takes 2.7% of cpu time
void daq_updateVout(void){
    static int32_t intError;
    int32_t propError, pwmOut;

    // Theoretical mv / LSB = 1000 * ((100+20)/20) * 4.096 / (1 << 16) = 0.375
    propError =  daq.mv_out - (3 * rawVout) / 8; // Units are mv

    intError += propError; // Units are mv * ms * 10, or giving a rate of 10e6/(v*s)

    // Integral Error Saturation
    intError = clamp(intError, -100000, 100000); // 10E5 means saturation at 10[mv*s]

    // Proportional Error Saturation, low to reduce overshoot and decrease integral settling time for large steps
    propError = clamp(propError, -350, 350); // Saturation at +/- 1v

    // Calculate PWM output value out of 7200
    // Theoretical Duty = mv_out * 7.2 / ((3.3*39 / (39+51.7)) * (3.48E6/182E3 + 1)), = 0.25218
    pwmOut = (daq.mv_out * 25218) / 100000 + intError / 400 + 2 * propError;
    pwmOut = clamp(pwmOut, 0, SYS_CLOCK_RATE/VOUT_PWM_RATE - 1);

    // Set PWM output
    Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 1, pwmOut);
}

// Sample timer
// 268cc no save, 701cc when saving to buffer
// 484cc total cpu time per 3-channel sample not writing to ring buffer, 917cc when writing to buffer
void RIT_IRQHandler(void){
	Chip_RIT_ClearIntStatus(LPC_RITIMER);

	/* Check sample time against DWT timer, 97cc */
	uint32_t dwt_currentTime = DWT_Get();
	dwt_elapsedTime += dwt_currentTime - dwt_lastTime;
	dwt_lastTime = dwt_currentTime;

	// Read current target sample time in clock cycles, increment sample counter
	uint64_t cc = (uint64_t)++sampleCount * (SYS_CLOCK_RATE / CONVERSION_RATE);

	// Compare to DWT time
	int32_t dT = cc - dwt_elapsedTime;

	// Error if sample timing is off by more than one conversion period
	if(dT > (SYS_CLOCK_RATE/CONVERSION_RATE) || dT < -(SYS_CLOCK_RATE/CONVERSION_RATE)){
		error(ERROR_SAMPLE_TIME);
	}

	/* Save data to the ring buffer for enabled channels after all sub-samples have been collected */
	if (subSampleCount == daq.subsamples){
		if(recordData){ // Only record data after recordData has been set true
			uint8_t i = 0;
			uint8_t ch = 0;
			uint16_t rawVal[MAX_CHAN];
			for(i=0;i<MAX_CHAN;i++){
				if(daq.channel[i].enable){
					rawVal[ch++] = (uint16_t) (rawValSum[i] / daq.subsamples);
				}
			}
			RingBuffer_writeData(rawBuff, &rawVal, 2*daq.channel_count); // 16 bit samples = 2bytes/sample
		}
		subSampleCount = 0;
	}

	/* Clear sub sample sums */
	if (subSampleCount == 0){
		uint8_t i = 0;
		for(i=0;i<MAX_CHAN;i++){
			rawValSum[i] = 0;
		}
	}

	// Reset MRT count and set MRT1 timer for ADC_US us repeating
	MRTCount = 0;
	Chip_MRT_SetInterval(LPC_MRT_CH(1), (ADC_US * (SYS_CLOCK_RATE / 1000000)) | MRT_INTVAL_LOAD);

	// Read vout from the last conversion
	rawVout = LPC_SPI1->RXDAT;

	// Start conversion of ch1
	LPC_SPI1->TXDATCTL = SPI_TXDATCTL_LEN(16-1) | SPI_TXDATCTL_EOT | SPI_TXCTL_ASSERT_SSEL0;

	// Increment sub sample counter
	subSampleCount++;
}

// ADC sample timing interrupt, called from main MRT interrupt in system
// 51cc, 49cc, 116cc no update /333cc vout update
void MRT1_IRQHandler(void){
	// Read result of last conversion
	rawValSum[MRTCount++] += LPC_SPI1->RXDAT;

	// Start the next conversion
	LPC_SPI1->TXDATCTL = SPI_TXDATCTL_LEN(16-1) | SPI_TXDATCTL_EOT | SPI_TXCTL_ASSERT_SSEL0;

	if(MRTCount == MAX_CHAN){
		// Stop MRT1 timer, run control loop for vout
		Chip_MRT_SetInterval(LPC_MRT_CH(1), MRT_INTVAL_LOAD);
		Chip_MRT_IntClear(LPC_MRT_CH(1));

		// Update output value at the PWM frequency
		if(subSampleCount % (CONVERSION_RATE/VOUT_PWM_RATE) == 0){
			daq_updateVout();
		}
	}
}

// Set up daq
void daq_init(void){
	log_string("Acquisition Ready");
	Board_LED_Color(LED_PURPLE);

	// Limit config values to valid values
	daq_configCheck();

	// Set up ADC
	adc_spi_setup();

	// Set up channel ranges in hardware mux
	int i;
	for(i=0;i<3;i++){
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, rsel_pins[i]);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, rsel_pins[i], daq.channel[i].range);
	}

	// Clear the raw data buffer
	RingBuffer_clear(rawBuff);

	// Initialize the string formatted buffer if in readable mode
	if(daq.data_type == READABLE){
		strBuff = RingBuffer_init(BLOCK_SIZE + SAMPLE_STR_SIZE);
	}

	// 0 the sample counts
	sampleCount = 0;
	sampleStrfCount = 0;
	subSampleCount = 0;

	// Save button time for trigger delay
	buttonTime = Chip_RTC_GetCount(LPC_RTC);

	// Set loop to wait for trigger delay to expire
	daq_loop = daq_triggerDelay;

	// Start interrupts to stabilize vout, but do not record data
	recordData = false;

	// Turn on vout
	daq_voutEnable();

	// Set ADC config
	uint16_t adcCFG = (1 << ADC_CFG ) | // Overwrite config
					  (6 << ADC_INCC) | // Unipolar, referenced to COM
					  (MAX_CHAN << ADC_IN) | // Sequence channels 0,1.. (MAX_CHAN), include vout sense
					  (1 << ADC_BW)   | // Full bandwidth
					  (1 << ADC_REF)  | // Internal reference output 4.096v
					  (3 << ADC_SEQ)  | // Channel sequencer enabled
					  (1 << ADC_RB);    // Do not read back config
	adc_SPI_Transfer(adcCFG);
	adc_SPI_Transfer(0);

	// Start first conversion
	while(~LPC_SPI1->STAT & SPI_STAT_TXRDY){};
	LPC_SPI1->TXDATCTL = SPI_TXDATCTL_LEN(16-1) | SPI_TXDATCTL_EOT | SPI_TXCTL_ASSERT_SSEL0;

	// Start time according to DWT timer
	dwt_lastTime = DWT_Get();
	dwt_elapsedTime = 0;

	// Set up sampling interrupt using RITimer
	Chip_RIT_Init(LPC_RITIMER);

	/* Set timer compare value and periodic mode */
	// Do not use Chip_RIT_SetTimerIntervalHz, for timing critical operations, it has an off by 1 error on the period in clock cycles
	uint64_t cmp_value = SystemCoreClock / CONVERSION_RATE - 1;
	Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);
	Chip_RIT_EnableCompClear(LPC_RITIMER);

	Chip_RIT_Enable(LPC_RITIMER);
	Chip_RIT_ClearIntStatus(LPC_RITIMER);

	NVIC_EnableIRQ(RITIMER_IRQn);
	NVIC_SetPriority(RITIMER_IRQn, 0x00); // Set to highest priority to ensure sample timing accuracy

	// Set up MRT timer interrupt
	Chip_MRT_IntClear(LPC_MRT_CH(1));
	Chip_MRT_SetEnabled(LPC_MRT_CH(1));
	Chip_MRT_SetMode(LPC_MRT_CH(1), MRT_MODE_REPEAT);

	// Delay 200ms at minimum to allow power to stabilize
	DWT_Delay(200000);
}

// Start acquiring data
void daq_record(){
	log_string("Acquisition Start");
	Board_LED_Color(LED_RED);

	// Make the data file
	daq_makeDataFile();

	// Write data file header
	daq_header();

	// Set loop to write data from buffer to file
	daq_loop = daq_writeData;

	// Begin recording data in RIT interrupt
	recordData = true;
}

// Make the data file
void daq_makeDataFile(void){
	time_t t = Chip_RTC_GetCount(LPC_RTC);
	struct tm * tm;
	tm = localtime(&t);
	char fn[40];
	uint8_t fn_size = 0;
	fn_size += strftime(fn,40,"%Y-%m-%d_%H-%M-%S_data",tm);
	if(daq.data_type == READABLE){
		strcat(fn+fn_size,".txt");
	}else{
		strcat(fn+fn_size,".dat");
	}
	f_open(&dataFile,fn,FA_CREATE_ALWAYS | FA_WRITE);
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
	// TODO: Check if this also has an off-by-one error
	Chip_SCTPWM_SetRate(LPC_SCT0, VOUT_PWM_RATE); // 10000Hz, 7200 counts per cycle
	Chip_SWM_MovablePinAssign(SWM_SCT0_OUT0_O, VOUT_PWM); // Assign PWM to output pin
	Chip_SCTPWM_SetOutPin(LPC_SCT0, 1, 0); // Set SCT PWM output 1 to SCT pin output 0
	Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 1, 0); // Set to duty cycle of 0
	Chip_SCTPWM_Start(LPC_SCT0); // Start PWM
}

// Disable the output voltage
void daq_voutDisable(void){
	// Disable Vout using ~SHDN
	Chip_GPIO_SetPinState(LPC_GPIO, 0, VOUT_N_SHDN, false);
}

// Write data file header
void daq_header(void){
	char hStr[1024];
	uint32_t hSize = 0;

	/**** Data type ****
	 * Ex.
	 * data type, BINARY
	 */
	hSize = sprintf(hStr, "data type, %s\n", dataType[daq.data_type]);

	/**** User comment ****
	 * Ex.
	 * comment, User header comment
	 */
	hSize += sprintf(hStr+hSize, "comment, %s\n", daq.user_comment);

	/**** Time Stamps ****
	 * Ex.
	 * date time, Mon Mar 02 20:02:43 2015
	 */
	hSize += sprintf(hStr+hSize, "date time, %s\n", getTimeStr());

	/**** Channel Scaling ****
	 * Ex.
	 * ch1, scale, 1.000000e+00, V/V, offset, 0.000000e+00, V, cscale, 7.454887e-04, V/LSB, coffset, 3.251113e+04, LSB
	 * ch2, scale, 1.000000e+00, V/V, offset, 0.000000e+00, V, cscale, 7.454887e-04, V/LSB, coffset, 3.251113e+04, LSB
	 * ch3, scale, 1.000000e+00, V/V, offset, 0.000000e+00, V, cscale, 7.454887e-04, V/LSB, coffset, 3.251113e+04, LSB
	 */
	uint8_t i;
	for(i=0;i<MAX_CHAN;i++){
		if(daq.channel[i].enable){
			hSize += sprintf(hStr+hSize, "ch%d, scale, ", i+1);
			hSize += fullDecFloatToStr(hStr+hSize, &daq.channel[i].units_per_volt, 6);
			hSize += sprintf(hStr+hSize, ", %s/V, offset, ", daq.channel[i].unit_name);
			hSize += fixToStr(hStr+hSize, &daq.channel[i].offset_uV, 6, -6);
			hSize += sprintf(hStr+hSize, ", V, ");
			fix64_t *v_per_LSB;
			fix64_t *off_LSB;
			if(daq.channel[i].range == V5){
				v_per_LSB = &daq.channel[i].v5_uV_per_LSB;
				off_LSB =	&daq.channel[i].v5_zero_offset;
			}else{
				v_per_LSB = &daq.channel[i].v24_uV_per_LSB;
				off_LSB =	&daq.channel[i].v24_zero_offset;
			}
			hSize += sprintf(hStr+hSize, "cscale, ");
			hSize += fixToStr(hStr+hSize, v_per_LSB, 6, -6);
			hSize += sprintf(hStr+hSize, ", V/LSB, coffset, ");
			hSize += fixToStr(hStr+hSize, off_LSB, 6, 0);
			hSize += sprintf(hStr+hSize, ", LSB\n");
		}
	}

	/**** Sample Rate ****
	 * Ex.
	 * sample rate, 1000, Hz
	 * sample period, 0.001, s
	 */
	hSize += sprintf(hStr+hSize, "sample rate, %d, Hz\n", daq.sample_rate);
	hSize += sprintf(hStr+hSize, "sample period, %.6f, s\n", 1.0 / daq.sample_rate);

	/**** End header ****
	 * Ex.
	 * end header
	 */
	hSize += sprintf(hStr+hSize, "end header\n");

	/**** Channel Labels ****
	 * Ex.
	 * time[s], ch1[V], ch1[V], ch1[V]
	 */
	if(daq.data_type == READABLE){
		hSize += sprintf(hStr+hSize, "time[s]");
		for(i=0;i<MAX_CHAN;i++){
			if(daq.channel[i].enable){
				hSize += sprintf(hStr+hSize, ", ch%d[%s]", i+1, daq.channel[i].unit_name);
			}
		}
		hSize += sprintf(hStr+hSize, "\n");
	}

	// Write data to file
	daq_writeBlock(hStr, hSize);

#if defined(DEBUG) && defined(PRINT_DATA_UART)
	putLineUART(hStr);
#endif
}

// Stop acquiring data
void daq_stop(void){
	// Stop RIT interrupt
	Chip_RIT_DeInit(LPC_RITIMER);
	NVIC_DisableIRQ(RITIMER_IRQn);

	// Stop MRT1 timer
	Chip_MRT_SetInterval(LPC_MRT_CH(1), MRT_INTVAL_LOAD);
	Chip_MRT_IntClear(LPC_MRT_CH(1));
	Chip_MRT_SetDisabled(LPC_MRT_CH(1));

	// Turn off output voltage
	daq_voutDisable();

	log_string("Acquisition Stop");

	if(daq_loop == daq_writeData){ // If data has been written
		// Flush data buffer to disk
		daq_flushData();

		// Write all buffered data to disk
		f_close(&dataFile);
	}

	// Destroy the string formatted buffer if it exists
	RingBuffer_destroy(strBuff);
}

// Write data from raw buffer to file, formatting to string  buffer as an intermediate step if needed
void daq_writeData(void){
	while(true){

		// Generate a block of file data, or return if a block cannot be made
		switch (daq.data_type){
		case READABLE:

			while(RingBuffer_getSize(strBuff) < BLOCK_SIZE){
				uint16_t rawData[MAX_CHAN];
				if(RingBuffer_read(rawBuff, rawData, daq.channel_count*2) == daq.channel_count*2){
					// Format data into string
					char sampleStr[SAMPLE_STR_SIZE];
					daq_readableFormat(rawData, sampleStr);
					RingBuffer_writeStr(strBuff, sampleStr);
#if defined(DEBUG) && defined(PRINT_DATA_UART)
					putLineUART(sampleStr);
#endif
				} else {
					return; // No more raw data, finished processing
				}
			}
			char data[BLOCK_SIZE];
			RingBuffer_read(strBuff, data, BLOCK_SIZE);
			daq_writeBlock(data, BLOCK_SIZE);

			break;
		case BINARY:

			if(RingBuffer_getSize(rawBuff) >= BLOCK_SIZE){
				char data[BLOCK_SIZE];
				RingBuffer_read(rawBuff, data, BLOCK_SIZE);
				daq_writeBlock(data, BLOCK_SIZE);
			} else {
				return;
			}

			break;
		}

	}
}

// Flush data from raw buffer to file, formatting to string  buffer as an intermediate step if needed
void daq_flushData(void){
	// Write full blocks of data to the file
	daq_writeData();

	// Flush remaining partial block
	char data[BLOCK_SIZE];
	int32_t br;
	switch (daq.data_type){
	case READABLE:
		br = RingBuffer_read(strBuff, data, BLOCK_SIZE);
		break;
	case BINARY:
		br = RingBuffer_read(rawBuff, data, BLOCK_SIZE);
		break;
	}
	daq_writeBlock(data, br);
}

// Write a single block to the data file
void daq_writeBlock(void *data, int32_t data_size){
	UINT bw;
	FRESULT errorCode;
	Board_LED_Color(LED_YELLOW);
	if((errorCode = f_write(&dataFile, data, data_size, &bw)) != FR_OK){
		error(ERROR_F_WRITE);
	}
	Board_LED_Color(LED_RED);
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
			intToFix((fix64_t*)(scaledVal+ch), rawData[ch]);

			if(daq.channel[i].range == V5){
				fix_sub((fix64_t*)(scaledVal+ch), &daq.channel[i].v5_zero_offset);
				fix_mult((fix64_t*)(scaledVal+ch), &daq.channel[i].v5_uV_per_LSB);
			} else {
				fix_sub((fix64_t*)(scaledVal+ch), &daq.channel[i].v24_zero_offset);
				fix_mult((fix64_t*)(scaledVal+ch), &daq.channel[i].v24_uV_per_LSB);
			}
			// Scale uV to [units] * 1000000, ignoring user scale exponent
			fix_sub((fix64_t*)(scaledVal+ch), &daq.channel[i].offset_uV);
			fix_mult((fix64_t*)(scaledVal+ch), (fix64_t*)&daq.channel[i].units_per_volt);
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
	daq.sample_rate = clamp(daq.sample_rate, 1, MAX_SAMPLE_RATE);
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

	// Set the number of subsamples
	daq.subsamples = CONVERSION_RATE / daq.sample_rate;

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
