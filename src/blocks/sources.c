#include "sources.h"
#include "config.h"
#include <math.h>
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

/** @brief One period of the secret "mystery" disturbance signal. Identical
 *samples are used regardless of the system sample-rate (i.e. pitch will be
 * shifted by changing the sample-rate) */
const float dist_sig[256]={-0.134159723, -0.0521249716, -0.067614791, 
0.167490132, 0.120439867, 0.232431992, 0.550196505, 0.378418394, -0.397042419, 
-0.720583936, 0.27905969, -0.567950872, 0.0206547434, 0.0109774353, 0.689090976, 
-0.0214150509, -0.156966934, 0.0729600603, 0.076052592, 0.0216721505, 
-0.188296223, -0.378273231, 0.0979259467, -0.41548686, -0.319360233, 
0.411881286, -0.129609621, -0.0434158667, 0.278407118, -0.0928550421, 
0.318488142, -0.106764213, 0.313363249, 0.194717582, -0.0659073613, 
-0.267848758, -0.322740131, -0.0835599178, -0.135561953, -0.126444782, 
0.30431118, -0.0921081877, 0.353856911, -0.164484499, 0.300914095, -0.161585513, 
0.0546335797, 0.300348537, -0.128083983, -0.135601916, 0.619853708, 0.294239597, 
-0.133662993, 0.200783771, -0.111408444, 0.218402464, 0.43806699, -0.496440874, 
0.318329529, 0.451098617, 0.0146877377, 0.540295882, 0.0480772578, -0.382767703, 
-0.678672224, -0.103156921, 0.220771955, 0.0982066898, 0.127972015, -0.17855162, 
0.0445544918, -0.507006982, -0.235173611, -0.253336486, 0.160805325, 
-0.00438115517, -0.35752347, -0.00294703116, -0.213428982, -0.206278266, 
0.267369838, 0.035092237, 0.12325439, 0.273502337, 0.0557721491, 0.170435668, 
0.211310733, 0.362189038, 0.147232371, 0.436948073, 0.00699511755, 
-0.0148109067, 0.526396893, -0.157705988, -0.000883331299, 0.284609011, 
0.0463511698, 0.434689671, 0.319959606, 0.0902125245, -0.240621734, 0.175337089, 
-0.427786198, 0.0756410324, 0.250132843, 0.0659155962, 0.272174158, 0.630833047, 
0.285866845, 0.0825848779, 0.19853156, 0.131646157, -0.406778505, -0.128838598, 
0.378921257, -0.0134850544, 0.180203115, -0.311413461, 0.0199616299, 
0.185731723, -0.421255889, 0.107545968, -0.0562627141, -0.290694239, 
-0.0116128558, -0.586721014, -0.658400954, -0.364142812, -0.306472969, 
-0.362938926, -0.533851365, 0.0891784403, -0.493244145, 0.0341019679, 
0.243520258, -0.000688972857, 0.0288080666, -0.117002672, -0.458743432, 
-0.0135575721, 0.297281552, 0.537817019, -0.13310681, -0.503497434, 
0.0514683005, 0.116417547, -0.0702190008, -0.355476131, 0.626333208, 
-0.730042054, -0.157786535, -0.408914027, -0.196819558, 0.0983439672, 
0.0427123639, -0.219903083, 0.240406696, 0.192570126, 0.200301149, -0.131691431, 
0.324433322, 0.204424307, 0.776220029, 0.329036882, 0.357954246, 0.0163917722, 
-0.398629621, -0.114856716, -0.234462579, -0.174493293, 0.17176116, 
-0.172268426, -0.276950083, -0.126646948, -0.0497787393, 0.126649023, 
-0.29474779, 0.0981787629, 0.0241395942, 0.409767934, -0.0659554436, 
-0.0416080049, -0.36242021, -0.428603491, 0.0960719949, -0.077192496, 
0.155859581, -0.276191511, 0.590498147, 0.0378184626, 0.323954524, 
-0.0702096367, -0.0502784781, 0.21350366, 0.171952436, -0.346609499, 
-0.474218778, -0.339682838, -0.438043554, 0.0184312784, -0.127241991, 
-0.113863375, -0.421085153, 0.241199973, 0.135954818, -0.0277294046, 0.3159555, 
-0.270411276, 0.128309234, 0.107808516, 0.108060128, -0.225630798, 0.101125132, 
-0.159305573, -0.277362521, -0.372293971, 0.321102569, -0.261737104, 
-0.053499945, -0.373959753, -0.091931722, -1, -0.33630771, -0.44134266, 
-0.313873423, -0.0659853633, -0.100663359, 0.601601192, -0.176907962, 
-0.0773605675, -0.485549851, -0.147703305, -0.413973084, 0.00937458833, 
0.263947027, 0.125076961, -0.216773521, -0.504493758, 0.45173147, 0.634287982, 
0.0372831654, -0.306277855, 0.370593276, -0.18336921, -0.145360108, 0.274247219, 
-0.428590223, -0.605424218, 0.130160514, 0.123989267, 0.0294372046, 0.153675304, 
0.334847754, 0.300258799, -0.175916745, 0.250607241};

