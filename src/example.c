#include "example.h"
#include <string.h>
#include <math.h>
#include "backend/arm_math.h"
#include "blocks/sources.h"
#include "blocks/sinks.h"
#include "blocks/gen.h"
#include "blocks/windows.h"
#include "util.h"
#include "config.h"
#include "backend/systime/systime.h"
#include "backend/printfn/printfn.h"
#include "arm_const_structs.h"
#include "backend/asciiplot.h"


#if defined(SYSMODE_TEST1)
/* Generate a few different audio DSP effects and switch between then on a
 * pushbutton press event */

/** @brief State-keeping variable type for the chosen effect */
enum test1_effect_e {TEST1_EFFECT_FIRST = 0, test1_effect_none = 0, test1_effect_flanger, test1_effect_ringmod, test1_effect_bitcrush, TEST1_EFFECT_NUM};

/** @brief State-keeping variable for the current effect */
enum test1_effect_e curr_effect = TEST1_EFFECT_FIRST;
/** @brief State-keeping variable for the selected audio source */
bool audio_src_mic = false;

/** @brief Maximum bit-crusher resolution */
#define BITCRUSH_MAXDEPTH 16

/** @brief Minimum bit-crusher resolution */
#define BITCRUSH_MINDEPTH 1

/** @brief log2 of maximum bit-crusher decimation */
#define BITCRUSH_MAXDEC   7

/** @brief log2 of minimum bit-crusher decimation */
#define BITCRUSH_MINDEC   0

/** @breif State-keeping variable for bit-crusher state */
int_fast8_t bitcrush_depth = 5;
int_fast8_t bitcrush_dec = BITCRUSH_MINDEC;
bool bitcrush_reconstruction_zoh = true;


/* Implement a flanger using a time-varying FIR filter.
 * This is very inefficient as the coefficients are almost only zero, but as we
 * have plenty of procssing power this is a simple way to implement this. */

//Set up the flanger filter
#define FLANGER_TAPS		(256)	//The maximum time delay in the flanger will be AUDIO_SAMPLE_RATE / FLANGER_TAPS
/* Note that the fir filter function assumes coefficients are stored in
 * time-reversed order, so we want the last element to be constantly one and
 * the other nonzero coeffient to move linearly across the entire vector */
float flanger_coeffs[FLANGER_TAPS] = {[0 ... (FLANGER_TAPS - 2)] = 0.0f, [FLANGER_TAPS - 1] = 0.5f};
float flanger_state[FLANGER_TAPS + AUDIO_BLOCKSIZE - 1];
arm_fir_instance_f32 flanger_s;
int_fast16_t flanger_delay_idx = FLANGER_TAPS - 2;	//State-keeping variable for the nonzero delay coefficient in flanger_coeffs
bool flanger_neg = true;							//The flanging direction (negative -> nonzero coefficient moves from end of flanger_coeffs to start)

/** @brief Center carrier frequency for ring modulator */
#define RING_MOD_CARRIER_CENTER		(750.0f)
/** @brief Width of carrier frequency modulation
 * Carrier will vary between [fc-width/2, fc+width/2] */
#define RING_MOD_CARRIER_WIDTH		(1000.0f)
/** @brief Time requied for one full sweep of carrier frequency */
#define RING_MOD_CARRIER_SWEEPTIME 	(10.0f)
/** @brief Value to change carrier by every AUDIO_BLOCKSIZE samples to achieve the desired sweeprate */
#define RING_MOD_CARRIER_DELTA		(2.0f * RING_MOD_CARRIER_WIDTH * AUDIO_BLOCKSIZE / (RING_MOD_CARRIER_SWEEPTIME * AUDIO_SAMPLE_RATE))
float ring_mod_carrier = RING_MOD_CARRIER_CENTER;	//Current carrier frequency
bool ring_mod_carrier_pos = false;					//If true carrier frequency is currently increasing

void example_test1_btnpress(void){
	//On a button press, switch the audio source
	audio_src_mic = !audio_src_mic;
	if(audio_src_mic){
		printf("Switching to the microphone as the signal source\n");
	}else{
		printf("Switching to the internal waveform as the signal source\n");
	}
}

void example_test1_init(void){
	BUILD_BUG_ON( (1<<BITCRUSH_MAXDEC) >= AUDIO_BLOCKSIZE);
	arm_fir_init_f32(&flanger_s, FLANGER_TAPS, flanger_coeffs, flanger_state, AUDIO_BLOCKSIZE);
	blocks_sources_trig_setfreq(ring_mod_carrier);
	printf("Usage guide;\n"
			"Press the user button to switch between audio sources.\n"
			"Press any of the following keys to change the current effect\n"
			"\t'1' - No effect applied\n"
			"\t'2' - Flanger\n"
			"\t'3' - Ring modulator\n"
			"\t'4' - Bit crusher\n"
			"\t'5' - Increase bit crusher amplitude bit-depth\n"
			"\t'6' - Decrease bit crusher amplitude bit-depth\n"
			"\t'7' - Increase bit crusher sample-rate\n"
			"\t'8' - Decrease bit crusher sample-rate\n"
			"\t'9' - Toggle bit crusher reconstruction mode\n");
	
}

