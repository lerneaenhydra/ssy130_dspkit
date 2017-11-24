#include "lab_lms.h"
#include "backend/arm_math.h"
#include "blocks/sources.h"
#include "blocks/sinks.h"
#include "util.h"
#include "config.h"
#include "backend/systime/systime.h"
#include "backend/printfn/printfn.h"
#include "backend/asciiplot.h"

/** @brief Relative change in mu on increase/decrease events */
#define		LAB_LMS_MU_CHANGE	(1.2)

/** @brief Number of filter taps to use for on-line LMS operation */
#define		LAB_LMS_TAPS_ONLINE		(128)

// Stateful variables for the LMS lab
float lms_coeffs[LAB_LMS_TAPS_ONLINE];
float lms_state[LAB_LMS_TAPS_ONLINE + AUDIO_BLOCKSIZE - 1];
float lms_mu_state = LAB_LMS_MU_INIT;
float lms_mu       = LAB_LMS_MU_INIT;
char pname[] = "h";

// enum type for different modes of operation
enum lms_modes {lms_updt, lms_enbl, lms_dsbl} lms_mode;
enum dist_srces {cos_src, noise_src} dist_src;
enum signal_modes {signal_off, signal_on} signal_mode;

#define PRINT_HELPMSG() 																					\
		printf("Usage guide;\n"																				\
			"Press the following keys to change the system behavior\n"										\
			"\t'd' - LMS filtering disabled, raw microphone data output to right speaker. Initial mode.\n"	\
			"\t'f' - LMS filtering applied, update disabled (mu=0), error signal output to right speaker\n"	\
			"\t'u' - LMS filtering applied with filter update, error signal output to right speaker\n"		\
			"\t't' - Toggle disturbance source between cosine signal and wide band noise\n"					\
			"\t'r' - Reset step size to %e, filter coefficients to 0, and turn off filter updating\n"		\
			"\t'1' - Increase step size mu\n"																\
			"\t'2' - Decrease step size mu\n"																\
			"\t'm' - Prints the filter coefficients h in a format useful for import in Matlab\n"			\
			"\t'p' - Plots the current filter coefficients directly in the terminal\n"						\
			"\t's' - Toggles music signal\n", lms_mu);														\

void lab_lms_init(void){
	//Manually initialize the LMS filter coefficients and state to all zeros
	arm_fill_f32(0.0f, lms_coeffs, NUMEL(lms_coeffs));
	arm_fill_f32(0.0f, lms_state, NUMEL(lms_state));
	blocks_sources_trig_setfreq(440);
	lms_mode = lms_dsbl; // start with disabled mode
	dist_src = noise_src; // start with wide band noise
	signal_mode = signal_on;
	PRINT_HELPMSG();
	BUILD_BUG_ON(LAB_LMS_TAPS_ONLINE >= AUDIO_BLOCKSIZE);	//LMS state update assumes one blocksize covers all taps
}

void lab_lms(void){
	//Update filter settings
	char key;
	if(board_get_usart_char(&key)){
		switch(key){
			default:
			printf("Invalid key pressed.\n");
			PRINT_HELPMSG();
		break;
			case 'd':
			printf("Filtering disabled, mic signal output to speaker\n");
			lms_mode = lms_dsbl;
		break;
			case 'f':
			printf("Filtering enabled, error signal output to speaker\n");
			lms_mode = lms_enbl;
			lms_mu = 0.0f;
			printf("Filter coefficients fixed. Step size mu set to %e\n", lms_mu);
		break;
			case 'u':
			printf("Filtering and lms update, error signal output to speaker\n");
			lms_mode = lms_updt;
			lms_mu = lms_mu_state;
			printf("Step size mu set to %e\n", lms_mu);
			break;
		case 'r':
			lms_mu_state = LAB_LMS_MU_INIT;
			lms_mu = lms_mu_state;
			arm_fill_f32(0.0f, lms_coeffs, NUMEL(lms_coeffs));
			printf("Filtering enabled, error signal output to speaker\n");
			printf("Reset step size mu to %e and filter coefficients to 0\n", lms_mu);
			break;
		case '1':
			lms_mu_state *= LAB_LMS_MU_CHANGE;
			lms_mu = lms_mu_state;
			printf("Step size mu increased to %e\n", lms_mu);
			break;
		case '2':
			lms_mu_state *= 1.0f/LAB_LMS_MU_CHANGE;
			lms_mu = lms_mu_state;
			printf("Step size mu decreased to %e\n", lms_mu);
			break;
		case 't':
			if (dist_src == cos_src){
				dist_src = noise_src;
				printf("Disturbance source set to noise\n");
			}else{
				dist_src = cos_src;
				printf("Disturbance source set to cosine\n");
			}
			break;
		case 'm':
			print_vector_f(pname, lms_coeffs, NUMEL(lms_coeffs));
			break;
		case 'p':
			{
				float h_hat[NUMEL(lms_coeffs)];
				//Generate h_hat by reversing lms_coeffs
				int i;
				for(i = 0; i < NUMEL(h_hat); i++){
					h_hat[i] = lms_coeffs[NUMEL(lms_coeffs) - 1 - i];
				}
				char plottitle[] = "Estimated channel coefficients (lms_coeffs in reversed order)";
				float axisdummy[] = {NAN, NAN, NAN, NAN};	//Auto-scale plot extents
				struct asciiplot_s dummyplot = {
					.cols = PLOT_COLS,
					.rows = PLOT_ROWS,
					.xdata = NULL,
					.ydata = h_hat,
					.data_len = NUMEL(h_hat),
					.xlabel = "n",
					.ylabel = "\\hat{h}(n)",
					.title = plottitle,
					.axis = axisdummy,
					.label_prec = 4
				};
				asciiplot_draw(&dummyplot);
			}
			break;
		case 's':
			if (signal_mode == signal_off){
				signal_mode = signal_on;
				printf("Signal source turned on\n");
			}else{
				signal_mode = signal_off;
				printf("Signal source turned off\n");
			}
			break;
		}
	}

	float outdata[AUDIO_BLOCKSIZE];
	float distdata[AUDIO_BLOCKSIZE];
	float signaldata[AUDIO_BLOCKSIZE];
	
	// Load music signal
	blocks_sources_waveform(signaldata);
	
	// Load desired disturbance signal
	if (dist_src == noise_src){
		blocks_sources_disturbance(distdata);
	} else { // cosine as disturbance
		blocks_sources_cos(distdata);
	};
	
	// Set amplitude of disturbance
	arm_scale_f32(distdata, 0.2f, distdata, AUDIO_BLOCKSIZE);
	
	// Send desired net signal to left output
	if (signal_mode == signal_on){
		arm_add_f32(distdata,signaldata,outdata,AUDIO_BLOCKSIZE); // add signal and noise
		blocks_sinks_leftout(outdata); // Send to left channel
	}else{
		blocks_sinks_leftout(distdata); // Send to left channel
	}
	
	
	//Update LMS filter if enabled
	float lms_mic[AUDIO_BLOCKSIZE];
	blocks_sources_microphone(lms_mic);
	float lms_output[AUDIO_BLOCKSIZE];
	float lms_err[AUDIO_BLOCKSIZE];
	if ((lms_mode == lms_enbl) || (lms_mode == lms_updt)){
		my_lms(distdata, lms_mic, lms_output, lms_err, AUDIO_BLOCKSIZE, lms_mu, lms_coeffs, lms_state, LAB_LMS_TAPS_ONLINE);
		blocks_sinks_rightout(lms_err); // Send cleaned signal to right channel
	}else{
		blocks_sinks_rightout(lms_mic);
	};
}

