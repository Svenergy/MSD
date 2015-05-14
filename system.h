#ifndef __SHUTDOWN_
#define __SHUTDOWN_

#include "board.h"
#include "ff.h"
#include "msc_main.h"
#include "daq.h"
#include "log.h"
#include "push_button.h"

#define VERSION "2.0"

#define VBAT_LOW 3.25 // Low battery indicator voltage
#define VBAT_SHUTDOWN 3.0 // Low battery shut down voltage

#define TICKRATE_HZ1 (100)	// 100 ticks per second
#define TIMEOUT_SECS (300)	// Shut down after X seconds in Idle

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

extern SYSTEM_STATE system_state;
extern SD_STATE sd_state;
extern MSC_STATE msc_state;

extern uint32_t enterIdleTime;

// Systick loop
void SysTick_Handler(void);

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
