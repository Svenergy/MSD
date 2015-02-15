#include "ring_buff.h"

// Allocate memory for the buffer
RingBuffer *RingBuffer_init(int32_t length){
    RingBuffer *buffer = malloc(sizeof(RingBuffer));
    buffer->length  = length + 1;
    buffer->start = 0;
    buffer->end = 0;
    buffer->buffer = malloc(buffer->length);
    return buffer;
}

// Free memory used by the buffer
void RingBuffer_destroy(RingBuffer *buffer){
    if(buffer) {
        free(buffer->buffer);
        free(buffer->returnBuffer);
        free(buffer);
    }
}

// Write string into the ring buffer
void RingBuffer_write(RingBuffer *b, char *string){
	// Get the length of the string
	int32_t len = strlen(string);

	// Error if data would be overwritten before being read
	if(len + ((b->end - b->start + b->length) % b->length)  >= b->length ){
	#ifdef DEBUG
		putLineUART("\nbf_ovr");
	#endif
		error();
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

// Read count bytes into data, return count of byte read
int32_t RingBuffer_read(RingBuffer *b, char *data, int32_t count){
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


/*
// Read all data in the ring buffer and output as a string
char* RingBuffer_reads(RingBuffer *b){
	int32_t count;
	int32_t end = b->end; // Buffer the value of end in case read is interrupted by a write

	// Copy data
	if(end > b->start){
		count = end - b->start;
		memcpy(b->returnBuffer, b->buffer + b->start, count);
	}else{
		count = b->length - b->start;
		memcpy(b->returnBuffer, b->buffer + b->start, count);
		memcpy(b->returnBuffer + count, b->buffer, end);
		count = end + b->length - b->start;
	}

	// Move start to end
	b->start = end;

	// Append null terminator
	b->returnBuffer[count] = '\0';

	// Return data read
	return b->returnBuffer;
}
*/