void my_lms(float * y, float * x, float * xhat, float * e, int block_size,
			float lms_mu, float * lms_coeffs, float * lms_state, int lms_taps){
	/** @brief Perform one block iteration of the LMS filter
	 * This function is missing some code that you need to add to get working.
	 * 
	 * The course book describes the LMS algorithm as;
	 * (Digital signal processing, Mulgrew et. al. 2003, pg. 229, table 8.2)
	 * 
	 * ----------------------------------------
	 * Initialization:
	 * set h to zero
	 * 
         * Update:
	 * For each sample n = 1, 2, 3, ... do
	 * xhat(n) = h(n-1) .* y(n)
	 * e(n) = x(n) - xhat(n)
	 * h(n) = h(n-1) + 2*mu*y(n)*e(n)
	 * ----------------------------------------
	 * 
	 * The initialization step has already been taken care of.
	 * You now need to write code that performs the update step.
	 * 
	 * The following variables are accessible by you:
	 * block_size		length of the following vectors;
	 * 		y			the block-size'th most recent input signals (see lms_state)
	 * 		x			vector of "desired" signal
	 * 		xhat		vector of filter output
	 * 		e			vector of error output
	 * 
	 * lms_mu			step size of the LMS filter update
	 * 
	 * lms_coeffs		vector of filter coefficients (i.e. h) **STORED IN REVERSE ORDER**
	 * lms_taps			the number of elements in lms_coeffs (i.e. filter length)
	 * 
	 * By reverse order we mean that the true filter is given as
	 * h = [lms_coeffs[lms_taps-1], ..., lms_coeffs[1], lms_coeffs[0]]
	 * 
	 * lms_state		The most recent (block_size + lms_taps - 1) elements of y
	 * The code that updates LMS state with the new elements in y is already
	 * implemented for you
	 * 
	 * (All signal vectors orderd with oldest element first, i.e.
	 * x = [x_(n-10), x_(n-9), ..., x_(n))])
	 * 
	 * Note that as the DSP-kit does block-based processing on each call to
	 * my_lms(...) you need to process block_size samples!
	 */

	/* Copy new input into lms_state, ordered as
	* [ ... <lms_taps-1 old elements> ... , ... <block_size new elements> ... ]
	*/
	arm_copy_f32(y, &(lms_state[lms_taps-1]), block_size);
	
#ifdef MASTER_MODE
	//If using the full code-base (including the solution) directly solve the LMS update
	#include "../../secret_sauce.h"
	DO_LMS();
#else
	/* TODO: Add code from here...*/
	
	
	/* ...to here */
#endif

	/* Update lms state, ensure the lms_taps-1 first values correspond to the
	* lms_taps-1 last values of y, i.e.
	* [ y[end - lms_taps - 1], ..., y[end], ... <don't care values, will be filled with new y on next call> ...]
	 */
	arm_copy_f32( &y[block_size - (lms_taps-1)], lms_state, lms_taps-1);
};
