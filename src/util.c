#include "util.h"
#include <math.h>
#include "backend/printfn/printfn.h"
#include "backend/hw/board.h"

#define RAND_R_MULT	1103515245L
#define RAND_R_INC	12345L
#define RAND_SHIFT	16

uint_fast32_t util_get_seed(void){
	return board_get_randseed();
}

/** @brief Simple (and relatively crude) LCG random number generator. */
uint_fast16_t util_rand_r(uint_fast32_t * const seed){
	uint_fast16_t retval = 0;
	uint_fast32_t seed_shdw = *seed;
	const uint_fast32_t dummy = (seed_shdw * ((uint_fast32_t)RAND_R_MULT)) + ((uint_fast32_t)RAND_R_INC);	//Raw output from LCG
	*seed = dummy;
	retval = (uint_fast16_t)((dummy >> RAND_SHIFT) % ( ( (uint_fast32_t)R_RAND_MAX ) + 1) );
	return retval;
}

/** @brief Outputs a random integer in the range [minval,maxval], given a random seed location */
uint_fast16_t util_rand_range(uint_fast16_t minval, uint_fast16_t maxval, uint_fast32_t * const seed){
	if(minval > maxval){
		uint_fast16_t temp  = minval;
		minval = maxval;
		maxval = temp;
	}
	if(minval == maxval){
		return minval;
	}
	
	//We now know minval < maxval and there are at least two valid return values
	const uint_fast16_t raw_rand = util_rand_r(seed);
	const uint_fast16_t range = maxval - minval + 1;	//The range of values we want to generate
	const uint_fast16_t rem = R_RAND_MAX % range;
	const uint_fast16_t bucket = R_RAND_MAX / range;
	
	if(raw_rand < R_RAND_MAX - rem){		//If the raw random value we got does not lie in the small last bucket, we have a safe value to return
		return minval + raw_rand / bucket;
	}else{
		//Otherwise, try again...
		return util_rand_range(minval, maxval, seed);
	}
}

void util_randN(const float mu, const float sigma, uint_fast32_t * const seed, float * res, size_t len){
	//Use the marsaglia polar method
	size_t i = 0;
	//Loop continuously, will break when done
	for(;;){
		float s, u, v;
		do{
			//Generate uniformly sampled variables in range [-1, 1]
			const uint_fast16_t ui = util_rand_r(seed);
			const uint_fast16_t vi = util_rand_r(seed);
			u = ui * (1.0f / R_RAND_MAX) * 2.0f - 1.0f; 	
			v = vi * (1.0f / R_RAND_MAX) * 2.0f - 1.0f;

			//Check norm of [u,v]
			s = u * u + v * v;
		}while(s >= 1.0f || s == 0.0f);


		//Generate two normally distributed values
		s = sqrtf(-2.0f * logf(s) / s);
		float r1 = mu + sigma * u * s;
		float r2 = mu + sigma * v * s;

		//Write results as needed
		res[i++] = r1;
		if(i >= len){
			return;
		}
		res[i++] = r2;
		if(i >= len){
			return;
		}
	}
}

void print_vector_f(char * name, float * vals, int_fast32_t len){
	printf("%s = [", name);
	int_fast32_t i;
	for(i = 0; i < len; i++){
		if(i != 0){
			//Suppress a tab character on the first output
			board_usart_write('\t');
		}

		printf("%g", *(vals + i));
		if(i == len - 1){
			//On the last call, add a ']' symbol to indicate the end of the vector
			board_usart_write(']');
		}
		board_usart_write(';');
		board_usart_write('\n');
		board_usart_write('\r');
	}
}

void print_cplx_vector_f(char * prefix, float * vals, int_fast32_t len){
	printf("%s [", prefix);
	int_fast32_t i;
	for(i = 0; i < len/2; i++){
		if(i != 0){
			//Suppress a tab character on the first output
			printf("\t");
		}

		printf("%e", *(vals + 2*i));
		printf(" + %e*i", *(vals + 2*i + 1));
		if(i == (len/2 - 1)){
			//On the last call, add a ']' symbol to indicate the end of the vector
			printf("]");
		}
		printf(";\n");
	}
}

void print_vector_i(char * prefix, int32_t * vals, int_fast32_t len){
	printf("%s [", prefix);
	int_fast32_t i;
	for(i = 0; i < len; i++){
		if(i != 0){
			//Suppress a tab character on the first output
			printf("\t");
		}

		printf("%d", *(vals + i));
		if(i == len - 1){
			//On the last call, add a ']' symbol to indicate the end of the vector
			printf("]");
		}
		printf(";\n");
	}
}

void vector_f2i(float * vals_f, int32_t * vals_i, int_fast32_t len, float scale){
	int_fast32_t i;
	for(i = 0; i < len; i++){
		*(vals_i + i) = (int32_t) (*(vals_f + i) * scale);
	}
}

float vector_mean(float * vals, int_fast32_t len){
	float retval = 0;
	int_fast32_t i;
	for(i = 0; i < len; i++){
		retval += *(vals + i);
	}
	retval /= len;
	return retval;
}

void halt_error(char * errmsg){
	printf("%s", errmsg);
	board_set_led(board_led_red, false);
	board_set_led(board_led_orange, false);
	board_set_led(board_led_green, false);
	board_set_led(board_led_blue, false);
	for(;;){
		//Blink all status LED's
		volatile uint_fast32_t i;
		for(i = 0; i < 10e6; i++){};	//Crude delay function
		board_toggle_led(board_led_red);
		board_toggle_led(board_led_orange);
		board_toggle_led(board_led_green);
		board_toggle_led(board_led_blue);
	}
}