void example_test1(void){
	//Get the selected input source
	float inpdata[AUDIO_BLOCKSIZE];
	if(audio_src_mic){
		blocks_sources_microphone(inpdata);
	}else{
		blocks_sources_waveform(inpdata);
	}

	//Update the effect if a key has been pressed
	char key;
	if(board_get_usart_char(&key)){
		switch(key){
		default:
			printf("Invalid key pressed.\n");
			break;
		case '1':
			curr_effect = test1_effect_none;
			printf("Switching to; no effect\n");
			break;
		case '2':
			curr_effect = test1_effect_flanger;
			printf("Switching to; flanger\n");
			break;
		case '3':
			curr_effect = test1_effect_ringmod;
			printf("Switching to; ring modulator\n");;
			break;
		case '4':
			curr_effect = test1_effect_bitcrush;
			printf("Switching to; bit-crusher\n");
			break;
		case '5':
			if(bitcrush_depth < BITCRUSH_MAXDEPTH){
				bitcrush_depth++;
			}
			printf("Updating bit-crusher depth to %d bits\n", bitcrush_depth);
			break;
		case '6':
			if(bitcrush_depth > BITCRUSH_MINDEPTH){
				bitcrush_depth--;
			}
			printf("Updating bit-crusher depth to %d bits\n", bitcrush_depth);
			break;
		case '7':
			if(bitcrush_dec > BITCRUSH_MINDEC){
				bitcrush_dec--;
			}
			printf("Updating bit-crusher sample-rate to %d Hz\n", AUDIO_SAMPLE_RATE / (1<<bitcrush_dec));
			break;
		case '8':
			if(bitcrush_dec < BITCRUSH_MAXDEC){
				bitcrush_dec++;
			}
			printf("Updating bit-crusher sample-rate to %d Hz\n", AUDIO_SAMPLE_RATE / (1<<bitcrush_dec));
			break;
		case '9':
			{
				bitcrush_reconstruction_zoh = !bitcrush_reconstruction_zoh;
				int_fast32_t samprate;
				if(bitcrush_reconstruction_zoh){
					samprate = AUDIO_SAMPLE_RATE / (1<<bitcrush_dec);
				}else{
					samprate = AUDIO_SAMPLE_RATE;
				}
				printf("Applying zero-order-hold reconstruction at sample rate %d for bitcrusher\n", samprate);
			}
			break;
		}
	}

	float output[AUDIO_BLOCKSIZE];

	//Apply the actual effect
	switch(curr_effect){
	case test1_effect_none:
		arm_copy_f32(inpdata, output, AUDIO_BLOCKSIZE);
		break;
	case test1_effect_flanger:
		arm_fir_f32(&flanger_s, inpdata, output, AUDIO_BLOCKSIZE);
		flanger_coeffs[flanger_delay_idx] = 0.0f;

		//Update the flanger index
		if(flanger_neg){
			//For the smoothest flanger modulation only move the flanger delay by one index per call
			if(--flanger_delay_idx <= 0){
				flanger_neg = false;
			}
		}else{
			if(++flanger_delay_idx >= FLANGER_TAPS-2){
				flanger_neg = true;
			}
		}

		//Update the flanger filter
		flanger_coeffs[flanger_delay_idx] = 0.5f;
		break;
	case test1_effect_ringmod:
	{
		/* Apply the ring modulator effect, which is simply
		 * input(t) * carrier(t) */
		float sin_samples[AUDIO_BLOCKSIZE];
		blocks_sources_sin(sin_samples);
		arm_mult_f32(inpdata, sin_samples, output, AUDIO_BLOCKSIZE);

		//Update the ring modulator carrier frequency
		if(ring_mod_carrier_pos){
			ring_mod_carrier += RING_MOD_CARRIER_DELTA;
			if(ring_mod_carrier > RING_MOD_CARRIER_CENTER + 0.5*RING_MOD_CARRIER_WIDTH){
				ring_mod_carrier_pos = false;
			}
		}else{
			ring_mod_carrier -= RING_MOD_CARRIER_DELTA;
			if(ring_mod_carrier < RING_MOD_CARRIER_CENTER - 0.5*RING_MOD_CARRIER_WIDTH){
				ring_mod_carrier_pos = true;
			}
		}
		blocks_sources_trig_setfreq(ring_mod_carrier);
		break;
	}
	case test1_effect_bitcrush:
	{
		/* Apply the bit-crusher effect, which is generated by selectively
		 * applying an adjustable (and coarse) quantization error to the input
		 * signal and decimating/upsampling the signal to reduce the effective
		 * sample-rate */
		float scaled_sig[AUDIO_BLOCKSIZE];
		arm_scale_f32(inpdata, (1<<(bitcrush_depth-1)), scaled_sig, AUDIO_BLOCKSIZE);
		//As we've scaled the input, simply round to the nearest integer
		int_fast32_t i;
		for(i = 0; i < AUDIO_BLOCKSIZE; i++){
			scaled_sig[i] = round(scaled_sig[i]);
		}
		//Now, rescale the result back to a [-1, 1] range
		arm_scale_f32(scaled_sig, 1.0f/(1<<(bitcrush_depth-1)), output, AUDIO_BLOCKSIZE);
		
		//Now apply sample-rate-reduction
		for(i = 0; i < AUDIO_BLOCKSIZE; i+=(1<<bitcrush_dec)){
			if(bitcrush_reconstruction_zoh){
				arm_fill_f32(output[i], &output[i], 1<<bitcrush_dec);	//Set 2^bitcrush_dec samples to the same value, i.e. equivalent to ZOH at low sample-rate
			}else{
				arm_fill_f32(0.0f, &output[i+1], (1<<bitcrush_dec)-1);	//Set 2^bitcrush_dec samples to zero, i.e. equivalent to traditional upsampling
			}
		}
	}
		break;
	default:
		//Should never occur!
		arm_fill_f32(0.0f, output, AUDIO_BLOCKSIZE);
		break;
	}

	//Send the data to the outputs
	blocks_sinks_leftout(output);
	blocks_sinks_rightout(output);
}
#endif

#if defined(SYSMODE_TEST2)
void example_test2_init(void){
	//No setup needed
}

void example_test2(void){
	//Allocate space for the microphone and waveform samples.
	float micdata[AUDIO_BLOCKSIZE];
	float wavedata[AUDIO_BLOCKSIZE];
        
	//Write AUDIO_BLOCKSIZE samples to the microphone and waveform buffer
	blocks_sources_microphone(micdata);
	blocks_sources_waveform(wavedata);

	//Send the waveform data to the left output and the microphone data to the right output without any processing
	blocks_sinks_leftout(wavedata);
	blocks_sinks_rightout(micdata);
}
#endif

#if defined(SYSMODE_TEST3)

