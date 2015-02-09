#ifndef __PUSH_BUTTON_
#define __PUSH_BUTTON_

#include <stdbool.h>
#include "board.h"

int pbCounter;
int pbLongPress;
int pbShortPress;
int pbFirstPress;

int pb_tickRateHz;

// Initialize the push button
void pb_init(int tickRateHz);

// Run in the main program loop, ideally at 100Hz or greater
void pb_loop(void);

// Returns true if a short press has occurred
int pb_shortPress(void);

// Returns true if a long press has occurred
int pb_longPress(void);


#endif /* __PUSH_BUTTON_ */
