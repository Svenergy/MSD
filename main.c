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
#include "sys_error.h"
#include "system.h"
#include "config.h"
#include "log.h"

/* Size of the output file write buffer */
#define RAW_BUFF_SIZE 0x4FFF // 0x5000 = 20kB, set 1 smaller for the extra byte required by the ring buffer

#define VBAT_LOW 3.25 // Low battery indicator voltage
#define VBAT_SHUTDOWN 3.0 // Low battery shut down voltage

#define TICKRATE_HZ1 (100)	// 100 ticks per second
#define TIMEOUT_SECS (300)	// Shut down after X seconds in Idle

RingBuffer *rawBuff;

FATFS fatfs[_VOLUMES];

SD_CardInfo cardinfo;

uint32_t enterIdleTime; // Time that the idle state was entered

void SysTick_Handler(void){
	static bool lowBat; // Set when battery voltage drops below VBAT_LOW
	static uint32_t sysTickCounter;

	sysTickCounter++; // Used to schedule less frequent tasks

	switch(system_state){
	case STATE_IDLE:
#ifndef NO_USB
		// If VBUS is connected and SD card is ready, try to connect as MSC
		if (Chip_GPIO_GetPinState(LPC_GPIO, 0, VBUS) && sd_state == SD_READY){
			f_mount(NULL,"",0); // unmount file system
			if (msc_init() == MSC_OK){
				Board_LED_Color(LED_YELLOW);
				system_state = STATE_MSC;
			}else{ // Error on MSC initialization
				error(ERROR_MSC_INIT);
			}
		}
#endif
		// If user has short pressed PB and SD card is ready, initiate acquisition
		if (pb_shortPress() && sd_state == SD_READY){
			Board_LED_Color(LED_PURPLE);
			daq_init();
			Board_LED_Color(LED_RED);
			system_state = STATE_DAQ;
		}

		// Blink LED if in low battery state, otherwise solid green
		if (lowBat && sysTickCounter % TICKRATE_HZ1 < TICKRATE_HZ1/2){
			Board_LED_Color(LED_OFF);
		} else {
			Board_LED_Color(LED_GREEN);
		}
		break;
	case STATE_MSC:
		// If VBUS is disconnected
		if (Chip_GPIO_GetPinState(LPC_GPIO, 0, VBUS) == 0){
			msc_stop();
			pb_shortPress(); // Clear pending button presses
			Board_LED_Color(LED_GREEN);
			f_mount(&fatfs,"",0); // mount file system
			system_state = STATE_IDLE;
			enterIdleTime = Chip_RTC_GetCount(LPC_RTC);
		}
		break;
	case STATE_DAQ:
		Board_LED_Color(LED_YELLOW);
		daq_writeData(); // Write data from buffer to file
		Board_LED_Color(LED_RED);

		// If user has short pressed PB to stop acquisition
		if (pb_shortPress()){
			Board_LED_Color(LED_PURPLE);
			daq_stop();
			Board_LED_Color(LED_GREEN);
			system_state = STATE_IDLE;
			enterIdleTime = Chip_RTC_GetCount(LPC_RTC);
		}
		break;
	}

	// Initialize SD card after every insertion
	if (Chip_GPIO_GetPinState(LPC_GPIO, 0, CARD_DETECT)){
		// Card out
		Board_LED_Color(LED_CYAN);
		sd_state = SD_OUT;
	}else{
		// Card in
		if (sd_state == SD_OUT){
			// Delay 100ms to let connections and power stabilize
			DWT_Delay(100000);
			if(init_sd_spi(&cardinfo) != SD_OK) {
				error(ERROR_SD_INIT);
			}
			switch(system_state){
			case STATE_IDLE:
				Board_LED_Color(LED_GREEN);
				break;
			case STATE_MSC:
				Board_LED_Color(LED_YELLOW);
				break;
			case STATE_DAQ:
				Board_LED_Color(LED_RED);
				break;
			}
			sd_state = SD_READY;
		}
	}

	/* Run once per second */
	if(sysTickCounter % TICKRATE_HZ1 == 0){
		float vBat = read_vBat(10);
		lowBat = vBat < VBAT_LOW ? true : false; // Set low battery state
		if (vBat < VBAT_SHUTDOWN){
			shutdown_message("Low Battery");
		}

		if ((Chip_RTC_GetCount(LPC_RTC) - enterIdleTime > TIMEOUT_SECS && system_state == STATE_IDLE) ){
			shutdown_message("Idle Time Out");
		}
	}

	/* Shut down conditions */
	if (pb_longPress()){
		shutdown_message("Power Button Pressed");
	}
}

int main(void) {
	uint32_t sysTickRate;

	Board_Init();

	//setTime("2015-02-24 4:41:00");

#ifdef DEBUG
	// Set up UART for debug
	init_uart(115200);
	putLineUART("\n");
#endif

	// Set up clocking for SD lib
	SystemCoreClockUpdate();
	DWT_Init();

	// Set up the FatFS Object
	f_mount(&fatfs,"",0);

	// Initialize SD card
	Board_LED_Color(LED_CYAN);
	if(sd_reset(&cardinfo) != SD_OK) {
		error(ERROR_SD_INIT);
	}
	sd_state = SD_READY;
	Board_LED_Color(LED_GREEN);

	// Log startup
	log_string("Startup");

	// Set up ADC for reading battery voltage
	read_vBat_setup();

	// Initialize ring buffer used to buffer raw data samples
	rawBuff = RingBuffer_initWithBuffer(RAW_BUFF_SIZE, RAM1_BASE);

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