//Frequency-shift anti-alias bandpass filter (remove low frequencies to suppress constant microphone bias)
#define TEST3_LP_TAPS 257
float test3_lp_filt_coeffs[TEST3_LP_TAPS] = { -0, 1.19825e-07, 4.44618e-07, -7.27863e-07, 2.70288e-06, -6.69514e-07, -1.37716e-06, 6.88468e-06, -9.09659e-06, 3.20771e-06, 2.83699e-06, -2.21577e-05, 1.29173e-05, -2.13187e-05, -2.72683e-05, 1.34271e-05, -6.49697e-05, -1.42944e-05, -1.9333e-05, -0.000106406, 6.48035e-06, -9.86873e-05, -0.000113308, -1.39721e-06, -0.000204096, -7.14905e-05, -8.03624e-05, -0.000278129, -1.19404e-05, -0.00023832, -0.00025763, -7.93432e-06, -0.000416107, -0.000127345, -0.000128519, -0.000497854, 4.24158e-05, -0.000368516, -0.000376913, 0.000116328, -0.000606198, -4.65452e-05, -1.69301e-05, -0.00063943, 0.000351097, -0.000331976, -0.000306589, 0.000577824, -0.000615253, 0.000370121, 0.000454429, -0.000545039, 0.00112599, 3.38275e-05, 0.000105563, 0.00156123, -0.000340389, 0.00125379, 0.00139452, -0.00018273, 0.00244381, 0.000719492, 0.000813936, 0.00303999, 6.63427e-05, 0.00246896, 0.00262177, 0.000151051, 0.00405365, 0.00135228, 0.00139214, 0.00461113, 3.67289e-05, 0.00347663, 0.00354677, -0.000273875, 0.00531143, 0.00114548, 0.00101094, 0.00551941, -0.00138019, 0.0034199, 0.00329394, -0.00248685, 0.00538703, -0.000885153, -0.00130981, 0.00498121, -0.0052152, 0.00150326, 0.00111372, -0.00743222, 0.00376306, -0.00546854, -0.00623171, 0.00276174, -0.0121349, -0.00244371, -0.00304666, -0.015557, 0.00088422, -0.0126357, -0.0136837, -0.000159923, -0.0222439, -0.00745253, -0.00804826, -0.0268774, -0.0011183, -0.0217286, -0.0229721, -0.000695233, -0.0360123, -0.0109452, -0.0110355, -0.0429906, 0.00371085, -0.0329441, -0.0347753, 0.01121, -0.061916, -0.00513354, -0.00206648, -0.089382, 0.0591558, -0.07345, -0.106091, 0.563131, 0.563131, -0.106091, -0.07345, 0.0591558, -0.089382, -0.00206648, -0.00513354, -0.061916, 0.01121, -0.0347753, -0.0329441, 0.00371085, -0.0429906, -0.0110355, -0.0109452, -0.0360123, -0.000695233, -0.0229721, -0.0217286, -0.0011183, -0.0268774, -0.00804826, -0.00745253, -0.0222439, -0.000159923, -0.0136837, -0.0126357, 0.00088422, -0.015557, -0.00304666, -0.00244371, -0.0121349, 0.00276174, -0.00623171, -0.00546854, 0.00376306, -0.00743222, 0.00111372, 0.00150326, -0.0052152, 0.00498121, -0.00130981, -0.000885153, 0.00538703, -0.00248685, 0.00329394, 0.0034199, -0.00138019, 0.00551941, 0.00101094, 0.00114548, 0.00531143, -0.000273875, 0.00354677, 0.00347663, 3.67289e-05, 0.00461113, 0.00139214, 0.00135228, 0.00405365, 0.000151051, 0.00262177, 0.00246896, 6.63427e-05, 0.00303999, 0.000813936, 0.000719492, 0.00244381, -0.00018273, 0.00139452, 0.00125379, -0.000340389, 0.00156123, 0.000105563, 3.38275e-05, 0.00112599, -0.000545039, 0.000454429, 0.000370121, -0.000615253, 0.000577824, -0.000306589, -0.000331976, 0.000351097, -0.00063943, -1.69301e-05, -4.65452e-05, -0.000606198, 0.000116328, -0.000376913, -0.000368516, 4.24158e-05, -0.000497854, -0.000128519, -0.000127345, -0.000416107, -7.93432e-06, -0.00025763, -0.00023832, -1.19404e-05, -0.000278129, -8.03624e-05, -7.14905e-05, -0.000204096, -1.39721e-06, -0.000113308, -9.86873e-05, 6.48035e-06, -0.000106406, -1.9333e-05, -1.42944e-05, -6.49697e-05, 1.34271e-05, -2.72683e-05, -2.13187e-05, 1.29173e-05, -2.21577e-05, 2.83699e-06, 3.20771e-06, -9.09659e-06, 6.88468e-06, -1.37716e-06, -6.69514e-07, 2.70288e-06, -7.27863e-07, 4.44618e-07, 1.19825e-07, -0};
#define TEST3_LP_RELCUTOFF 0.75f
float test3_lp_filt_state[TEST3_LP_TAPS + AUDIO_BLOCKSIZE - 1];
arm_fir_instance_f32 test3_lp_filt;

//Frequency shift FFT setup
//Note; only lengths of 16, 32, 64, ... , 4096 samples are supported by cfft
#define TEST3_FFT_LEN AUDIO_BLOCKSIZE	//For simplicity, force the FFT length to be equal to the audio block size

#define TEST3_FREQSHIFT_BINS 5	//Number of bins to shift DFT result up

