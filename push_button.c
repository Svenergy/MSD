#include "push_button.h"

PB_STATE pbState;
uint32_t pbTenths;

bool pbLongPress;
bool pbShortPress;

// Pin interrupt, triggered when push button is pressed or released
void PIN_INT0_IRQHandler(void){
	// Clear interrupt, in level mode also flips active level
	PININT_ClearIntAndFlipActiveLevel(LPC_GPIO_PIN_INT, 1 << 0);

	switch (pbState){
	case READY:
		PININT_DisableLevelInt(LPC_GPIO_PIN_INT, 1 << 0);
		Chip_MRT_SetEnabled(LPC_MRT_CH(0));
		Chip_MRT_SetMode(LPC_MRT_CH(0), MRT_MODE_ONESHOT);
		Chip_MRT_SetInterval(LPC_MRT_CH(0), (20 * (Chip_Clock_GetSystemClockRate() / 1000)) | MRT_INTVAL_LOAD); //20ms
		break;
	case TRIGGERED:
		pbShortPress = true;
		Chip_MRT_SetDisabled(LPC_MRT_CH(0));
	case LONGPRESS:
		PININT_EnableLevelInt(LPC_GPIO_PIN_INT, 1 << 0);
		PININT_HighActive(LPC_GPIO_PIN_INT, 1 << 0);
	}

	pbState = READY;
}

// Timer interrupt, used to track press time for long press detection
void MRT_IRQHandler(void){
	Chip_MRT_IntClear(LPC_MRT_CH(0));
	if(pbState == READY){
		if(Chip_GPIO_GetPinState(LPC_GPIO, 0, PWR_PB_SENSE)){
			pbState = TRIGGERED;
			PININT_EnableLevelInt(LPC_GPIO_PIN_INT, 1 << 0);
			PININT_LowActive(LPC_GPIO_PIN_INT, 1 << 0);
			Chip_MRT_SetEnabled(LPC_MRT_CH(0));
			Chip_MRT_SetMode(LPC_MRT_CH(0), MRT_MODE_REPEAT);
			Chip_MRT_SetInterval(LPC_MRT_CH(0), (100 * (Chip_Clock_GetSystemClockRate() / 1000)) | MRT_INTVAL_LOAD); //100ms
			pbTenths = 0;
		}else{
			PININT_EnableLevelInt(LPC_GPIO_PIN_INT, 1 << 0);
			PININT_HighActive(LPC_GPIO_PIN_INT, 1 << 0);
		}
	}else if(pbState == TRIGGERED){
		pbTenths++;
		if(pbTenths >= 10){
			pbState = LONGPRESS;
			pbLongPress = true;
			Chip_MRT_SetDisabled(LPC_MRT_CH(0));
		}
	}
}

// Initialize the push button
void pb_init(void){
	// Set initial values
	pbState = READY;
	pbTenths = 0;
	pbLongPress = false;
	pbShortPress = false;

	// Set up timer interrupt
	Chip_MRT_Init();
	Chip_MRT_SetDisabled(LPC_MRT_CH(0));
	Chip_MRT_IntClear(LPC_MRT_CH(0));

	NVIC_ClearPendingIRQ(MRT_IRQn);
	NVIC_EnableIRQ(MRT_IRQn);
	NVIC_SetPriority(MRT_IRQn, 0x02); // Set higher than systick, but lower than sampling

	// Set up pin interrupt
	Chip_PININT_Init(LPC_GPIO_PIN_INT);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_PININT);
	Chip_SYSCTL_PeriphReset(RESET_PININT);
	Chip_INMUX_PinIntSel(0, 0, PWR_PB_SENSE);
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, 1 << 0);
	Chip_PININT_SetPinModeLevel(LPC_GPIO_PIN_INT, 1 << 0);

	PININT_EnableLevelInt(LPC_GPIO_PIN_INT, 1 << 0);
	PININT_LowActive(LPC_GPIO_PIN_INT, 1 << 0); // Enable low first so that initial press is not detected

	NVIC_ClearPendingIRQ(PIN_INT0_IRQn);
	NVIC_EnableIRQ(PIN_INT0_IRQn);
	NVIC_SetPriority(PIN_INT0_IRQn, 0x02); // Set higher than systick, but lower than sampling
}

// Returns true if a short press has occurred
bool pb_shortPress(void){
	if(pbShortPress){
		pbShortPress = false;
		return true;
	}
	return false;
}

// Returns true if a long press has occurred
bool pb_longPress(void){
	if(pbLongPress){
		pbLongPress = false;
		return true;
	}
	return false;
}
