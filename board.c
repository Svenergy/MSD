#include "board.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/* System oscillator rate and RTC oscillator rate */
const uint32_t OscRateIn = 12000000;
const uint32_t RTCOscRateIn = 32768;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Sends a character on the UART */
void Board_UARTPutChar(char ch)
{
#if defined(DEBUG_UART)
	Chip_UART_SendBlocking(DEBUG_UART, &ch, 1);
#endif
}

/* Gets a character from the UART, returns EOF if no character is ready */
int Board_UARTGetChar(void)
{
#if defined(DEBUG_UART)
	uint8_t data;

	if (Chip_UART_Read(DEBUG_UART, &data, 1) == 1) {
		return (int) data;
	}
#endif
	return EOF;
}

/* Outputs a string on the debug UART */
void Board_UARTPutSTR(char *str)
{
#if defined(DEBUG_UART)
	while (*str != '\0') {
		Board_UARTPutChar(*str++);
	}
#endif
}

/* Initialize debug output via UART for board */
void Board_Debug_Init(void)
{
#if defined(DEBUG_UART)
	/* Disables pullups/pulldowns and enable digitial mode */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, UART0_RX, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, UART0_TX, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));

	/* UART signal muxing via SWM */
	Chip_SWM_MovablePortPinAssign(SWM_UART0_TXD_O, 0, UART0_TX);
	Chip_SWM_MovablePortPinAssign(SWM_UART0_RXD_I, 0, UART0_RX);

	/* Use main clock rate as base for UART baud rate divider */
	Chip_Clock_SetUARTBaseClockRate(Chip_Clock_GetMainClockRate(), false);

	/* Setup UART */
	Chip_UART_Init(DEBUG_UART);
	Chip_UART_ConfigData(DEBUG_UART, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1);
	Chip_UART_SetBaud(DEBUG_UART, 115200);
	Chip_UART_Enable(DEBUG_UART);
	Chip_UART_TXEnable(DEBUG_UART);
#endif
}

#define MAXLEDS 3
static const uint8_t ledpins[MAXLEDS] = {RED, GREEN, BLUE};
static const uint8_t ledports[MAXLEDS] = {0, 0, 0};

/* Initializes board LED(s) */
static void Board_LED_Init(void)
{
	int idx;

	for (idx = 0; idx < MAXLEDS; idx++) {
		/* Set the GPIO as output with initial state off (high) */
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, ledports[idx], ledpins[idx]);
		Chip_GPIO_SetPinState(LPC_GPIO, ledports[idx], ledpins[idx], true);
	}
}

/* Sets the state of a board LED to on or off */
void Board_LED_Set(uint8_t LEDNumber, bool On)
{
	if (LEDNumber < MAXLEDS) {
		/* Toggle state, low is on, high is off */
		Chip_GPIO_SetPinState(LPC_GPIO, ledports[LEDNumber], ledpins[LEDNumber], !On);
	}
}

/* Returns the current state of a board LED */
bool Board_LED_Test(uint8_t LEDNumber)
{
	bool state = false;

	if (LEDNumber < MAXLEDS) {
		state = !Chip_GPIO_GetPinState(LPC_GPIO, ledports[LEDNumber], ledpins[LEDNumber]);
	}

	return state;
}

/* Toggles the current state of a board LED */
void Board_LED_Toggle(uint8_t LEDNumber)
{
	Chip_GPIO_SetPinToggle(LPC_GPIO, ledports[LEDNumber], ledpins[LEDNumber]);
}

/* Set up and initialize all required blocks and functions related to the
   board hardware */
void Board_Init(void)
{
	/* Sets up DEBUG UART */
	DEBUGINIT();

	/* Initialize GPIO */
	Chip_GPIO_Init(LPC_GPIO);

	/* Initialize LEDs */
	Board_LED_Init();
}
