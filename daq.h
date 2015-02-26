#ifndef __DAQ_
#define __DAQ_

#include <string.h>
#include <time.h>

#include "board.h"
#include "adc_spi.h"
#include "delay.h"
#include "adc_spi.h"
#include "ff.h"
#include "ring_buff.h"
#include "sys_error.h"
#include "log.h"

#define BLOCK_SIZE 512 // Size of blocks to write to the file system

#define clamp(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

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

// Configuration data for the entire DAQ
typedef struct DAQ {
	Channel_Config channel[3];
	int32_t mv_out;		// Output voltage in mv, valid_range = <5000..24000>
	int32_t sample_rate;	// Sample rate in Hz, valid range = <1..10000>
	char user_comment[80];	// User comment to appear at the top of each data file
} DAQ;

extern uint8_t rsel_pins[3];

// FatFS volume and file
extern FATFS *fatfs;
extern FIL dataFile;

// write buffer
extern struct RingBuffer *ringBuff;

// DAQ configuration data
extern DAQ daq;

// Start acquiring data
void daq_init(void);

// Write data file header
void daq_header(void);

// Stop acquiring data
void daq_stop(void);

// Write the ring buffer to disk in blocks
void daq_writeBuffer(void);

// Flush the ring buffer to disk
void daq_flushBuffer(void);

// Limit configuration values to valid ranges
void daq_configCheck(void);

// Set channel configuration from the config file on the SD card
void daq_configFromFile(void);

// Set channel configuration defaults
void daq_configDefault(void);

#endif /* __DAQ_ */
