#ifndef __RING_BUFF_
#define __RING_BUFF_

#include <string.h>
#include <stdlib.h>
#include "board.h"
#include "sys_error.h"

typedef struct RingBuffer{
    char *buffer; // ring buffer data
    int32_t length; // number of bytes the ring buffer can hold + 1
    int32_t start; // index of the start
    int32_t end; // index of the end
} RingBuffer;

// Allocate memory for the buffer and return a ring buffer struct
RingBuffer *RingBuffer_init(int32_t length);

// Return a ring buffer struct, using a user set buffer
RingBuffer *RingBuffer_init_with_buf(int32_t length, char *pBuffer);

// Free memory used by the buffer
void RingBuffer_destroy(RingBuffer *buffer);

// Write string into the ring buffer
void RingBuffer_write(RingBuffer *buffer, char *data);

// Read count bytes into data, return count of byte read
int32_t RingBuffer_read(RingBuffer *b, char *data, int32_t count);

// Return the size of the current data in the buffer
int32_t RingBuffer_get_size(RingBuffer *b);

#endif /* __RING_BUFF_ */