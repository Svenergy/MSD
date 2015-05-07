/************************************************************************
* Implements a ring buffer using the 23LC1024 spi ram chip
* http://ww1.microchip.com/downloads/en/DeviceDoc/20005142C.pdf
* Kyle Smith
* 5-7-2015
*
************************************************************************/
#ifndef RAM_BUFFER_H_
#define RAM_BUFFER_H_

#include "board.h"
#include "sys_error.h"

// 23LC1024 instructions
#define INST_READ  0x03
#define INST_WRITE 0x02
#define INST_EDIO  0x3B
#define INST_EQIO  0x38
#define INST_RSTIO 0xFF
#define INST_RDMR  0x05
#define INST_WRMR  0x01

// Size of the ram buffer in bytes (1Mbit = 128kB = 2^17B = 0x020000B)
#define RAM_BUFF_SIZE 0x020000

// Initialize the ram buffer and clear
void ramBuffer_init(void);

// Write count bytes from data into the ram buffer
void ramBuffer_write(void *data, uint32_t count);

// Read count bytes from the ram buffer into data
uint32_t ramBuffer_read(void *data, uint32_t count);

#endif /* RAM_BUFFER_H_ */
