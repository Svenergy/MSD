#ifndef __SD_UTIL_
#define __SD_UTIL_

#include "sd_spi.h"
#include "uart.h"
#include <stdio.h>

// SPI card command response
extern uint8_t response[];

// sprintf buffer
char buffer[128];

// SPI read test buffer
uint8_t bf[512];

extern SD_CardInfo cardinfo;

// Dump length bytes starting at ptr over uart0 in 8-bit blocks
void sMemDump_8(uint8_t *ptr, uint32_t length);

// Dump length bytes starting at ptr over uart0 in 32-bit blocks
void sMemDump_32(uint32_t *ptr, uint32_t length);

// Print all sd card info
SD_ERROR sd_print_info(void);

// Test speed by reading first 1000 blocks
void sd_speed_test(void);

#endif /* __SD_UTIL_ */
