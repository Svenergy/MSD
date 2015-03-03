#ifndef __SHUTDOWN_
#define __SHUTDOWN_

#include "board.h"
#include "ff.h"
#include "msc_main.h"
#include "daq.h"
#include "log.h"

typedef enum {
	STATE_IDLE,
	STATE_MSC,
	STATE_DAQ,
} SYSTEM_STATE;

typedef enum {
	SD_OUT,
	SD_READY,
} SD_STATE;

typedef enum {
	MSC_ENABLED,
	MSC_DISABLED,
} MSC_STATE;

SYSTEM_STATE system_state;
SD_STATE sd_state;
MSC_STATE msc_state;

// Halt and power off
void shutdown(void);

// Halt and power off with message
void shutdown_message(char *message);

// Safely stop all running system processes
void system_halt(void);

// Turn off system power
void system_power_off(void);

// Prepare ADC for reading battery voltage
void read_vBat_setup(void);

// Read battery voltage
float read_vBat(int32_t n);

#endif
