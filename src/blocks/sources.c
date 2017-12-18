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

/** @brief One period of the broadband disturbance signal. Identical
 *samples are used regardless of the system sample-rate (i.e. pitch will be
 * shifted by changing the sample-rate) */
float dist_sig[] = {1.000000e+00, 6.387559e-01, 2.995840e-17, -2.234141e-01, -1.587302e-02, 1.212461e-01, -1.233581e-17, -1.000984e-01, -1.587302e-02, 6.362938e-02, 1.321694e-17, -6.637212e-02, -1.587302e-02, 4.138879e-02, 1.762259e-18, -5.056342e-02, -1.587302e-02, 2.955261e-02, -2.366738e-17, -4.135557e-02, -1.587302e-02, 2.217511e-02, 6.230526e-18, -3.530428e-02, -1.587302e-02, 1.711572e-02, 8.660116e-18, -3.100624e-02, -1.587302e-02, 1.341481e-02, -1.057355e-17, -2.778233e-02, -1.587302e-02, 1.057796e-02, 1.057355e-17, -2.526385e-02, -1.587302e-02, 8.324488e-03, -3.826873e-18, -2.323329e-02, -1.587302e-02, 6.483175e-03, 9.390067e-18, -2.155400e-02, -1.587302e-02, 4.943630e-03, -2.366970e-18, -2.013580e-02, -1.587302e-02, 3.631446e-03, -4.695034e-18, -1.891675e-02, -1.587302e-02, 2.494625e-03, -1.078735e-17, -1.785286e-02, -1.587302e-02, 1.495705e-03, 1.364454e-17, -1.691203e-02, -1.587302e-02, 6.069947e-04, 5.286776e-17, -1.607024e-02, -1.587302e-02, -1.924393e-04, 1.762259e-18, -1.530915e-02, -1.587302e-02, -9.187364e-04, 1.762259e-18, -1.461449e-02, -1.587302e-02, -1.584560e-03, 8.811294e-19, -1.397499e-02, -1.587302e-02, -2.200006e-03, 8.811294e-18, -1.338156e-02, -1.587302e-02, -2.773243e-03, -1.775228e-18, -1.282684e-02, -1.587302e-02, -3.310973e-03, -6.230526e-18, -1.230471e-02, -1.587302e-02, -3.818768e-03, -3.373340e-18, -1.181008e-02, -1.587302e-02, -4.301320e-03, 7.049035e-18, -1.133861e-02, -1.587302e-02, -4.762633e-03, -1.762259e-17, -1.088659e-02, -1.587302e-02, -5.206170e-03, 5.286776e-18, -1.045078e-02, -1.587302e-02, -5.634963e-03, 2.764178e-19, -1.002833e-02, -1.587302e-02, -6.051711e-03, -1.027120e-17, -9.616680e-03, -1.587302e-02, -6.458846e-03, -3.124990e-18, -9.213503e-03, -1.587302e-02, -6.858601e-03, 2.137978e-19, -8.816650e-03, -1.587302e-02, -7.253057e-03, -4.833243e-18, -8.424098e-03, -1.587302e-02, -7.644189e-03, 0.000000e+00, -8.033908e-03, -1.587302e-02, -8.033908e-03, -5.286776e-18, -7.644189e-03, -1.587302e-02, -8.424098e-03, 1.762259e-18, -7.253057e-03, -1.587302e-02, -8.816650e-03, 2.643388e-18, -6.858601e-03, -1.587302e-02, -9.213503e-03, 8.811294e-18, -6.458846e-03, -1.587302e-02, -9.616680e-03, -8.908466e-18, -6.051711e-03, -1.587302e-02, -1.002833e-02, 9.755043e-18, -5.634963e-03, -1.587302e-02, -1.045078e-02, 1.611081e-18, -5.206170e-03, -1.587302e-02, -1.088659e-02, -3.524518e-18, -4.762633e-03, -1.587302e-02, -1.133861e-02, 1.057355e-17, -4.301320e-03, -1.587302e-02, -1.181008e-02, -1.379571e-17, -3.818768e-03, -1.587302e-02, -1.230471e-02, -5.787736e-19, -3.310973e-03, -1.587302e-02, -1.282684e-02, -2.230465e-17, -2.773243e-03, -1.587302e-02, -1.338156e-02, 1.028417e-17, -2.200006e-03, -1.587302e-02, -1.397499e-02, 6.835237e-18, -1.584560e-03, -1.587302e-02, -1.461449e-02, -7.502569e-18, -9.187364e-04, -1.587302e-02, -1.530915e-02, -3.172066e-17, -1.924393e-04, -1.587302e-02, -1.607024e-02, 1.762259e-18, 6.069947e-04, -1.587302e-02, -1.691203e-02, 1.762259e-18, 1.495705e-03, -1.587302e-02, -1.785286e-02, 8.811294e-19, 2.494625e-03, -1.587302e-02, -1.891675e-02, 1.762259e-18, 3.631446e-03, -1.587302e-02, -2.013580e-02, -8.940984e-19, 4.943630e-03, -1.587302e-02, -2.155400e-02, -2.706008e-18, 6.483175e-03, -1.587302e-02, -2.323329e-02, 3.675695e-18, 8.324488e-03, -1.587302e-02, -2.526385e-02, -2.114711e-17, 1.057796e-02, -1.587302e-02, -2.778233e-02, -1.762259e-17, 1.341481e-02, -1.587302e-02, -3.100624e-02, 5.286776e-18, 1.711572e-02, -1.587302e-02, -3.530428e-02, -1.966126e-17, 2.217511e-02, -1.587302e-02, -4.135557e-02, -3.023558e-19, 2.955261e-02, -1.587302e-02, -5.056342e-02, 1.868296e-17, 4.138879e-02, -1.587302e-02, -6.637212e-02, 1.078735e-17, 6.362938e-02, -1.587302e-02, -1.000984e-01, -1.893131e-17, 1.212461e-01, -1.587302e-02, -2.234141e-01, 2.114711e-17, 6.387559e-01};


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

void blocks_sources_disturbance(float * sample_block){
	int_fast32_t i;
	for (i = 0; i < AUDIO_BLOCKSIZE; i++) {
		sample_block[i] = dist_sig[dist_idx++];
		if (dist_idx >= NUMEL(dist_sig)) {
			dist_idx = 0;
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