#if TEST3_FFT_LEN == 16
const arm_cfft_instance_f32 *cfft_instance_hndl = &arm_cfft_sR_f32_len16;
#elif TEST3_FFT_LEN == 32
const arm_cfft_instance_f32 *cfft_instance_hndl = &arm_cfft_sR_f32_len32;
#elif TEST3_FFT_LEN == 64
const arm_cfft_instance_f32 *cfft_instance_hndl = &arm_cfft_sR_f32_len64;
#elif TEST3_FFT_LEN == 128
const arm_cfft_instance_f32 *cfft_instance_hndl = &arm_cfft_sR_f32_len128;
#elif TEST3_FFT_LEN == 256
const arm_cfft_instance_f32 *cfft_instance_hndl = &arm_cfft_sR_f32_len256;
#elif TEST3_FFT_LEN == 512
const arm_cfft_instance_f32 *cfft_instance_hndl = &arm_cfft_sR_f32_len512;
#elif TEST3_FFT_LEN == 1024
const arm_cfft_instance_f32 *cfft_instance_hndl = &arm_cfft_sR_f32_len1024;
#elif TEST3_FFT_LEN == 2048
const arm_cfft_instance_f32 *cfft_instance_hndl = &arm_cfft_sR_f32_len2048;
#elif TEST3_FFT_LEN == 4096
const arm_cfft_instance_f32 *cfft_instance_hndl = &arm_cfft_sR_f32_len4096;
#else
#error invalid FFT length requested!
#endif

//Allocate space for window coefficients
float window[TEST3_FFT_LEN];

systime_t print_delay;

void example_test3_init(void){
	arm_fir_init_f32(&test3_lp_filt, TEST3_LP_TAPS, test3_lp_filt_coeffs, test3_lp_filt_state, AUDIO_BLOCKSIZE);
	windows_blackman(window, NUMEL(window));
	print_delay = systime_get_delay(0);
}

void example_test3(void){
	BUILD_BUG_ON(TEST3_FREQSHIFT_BINS < 0);	//Require shift to be up in frequency
	BUILD_BUG_ON(TEST3_LP_RELCUTOFF > (1.0f - (1.0f*TEST3_FREQSHIFT_BINS)/TEST3_FFT_LEN));	//Require shift to be less than the range of the anti-aliasing filter
	//Get the microphone data and apply a simple low-pass antialias filter
	float inpdata[AUDIO_BLOCKSIZE];
	blocks_sources_microphone(inpdata);
	float lpdata[AUDIO_BLOCKSIZE];
	arm_fir_f32(&test3_lp_filt, inpdata, lpdata, AUDIO_BLOCKSIZE);

	//Prepare the data for the DFT
	//Apply the window function
	float final_td_data[AUDIO_BLOCKSIZE];
	arm_mult_f32(lpdata, window, final_td_data, AUDIO_BLOCKSIZE);


	//For generality, use the cfft function rather than rfft
	float fftdata[TEST3_FFT_LEN*2];
	int_fast32_t i;

	//Set up the complex input to the DFT with a purely real input
	for(i = 0; i < NUMEL(fftdata); i++){
		//For each value pair in fftdata, write the real-valued input and a zero imaginary input
		if(i%2 == 0){
			fftdata[i] = final_td_data[i/2];
		}else{
			fftdata[i] = 0.0f;
		}
	}

	//Apply the FFT
	arm_cfft_f32(cfft_instance_hndl, fftdata, 0, 1);

	float shift_fftdata[TEST3_FFT_LEN*2];

	//Generate the frequency shift, first shift bins corresponding to frequencies in the range [0, fs/2]
	for(i = 0; i < NUMEL(fftdata)/2; i++){
		if(i <= TEST3_FREQSHIFT_BINS*2){
			shift_fftdata[i] = 0.0f;
		}else{
			shift_fftdata[i] = fftdata[i - TEST3_FREQSHIFT_BINS*2];
		}
	}

	//Now generate the contents corresponding to [fs/2, fs]
	for(i = 0; i < NUMEL(shift_fftdata)/2; i++){
		shift_fftdata[NUMEL(shift_fftdata) - 1 - i] = shift_fftdata[i];
	}

	//Apply the IFFT
	arm_cfft_f32(cfft_instance_hndl, shift_fftdata, 1, 1);

	//Finally, take the real component of the IDFT results
	float output[AUDIO_BLOCKSIZE];
	for(i = 0; i < NUMEL(output); i++){
		output[i] = shift_fftdata[2*i];
	}

	blocks_sinks_leftout(inpdata);

	blocks_sinks_rightout(output);

	//If it's time to print data
	if(systime_get_delay_passed(print_delay)){
		print_delay = systime_get_delay(MS2US(TEST3_PRINT_PERIOD_ms));
		//Determine the bin with the maximum amplitude
		float mag[TEST3_FFT_LEN];
		arm_cmplx_mag_squared_f32(fftdata, mag, TEST3_FFT_LEN);
		float max_mag;
		uint32_t idx;

		//Scan through all bins, except the first and last, save the one with the highest magnitude
		arm_max_f32(mag, TEST3_FFT_LEN, &max_mag, &idx);

		//Print the frequency corresponding to the bin with the maximum amplitude
		printf("Peak frequency at %f Hz with amplitude %f\n", (1.0f * idx * AUDIO_SAMPLE_RATE) / (TEST3_FFT_LEN), sqrt(max_mag));
	}
}
#endif

#if defined(SYSMODE_TEST4)

/** @brief The carrier frequency for the final signal [Hz] */
#define QPSK_CARRIER_Hz		(500.0f)

/** @brief QPSK message length [symbols] */
#define QPSK_MSG_LEN		(64)

/** @brief Time to keep receiver enabled after starting a message transmission */
#define QPSK_RX_ENABLETIME_s	(QPSK_MSG_LEN * AUDIO_BLOCKTIME_s)

/** @brief Flag to indicate a new message transmission is requested */
bool qpsk_newmsg_g = false;

float cos_ref[AUDIO_BLOCKSIZE];
float sin_ref[AUDIO_BLOCKSIZE];

systime_t receiver_timer;

/** @brief Enumerated type for indicating a QPSK symbol */
enum qpsk_sym_e {qpsk_sym_0 = 0, qpsk_sym_1, qpsk_sym_2, qpsk_sym_3, QPSK_SYM_NUM};

enum qpsk_sym_e qpsk_msg[QPSK_MSG_LEN];

