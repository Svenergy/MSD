#include "sys_error.h"

void error(ERROR_CODE errorCode){
	system_halt();

#ifdef DEBUG
	// Print the error code
	char ec_b[20];
	sprintf(ec_b, "\n<Error Code %d>\n", errorCode);
	putLineUART(ec_b);
#endif

	// Blue LED on in error
	Board_LED_Color(LED_BLUE);

	// Delay 5 seconds
	DWT_Delay(5000000);

	system_power_off();
}
