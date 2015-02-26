#include "sys_error.h"

volatile bool inError; // Set when handling an error to prevent recursion

const char* const errorString[] = {
	"ERROR_UNKNOWN",
	"ERROR_F_WRITE",
	"ERROR_BUF_OVF",
	"ERROR_MSC_SD_READ",
	"ERROR_MSC_SD_WRITE",
	"ERROR_MSC_INIT",
	"ERROR_SD_INIT",
	"ERROR_SAMPLE_TIME",
};

void error(ERROR_CODE errorCode){
	// Only handle the first Error
	if(inError)
		return;
	inError = true;

	// Reset SD card
	sd_reset(&cardinfo);

	// Clear file system locks to allow writing error code to log
	ff_rel_grant(fatfs->sobj);

	// Write error code to log file
	log_string(errorString[errorCode]);

	// Disable interrupts and shutdown current processes
	system_halt();

	// Blue LED on in error
	Board_LED_Color(LED_BLUE);

	// Delay 5 seconds
	DWT_Delay(5000000);

	system_power_off();
}
