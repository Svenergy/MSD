#ifndef __DAQ_
#define __DAQ_

#include <string.h>

#include "board.h"
#include "adc_spi.h"
#include "delay.h"

// Voltage range type
typedef enum {
	V5,
	V24,
} VRANGE_T;

// Configuration data for each channel
typedef struct Channel_Config {

// Configured for each use case in config file
	bool enable;			// enable / disable channel
	VRANGE_T range;			// select input voltage range
	float units_per_volt;	// sensitivity of sensor in units / volt
	char unit_name[8];		// name of channel unit

// Configured by setting the calibration flag in the config file, then running a calibration cycle. Backed up in device eeprom
	// volts = (raw_val - v5_zero_offset) / v5_LSB_per_volt
	float v5_zero_offset;	// value of raw 16-bit sample for 0 input voltage
	float v5_LSB_per_volt;	// sensitivity of reading in LSB / volt

	// volts = (raw_val - v24_zero_offset) / v24_LSB_per_volt
	float v24_zero_offset;	// value of raw 16-bit sample for 0 input voltage
	float v24_LSB_per_volt;	// sensitivity of reading in LSB / volt
} Channel_Config;

extern uint8_t *rsel_pins;

// Start acquiring data
void daq_init(void);

// Stop acquiring data
void daq_stop(void);

// Set channel configuration from the config file on the SD card
void daq_config_from_file(void);

// Set channel configuration defaults
void daq_config_default(void);

#endif /* __DAQ_ */
