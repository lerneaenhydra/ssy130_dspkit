#include "sources.h"
#include "config.h"
#include <math.h>
#include <stdint.h>
#include "backend/hw/headphone.h"
#include "backend/printfn/printfn.h"
#include "util.h"
#include "arm_math.h"
#include "waveform1.h"
#include "waveform2.h"
#include "waveform3.h"
#include "waveform4.h"
#include "waveform5.h"
#include "waveform6.h"
#include "waveform_theonlysolution.h"
#include "waveform_jul.h"

/** @brief Channel response for unit-testing LMS algorithm */
float h_sim_int[] = {10, -8, 6, -4, 2, -1, 1};

/** @brief Reference input signal to pass through simualted channel h, used for 
unit-testing LMS filter */
float test_x[] = {14.8932485, -36.4125825, 49.6098742, -35.4846256, 33.5805224, 
-37.2492035, 20.8040165, -15.8582675, 13.4601244, -23.3828053, 23.2106558, 
-29.2438718, 20.4087478, -7.21076326, 6.33646669, 7.29170102, 2.56950095, 
-9.34445857, 9.55170621, -16.9394193, -0.260314187, 11.902817, -9.38023172, 
7.48980224, 4.40947625, 0.554168547, 2.18526666, 10.4388343, 0.895980308, 
1.28314079, -7.22703933, -0.528340904, 12.9917571, -26.003008, 21.7181535, 
-34.105149, 34.4417038, -16.2093727, 9.7912756, -5.11442585, -23.7706529, 
24.0264104, -41.0469766, 8.4977257, -1.0262878, -11.6924303, 14.0071515, 
-4.39772103, 9.71108348, -15.6537233, 14.3129292, -10.1725884, 14.3665133, 
-4.00377257, 11.6911611, -9.52909365, 4.82707489, 7.2190423, -27.5685094, 
17.9532188, -24.7068563, 13.8368214, -1.46003582, 7.97696244};

/** @brief Result of passing x through h, used tor unit-testing LMS filter */
float test_y[]={2.02369089, -2.25835397, 2.22944568, 0.337563701, 1.00006082, 
-1.66416447, -0.590034564, -0.278064164, 0.422715691, -1.6702007, 0.471634326, 
-1.2128472, 0.0661900484, 0.652355889, 0.327059967, 1.0826335, 1.00607711, 
-0.650907737, 0.257056157, -0.944377806, -1.32178852, 0.924825933, 
4.98490753e-05, -0.0549189146, 0.911127266, 0.594583697, 0.350201174, 
1.25025123, 0.929789459, 0.239763257, -0.690361103, -0.651553642, 1.19210187, 
-1.61183039, -0.0244619366, -1.94884718, 1.02049801, 0.861716302, 0.00116208348, 
-0.0708372132, -2.48628392, 0.581172323, -2.19243492, -2.31928031, 0.0799337103, 
-0.948480984, 0.411490621, 0.676977806, 0.857732545, -0.691159125, 0.449377623, 
0.10063335, 0.826069998, 0.53615708, 0.897888426, -0.131937868, -0.147201456, 
1.00777341, -2.12365546, -0.504586406, -1.27059445, -0.382584803, 0.648679262, 
0.825727149};

/** @brief The next element from wavetable based waveforms */
uint_fast32_t dist_idx = 0;
uint_fast32_t waveform_idx = 0;
uint_fast32_t test_y_idx = 0;
uint_fast32_t test_x_idx = 0;

/** @brief The current angle for sine/cosine sources */
float trig_ang = 0;

/** @brief The current frequency to use for sine/cosine outputs, default to 1kHz */
float trig_freq = 1e3;

const int16_t * waveform_ptr = waveform;
size_t waveform_N = NUMEL(waveform);