void example_test4_init(void){
	int_fast32_t i;
	//Initialize qpsk_msg to the message we wish to transmit
	for(i = 0; i < NUMEL(qpsk_msg); i++){
		switch(i % 0x04){
		case 0:
			qpsk_msg[i] = qpsk_sym_0;
			break;
		case 1:
			qpsk_msg[i] = qpsk_sym_1;
			break;
		case 2:
			qpsk_msg[i] = qpsk_sym_2;
			break;
		case 3:
			qpsk_msg[i] = qpsk_sym_3;
			break;
		}
	}

	//Initialize the sin/cos reference waveforms
	blocks_gen_sin(QPSK_CARRIER_Hz, 0, sin_ref, NUMEL(sin_ref));
	blocks_gen_cos(QPSK_CARRIER_Hz, 0, cos_ref, NUMEL(cos_ref));
	receiver_timer = systime_get_delay(0);
}

void example_test4(void){
	BUILD_BUG_ON(!IS_INTEGER(AUDIO_BLOCKTIME_s*QPSK_CARRIER_Hz));	//Require that each symbol fits neatly within one output block
	//As the sin/cos source

	//For simplicity, we will output one symbol every AUDIO_BLOCKSIZE samples
	static bool qpsk_newmsg = false;
	static bool enable_receiver = false;
	static int_fast32_t curr_symbol = -1;	//The current symbol to output, negative if we don't want to output anything
	ATOMIC(qpsk_newmsg = qpsk_newmsg_g; qpsk_newmsg_g = false;);
	if(qpsk_newmsg){
		curr_symbol = 0;
		receiver_timer = systime_get_delay(S2US(QPSK_RX_ENABLETIME_s));
		enable_receiver = true;
		printf("Receiver enabled\n");
	}

	float output[AUDIO_BLOCKSIZE];
	if(curr_symbol >= 0){
		/* Map the four QPSK symbols to the corresponding I/Q signs. Assume a grey-coded scheme of
		 *        Q
		 *        ^
		 *        |
		 *   11   |   01
		 *        |
		 * --------------->  I
		 *        |
		 *   10   |   00
		 *        |
		 */

		//Generate the in-phase scale
		float inphase_scale;
		if(qpsk_msg[curr_symbol] == qpsk_sym_0 || qpsk_msg[curr_symbol] == qpsk_sym_1){
			inphase_scale = M_SQRT1_2;
		}else{
			inphase_scale = -M_SQRT1_2;
		}

		//Generate the quadrature-phase scale
		float quadphase_scale;
		if(qpsk_msg[curr_symbol] == qpsk_sym_1 || qpsk_msg[curr_symbol] == qpsk_sym_3){
			quadphase_scale = M_SQRT1_2;
		}else{
			quadphase_scale = -M_SQRT1_2;
		}

		//Generate the scaled in-phase/quadrature-phase signals
		float inphase[AUDIO_BLOCKSIZE];
		float quadphase[AUDIO_BLOCKSIZE];

		arm_scale_f32(cos_ref, inphase_scale, inphase, AUDIO_BLOCKSIZE);
		arm_scale_f32(sin_ref, quadphase_scale, quadphase, AUDIO_BLOCKSIZE);

		//Finally, generate the net output signal
		arm_add_f32(inphase, quadphase, output, AUDIO_BLOCKSIZE);
	}else{
		//Just write zeros to the output if we don't have anything transmit
		arm_fill_f32(0.0f, output, AUDIO_BLOCKSIZE);
	}

	//Send the output to the left output and set the right output to all zeros
	blocks_sinks_leftout(output);
	float zeros[AUDIO_BLOCKSIZE];
	blocks_sources_zeros(zeros);
	blocks_sinks_rightout(zeros);

	//Increment the current symbol, unless we are done in which case we indicate this by setting curr_symbol negative
	if(curr_symbol >= 0){
		if(curr_symbol >= NUMEL(qpsk_msg) - 1){
			curr_symbol = -1;
		}else{
			curr_symbol++;
		}
	}


	if(enable_receiver && !systime_get_delay_passed(receiver_timer)){
		if(systime_get_delay_passed(receiver_timer)){
			enable_receiver = false;
			printf("Receiver disabled");
		}else{
			/* Monitor the microphone input if the receiver is enabled and try to
			 * decode the current block as a single symbol.
			 *
			 * Note that this is incredibly crude as we have no guarantee that the
			 * microphone's data block isn't delayed wrt. the output data block, so we
			 * may well be processing two halves of symbols. Also, we neglect all forms
			 * of carrier recovery and phase jitter. */

			float micinp[AUDIO_BLOCKSIZE];
			blocks_sources_microphone(micinp);

			//Compare the received block with all possible symbols and choose the most likely one
			float max_pwr = -INFINITY;
			enum qpsk_sym_e best_sym = QPSK_SYM_NUM;
			enum qpsk_sym_e sym;
			for(sym = qpsk_sym_0; sym < QPSK_SYM_NUM; sym++){
				float candidate_i[AUDIO_BLOCKSIZE];
				float candidate_q[AUDIO_BLOCKSIZE];
				float candidate_sig[AUDIO_BLOCKSIZE];
				/* Again, use the same I/Q encoding map as before
				 *        Q
				 *        ^
				 *        |
				 *   11   |   01
				 *        |
				 * --------------->  I
				 *        |
				 *   10   |   00
				 *        |
				 */
				switch(sym){
				case qpsk_sym_0:
					arm_scale_f32(cos_ref, M_SQRT1_2, candidate_i, AUDIO_BLOCKSIZE);
					arm_scale_f32(sin_ref, -M_SQRT1_2, candidate_q, AUDIO_BLOCKSIZE);
					break;
				case qpsk_sym_1:
					arm_scale_f32(cos_ref, M_SQRT1_2, candidate_i, AUDIO_BLOCKSIZE);
					arm_scale_f32(sin_ref, M_SQRT1_2, candidate_q, AUDIO_BLOCKSIZE);
					break;
				case qpsk_sym_2:
					arm_scale_f32(cos_ref, -M_SQRT1_2, candidate_i, AUDIO_BLOCKSIZE);
					arm_scale_f32(sin_ref, -M_SQRT1_2, candidate_q, AUDIO_BLOCKSIZE);
					break;
				case qpsk_sym_3:
					arm_scale_f32(cos_ref, -M_SQRT1_2, candidate_i, AUDIO_BLOCKSIZE);
					arm_scale_f32(sin_ref, M_SQRT1_2, candidate_q, AUDIO_BLOCKSIZE);
					break;
				default:
					//Should never occur
					break;
				}
				//Generate the candidate output signal
				arm_add_f32(candidate_i, candidate_q, candidate_sig, AUDIO_BLOCKSIZE);

				//Now, compare the received input with each of the generated signals and store the best matching result
				float sig_pwr[AUDIO_BLOCKSIZE];
				arm_add_f32(candidate_sig, micinp, sig_pwr, AUDIO_BLOCKSIZE);
				float sig_pwr_abs[AUDIO_BLOCKSIZE];
				arm_abs_f32(sig_pwr, sig_pwr_abs, AUDIO_BLOCKSIZE);
				float candidate_pwr = vector_mean(sig_pwr_abs, AUDIO_BLOCKSIZE);
				if(candidate_pwr > max_pwr){
					max_pwr = candidate_pwr;
					best_sym = sym;
				}
			}

			switch(best_sym){
			case qpsk_sym_0:
				printf("Rx; 0\n");
				break;
			case qpsk_sym_1:
				printf("Rx; 1\n");
				break;
			case qpsk_sym_2:
				printf("Rx; 2\n");
				break;
			case qpsk_sym_3:
				printf("Rx; 3\n");
				break;
			default:
				//Should never occur
				printf("Rx; ERR\n");
				break;
			}
		}
	}
}

