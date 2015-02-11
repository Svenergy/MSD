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
	 * Write samples and timestamp to disk buffer
	 */
}

// Start acquiring data
void daq_init(void){
	/* Call read config
	 * Set up sample interrupt using RIT
	 * Set up channel ranges
	 * Enable Vout using ~SHDN
	 * Set up vout PWM and SCT0 interrupt
	 * Delay 200ms to allow system to stabilize
	 */
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
	/*
	 * Set Channel_Config defaults
	 * Set sample_rate
	 * Set mv_out
	 */
}
