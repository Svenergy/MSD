/*
 * semphr.h
 *
 *  Created on: Feb 24, 2015
 *      Author: Kyle
 *
 *  Lightweight binary semaphore implementation using a spinlock
 */

#ifndef SEMPHR_H_
#define SEMPHR_H_

#include <stdlib.h>

#include "board.h"
#include "delay.h"

typedef int Semaphore_t;

/* Create and return the mutex object */
Semaphore_t semaphore_Create();

/* Take the mutex
 * Returns false on timeout, true if mutex taken
 * timeOut is clock ticks to spin before giving up
 */
int semaphore_Take(volatile Semaphore_t *mutex, int timeOut);

/* Release the mutex */
void semaphore_Give(volatile Semaphore_t *mutex);

#endif /* SEMPHR_H_ */
