/*
 * fixed.c
 *
 *  Created on: Mar 1, 2015
 *      Author: Kyle
 */
#include "fixed.h"

// Convert floating point value to fixed point
fix64_t floatToFix(float fp){
	fp *= ((uint64_t)1 << 32); // Shift exponent by 32
	int64_t li = (int64_t)fp;
	return *(fix64_t*) &li;
}

// Convert integer value to fixed point
void intToFix(fix64_t *fx, int32_t i){
	fx->_int = i;
	fx->frac = 0;
}

// Convert fixed point value to floating point
float fixToFloat(fix64_t *fx){
	return (float)(*(int64_t*)fx) / ((uint64_t)1 << 32);
}

// Add dest to scr, store in dest
// inline if needed for performance
void fix_add(fix64_t *dest, fix64_t *src){
	int64_t *res = (int64_t*)dest;
	*res = *(int64_t*)dest + *(int64_t*)src;
}

// Subtract src from dest, store in dest
// inline if needed for performance
void fix_sub(fix64_t *dest, fix64_t *src){
	int64_t *res = (int64_t*)dest;
	*res = *(int64_t*)dest - *(int64_t*)src;
}

// Multiply dest by src, store in dest
// inline if needed for performance
void fix_mult(fix64_t *dest, fix64_t *src){
	int64_t *res = (int64_t*)dest;
	*res = (((int64_t)src->_int * (int64_t)dest->_int) << 32) +
			 (int64_t)src->_int * (int64_t)dest->frac +
			 (int64_t)src->frac * (int64_t)dest->_int +
		   (((int64_t)src->frac * (int64_t)dest->frac) >> 32);
}
