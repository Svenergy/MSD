#ifndef __BOARD_H_
#define __BOARD_H_

#include "chip.h"
/* board_api.h is included at the bottom of this file after DEBUG setup */


/* **** Set up board GPIO pin numbering **** */
/* all pins are on GPIO port 0 */

// RGB LED
#define RED   15
#define GREEN 14
#define BLUE  13

// Power system control
#define PWR_ON_OUT   7
#define PWR_PB_SENSE 6

// Battery voltage sensing
#define VBAT_A 3 // ADC0_3 same physical pin, different # for analog and digital peripherals
#define VBAT_D 5 // PIO0_5

// Sensor power output
#define VOUT_SHDN 25
#define VOUT_PWM 24

// Analog input range selection
#define RSEL1 26
#define RSEL2 27
#define RSEL3 28

// Debug UART
#define UART0_TX 26
#define UART0_RX 27

// ADC SPI
#define ADC_SS   29
#define ADC_MOSI 0
#define ADC_MISO 2
#define ADC_SCK  1

// SD card SPI and control pins
#define CARD_DETECT 12
#define SD_POWER    8

#define SD_SPI_CLK  18
#define SD_SPI_CS   11
#define SD_SPI_MOSI 10
#define SD_SPI_MISO 9

// USB pins
#define VBUS 16


/** Define DEBUG_ENABLE to enable IO via the DEBUGSTR, DEBUGOUT, and
    DEBUGIN macros. If not defined, DEBUG* functions will be optimized
    out of the code at build time.
 */
#define DEBUG_ENABLE

/** Board UART used for debug output and input using the DEBUG* macros. This
    is also the port used for Board_UARTPutChar, Board_UARTGetChar, and
    Board_UARTPutSTR functions.
 */
#define DEBUG_UART LPC_USART0

/* Board name */
#define BOARD_DAQ_SYSTEM

#include "board_api.h"

#endif /* __BOARD_H_ */
