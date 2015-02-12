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
YELLOW = mass storage mode idle
PURPLE = mass storage mode data read/write
CYAN = SD card not present
BLUE = error

*/

#include <stdio.h>

#include "board.h"
#include "daq.h"
#include "delay.h"
#include "msc_main.h"
#include "sd_spi.h"
#include "push_button.h"
#include "fatfstest.h"

#define TICKRATE_HZ1 (100)	/* 100 ticks per second */

typedef enum {
	STATE_IDLE,
	STATE_MSC,
	STATE_DAQ,
} SYSTEM_STATE;

SYSTEM_STATE system_state;

typedef enum {
	SD_OUT,
	SD_READY,
} SD_STATE;

SD_STATE sd_state;

SD_CardInfo cardinfo;

LPC_RTC_T RTC;

float read_vBat(int n);
void shutDown(void);
void error(void);

void SysTick_Handler(void){

	pb_loop();

	// Read current system state and change state if needed
	switch(system_state){
	case STATE_IDLE:
		// If VBUS is connected and SD card is ready, try to connect as MSC
		if (Chip_GPIO_GetPinState(LPC_GPIO, 0, VBUS) && sd_state == SD_READY){
			if (msc_init() == MSC_OK){
				Board_LED_Color(LED_YELLOW);
				system_state = STATE_MSC;
			}else{ // Error on MSC initialization
				error();
			}
		}
		// If user has short pressed PB and SD card is ready, initiate acquisition
		if (pb_shortPress() && sd_state == SD_READY){
			daq_init();
			Board_LED_Color(LED_RED);
			system_state = STATE_DAQ;
		}
		break;
	case STATE_MSC:
		// If VBUS is disconnected
		if (Chip_GPIO_GetPinState(LPC_GPIO, 0, VBUS) == 0){
			// Force shut down when USB is disconnected
			//shutDown();
			msc_stop();
			pb_shortPress(); // Clear pending button presses
			Board_LED_Color(LED_GREEN);
			system_state = STATE_IDLE;
		}
		break;
	case STATE_DAQ:
		// If user has short pressed PB to stop acquisition
		if (pb_shortPress()){
			daq_stop();
			Board_LED_Color(LED_GREEN);
			system_state = STATE_IDLE;
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
			// Delay 200ms to let connections and power stabilize
			DWT_Delay(200000);
			if(init_sd_spi(&cardinfo) != SD_OK) {
				error(); // SD card could not be initialized
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

	// Shut down if PB is long pressed
	if (pb_longPress()){
		shutDown();
	}

	// Read battery voltage, shutdown if too low
	if (read_vBat(10) < 3.0){
		shutDown();
	}
}

void error(void){
	// Blue LED on in error
	Board_LED_Color(LED_BLUE);

	// Delay 5 seconds
	DWT_Delay(5000000);

	shutDown();
}

void shutDown(void){
	// Shut down procedures
	Board_LED_Color(LED_OFF);

	// Safely stop data acquisition if needed
	if(system_state == STATE_DAQ){
		daq_stop();
	}

#ifdef DEBUG
	// Send Shutdown debug string
	putLineUART("SHUTDOWN\n");
#endif

	// Wait for UART to finish transmission
	while ( !(Chip_UART_GetStatus(LPC_USART0)&UART_STAT_TXIDLE) ){};

	// Turn system power off
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PWR_ON_OUT, 0);
	// Wait for interrupts
	while (1) {
		__WFI();
	}
}

void ADC_setup(void){
	// Connect pin in switch matrix adn set IOCON
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, VBAT_D, IOCON_ADMODE_EN);
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_3);

	// Initialize ADC
	Chip_ADC_Init(LPC_ADC0, 0);

	// Set ADC clock to 2MHz
	Chip_ADC_SetClockRate(LPC_ADC0, 2000000);

	// Start up calibration
	Chip_ADC_StartCalibration(LPC_ADC0);

	// Wait for calibration to complete
	while(!Chip_ADC_IsCalibrationDone(LPC_ADC0));

	// Select appropriate voltage range
	Chip_ADC_SetTrim(LPC_ADC0, ADC_TRIM_VRANGE_HIGHV);

	// Select ADC channel and Set TRIGPOL to 1 and SEQA_ENA to 1
	Chip_ADC_SetupSequencer(LPC_ADC0, ADC_SEQA_IDX,
			ADC_SEQ_CTRL_CHANSEL(3) | ADC_SEQ_CTRL_HWTRIG_POLPOS |
			ADC_SEQ_CTRL_SEQ_ENA);
}

// Returns the battery voltage reading in volts after averaging n samples
float read_vBat(int n){
	uint32_t sum = 0;
	int i;
	for(i=0;i<n;i++){
		Chip_ADC_StartSequencer(LPC_ADC0, ADC_SEQA_IDX); // Set START bit to 1
		while(!(Chip_ADC_GetDataReg(LPC_ADC0, 3)&ADC_DR_DATAVALID)); // Wait for conversion to complete
		sum += ADC_DR_RESULT(Chip_ADC_GetDataReg(LPC_ADC0, 3)); // Read result from the DAT1 register
	}

	// Convert raw value to volts
	return (3.3*sum) / (4096 * n);
}

int main(void) {

	uint32_t sysTickRate;

	// Set up debug UART and GPIO
	Board_Init();

#ifdef DEBUG
	// Set up UART for debug
	init_uart(115200);

	// Send Startup debug string
	putLineUART("STARTUP\n");
#endif

	// Set up ADC for reading battery voltage
	ADC_setup();

	/* Enable and setup SysTick Timer at a periodic rate */
	Chip_Clock_SetSysTickClockDiv(1);
	sysTickRate = Chip_Clock_GetSysTickClockRate();
	SysTick_Config(sysTickRate / TICKRATE_HZ1);

	// Set up clocking used by SD lib
	SystemCoreClockUpdate();
	DWT_Init();

	// Initialize RTC
	Chip_RTC_Init(&RTC);
	Chip_RTC_Enable(&RTC);

	// Initialize sub-systems
	pb_init(TICKRATE_HZ1);

	// Set SD state out to force card init in sysTick loop
	sd_state = SD_OUT;
	Board_LED_Color(LED_CYAN);

	// Turn on SD card power
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, SD_POWER);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, SD_POWER, 0);

	// Idle and run systick loop until triggered or plugged in as a USB device
	system_state = STATE_IDLE;

    // Wait for interrupts
    while (1) {
    	__WFI();
    }

    return 0 ;
}