#define MSG "\n\n  .-\" +' \"-.    __,  ,___,\n /.'.'A_'*`.\\  (--|__| _,,_ ,_ \n|:.*'/\\-\\. ':|   _|  |(_||_)|_)\\/\n|:.'.||\"|.'*:|  (        |  | _/\n \\:~^~^~^~^:/          __,  ,___,\n  /`-....-'\\          (--|__| _ |' _| _,   ,\n /          \\           _|  |(_)||(_|(_|\\//_)\n `-.,____,.-'          (               _/\n\n" DEBUG_LINESEP
void blocks_sources_init(void){
	#ifdef JUL_AVAILABLE
	#ifdef SYSMODE_LMS
		uint_fast32_t seed = util_get_seed();
		uint_fast16_t r = util_rand_range(0, 25, &seed);
		if(r == 0){
			waveform_ptr = waveform_jul;
			waveform_N = NUMEL(waveform_jul);
			printf(MSG);
		}
	#endif
	#endif
}

int blocks_sources_get_h_sim_len(void){
	return NUMEL(h_sim_int);
}

void blocks_sources_get_h_sim(float * h_sim){
	int i;
	for(i = 0; i < NUMEL(h_sim_int); i++){
		h_sim[i] = h_sim_int[i];
	}
}

void blocks_sources_zeros(float * sample_block){
	arm_fill_f32(0.0f, sample_block, AUDIO_BLOCKSIZE);
}

void blocks_sources_ones(float * sample_block){
	arm_fill_f32(1.0f, sample_block, AUDIO_BLOCKSIZE);
}

void blocks_sources_trig_setfreq(float frequency){
	ATOMIC(trig_freq = frequency);
}

void blocks_sources_sin(float * sample_block){
	int_fast32_t i;
	float ang = trig_ang;
	volatile float freq;	//Declare volatile to ensure the math operations are not re-ordered into the atomic block
	ATOMIC(freq = trig_freq);
	const float delta_ang = M_TWOPI * freq / AUDIO_SAMPLE_RATE;
	for(i = 0; i < AUDIO_BLOCKSIZE; i++){
		sample_block[i] = arm_sin_f32(ang);
		ang += delta_ang;
	}
}

void blocks_sources_cos(float * sample_block){
	int_fast32_t i;
	float ang = trig_ang;
	volatile float freq;	//Declare volatile to ensure the math operations are not re-ordered into the atomic block
	ATOMIC(freq = trig_freq);
	const float delta_ang = M_TWOPI * freq / AUDIO_SAMPLE_RATE;
	for(i = 0; i < AUDIO_BLOCKSIZE; i++){
		sample_block[i] = arm_cos_f32(ang);
		ang += delta_ang;
	}
}

void blocks_sources_waveform(float * sample_block){
	int_fast32_t i;
	for(i = 0; i < AUDIO_BLOCKSIZE; i++){
		sample_block[i] = waveform_ptr[waveform_idx++] * (1.0f/INT16_MAX);
		if(waveform_idx >= waveform_N){
			waveform_idx = 0;
		}
	}
}

void blocks_sources_test_x(float * sample_block){
	int_fast32_t i;
	for (i = 0; i < AUDIO_BLOCKSIZE; i++) {
		sample_block[i] = test_x[test_x_idx++];
		if (test_x_idx >= NUMEL(test_x)) {
			test_x_idx = 0;
		}
	}
}

void blocks_sources_test_y(float * sample_block){
	int_fast32_t i;
	for (i = 0; i < AUDIO_BLOCKSIZE; i++) {
		sample_block[i] = test_y[test_y_idx++];
		if (test_y_idx >= NUMEL(test_y)) {
			test_y_idx = 0;
		}
	}
}

void blocks_sources_microphone(float * sample_block){
	arm_copy_f32(processed_micdata, sample_block, AUDIO_BLOCKSIZE);
}

void blocks_sources_update(void){
	//Compute the relative change in angle between each sample for the requested frequency
	volatile float freq;	//Declare volatile to ensure the math operations are not re-ordered into the atomic block
	ATOMIC(freq = trig_freq);
	const float delta_ang = M_TWOPI * freq / AUDIO_SAMPLE_RATE;
 	trig_ang += delta_ang * AUDIO_BLOCKSIZE;
	//Keep trig_ang in range [0, 2*pi] to keep float epsilon small
	//For reasonable frequencies and block-sizes, repeated subtraction is
	//cheaper than using a divide and multiply operation (and has clearer
	//intent).
	while(trig_ang > M_TWOPI){
		trig_ang -= M_TWOPI;
	}
}
