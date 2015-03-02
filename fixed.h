/*
 * fixed.h
 *
 *  Created on: Mar 1, 2015
 *      Author: Kyle
 */

#ifndef FIXED_H_
#define FIXED_H_

#include "board.h"

// Signed, 32-bit fractional paer, 32-bit integer part
typedef struct fix64_t {
	uint32_t frac;
	int32_t  _int;
} fix64_t;

// Floating point type built from a fix64_t and a signed 8-bit decimal exponent
typedef struct dec_float_t {
	uint32_t frac;
	int32_t  _int;
	int32_t exp;
} dec_float_t;

// Convert time in seconds and microseconds to string, return length
int32_t secondsToStr(char *str, uint32_t s, uint32_t us, int8_t precision);

// Convert floating point value to decimal exponent floating point
dec_float_t floatToDecFloat(float fp);

// Convert decimal floating point to string, return length of string
int32_t decFloatToStr(char *str, dec_float_t *df, int8_t precision);

// Convert floating point value to fixed point
fix64_t floatToFix(float fp_val);

// Convert integer value to fixed point
void intToFix(fix64_t *fx, int32_t i);

// Convert fixed point value to floating point
float fixToFloat(fix64_t *fix_val);

// Add dest to scr, store in dest
void fix_add(fix64_t *dest, fix64_t *src);

// Subtract src from dest, store in dest
void fix_sub(fix64_t *dest, fix64_t *src);

// Multiply dest by src, store in dest
void fix_mult(fix64_t *dest, fix64_t *src);

#endif /* FIXED_H_ */
