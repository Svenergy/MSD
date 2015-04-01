#include "sys_error.h"

static volatile bool inError; // Set when handling an error to prevent recursion
static volatile ERROR_CODE globalError;

static const char* const errorString[] = {
	"ERROR_UNKNOWN",
	"ERROR_F_WRITE",
	"ERROR_BUF_OVF",
	"ERROR_MSC_SD_READ",
	"ERROR_MSC_SD_WRITE",
	"ERROR_MSC_INIT",
	"ERROR_SD_INIT",
	"ERROR_SAMPLE_TIME",
	"ERROR_READ_CONFIG",
	"ERROR_WRITE_CONFIG",
	"ERROR_DISK_FULL"
};

void error(ERROR_CODE errorCode){
	// Only handle the first Error
	if(inError){
		return;
	}
	inError = true;

	// Set error code
	globalError = errorCode;
}

void error_handler(void){
	if(inError){

		// Disable systick and shutdown current processes
		system_halt();

		// Blue LED on in error
		Board_LED_Color(LED_BLUE);

		// Mount file system
		f_mount(fatfs,"",0);

		// Reset SD card
		sd_reset(&cardinfo);

		// Write error code to log file
		log_string(errorString[globalError]);

		// Delay 5 seconds
		DWT_Delay(5000000);

		system_power_off();
	}
}
