#include "push_button.h"

// Initialize the push button
void pb_init(int tickRateHz){
	pb_tickRateHz = tickRateHz;
	pbFirstPress = true; // set until the first time the button is released after startup
}

// Run in the main program loop, ideally at 100Hz or greater
void pb_loop(void){
	if (Chip_GPIO_GetPinState(LPC_GPIO, 0, PWR_PB_SENSE)){
		pbCounter++;
		if (pbFirstPress){
			pbCounter = 0;
		}

		// Long press
		if (pbCounter > pb_tickRateHz){
			pbLongPress = true;
		}
	} else {
		if(pbFirstPress){
			pbFirstPress = false;
		}else{
			// Short press
			if(pbCounter > 0){
				pbShortPress = true;
			}
		}
		pbCounter = 0;
	}
}

// Returns true if a short press has occurred
int pb_shortPress(void){
	if(pbShortPress){
		pbShortPress = false;
		return true;
	}
	return false;
}

// Returns true if a long press has occurred
int pb_longPress(void){
	if(pbLongPress){
		pbLongPress = false;
		return true;
	}
	return false;
}
