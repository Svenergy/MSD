/*
 * @brief LPCXPresso LPC1549 Sysinit file
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

 #include "board.h"
 #include "string.h"

/* The System initialization code is called prior to the application and
   initializes the board for run-time operation. Board initialization
   includes clock setup and default pin muxing configuration. */

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* IOCON setup table, only items that need changing from their default pin
   state are in this table. */
STATIC const PINMUX_GRP_T ioconSetup[] = {
	/* RGB LED */
	{0, RED,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},			/* PIO0_15 (low enable) */
	{0, GREEN, (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},			/* PIO0_14 */
	{0, BLUE,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},			/* PIO1_13 */

	/* Power system control */
	{0, PWR_ON_OUT,	  (IOCON_MODE_PULLUP | IOCON_DIGMODE_EN)},	/* PIO0_7 */
	{0, PWR_PB_SENSE, (IOCON_MODE_PULLUP | IOCON_DIGMODE_EN)},	/* PIO0_6 */

	/* Battery voltage sense */
	{0, VBAT_D,	(IOCON_MODE_INACT)},							/* PIO0_5-ADC0_3 */

	/* Sensor power output */
	{0, VOUT_N_SHDN, (IOCON_MODE_PULLDOWN | IOCON_DIGMODE_EN)},	/* PIO0_25 */
	{0, VOUT_PWM,  (IOCON_MODE_PULLDOWN | IOCON_DIGMODE_EN)},	/* PIO0_24 */

	/* Analog range selection / debug UART */
	{0, RSEL1, (IOCON_MODE_PULLUP | IOCON_DIGMODE_EN)},			/* PIO0_26-UART0_TX */
	{0, RSEL2, (IOCON_MODE_PULLUP | IOCON_DIGMODE_EN)},			/* PIO0_27-UART0_RX */
	{0, RSEL3, (IOCON_MODE_PULLUP | IOCON_DIGMODE_EN)},			/* PIO0_28 */

	/* ADC SPI */
	{0, ADC_SS,	  (IOCON_MODE_PULLUP | IOCON_DIGMODE_EN)},		/* PIO0_29 */
	{0, ADC_MOSI, (IOCON_MODE_PULLUP | IOCON_DIGMODE_EN)},		/* PIO0_0 */
	{0, ADC_MISO, (IOCON_MODE_PULLUP | IOCON_DIGMODE_EN)},		/* PIO0_2 */
	{0, ADC_SCK,  (IOCON_MODE_PULLUP | IOCON_DIGMODE_EN)},		/* PIO0_1 */

	/* SD SPI and control */
	{0, CARD_DETECT, (IOCON_MODE_PULLUP | IOCON_DIGMODE_EN)},	/* PIO0_12 */
	{0, SD_POWER,    (IOCON_MODE_PULLUP | IOCON_DIGMODE_EN)},	/* PIO0_8 */

	{0, SD_SPI_CLK,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},	/* PIO0_18 */
	{0, SD_SPI_CS,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},	/* PIO0_11 */
	{0, SD_SPI_MOSI, (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},	/* PIO0_10 */
	{0, SD_SPI_MISO, (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},	/* PIO0_9 */

	/* USB */
	{0, VBUS,  (IOCON_MODE_PULLDOWN | IOCON_DIGMODE_EN)},		/* PIO0_16-ISP_1 (VBUS) */
};

/* SWIM pin assignment definitions for pin assignment/muxing */
typedef struct {
	uint16_t assignedpin : 9;		/* Function and mode */
	uint16_t port : 2;				/* Pin port */
	uint16_t pin : 5;				/* Pin number */
} SWM_GRP_T;

/* Pin muxing table, only items that need changing from their default pin
   state are in this table. */
STATIC const SWM_GRP_T swmSetup[] = {
	/* USB related */
	{(uint16_t) SWM_USB_VBUS_I, 0, VBUS},		/* PIO0_16-ISP_1-AIN_CTRL */

	/* UART */
#ifdef DEBUG
	{(uint16_t) SWM_UART0_RXD_I, 0, UART0_RX},		/* PIO0_27-UART0_RX */
	{(uint16_t) SWM_UART0_TXD_O, 0, UART0_TX},		/* PIO0_26-UART0_TX */
#endif
};

/* Setup fixed pin functions (GPIOs are fixed) */
/* No fixed pins except GPIOs */
#define PINENABLE0_VAL 0xFFFFFFFF

/* No fixed pins except GPIOs */
#define PINENABLE1_VAL 0x00FFFFFF

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Sets up system pin muxing */
void Board_SetupMuxing(void)
{
	int i;

	/* Enable SWM and IOCON clocks */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);
	Chip_SYSCTL_PeriphReset(RESET_IOCON);

	/* IOCON setup */
	Chip_IOCON_SetPinMuxing(LPC_IOCON, ioconSetup, sizeof(ioconSetup) / sizeof(PINMUX_GRP_T));

	/* SWM assignable pin setup */
	for (i = 0; i < (sizeof(swmSetup) / sizeof(SWM_GRP_T)); i++) {
		Chip_SWM_MovablePortPinAssign((CHIP_SWM_PIN_MOVABLE_T) swmSetup[i].assignedpin,
									  swmSetup[i].port, swmSetup[i].pin);
	}

	/* SWM fixed pin setup */
	//	LPC_SWM->PINENABLE[0] = PINENABLE0_VAL;
	//	LPC_SWM->PINENABLE[1] = PINENABLE1_VAL;

	/* Note SWM and IOCON clocks are left on */
}

/* Set up and initialize clocking prior to call to main */
void Board_SetupClocking(void)
{
	// Enable external 12MHz clock
	Chip_SetupXtalClocking();

	/* Set SYSTICKDIV to 1 so CMSIS Systick functions work */
	Chip_Clock_SetSysTickClockDiv(1);
}

/* Set up and initialize hardware prior to call to main */
void Board_SystemInit(void)
{
	/* Setup system clocking and muxing */
	Board_SetupMuxing();
	Board_SetupClocking();
}
