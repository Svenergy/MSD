/*
 * ram_buffer.c
 *
 *  Created on: May 7, 2015
 *      Author: Kyle Smith
 */

#include "ram_buffer.h"

static int32_t length;	// number of bytes the ring buffer can hold + 1
static int32_t start;	// index of the start
static int32_t end;

// Write count bytes into the spi ram starting at address
static void SPI_WriteBytes(void *data, uint32_t address, uint32_t count) {
	// Send write command
	while(~LPC_SPI0->STAT & SPI_STAT_TXRDY){};
	LPC_SPI0->TXDATCTL = SPI_TXDATCTL_LEN(8-1) | SPI_TXCTL_DEASSERT_SSEL0 | SPI_TXCTL_ASSERT_SSEL1 | SPI_TXCTL_RXIGNORE | INST_WRITE;

	// Send address
	for(int32_t sh = 16;sh >= 0;sh-=8){
		while(~LPC_SPI0->STAT & SPI_STAT_TXRDY){};
		LPC_SPI0->TXDATCTL = SPI_TXDATCTL_LEN(8-1) | SPI_TXCTL_DEASSERT_SSEL0 | SPI_TXCTL_ASSERT_SSEL1 | SPI_TXCTL_RXIGNORE | ((address >> sh) & 0xff);
	}

	// Send data
	for(uint32_t i=0;i<count;i++){
		while(~LPC_SPI0->STAT & SPI_STAT_TXRDY){};
		uint32_t datCtl = SPI_TXDATCTL_LEN(8-1) | SPI_TXCTL_DEASSERT_SSEL0 | SPI_TXCTL_ASSERT_SSEL1 | SPI_TXCTL_RXIGNORE | ((uint8_t*)data)[i];
		if(i == count-1){
			datCtl |= SPI_TXDATCTL_EOT;
		}
		LPC_SPI0->TXDATCTL = datCtl;
	}
}

// Read count bytes from the spi ram starting at address
static void SPI_ReadBytes(void *data, uint32_t address, uint32_t count) {
	// Send read command
	while(~LPC_SPI0->STAT & SPI_STAT_TXRDY){};
	LPC_SPI0->TXDATCTL = SPI_TXDATCTL_LEN(8-1) | SPI_TXCTL_DEASSERT_SSEL0 | SPI_TXCTL_ASSERT_SSEL1 | SPI_TXCTL_RXIGNORE | INST_READ;

	// Send address
	for(int32_t sh = 16;sh >= 0;sh-=8){
		while(~LPC_SPI0->STAT & SPI_STAT_TXRDY){};
		LPC_SPI0->TXDATCTL = SPI_TXDATCTL_LEN(8-1) | SPI_TXCTL_DEASSERT_SSEL0 | SPI_TXCTL_ASSERT_SSEL1 | SPI_TXCTL_RXIGNORE | ((address >> sh) & 0xff);
	}

	// read data
	for(uint32_t i=0;i<count;i++){
		while(~LPC_SPI0->STAT & SPI_STAT_TXRDY){};
		uint32_t datCtl = SPI_TXDATCTL_LEN(8-1) | SPI_TXCTL_DEASSERT_SSEL0 | SPI_TXCTL_ASSERT_SSEL1;
		if(i == count-1){
			datCtl |= SPI_TXDATCTL_EOT;
		}
		LPC_SPI0->TXDATCTL = datCtl;
		while(~LPC_SPI0->STAT & SPI_STAT_RXRDY){};
		((uint8_t*)data)[i] = LPC_SPI0->RXDAT;
	}
}

// Initialize the ram buffer and clear
void ramBuffer_init(void){
	length = RAM_BUFF_SIZE;
	start = 0;
	end = 0;
}

// Write count bytes from data into the ram buffer
void ramBuffer_write(void *data, uint32_t count){
	// Error if data would be overwritten before being read
	if(count + ((end - start + length) % length)  >= length ){
		error(ERROR_BUF_OVF);
	}

	// Copy the data into the ring buffer
	if(end + count > length){
		int32_t partLen = length - end;
		SPI_WriteBytes(data, end, partLen);
		SPI_WriteBytes(data + partLen, 0, count-partLen);
	}else{
		SPI_WriteBytes(data, end, count);
	}

	// Move the end
	end = (end + count) % length;
}

// Read count bytes from the ram buffer into data
uint32_t ramBuffer_read(void *data, uint32_t count){
	int32_t br;
	int32_t rend; // read up to this value

	if(count >= (end - start + length) % length){ // if count >= than available data, read all
		rend = end;
	}else{
		rend = (start + count) % length; // if count < available data, read count
	}

	// Copy data
	if(rend == start){
		br = 0;
	}else if(rend > start){
		br = rend - start;
		memcpy(data, b->buffer + b->start, br);
		SPI_ReadBytes(data, start, br)
	}else{
		br = length - start;
		SPI_ReadBytes(data, start, br);
		SPI_ReadBytes(data + br, 0, rend);
		br = rend + length - start;
	}

	// Move start to end
	start = rend;

	// Return number of bytes read
	return br;
}