void example_test4_btnpress(void){
	ATOMIC(qpsk_newmsg_g = true);
	printf("Transmitting QPSK message\n");
}
#endif


#if defined(SYSMODE_TEST5)
void example_test5_init(void){
	//TODO: initialize
}

void example_test5(void){
	//TODO: do frequency-shift, suppress carrier
}
#endif


#if defined(SYSMODE_RADAR)
#define REL_CHAR	'z'					//Character to enter to set current channel distance to zero
#define RESET_CHAR	'r'					//Character to reset the relative channel distance to zero
#define PLOT_TOGGLE_CHAR 'p'					//Character to toggle displaying the plot
#define WAVEFORM_CORR	'1'				//Character to select the correlation plot
#define WAVEFORM_CHIRP	'2'				//Character to select the chirp (sent signal) plot
#define WAVEFORM_RX		'3'				//Character to select the received plot
float radar_chirp[AUDIO_BLOCKSIZE];		//Buffer that contains (after initialization) a single, constant chirp waveform
float radar_rx[RADAR_RX_SIZE];			//receive buffer for radar
float radar_zerobuffer[AUDIO_BLOCKSIZE];//Dummy buffer containing (after initialization) all zeros
float corr_result[2 * NUMEL(radar_rx) - 1];	//Buffer for storing correlation result
systime_t tx_delay;
bool rx_pending;
bool rx_done;
bool plot_pending;
bool force_plot;
int_fast32_t rxbuf_idx;
int_fast32_t idx_offset = 0;			//User-settable offset for relative channel distance
float max_val;							//The last stored maximum correlation value
bool pause = false;
bool plot_enabled = false;
int_fast32_t max_idx = 0;
float plot_axis[] = {0, NUMEL(corr_result), NAN, NAN};
float * ydata_plot = corr_result;
int_fast32_t ydata_num = NUMEL(corr_result);
char plot_title_corr[] = "correlation(n)";
char plot_title_chirp[] = "signal_transmit(n)";
char plot_title_rx[] = "signal_receive(n)";
char * plot_title_p = plot_title_corr;

static void example_radar_helptext(void){
	printf("\nPress '%c' to set the current channel length to 0 or '%c' to remove the offset.\n"
	"Press ' ' (space) to pause/resume transmission.\n"
	"Press '%c' to toggle displaying the selected plot.\n"
	"Press 'w' to reduce the plot range based on the current centerpoint.\n"
	"Press 'd' to increase the plot range based on the current centerpoint.\n"
	"Press 'a' to move the plot range to the left.\n"
	"Press 's' to move the plot range to the right.\n"
	"Press '%c', '%c', or '%c' to set the output content to the correlation, sent, or received signals respectively. (Also resetting plot extents).\n"
	"Due to the large length of these vectors, they unfortunately cannot reasonably be output in a matlab-compatible form.\n\n",
	REL_CHAR, RESET_CHAR, PLOT_TOGGLE_CHAR, WAVEFORM_CORR, WAVEFORM_CHIRP, WAVEFORM_RX);
}

void example_radar_init(void){
	BUILD_BUG_ON(RADAR_CHIRP_SIZE > AUDIO_BLOCKSIZE);	//Radar chirp must fit within one AUDIO_BLOCKSIZE block
	BUILD_BUG_ON(NUMEL(radar_chirp) > NUMEL(radar_rx));	//Radar receive buffer must be at least as large as the chirp
	float f0 = (float) RADAR_F_START * 2 * M_PI /((float) AUDIO_SAMPLE_RATE);
	float f1 = (float) RADAR_F_STOP * 2 * M_PI / ((float) AUDIO_SAMPLE_RATE);
	arm_fill_f32(0.0f, radar_rx, NUMEL(radar_rx));
	arm_fill_f32(0.0f, corr_result, NUMEL(corr_result));
	
	//Set radar waveform to be chirp (potentially) followed by zeros
	arm_fill_f32(0.0f, radar_chirp, NUMEL(radar_chirp));
	int i;
	for (i = 0; i < RADAR_CHIRP_SIZE; i++) {
		float alpha = i / ((float) RADAR_CHIRP_SIZE);	//Alpha goes from 0 to 1 as i goes from 0 to RADAR_CHIRP_SIZE
		radar_chirp[i] = arm_sin_f32(i * (alpha * f1 + (1 - alpha) * f0));
	}
	
	//Schedule first transmission at after an initial delay
	tx_delay = systime_get_delay(S2US(RADAR_DELAY_s));
	
	example_radar_helptext();
	
	//Initially we don't need to log microphone data and there's no data to process
	rx_pending = false;
	rx_done = false;
	plot_pending = false;
	force_plot = false;
	max_val = 0.0f;
}

