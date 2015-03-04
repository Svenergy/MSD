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

	// Disable systick and shutdown current processes
	system_halt();

	// Blue LED on in error
	Board_LED_Color(LED_BLUE);

	// Reset SD card
	sd_reset(&cardinfo);

	// Write error code to log file
	log_string(errorString[errorCode]);

	// Delay 5 seconds
	DWT_Delay(5000000);

	system_power_off();
}
