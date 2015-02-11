#include "daq.h"

// Output voltage
uint32_t mv_out; // valid_range = <5000..24000>

// Sample rate
uint32_t sample_rate; // valid range = <1..10000>

Channel_Config channel_config[3];

// Vout PWM interrupt
void SCT0_IRQHandler(void){
	/* Run PID control loop
	 * Set new PWM value
	 */
}

// Sample timer interrupt
void RIT_IRQHandler(void){
	/* Read samples according to config
	 * Read current time as RTC start time + (sample_interrupt_counter * sample_interval)
	 * Format samples according to config
	 * Write samples and time stamp to disk buffer
	 */
}

// Start acquiring data
void daq_init(void){
	int i;

	// Read config
	daq_config_default();

	// Set up sampling interrupt using RIT
	Chip_RIT_Init(LPC_RITIMER);
	Chip_RIT_SetTimerIntervalHz(LPC_RITIMER, sample_rate);
	Chip_RIT_Enable(LPC_RITIMER);

	NVIC_EnableIRQ(RITIMER_IRQn);
	NVIC_SetPriority(RITIMER_IRQn, 0x00); // Set to highest priority to ensure sample timing accuracy

	// Set up channel ranges in hardware mux
	for(i=0;i<3;i++){
		Chip_GPIO_SetPinState(LPC_GPIO, 0, rsel_pins[i], channel_config[i].range);
	}

	// Enable Vout using ~SHDN
	Chip_GPIO_SetPinState(LPC_GPIO, 0, VOUT_N_SHDN, true);

	// Set up Vout PWM
	Chip_SCTPWM_Init(LPC_SCT0);
	Chip_SCTPWM_SetRate(LPC_SCT0, 7200); // 7200Hz
	Chip_SWM_MovablePinAssign(SWM_SCT0_OUT0_O, VOUT_PWM); // Assign PWM to output pin
	Chip_SCTPWM_SetOutPin(LPC_SCT0, 1, 0); // Set SCT PWM output 1 to SCT pin output 0
	Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 1, 0); // Set to duty cycle of 0
	Chip_SCTPWM_Start(LPC_SCT0); // Start PWM

	// Create SCT0 Vout PWM interrupt
	NVIC_EnableIRQ(SCT0_IRQn);
	NVIC_SetPriority(RITIMER_IRQn, 0x01); // Set to lower priority that sampling

	// Delay 200ms to allow system to stabilize
	DWT_Delay(200000);
}

// Stop acquiring data
void daq_stop(void){
	/* Stop RIT interrupt
	 * Write all buffered data to disk
	 * Stop Vout interrupt
	 * Disable Vout using ~SHDN
	 */
}

// Set channel configuration from the config file on the SD card
void daq_config_from_file(void){
	/* Read SD card config file
	 * Parse Data
	 * Set Channel_Config
	 * Set sample_rate
	 * Set mv_out
	 */
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
	sample_rate = 10;

	// Vout = 5v
	mv_out = 5000;
}