void example_radar(void){
	char key;
	bool force_start = false;
	
	if(board_get_usart_char(&key)){
		float x_center = (plot_axis[0] + plot_axis[1]) / 2;
		float x_range = plot_axis[1] - plot_axis[0];
		switch(key){
		default:
			printf("Invalid key pressed.\n");
			example_radar_helptext();
			break;
		case REL_CHAR:
			printf("Setting relative distance to zero.\n");
			idx_offset =  max_idx;
			break;
		case RESET_CHAR:
			printf("Removing relative distance offset.\n");
			idx_offset = 0;
			break;
		case ' ':
			if(pause){
				printf("Resuming operation\n");
				pause = false;
				force_start = true;
			}else{
				printf("Pausing operation\n");
				pause = true;
			}
			break;
		case PLOT_TOGGLE_CHAR:
			if(plot_enabled){
				printf("Disabling plot.\n");
				plot_enabled = false;
			}else{
				printf("Enabling plot.\n");
				plot_enabled = true;
			}
			break;
		case 'w':
			/* "zoom in" on center, halving range */
			plot_axis[0] = x_center - x_range/4;
			plot_axis[1] = x_center + x_range/4;
			force_plot = true;
			break;
		case 's':
			/* "zoom out" from center, doubling range */
			plot_axis[0] = x_center - x_range;
			plot_axis[1] = x_center + x_range;
			force_plot = true;
			break;
		case 'a':
			/* move left from center by 1/4 of range, leaving range unchanged */
			plot_axis[0] = x_center - (x_range*3)/4;
			plot_axis[1] = x_center + x_range/4;
			force_plot = true;
			break;
		case 'd':
			/* move right from center by 1/4 of range, leaving range unchanged */
			plot_axis[0] = x_center - x_range/4;
			plot_axis[1] = x_center + (x_range*3)/4;
			force_plot = true;
			break;
		case WAVEFORM_CORR:
			plot_axis[0] = 0;
			plot_axis[1] = NUMEL(corr_result);
			ydata_plot = corr_result;
			ydata_num = NUMEL(corr_result);
			plot_title_p = plot_title_corr;
			printf("Selecting correlation function for display\n");
			force_plot = true;
			break;
		case WAVEFORM_CHIRP:
			plot_axis[0] = 0;
			plot_axis[1] = NUMEL(radar_chirp);
			ydata_plot = radar_chirp;
			ydata_num = NUMEL(radar_chirp);
			plot_title_p = plot_title_chirp;
			printf("Selecting transmitted signal for display\n");
			force_plot = true;
			break;
		case WAVEFORM_RX:
			plot_axis[0] = 0;
			plot_axis[1] = NUMEL(radar_rx);
			ydata_plot = radar_rx;
			ydata_num = NUMEL(radar_rx);
			plot_title_p = plot_title_rx;
			printf("Selecting received signal for display\n");
			force_plot = true;
			break;
		}
	}
	
	bool tx_pending = false;	//Indicates if a chirp has been queued for transmission
	//If we are still waiting for data to be received, log it
	if(rx_pending){
		blocks_sources_microphone(&radar_rx[rxbuf_idx]);
		rxbuf_idx += AUDIO_BLOCKSIZE;
		
		/* If the buffer can't contain another AUDIO_BLOCKSIZE elements, stop
		   recording and set and remaining elements to zero */
		if(rxbuf_idx + AUDIO_BLOCKSIZE > NUMEL(radar_rx)){
			rx_pending = false;
			rx_done = true;
			while(rxbuf_idx < NUMEL(radar_rx)){
				radar_rx[rxbuf_idx++] = 0;
			}
		}
	}else if(rx_done){
		//All data received, process data
		rx_done = false;
		
		/* Take the magnitude of the correlation between the received waveform
		 * and the transmitted chirp. As only the first RADAR_CHIRP_SIZE
		 * elements of radar_chirp are nonzero we only need to work with these */
		arm_correlate_f32(radar_rx, NUMEL(radar_rx), radar_chirp, RADAR_CHIRP_SIZE, corr_result);
		arm_abs_f32(corr_result, corr_result, NUMEL(corr_result));
		//Find the element with greatest magnitude and store its index and value
		int_fast32_t i;
		for(i = 0, max_idx = 0, max_val = 0.0f; i < NUMEL(corr_result); i++){
			if(corr_result[i] > max_val){
				max_val = corr_result[i];
				max_idx = i;
			}
		}
		
		//Finally, print results to terminal on the next call (in order to reduce the execution time spent in during this call)
		plot_pending = true;
	}else if((systime_get_delay_passed(tx_delay) && !pause) || force_start){
		//Time to start a new transmission if delay passed and no other processing in progress
		blocks_sinks_leftout(radar_chirp);
		tx_pending = true;
		rx_pending = true;	//Start listening to the microphone on the *next* call to example_radar(), as the radar response will be received no earlier than then
		rxbuf_idx = 0; 		//Start saving the received input with zero offset
		tx_delay = systime_get_delay(S2US(RADAR_DELAY_s));
	}else if(plot_pending || force_plot){
		struct asciiplot_s dummyplot = {
			.cols = PLOT_COLS,
			.rows = PLOT_ROWS,
			.xdata = NULL,
			.ydata = ydata_plot,
			.data_len = ydata_num,
			.xlabel = "n",
			.ylabel = "|f(n)|",
			.title = plot_title_p,
			.axis = plot_axis,
			.label_prec = 4
		};
		
		if(plot_enabled || force_plot){
			printf("\n\n");
			asciiplot_draw(&dummyplot);
		}
		if(plot_pending){
			printf("Index maxcorr: %5d, Offset: %5d, Channel length %2.3f [m], Amplitude %4f [-]\n", max_idx, idx_offset, (max_idx - idx_offset) * RADAR_V_PROP / (1.0f * AUDIO_SAMPLE_RATE), max_val);
		}
		
		plot_pending = false;
		force_plot = false;
	}
	
	//If there's no transmission queued for the left speaker, send all zeros
	if(!tx_pending){
		blocks_sinks_leftout(radar_zerobuffer);
	}
	blocks_sinks_rightout(radar_zerobuffer);
 }
