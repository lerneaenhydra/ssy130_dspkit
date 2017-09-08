#include "windows.h"
#include <math.h>

void windows_blackman(float * coeffs, uint_fast32_t len){
	uint_fast32_t i;
	const float alpha = 0.16f;
	const float a0 = (1.0f - alpha)/2.0f;
	const float a1 = 1.0f/2.0f;
	const float a2 = alpha / 2.0f;
	for(i = 0; i < len; i++){
		*(coeffs + i) = a0 - a1 * cos(2.0f * M_PI * i / (len - 1)) + a2 * cos(4.0f * M_PI * i / (len - 1));
	}
}
