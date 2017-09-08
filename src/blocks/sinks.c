#include "sinks.h"
#include "backend/hw/headphone.h"
#include "arm_math.h"

float left_buf[AUDIO_BLOCKSIZE];
float right_buf[AUDIO_BLOCKSIZE];

/** @brief Copy AUDIO_BLOCKSIZE samples to the left output buffer */
void blocks_sinks_leftout(float * data){
	arm_copy_f32(data, left_buf, AUDIO_BLOCKSIZE);
}

/** @brief Copy AUDIO_BLOCKSIZE samples to the right output buffer */
void blocks_sinks_rightout(float * data){
	arm_copy_f32(data, right_buf, AUDIO_BLOCKSIZE);
}

float * blocks_sinks_leftout_ptr(void){
	return left_buf;
}

float * blocks_sinks_rightout_ptr(void){
	return right_buf;
}
