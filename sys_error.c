#include "sys_error.h"

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
