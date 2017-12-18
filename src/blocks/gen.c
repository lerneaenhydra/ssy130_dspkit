#include "gen.h"
#include <string.h>
#include <math.h>
#include "arm_math.h"
#include "config.h"
#include "str_table.h"
#include "../util.h"

void blocks_gen_sin(float frequency, float phase, float * dest, int_fast32_t len){
	const float delta_ang = M_TWOPI * frequency / AUDIO_SAMPLE_RATE;
	float ang = phase;
	while(len--){
		*(dest++) = arm_sin_f32(ang);
		ang += delta_ang;
		if(ang > M_TWOPI){
			ang -= M_TWOPI;
		}
	}
}

void blocks_gen_cos(float frequency, float phase, float * dest, int_fast32_t len){
	const float delta_ang = M_TWOPI * frequency / AUDIO_SAMPLE_RATE;
	float ang = phase;
	while(len--){
		*(dest++) = arm_cos_f32(ang);
		ang += delta_ang;
		if(ang > M_TWOPI){
			ang -= M_TWOPI;
		}
	}
}

void blocks_gen_sinc(float norm_freq, float * dest, size_t len){
	size_t i;
	for(i = 0; i < len; i++){
		float x = (i + 0.5f) - ((float) len) / 2;
		if(x == 0){
			dest[i] = 1.0f;
		}else{
			dest[i] = arm_sin_f32(M_TWOPI * norm_freq * x) / (M_TWOPI * norm_freq * x);
		}
	}
}

void blocks_gen_str(char * str, int_fast32_t str_len, uint_fast32_t * seed){
	//str must be able to contain at least one element
	if(str_len < STR_TABLE_MAXLEN + 1){
		size_t i;
		for(i = 0; i < str_len; i++){
			str[i] = '\0';
		}
		return;
	}
	
	//We now know at least one element can be added
	size_t strend = str_len;
	size_t currpos = 0;
	
	for(;;){
		//Draw an element from str1, check if it and any element from str2 will fit, if so add it
		uint_fast16_t idx = util_rand_range(0, NUMEL(str1), seed);
		//printf("Generated index %d\n", idx);
		
		char const * append = str1[idx];
		size_t len = strlen(append);
		
		//printf("Candidate string to append '%s' (length %d).\nString currpos %d, strend %d.\n", append, len, currpos, strend);
		
		if(currpos + len < strend - STR_TABLE_MAXLEN - 1){
			strcpy(&str[currpos], append);
			currpos += len;
			//Replace null-terminator with space character
			str[currpos++] = ' ';
			str[currpos] = '\0';
			//printf("Added candidate. Total string now '%s'. Currpos now %d\n", str, currpos);
		}else{
			//If didn't fit, just add an element from str2 and quit
			char const * laststr = str2[idx];
			strcpy(&str[currpos], laststr);
			currpos += strlen(laststr);
			
			//printf("Didn't fit, Added '%s' instead. Total string now '%s', currpos now %d\n", str2[idx], str, currpos);
			break;
		}
	}
	
	//Ensure str is null-padded and explicitly null-terminated
	while(currpos < str_len){
		str[currpos++] = '\0';
	}
	str[str_len - 1] = '\0';
}