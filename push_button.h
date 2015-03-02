#ifndef __PUSH_BUTTON_
#define __PUSH_BUTTON_

#include "board.h"

typedef enum{
	READY,
	TRIGGERED,
	LONGPRESS
} PB_STATE;

// Initialize the push button
void pb_init(void);

// Returns true if a short press has occurred
bool pb_shortPress(void);

// Returns true if a long press has occurred
bool pb_longPress(void);


/* Functions for controlling Pin Interrupt in Level-sensitive mode */
STATIC INLINE void PININT_ClearIntAndFlipActiveLevel(LPC_PIN_INT_T *pPININT, uint32_t pins)
{
	pPININT->IST = pins;
}

STATIC INLINE void PININT_EnableLevelInt(LPC_PIN_INT_T *pPININT, uint32_t pins)
{
	pPININT->SIENR = pins;
}

STATIC INLINE void PININT_DisableLevelInt(LPC_PIN_INT_T *pPININT, uint32_t pins)
{
	pPININT->CIENR = pins;
}

STATIC INLINE void PININT_HighActive(LPC_PIN_INT_T *pPININT, uint32_t pins)
{
	pPININT->SIENF = pins;
}

STATIC INLINE void PININT_LowActive(LPC_PIN_INT_T *pPININT, uint32_t pins)
{
	pPININT->CIENF = pins;
}

#endif /* __PUSH_BUTTON_ */
