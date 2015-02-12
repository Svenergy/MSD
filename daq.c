#include "daq.h"

// Output voltage
uint32_t mv_out; // valid_range = <5000..24000>

// Sample rate
uint32_t sampleRate; // valid range = <1..10000>

Channel_Config channel_config[3];

uint32_t sampleCount; // Count of samples taken in the current recording

uint32_t startTime; // Absolute start time in seconds

// Vout PWM interrupt
void SCT0_IRQHandler(void){
	/* Run PI control loop
	 * Set new PWM value
	 */

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
    propError =  mv_out - 0.375 * adc_read(0); // Units are mv

    intError += propError; // Units are mv * ms * 7.2, or giving a rate of 7.2e6/(v*s)

    // Integral Error Saturation
    intError = clamp(intError, 0, 14400000L); // 14.4E6 means saturation at 2[v*s]

    // Proportional Error Saturation, low to reduce overshoot and decrease integral settling time for large steps
    propError = clamp(propError, -1000, 1000); // Saturation at +/- 1v

    // Calculate PWM output value out of 10000
    pwmOut = intError / 1440 + propError / 2;
    pwmOut = clamp(pwmOut, 0, 9999);

    // Set PWM output
    Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 1, pwmOut);
}

// Sample timer interrupt
void RIT_IRQHandler(void){
	/* Read samples according to config
	 * Read current time as RTC start time + (sample_interrupt_counter * sample_interval)
	 * Format samples according to config
	 * Write samples and time stamp to disk buffer
	 */
	uint32_t i, j;
	uint16_t rawVal[3];

	char sampleStr[80]; // 9999.9999s, 1.23456E+1[units ], 1.23456E+1[units ], 1.23456E+1[units ]
	uint8_t sampleStr_size = 0;

	float scaledVal;

	uint32_t seconds;
	uint64_t microseconds;

	if(sampleRate > 1000){
		// Set ADC config
		uint16_t sampleCFG = (1 << ADC_CFG ) | // Overwrite config
							 (6 << ADC_INCC) | // Unipolar, referenced to COM
							 (2 << ADC_IN)   | // Sequence channels 0,1,2
							 (1 << ADC_BW)   | // Full bandwidth
							 (1 << ADC_REF)  | // Internal reference output 4.096v
							 (3 << ADC_SEQ)  | // Channel sequencer enabled
							 (1 << ADC_RB);    // Do not read back config
		adc_read(sampleCFG);
		adc_read(0); // Buffering read, next read returns sample from channel 0

		// Read all channels
		for(i=0;i<3;i++){
			rawVal[i] = adc_read(0);
		}

	} else { // Average a bunch of samples for each channel when recording at slow rates for better noise rejection
		// Read all enabled channels
		for(i=0;i<3;i++){
			if(channel_config[i].enable){
				// Set ADC config
				uint16_t sampleCFG = (1 << ADC_CFG ) | // Overwrite config
									 (6 << ADC_INCC) | // Unipolar, referenced to COM
									 (i << ADC_IN)   | // Read channel i
									 (1 << ADC_BW)   | // Full bandwidth
									 (1 << ADC_REF)  | // Internal reference output 4.096v
									 (0 << ADC_SEQ)  | // Channel sequencer disabled
									 (1 << ADC_RB);    // Do not read back config
				adc_read(sampleCFG);
				adc_read(0); // Buffering read, next read returns sample from channel 0

				uint16_t count = 10000/sampleRate;
				uint32_t sum = 0;
				for(j=0;j<count;j++){
					sum += adc_read(0);
				}
				rawVal[i] = sum / count;
			}
		}
	}

	// Read current time with microsecond precision
	microseconds = ((uint64_t)sampleCount * 1000000 ) / sampleRate;
	seconds = startTime + microseconds / 1000000;
	microseconds = microseconds % 1000000;

	// Format time in output string
	sampleStr_size += sprintf(sampleStr+sampleStr_size, "\n%u.%06us", seconds, microseconds);

	// Format each sample into the output string
	for(i=0;i<3;i++){
		if(channel_config[i].enable){ // Only print enabled channels

			// Calculate value scaled to units
			if(channel_config[i].range == V5){
				scaledVal = channel_config[i].units_per_volt * (rawVal[i] - channel_config[i].v5_zero_offset) / channel_config[i].v5_LSB_per_volt;
			} else {
				scaledVal = channel_config[i].units_per_volt * (rawVal[i] - channel_config[i].v24_zero_offset) / channel_config[i].v24_LSB_per_volt;
			}

			// Format and append sample string to output string
			sampleStr_size += sprintf(sampleStr+sampleStr_size, ", %.5E%s", scaledVal, channel_config[i].unit_name);
		}
	}

#ifdef DEBUG
	putLineUART(sampleStr);
#endif

	// TODO: actually write data to file buffer
}

