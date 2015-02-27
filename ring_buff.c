#include "ring_buff.h"

// Allocate memory for the buffer and return a ring buffer struct
RingBuffer *RingBuffer_init(int32_t length){
    RingBuffer *buffer = malloc(sizeof(RingBuffer));
    buffer->length  = length + 1;
    buffer->start = 0;
    buffer->end = 0;
    buffer->buffer = malloc(buffer->length);
    return buffer;
}

// Return a ring buffer struct, using a user set buffer
RingBuffer *RingBuffer_initWithBuffer(int32_t length, char *pBuffer){
    RingBuffer *buffer = malloc(sizeof(RingBuffer));
    buffer->length  = length + 1;
    buffer->start = 0;
    buffer->end = 0;
    buffer->buffer = pBuffer;
    return buffer;
}

// Free memory used by the buffer
void RingBuffer_destroy(RingBuffer *buffer){
    if(buffer) {
        free(buffer->buffer);
        free(buffer);
    }
}

// Write string into the ring buffer
void RingBuffer_writeStr(RingBuffer *b, char *string){
	// Get the length of the string
	int32_t len = strlen(string);

	// Error if data would be overwritten before being read
	if(len + ((b->end - b->start + b->length) % b->length)  >= b->length ){
		error(ERROR_BUF_OVF);
	}

	// Copy the string into the ring buffer
	if(b->end + len > b->length){
		int32_t partLen = b->length - b->end;
		memcpy(b->buffer + b->end, string, partLen);
		memcpy(b->buffer, string + partLen, len-partLen);
	}else{
		memcpy(b->buffer + b->end, string, len);
	}

	// Move the end
	b->end = (b->end + len) % b->length;
}

// Write data into the ring buffer
void RingBuffer_writeData(RingBuffer *b, void *data, int32_t count){
	// Error if data would be overwritten before being read
	if(count + ((b->end - b->start + b->length) % b->length)  >= b->length ){
		error(ERROR_BUF_OVF);
	}

	// Copy the data into the ring buffer
	if(b->end + count > b->length){
		int32_t partLen = b->length - b->end;
		memcpy(b->buffer + b->end, data, partLen);
		memcpy(b->buffer, data + partLen, count-partLen);
	}else{
		memcpy(b->buffer + b->end, data, count);
	}

	// Move the end
	b->end = (b->end + count) % b->length;
}

// Read count bytes into data, return count of byte read
int32_t RingBuffer_read(RingBuffer *b, void *data, int32_t count){
	int32_t br;
	int32_t end; // read up to this value

	if(count >= (b->end - b->start + b->length) % b->length){ // if count >= than available data, read all
		end = b->end;
	}else{
		end = (b->start + count) % b->length; // if count < available data, read count
	}

	// Copy data
	if(end == b->start){
		br = 0;
	}else if(end > b->start){
		br = end - b->start;
		memcpy(data, b->buffer + b->start, br);
	}else{
		br = b->length - b->start;
		memcpy(data, b->buffer + b->start, br);
		memcpy(data + br, b->buffer, end);
		br = end + b->length - b->start;
	}

	// Move start to end
	b->start = end;

	// Return number of bytes read
	return br;
}

// Return the size of the current data in the buffer
int32_t RingBuffer_getSize(RingBuffer *b){
	return (b->end - b->start + b->length) % b->length;
}

// Clear the buffer
void RingBuffer_clear(RingBuffer *b){
	b->end = b->start = 0;
}
