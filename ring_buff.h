#ifndef __RING_BUFF_
#define __RING_BUFF_

#include <string.h>
#include <stdlib.h>
#include "board.h"

typedef struct RingBuffer{
    char *buffer; // ring buffer data
    char *returnBuffer; // returned data
    int32_t length; // number of bytes the ring buffer can hold + 1
    int32_t start; // index of the start
    int32_t end; // index of the end
} RingBuffer;

// Allocate memory for the buffer
RingBuffer *RingBuffer_init(int32_t length);

// Free memory used by the buffer
void RingBuffer_destroy(RingBuffer *buffer);

// Write string into the ring buffer
void RingBuffer_write(RingBuffer *buffer, char *data);

// Read count bytes into data, return count of byte read
int32_t RingBuffer_read(RingBuffer *b, char *data, int32_t count);

/*
// Read all data in the ring buffer and output as a string
char* RingBuffer_reads(RingBuffer *b);
*/

#endif /* __RING_BUFF_ */
