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
#include "fixed.h"
#include "config.h"

#define MAX_CHAN 3 // Total count of available channels

#define BLOCK_SIZE 512 // Size of blocks to write to the file system

#define SAMPLE_STR_SIZE 60 // Maximum size of a single sample string

#define clamp(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

// Voltage range type
typedef enum {
	V5,
	V24,
} VRANGE_T;

// Data mode type
typedef enum {
	READABLE,
	HEX,
	BINARY
} DATA_T;

// Configuration data for each channel
typedef struct Channel_Config {

// Configured for each use case in config file
	bool enable;			// enable / disable channel
	VRANGE_T range;			// select input voltage range
	dec_float_t units_per_volt; // sensitivity in units/volt
	fix64_t offset_uV;		// zero offset in uV
	char* unit_name;		// name of channel unit

// Configured by setting the calibration flag in the config file, then running a calibration cycle. Backed up in device eeprom
	// volts = (raw_val - v5_zero_offset) / v5_LSB_per_volt
	fix64_t v5_zero_offset;	// value of raw 16-bit sample for 0 input voltage
	fix64_t v5_uV_per_LSB;	// sensitivity of reading in LSB / volt

	// volts = (raw_val - v24_zero_offset) / v24_LSB_per_volt
	fix64_t v24_zero_offset;	// value of raw 16-bit sample for 0 input voltage
	fix64_t v24_uV_per_LSB;	// sensitivity of reading in LSB / volt
} Channel_Config;

// Configuration data for the entire DAQ
typedef struct DAQ {
	Channel_Config channel[MAX_CHAN];
	uint8_t channel_count;	// Number of channels enabled, calculated from Channel_Config enables
	int32_t mv_out;			// Output voltage in mv, valid_range = <5000..24000>
	int32_t sample_rate;	// Sample rate in Hz, valid range = <1..10000>
	int8_t time_res;		// Sample time resolution in n digits where time is s.n
	DATA_T data_mode;		// data mode, can be READABLE or COMPACT
	char* user_comment;		// User comment to appear at the top of each data file
} DAQ;

extern uint8_t rsel_pins[3];

// Raw data buffer
extern struct RingBuffer *rawBuff;

// FatFS volume
extern FATFS fatfs[_VOLUMES];

// DAQ configuration data
extern DAQ daq;

// Start acquiring data
void daq_init(void);

// Write data file header
void daq_header(void);

// Stop acquiring data
void daq_stop(void);

// Write a single block to the data file from the string buffer
void daq_writeBlock(void);

// Write data from raw buffer to file, formatting to string  buffer as an intermediate step
// Stop when the raw buffer is empty
void daq_writeData(void);

// Convert rawData into a readable formatted output string
void daq_readableFormat(uint16_t *rawData, char *sampleStr);

// Convert rawData into a hex output string
void daq_hexFormat(uint16_t *rawData, char *sampleStr);

// Limit configuration values to valid ranges
void daq_configCheck(void);

// Set channel configuration from the config file on the SD card
void daq_configFromFile(void);

// Set channel configuration defaults
void daq_configDefault(void);

#endif /* __DAQ_ */