// Start acquiring data
void daq_init(void){
	int i;

	// Read config
	daq_config_default();

	// Set up ADC
	adc_spi_setup();

	// Set up channel ranges in hardware mux
	for(i=0;i<3;i++){
		Chip_GPIO_SetPinState(LPC_GPIO, 0, rsel_pins[i], channel_config[i].range);
	}

	// Enable Vout using ~SHDN
	Chip_GPIO_SetPinState(LPC_GPIO, 0, VOUT_N_SHDN, true);

	// Set up Vout PWM
	Chip_SCTPWM_Init(LPC_SCT0);
	Chip_SCTPWM_SetRate(LPC_SCT0, 7200); // 7200Hz, 10000 counts per cycle
	Chip_SWM_MovablePinAssign(SWM_SCT0_OUT0_O, VOUT_PWM); // Assign PWM to output pin
	Chip_SCTPWM_SetOutPin(LPC_SCT0, 1, 0); // Set SCT PWM output 1 to SCT pin output 0
	Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 1, 0); // Set to duty cycle of 0
	Chip_SCTPWM_Start(LPC_SCT0); // Start PWM

	// Create SCT0 Vout PWM interrupt
	NVIC_EnableIRQ(SCT0_IRQn);
	NVIC_SetPriority(SCT0_IRQn, 0x01); // Set to lower priority than sampling

	// Delay 200ms to allow system to stabilize
	DWT_Delay(200000);

	// 0 the sample count
	sampleCount = 0;

	// TODO: get actual start time from RTC
	startTime = 0;

	// Set up sampling interrupt using RIT
	Chip_RIT_Init(LPC_RITIMER);
	Chip_RIT_SetTimerIntervalHz(LPC_RITIMER, sampleRate);
	Chip_RIT_Enable(LPC_RITIMER);

	NVIC_EnableIRQ(RITIMER_IRQn);
	NVIC_SetPriority(RITIMER_IRQn, 0x00); // Set to highest priority to ensure sample timing accuracy
}

// Stop acquiring data
void daq_stop(void){
	// Stop RIT interrupt
	NVIC_DisableIRQ(RITIMER_IRQn);

	// Stop Vout interrupt
	NVIC_DisableIRQ(SCT0_IRQn);

	// Disable Vout using ~SHDN
	Chip_GPIO_SetPinState(LPC_GPIO, 0, VOUT_N_SHDN, false);

	// TODO: Write all buffered data to disk

}

// Set channel configuration from the config file on the SD card
void daq_config_from_file(void){
	/* Read SD card config file
	 * Parse Data
	 * Set Channel_Config
	 * Set sample_rate
	 * Set mv_out
	 */

	//TODO: read config from sd card
}

// Set channel configuration defaults
void daq_config_default(void){
	int i;

	// Channel_Config defaults
	for(i=0;i<3;i++){
		channel_config[i].enable = true;			// enable channel
		channel_config[i].range = V5;				// 0-5v input range
		channel_config[i].units_per_volt = 1.0;		// output in volts
		strcpy(channel_config[i].unit_name, "V");	// name of channel units

		channel_config[i].v5_zero_offset = 0.0;			// theoretical value of raw 16-bit sample for 0 input voltage
		channel_config[i].v5_LSB_per_volt = 12812.749;	// theoretical sensitivity of reading in LSB / volt = (1 << 16) / (4.096 * ( (402+100) / 402 ))
		channel_config[i].v24_zero_offset = 32511.134;	// theoretical value of raw 16-bit sample for 0 input voltage = (1 << 16) * ( (1/(1/100+1/402+1/21)) / (1/(1/100+1/402+1/21) + 16.9) )
		channel_config[i].v24_LSB_per_volt = 1341.402;	// theoretical sensitivity of reading in LSB / volt = ((1 << 16)/4.096) * (1/(1/100+1/402+1/21+1/16.9)) / 100
	}

	// Sample rate 10Hz
	sampleRate = 10;

	// Vout = 5v
	mv_out = 5000;
}
