#include "system.h"

SYSTEM_STATE system_state;
SD_STATE sd_state;
MSC_STATE msc_state;

void SysTick_Handler(void){
	static bool lowBat; // Set when battery voltage drops below VBAT_LOW
	static uint32_t sysTickCounter;

	sysTickCounter++; // Used to schedule less frequent tasks

	switch(system_state){
	case STATE_IDLE:
		// Enable USB if VBUS is disconnected
		;bool vBus = Chip_GPIO_GetPinState(LPC_GPIO, 0, VBUS);
		if(!vBus){
			msc_state = MSC_ENABLED;
		}
		// If MSC enabled, VBUS is connected, and SD card is ready, try to connect as MSC
		if (msc_state == MSC_ENABLED && vBus && sd_state == SD_READY){
			f_mount(NULL,"",0); // unmount file system
			if (msc_init() == MSC_OK){
				Board_LED_Color(LED_YELLOW);
				system_state = STATE_MSC;
				break;
			}else{ // Error on MSC initialization
				error(ERROR_MSC_INIT);
			}
		}
		// If user has short pressed PB and SD card is ready, initiate acquisition
		if (pb_shortPress() && sd_state == SD_READY){
			daq_init();
			system_state = STATE_DAQ;
			break;
		}

		// Blink LED if in low battery state, otherwise solid green
		if (lowBat && sysTickCounter % TICKRATE_HZ1 < TICKRATE_HZ1/2){
			Board_LED_Color(LED_OFF);
		} else {
			Board_LED_Color(LED_GREEN);
		}
		break;
	case STATE_MSC:
		// If VBUS is disconnected or button is short pressed
		;bool pb;
		if (Chip_GPIO_GetPinState(LPC_GPIO, 0, VBUS) == 0 || (pb = pb_shortPress())){
			if(pb){
				msc_state = MSC_DISABLED;
			}
			msc_stop();
			f_mount(fatfs,"",0); // mount file system
			Board_LED_Color(LED_GREEN);
			system_state = STATE_IDLE;
			enterIdleTime = Chip_RTC_GetCount(LPC_RTC);
		}
		break;
	case STATE_DAQ:
		// Perform the current asynchronous daq action
		daq_loop();

		// If user has short pressed PB to stop acquisition
		if (pb_shortPress()){
			Board_LED_Color(LED_PURPLE);
			daq_stop();
			Board_LED_Color(LED_GREEN);
			system_state = STATE_IDLE;
			enterIdleTime = Chip_RTC_GetCount(LPC_RTC);
			msc_state = MSC_DISABLED;
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

	/* Handle errors */
	error_handler();
}

// Halt and power off
void shutdown(void){
	system_halt();
	system_power_off();
}

// Halt and power off with message
void shutdown_message(char *message){
	system_halt();
	log_string(message);
	system_power_off();
}

// Safely stop all running system processes
void system_halt(void){
	// Disable Systick
	SysTick->CTRL  = 0;

	// Shut down procedures
	if(system_state == STATE_DAQ){
		daq_stop();
	} else if(system_state == STATE_MSC){
		msc_stop();
	}

	Board_LED_Color(LED_OFF);
}

// Turn off system power
void system_power_off(void){
	// Log shutdown and battery voltage
	char shut_str[32];
	sprintf(shut_str, "Shutdown, VBatt = %.2f", read_vBat(10));
	log_string(shut_str);

#ifdef DEBUG
	// Wait for UART to finish transmission
	while ( !(Chip_UART_GetStatus(LPC_USART0)&UART_STAT_TXIDLE) ){};
#endif

	// Turn system power off
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PWR_ON_OUT, 0);
	Board_LED_Color(LED_OFF);

	// Wait for user to release power button
	while (1) {
		__WFI();
	}
}

// Prepare ADC for reading battery voltage
void read_vBat_setup(void){
	// Connect pin in switch matrix adn set IOCON
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, VBAT_D, IOCON_ADMODE_EN);
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_3);

	// Initialize ADC
	Chip_ADC_Init(LPC_ADC0, 0);
	Chip_ADC_SetClockRate(LPC_ADC0, 2000000); // Set ADC clock to 2MHz
	Chip_ADC_StartCalibration(LPC_ADC0); // Start up calibration
	while(!Chip_ADC_IsCalibrationDone(LPC_ADC0));
	Chip_ADC_SetTrim(LPC_ADC0, ADC_TRIM_VRANGE_HIGHV); // Select appropriate voltage range

	// Select ADC channel and Set TRIGPOL to 1 and SEQA_ENA to 1
	Chip_ADC_SetupSequencer(LPC_ADC0, ADC_SEQA_IDX,
			ADC_SEQ_CTRL_CHANSEL(3) | ADC_SEQ_CTRL_HWTRIG_POLPOS |
			ADC_SEQ_CTRL_SEQ_ENA);
}

// Returns the battery voltage reading in volts after averaging n samples
float read_vBat(int32_t n){
	uint32_t sum = 0;
	int32_t i;
	for(i=0;i<n;i++){
		Chip_ADC_StartSequencer(LPC_ADC0, ADC_SEQA_IDX); // Set START bit to 1
		while(!(Chip_ADC_GetDataReg(LPC_ADC0, 3)&ADC_DR_DATAVALID)); // Wait for conversion to complete
		sum += ADC_DR_RESULT(Chip_ADC_GetDataReg(LPC_ADC0, 3)); // Read result from the DAT1 register
	}

	// Convert raw value to volts
	return (2 * 3.3 * sum) / (4096 * n);
}

/* Interrupt handlers with multiple functions */
void MRT_IRQHandler(void){
	uint32_t int_pend;

	/* Get and clear interrupt pending status for all timers */
	int_pend = Chip_MRT_GetIntPending();
	Chip_MRT_ClearIntPending(int_pend);

	/* Channel 0, Used to track press time for long press detection */
	if (int_pend & MRTn_INTFLAG(0)) {
		MRT0_IRQHandler();
	}

	/* Channel 1, Used to track ADC sampling progress */
	if (int_pend & MRTn_INTFLAG(1)) {
		MRT1_IRQHandler();
	}
}

