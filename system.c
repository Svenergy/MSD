#include "system.h"

SYSTEM_STATE system_state;
SD_STATE sd_state;
MSC_STATE msc_state;

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
		f_mount(&fatfs,"",0); // mount file system
	}

	Board_LED_Color(LED_OFF);
}

// Turn off system power
void system_power_off(void){
	// Log shutdown event
	log_string("Shutdown");
#ifdef DEBUG
	// Wait for UART to finish transmission
	while ( !(Chip_UART_GetStatus(LPC_USART0)&UART_STAT_TXIDLE) ){};
#endif

	// Unmount file system
	f_mount(NULL,"",0);

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
	return (3.3*sum) / (4096 * n);
}

