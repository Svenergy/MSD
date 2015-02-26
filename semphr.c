/*
 * semphr.c
 *
 *  Created on: Feb 24, 2015
 *      Author: Kyle
 *
 *  Lightweight binary semaphore implementation using a spinlock
 */

#include "semphr.h"

/* Create and return the mutex object */
Semaphore_t semaphore_Create()
{
	Semaphore_t mutex = 0;
	return mutex;
}

/* Take the mutex
 * Returns false on timeout, true if mutex taken
 * timeOut is clock ticks to spin before giving up
 */
int semaphore_Take(volatile Semaphore_t *mutex, int timeOut)
{
	uint32_t startTime = DWT_Get();

	while(DWT_Get() - startTime < timeOut)
	{
		void __disable_irq(void);
		if(*mutex != 1){
			*mutex = 1;
			return true;
		}
		void __enable_irq(void);
	}
	return false; // Time out
}

/* Release the mutex */
void semaphore_Give(volatile Semaphore_t *mutex)
{
	*mutex = 0;
}
