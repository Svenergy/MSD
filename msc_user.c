/*
 * msc_user.c
 *
 *  Created on: 2014.07.04
 *      Author: Kestutis Bivainis
 *
 *  Modified by Kyle Smith to allow multiple block read
 */

#include <cr_section_macros.h>
#include <string.h>
#include <stdio.h>

#include "board.h"
#include "msc_usb.h"
#include "app_usbd_cfg.h"
#include "error.h"
#include "sd_spi.h"
#include "sys_error.h"

#define BLOCK_COUNT 8

// read buffer
uint8_t bufr[SD_BLOCKSIZE*BLOCK_COUNT];

// write buffer
uint8_t bufw[SD_BLOCKSIZE];

// verify buffer
uint8_t bufv[SD_BLOCKSIZE];

// Zero-Copy Data Transfer model
void MSC_Read(uint32_t offset, uint8_t** buff_adr, uint32_t length, uint32_t high_offset) { // high offset is always 0 and cannot be used to increase capacity >4gb
  static bool bufferValid = false; // Set to indicate that the buffer data can be read

  // Host requests data in chunks of 512 bytes, USB bulk endpoint size is 64 bytes.
  // For each sector of 512 bytes, this function gets called 8 times with length=64 bytes
  Board_LED_Color(LED_PURPLE); // Purple MSC r/w

  // SD block address of the buffer data
  static uint32_t address;

  // Block address is a multiple of BLOCK_COUNT
  uint32_t new_address = ((offset/SD_BLOCKSIZE)/BLOCK_COUNT)*BLOCK_COUNT;

  // If the requested data is not in the current buffered blocks, or is in the very first buffer
  // address is initialized to 0, therefore valid data is not guaranteed for blocks near 0, even when address is 0
  if(new_address < address || new_address >= address + BLOCK_COUNT || bufferValid == false){
	address = new_address;
	if(sd_read_multiple_blocks(address, BLOCK_COUNT, bufr)!=SD_OK){
	  error(ERROR_MSC_SD_READ);
	}
	bufferValid = true;
  }

  // Set pointer in buffer
  uint32_t j = offset%(SD_BLOCKSIZE*BLOCK_COUNT);
  *buff_adr = &bufr[j];

  Board_LED_Color(LED_YELLOW); // Yellow MSC idle
}

// TODO: Rewrite to use multiple block write
void MSC_Write(uint32_t offset, uint8_t** buff_adr, uint32_t length, uint32_t high_offset) {
  Board_LED_Color(LED_PURPLE); // Purple MSC r/w
  uint32_t j = offset%SD_BLOCKSIZE;

  // Host requests data in chunks of 512 bytes, USB bulk endpoint size is 64 bytes.
  // For each sector of 512 bytes, this function gets called 8 times with length=64 bytes
  // Accumulate all requests in the buffer, and on the last request write 512 bytes to the card.
  memcpy(&bufw[j],*buff_adr,length);

  if((offset+USB_FS_MAX_BULK_PACKET)%SD_BLOCKSIZE==0) {
    if(sd_write_block(offset/SD_BLOCKSIZE, bufw)!=SD_OK){
    	error(ERROR_MSC_SD_WRITE);
    }
  }
  Board_LED_Color(LED_YELLOW); // Yellow MSC idle
}

// TODO: untested
ErrorCode_t MSC_Verify(uint32_t offset, uint8_t* src, uint32_t length, uint32_t high_offset) {
  
  uint32_t j = offset%SD_BLOCKSIZE;
  
  if(j==0) {
    sd_read_block(offset/SD_BLOCKSIZE,bufv);
  }  
  
  if(!memcmp(src,&bufv[j],length)) {
    return ERR_FAILED;
  }
  else {
    return LPC_OK;
  }
}