/** @brief Channel response for unit-testing LMS algorithm */
float h_sim_int[8] = {10, -8, 6, -4, 2, -1, 1, 0};

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

/** @brief The current angles for sine/cosine sources */
float sin_ang = 0;
float cos_ang = 0;

/** @brief The current frequency to use for sine/cosine outputs, default to 1kHz */
float trig_freq = 1e3;

int16_t const * waveform_ptr = waveform;
size_t    waveform_N = NUMEL(waveform);

#define MSG "\n\n  .-\" +' \"-.    __,  ,___,\n /.'.'A_'*`.\\  (--|__| _,,_ ,_ \n|:.*'/\\-\\. ':|   _|  |(_||_)|_)\\/\n|:.'.||\"|.'*:|  (        |  | _/\n \\:~^~^~^~^:/          __,  ,___,\n  /`-....-'\\          (--|__| _ |' _| _,   ,\n /          \\           _|  |(_)||(_|(_|\\//_)\n `-.,____,.-'          (               _/\n\n" DEBUG_LINESEP
void blocks_sources_init(void){
	#ifdef JUL_AVAILABLE
		uint_fast32_t seed = util_get_seed();
		uint_fast16_t r = util_rand_range(0, 25, &seed);
		if(r == 0){
			waveform_ptr = waveform_jul;
			waveform_N = NUMEL(waveform_jul);
			printf(MSG);
		}
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
	float ang = sin_ang;
	volatile float freq;	//Declare volatile to ensure the math operations are not re-ordered into the atomic block
	ATOMIC(freq = trig_freq);
	const float delta_ang = M_TWOPI * freq / AUDIO_SAMPLE_RATE;
	for(i = 0; i < AUDIO_BLOCKSIZE; i++){
		sample_block[i] = arm_sin_f32(ang);
		ang += delta_ang;
		if(sin_ang > M_TWOPI){
			sin_ang -= M_TWOPI;
		}
	}
}

void blocks_sources_cos(float * sample_block){
	int_fast32_t i;
	float ang = sin_ang;
	volatile float freq;	//Declare volatile to ensure the math operations are not re-ordered into the atomic block
	ATOMIC(freq = trig_freq);
	const float delta_ang = M_TWOPI * freq / AUDIO_SAMPLE_RATE;
	for(i = 0; i < AUDIO_BLOCKSIZE; i++){
		sample_block[i] = arm_cos_f32(ang);
		ang += delta_ang;
		if(sin_ang > M_TWOPI){
			sin_ang -= M_TWOPI;
		}
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
	sin_ang += delta_ang * AUDIO_BLOCKSIZE;
	cos_ang += delta_ang * AUDIO_BLOCKSIZE;
}
