/*
===============================================================================
 Name        : daq_system.c
 Authors      : Kyle Smith and Sven Opdyke
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
COLOR CODES:
N/A = off
RED = recording data
GREEN = power on idle
YELLOW = mass storage mode idle, DAQ data writing
PURPLE = mass storage mode data read/write, DAQ initializing
CYAN = SD card not present
BLUE = error
*/

#include <stdio.h>
#include <time.h>

#include "board.h"
#include "daq.h"
#include "delay.h"
#include "msc_main.h"
#include "sd_spi.h"
#include "push_button.h"
#include "ff.h"
#include "ring_buff.h"
#include "ram_buffer.h"
#include "sys_error.h"
#include "system.h"
#include "config.h"
#include "log.h"

/* Size of the output file write buffer */
#define RAW_BUFF_SIZE 0x4FFF // 0x5000 = 20kB, set 1 smaller for the extra byte required by the ring buffer

RingBuffer *rawBuff;

FATFS fatfs[_VOLUMES];

SD_CardInfo cardinfo;

uint32_t enterIdleTime; // Time that the idle state was entered

int main(void) {
	uint32_t sysTickRate;

	Board_Init();

#ifdef DEBUG
	// Set up UART for debug
	init_uart(115200);
	putLineUART("\n");
#endif

	// Initialize ring buffer used to buffer raw data samples
	rawBuff = RingBuffer_initWithBuffer(RAW_BUFF_SIZE, RAM1_BASE);
	ramBuffer_init();

	// Enable EEPROM clock and reset EEPROM controller
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_EEPROM);
	Chip_SYSCTL_PeriphReset(RESET_EEPROM);

	// Set up clocking for SD lib
	SystemCoreClockUpdate();
	DWT_Init();

	// Set up the FatFS Object
	f_mount(fatfs,"",0);

	// Initialize SD card, try up to 10 times before giving up
	Board_LED_Color(LED_CYAN);
	int i = 0;
	while(i<10 && sd_reset(&cardinfo) != SD_OK){
		i++;
	}
	if(i == 10) {
		error(ERROR_SD_INIT);
	}
	sd_state = SD_READY;

	// Setup config
	Board_LED_Color(LED_CYAN);
	configStart();
	Board_LED_Color(LED_GREEN);

	// Allow MSC mode on startup
	msc_state = MSC_ENABLED;

	// Set up ADC for reading battery voltage
	read_vBat_setup();

	// Log startup and battery voltage
	char start_str[32];
	sprintf(start_str, "Startup, VBatt = %.2f", read_vBat(10));
	log_string(start_str);

	// Set up MRT used by pb and daq
	Chip_MRT_Init();
	NVIC_ClearPendingIRQ(MRT_IRQn);
	NVIC_EnableIRQ(MRT_IRQn);
	NVIC_SetPriority(MRT_IRQn, 0x02); // Set higher than systick, but lower than sampling

	// Initialize push button
	pb_init();

	// Enable and setup SysTick Timer at a periodic rate
	Chip_Clock_SetSysTickClockDiv(1);
	sysTickRate = Chip_Clock_GetSysTickClockRate();
	SysTick_Config(sysTickRate / TICKRATE_HZ1);

	// Idle and run systick loop until triggered or plugged in as a USB device
	system_state = STATE_IDLE;
	enterIdleTime = Chip_RTC_GetCount(LPC_RTC);

    // Wait for interrupts
    while (1) {
    	__WFI();
    }

    return 0 ;
}