#endif

#if defined(SYSMODE_FFT)
#define FILTER_SIZE (64)
#define FFT_SIZE (AUDIO_BLOCKSIZE*2)
#define FFT_FLAG (0)
#define IFFT_FLAG (1)

float lp_filter[FILTER_SIZE] = {
  -2.496956319982458e-03f,
   1.061123057910895e-03f,
   1.799575981839800e-03f,
   2.677675432046559e-03f,
   3.246366234977765e-03f,
   3.090521427050170e-03f,
   1.954953083021346e-03f,
  -1.303892962678101e-04f,
  -2.795326496320816e-03f,
  -5.356144072505601e-03f,
  -6.972193079604380e-03f,
  -6.867088217447664e-03f,
  -4.610028643185672e-03f,
  -3.312566693466183e-04f,
   5.175765824001333e-03f,
   1.054753460776610e-02f,
   1.411890548432881e-02f,
   1.436065007374038e-02f,
   1.032985773994797e-02f,
   2.132543581602375e-03f,
  -8.919519113938091e-03f,
  -2.035820813689276e-02f,
  -2.895083648554942e-02f,
  -3.135674961111528e-02f,
  -2.489424533023874e-02f,
  -8.266237880079758e-03f,
   1.794823774717696e-02f,
   5.118254654955430e-02f,
   8.719283811339594e-02f,
   1.207736510603734e-01f,
   1.467272709948181e-01f,
   1.608677439150870e-01f,
   1.608677439150870e-01f,
   1.467272709948181e-01f,
   1.207736510603734e-01f,
   8.719283811339594e-02f,
   5.118254654955430e-02f,
   1.794823774717696e-02f,
  -8.266237880079758e-03f,
  -2.489424533023874e-02f,
  -3.135674961111528e-02f,
  -2.895083648554942e-02f,
  -2.035820813689276e-02f,
  -8.919519113938091e-03f,
   2.132543581602375e-03f,
   1.032985773994797e-02f,
   1.436065007374038e-02f,
   1.411890548432881e-02f,
   1.054753460776610e-02f,
   5.175765824001333e-03f,
  -3.312566693466183e-04f,
  -4.610028643185672e-03f,
  -6.867088217447664e-03f,
  -6.972193079604380e-03f,
  -5.356144072505601e-03f,
  -2.795326496320816e-03f,
  -1.303892962678101e-04f,
   1.954953083021346e-03f,
   3.090521427050170e-03f,
   3.246366234977765e-03f,
   2.677675432046559e-03f,
   1.799575981839800e-03f,
   1.061123057910895e-03f,
  -2.496956319982458e-03f,
   };

float H[FFT_SIZE];
float Hhp[FFT_SIZE];
float X[FFT_SIZE];
float Y[FFT_SIZE];
float buffer[FFT_SIZE];
float buffer2[FFT_SIZE];
float y[FFT_SIZE];
float saved[FILTER_SIZE];
enum f_mode {lp_mode, hp_mode} filter_mode;
arm_rfft_fast_instance_f32 S_fft;
void example_fft_init(void){
	arm_rfft_fast_init_f32(&S_fft, FFT_SIZE);
	arm_fill_f32(0.0f, saved, FILTER_SIZE);
	arm_fill_f32(0.0f, buffer, FFT_SIZE);
	arm_copy_f32(lp_filter, buffer, FILTER_SIZE);
	arm_rfft_fast_f32(&S_fft, buffer, H, FFT_FLAG);
	arm_fill_f32(0.0f, buffer, FFT_SIZE);
	int i;
	for (i = 0; i < FILTER_SIZE/2; i++) {
		buffer[i]=lp_filter[i];
		buffer[i+1] = - lp_filter[i+1];
	}
	arm_rfft_fast_f32(&S_fft, buffer, Hhp, FFT_FLAG);
	arm_fill_f32(0.0f, buffer, FFT_SIZE);
	filter_mode = lp_mode;
}

void example_fft(void){
	//Allocate space for the microphone and waveform samples.
	float micdata[AUDIO_BLOCKSIZE];
	//Write AUDIO_BLOCKSIZE samples to the microphone and waveform buffer
	blocks_sources_microphone(micdata);
	// blocks_sources_waveform(y); // The endo fo buffer is zeros already
	arm_fill_f32(0.0f, &buffer[AUDIO_BLOCKSIZE], FFT_SIZE-AUDIO_BLOCKSIZE);
	blocks_sources_waveform(buffer); // The end of buffer is zero already
	arm_rfft_fast_f32(&S_fft, buffer, X, FFT_FLAG);
	if (filter_mode == lp_mode) {
		arm_cmplx_mult_cmplx_f32	(	X, H, Y, FFT_SIZE-AUDIO_BLOCKSIZE);
	} else {
		arm_cmplx_mult_cmplx_f32	(	X, Hhp, Y, FFT_SIZE-AUDIO_BLOCKSIZE);
	}
	arm_rfft_fast_f32(&S_fft, Y, buffer, IFFT_FLAG);
	int i;
	for ( i = 0; i < FILTER_SIZE; i++) {
    buffer[i] += saved[i];
		saved[i] = buffer[AUDIO_BLOCKSIZE+i];
	}

	//Send the waveform data to the left output and the microphone data to the right output without any processing
	blocks_sinks_leftout(micdata);
	blocks_sinks_rightout(buffer);
}
void example_fft_btnpress(){
	if (filter_mode == lp_mode){
		filter_mode = hp_mode;
		printf("High pass filter\n");
	} else {
		filter_mode = lp_mode;
		printf("Low pass filter\n");
	};
}
#endif
