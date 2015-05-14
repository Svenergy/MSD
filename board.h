#ifndef __BOARD_H_
#define __BOARD_H_

#include "chip.h"

/* Build options */

//#define PRINT_DATA_UART

/* End Build options */

#ifdef DEBUG
#include "uart.h"
#endif

/* Address of the base of each ram bank */
#define RAM0_BASE (void *)0x02000000 // 16kB
#define RAM1_BASE (void *)0x02004000 // 16kB
#define RAM2_BASE (void *)0x02008000 // 4kB

/* Set up board led colors */
typedef enum {
	LED_OFF,
	LED_RED,
	LED_GREEN,
	LED_BLUE,
	LED_YELLOW,
	LED_CYAN,
	LED_PURPLE,
	LED_WHITE,
} COLOR_T;

/* **** Set up board GPIO pin numbering **** */
/* all pins are on GPIO port 0 */

// RGB LED
#define RED   17
#define GREEN 15
#define BLUE  14

// Power system control
#define PWR_ON_OUT   7
#define PWR_PB_SENSE 6

// Battery voltage sensing
#define VBAT_A 3 // ADC0_3 same physical pin, different # for analog and digital peripherals
#define VBAT_D 5 // PIO0_5

// Sensor power output
#define VOUT_N_SHDN 25
#define VOUT_PWM 24

// Analog input range selection
#define RSEL1 26
#define RSEL2 27
#define RSEL3 28

// Debug UART
#define UART0_RX 19
#define UART0_TX 20

// ADC SPI
#define ADC_SS   29
#define ADC_MOSI 0
#define ADC_MISO 2
#define ADC_SCK  1

// RAM buffer / SD card SPI
#define SD_SPI_CLK  3
#define SD_SPI_MOSI 10 // Shared between RAM and SD
#define SD_SPI_MISO 23
#define SD_SPI_CS   11

#define RAM_SPI_MISO 9
#define RAM_SPI_CLK  18
#define RAM_SPI_CS   13

// SD card control
#define CARD_DETECT 12
#define SD_POWER    8

// USB pins
#define VBUS 16

#include "board_api.h"
#endif /* __BOARD_H_ */
