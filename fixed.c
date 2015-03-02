/*
 * fixed.c
 *
 *  Created on: Mar 1, 2015
 *      Author: Kyle
 */
#include <math.h>

#include "fixed.h"

// Powers of 10 used for fast lookup
const uint32_t pow10[10] = {1,10,100,1000,10000,100000,1000000,10000000,100000000,100000000};

// Convert floating point value to decimal exponent floating point
dec_float_t floatToDecFloat(float fp){
	dec_float_t df;

	// Find nearest smaller power of 10
	df.exp = (int32_t)floor( log10( fabs(fp) ) );
	float pow10 = pow(10, df.exp);

	fp /= pow10; // Divide floating point by this power of 10
	fp *= ((uint64_t)1 << 32); // Multiply floating point by (1 << 32)
	int64_t *res = (int64_t*) &df; // cast to int64_t
	*res = (int64_t)fp;
	return df;
}

// Convert decimal floating point to string, return length of string
// Conversion assumes only integer portion of fix64_t is significant
// Precision should be set in the range 1 to 6
int32_t decFloatToStr(dec_float_t *df, char *str, int8_t precision){
	int32_t strSize = 0;

	/* Calculate and print sign */
	int32_t sig = df->_int;
	if(sig<0){
		str[strSize++] = '-';
		sig = -sig;
	}

	/* Calculate exponent */
	int8_t j;
	for(j=9;j>=0;j--){
		if(pow10[j] < sig){
			break;
		}
	}
	int32_t exp = df->exp + j;

	/* Calculate significand */
	if(j >= precision){
		sig /= pow10[j-precision];
	}else{
		sig *= pow10[precision-j];
	}

	/* Print integer part */
	int32_t f = sig/pow10[precision];
	str[strSize++] = '0' + (char)f;
	str[strSize++] = '.';
	sig -= f * pow10[precision];

	/* Print fractional part */
	for(j=1;j<=precision;j++){
		str[strSize+precision-j] = '0' + (char)(sig%10);
		sig /= 10;
	}
	strSize += precision;

	/*Print exponent */
	str[strSize++] = 'e';
	if(exp<0){
		str[strSize++] = '-';
		exp = -sig;
	}else{
		str[strSize++] = '+';
	}
	str[strSize+1] = '0' + (char)(exp%10);
	exp /= 10;
	str[strSize] = '0' + (char)(exp%10);
	return strSize+2;
}

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
