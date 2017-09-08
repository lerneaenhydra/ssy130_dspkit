#include "gen.h"
#include "arm_math.h"
#include "config.h"

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
