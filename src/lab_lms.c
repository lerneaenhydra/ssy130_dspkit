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
#define		LAB_LMS_MU_CHANGE		(1.2f)

/** @brief Maximual number of filter taps to use for on-line LMS operation */
#define		LAB_LMS_TAPS_ONLINE_MAX	(512)


/** @brief Length of stored lms filter error log [s]
 * The first error output from the LMS filter will be saved on every LMS call
 * this effectively means every AUDIO_BLOCKSIZE'th error value will be saved.
 * Log will record over this period after resetting the LMS filter coefficients */
#define 	LAB_LMS_ERRLOG_LEN_s	(20)

/** @brief Decimation factor for colored gaussian noise
 * Gaussian noise source is bandwidth-limited to
 * AUDIO_SAMPLE_RATE/LAB_LMS_CGN_FAC
 * This also reduces the computational burden (yay!) */
#define 	LAB_LMS_CGN_FAC			(4)

/** @brief Number of AUDIO_BLOCKSIZE blocks that the I/O buffers increase the
 * round-trip latency by */
#define		LAB_LMS_IOBUF_LATENCY_BLKS	(4)

// Stateful variables for the LMS lab
float lms_coeffs[LAB_LMS_TAPS_ONLINE_MAX];
float lms_state[LAB_LMS_TAPS_ONLINE_MAX + AUDIO_BLOCKSIZE - 1];
float lms_err_buf[LAB_LMS_ERRLOG_LEN_s * AUDIO_SAMPLE_RATE / AUDIO_BLOCKSIZE];
float lms_distbuf[LAB_LMS_IOBUF_LATENCY_BLKS*AUDIO_BLOCKSIZE];
size_t lms_err_buf_idx;
float lms_err_buf_time;
float lms_mu = LAB_LMS_MU_INIT;
uint_fast32_t seed;			//PRNG seed for gaussian noise generator
size_t n_lms_taps = 128;	//Number of LMS taps to use right now

// enum type for different modes of operation
enum lms_modes {lms_updt, lms_enbl, lms_dsbl} lms_mode;
enum dist_srces {cos_src, noise_src} dist_src;
enum signal_modes {signal_off, signal_on} signal_mode;

// Internal functions
static void lab_lms_reset_errlog(void){
	arm_fill_f32(NAN, lms_err_buf, NUMEL(lms_err_buf));
	lms_err_buf_idx = 0;
	lms_err_buf_time = 0;
}

#define PRINT_HELPMSG() 																								\
		printf("Usage guide;\n"																							\
			"Press the following keys to change the system behavior\n"													\
			"\t'd' - LMS filtering disabled, raw microphone data output to left speaker. Initial mode.\n"				\
			"\t'f' - LMS filtering applied, update disabled (h held constant), error signal output to left speaker\n"	\
			"\t'u' - LMS filtering applied with filter update, error signal output to left speaker\n"					\
			"\t't' - Toggle disturbance source between cosine signal and wide band noise\n"								\
			"\t'r' - Reset filter coefficients to 0 and empty logged error output\n"									\
			"\t'1' - Increase step size mu\n"																			\
			"\t'2' - Decrease step size mu\n"																			\
			"\t'3' - Increase number of used LMS taps (maximum " xstr(LAB_LMS_TAPS_ONLINE_MAX) ")\n"					\
			"\t'4' - Decrease number of used LMS taps (minimum 1, unused taps set to zero)\n"							\
			"\t'm' - Prints the filter coefficients h in a format useful for import in Matlab\n"						\
			"\t'h' - Plots the current filter coefficients directly in the terminal\n"									\
			"\t'e' - Plots the most recent " xstr(LAB_LMS_ERRLOG_LEN_s) " seconds of the LMS filter error output\n"		\
			"\t's' - Toggles music signal\n");																			\

void lab_lms_init(void){
	//Manually initialize the LMS filter coefficients and state to all zeros
	arm_fill_f32(0.0f, lms_coeffs, NUMEL(lms_coeffs));
	arm_fill_f32(0.0f, lms_state, NUMEL(lms_state));
	arm_fill_f32(0.0f, lms_distbuf, NUMEL(lms_distbuf));
	lab_lms_reset_errlog();
	blocks_sources_trig_setfreq(LAB_LMS_SINE_TONE_HZ);
	lms_mode = lms_dsbl; // start with disabled mode
	dist_src = noise_src; // start with wide band noise
	signal_mode = signal_on;
	seed = util_get_seed();
	PRINT_HELPMSG();
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
			printf("Filtering enabled, h kept constant, error signal output to speaker\n");
			lms_mode = lms_enbl;
		break;
			case 'u':
			printf("Filtering enabled, h updated, error signal output to speaker\n");
			lms_mode = lms_updt;
			printf("Step size mu set to %e\n", lms_mu);
			break;
		case 'r':
			lab_lms_reset_errlog();
			arm_fill_f32(0.0f, lms_coeffs, NUMEL(lms_coeffs));
			printf("Reset filter coefficients (h) to zero\n");
			break;
		case '1':
			lms_mu *= LAB_LMS_MU_CHANGE;
			printf("Step size mu increased to %e\n", lms_mu);
			break;
		case '2':
			lms_mu *= 1.0f/LAB_LMS_MU_CHANGE;
			printf("Step size mu decreased to %e\n", lms_mu);
			break;
		case '3':
			if(n_lms_taps < LAB_LMS_TAPS_ONLINE_MAX){
				n_lms_taps++;
				//Preserve n_lms_taps coefficients by shuffling them all up one index
				size_t i;
				for(i = n_lms_taps; i; i--){
					lms_coeffs[i] = lms_coeffs[i-1];
				}
				//Initialize newly added coefficient
				lms_coeffs[0] = 0.0f;
			}
			printf("Using %d taps for LMS filter (unused taps set to zero)\n", n_lms_taps);
			break;
		case '4':
			if(n_lms_taps > 1){
				n_lms_taps--;
				//Preserve n_lms_taps coefficients by shuffling them all down one index
				size_t i;
				for(i = 0; i < n_lms_taps; i++){
					lms_coeffs[i] = lms_coeffs[i+1];
				}
				//Clear now unused tap value
				lms_coeffs[n_lms_taps] = 0.0f;
			}
			printf("Using %d taps for LMS filter (unused taps set to zero)\n", n_lms_taps);
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
			printf("Filter coefficients in reversed order (use e.g. 'flipud(h)' in Matlab to restore actual order)\n");
			print_vector_f("h_reversedOrder", lms_coeffs, n_lms_taps);
			break;
		case 'h':
			{
				float h_hat[n_lms_taps];
				//Generate h_hat by reversing lms_coeffs
				int i;
				for(i = 0; i < n_lms_taps; i++){
					h_hat[i] = lms_coeffs[n_lms_taps - 1 - i];
				}
				char plottitle[] = "Current estimated channel coefficients (equivalent to 'plot(flipud(h))' in Matlab)";
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
		case 'e':
			{
				//Generate LMS log error array
				float lms_err_log[NUMEL(lms_err_buf)];

				//Copy last lms_err_buf_idx elements, corresponds to oldest stored data
				arm_copy_f32(&lms_err_buf[lms_err_buf_idx], lms_err_log, NUMEL(lms_err_buf) - lms_err_buf_idx);

				//Copy remaining elements, corresponds to newest stored data
				arm_copy_f32(lms_err_buf, &lms_err_log[NUMEL(lms_err_buf) - lms_err_buf_idx], lms_err_buf_idx);

				//Generate LMS log time array
				float errlog_time[NUMEL(lms_err_buf)];
				errlog_time[0] = lms_err_buf_time - (1.0f * NUMEL(lms_err_buf) * AUDIO_BLOCKSIZE) / AUDIO_SAMPLE_RATE;

				size_t i;
				for(i = 0; i < NUMEL(errlog_time) - 1; i++){
					errlog_time[i+1] = errlog_time[i] + (1.0f * AUDIO_BLOCKSIZE) / AUDIO_SAMPLE_RATE;
				}

				//Plot result
				char plottitle[] = "Logged LMS error output";
				float axisdummy[] = {NAN, NAN, 0, NAN};	//Auto-scale plot extents, except minimum error (force zero)
				struct asciiplot_s dummyplot = {
					.cols = PLOT_COLS,
					.rows = PLOT_ROWS,
					.xdata = errlog_time,
					.ydata = lms_err_log,
					.data_len = NUMEL(lms_err_log),
					.xlabel = "Time since filter reset [s]",
					.ylabel = "abs(error)",
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
		//Generate colored gaussian noise by low-pass filtering white guassian noise
		
		/* As it's expensive to generate gaussian samples and we want a
		 * low-passed gaussian process, simply create a gaussian process at
		 * low sample rate and apply ZOH to up-sample to the globak sample rate */
		float rawdist[AUDIO_BLOCKSIZE/LAB_LMS_CGN_FAC];
		util_randN(0, 0.5, &seed, rawdist, NUMEL(rawdist));

		//Upsample to the global sample rate
		size_t i;
		for(i = 0; i < NUMEL(rawdist); i++){
			arm_fill_f32(rawdist[i], &distdata[i*LAB_LMS_CGN_FAC], LAB_LMS_CGN_FAC);
		}
	} else { // cosine as disturbance
		blocks_sources_cos(distdata);
	};
	
	// Set amplitude of disturbance
	arm_scale_f32(distdata, 0.2f, distdata, AUDIO_BLOCKSIZE);
	
	// Send desired net signal to right output
	if (signal_mode == signal_on){
		arm_add_f32(distdata,signaldata,outdata,AUDIO_BLOCKSIZE); // add signal and noise
		blocks_sinks_rightout(outdata);
	}else{
		blocks_sinks_rightout(distdata);
	}
	
	//Do selected filtering operation
	float lms_mic[AUDIO_BLOCKSIZE];
	blocks_sources_microphone(lms_mic);
	float lms_output[AUDIO_BLOCKSIZE];
	float lms_err[AUDIO_BLOCKSIZE];
	bool do_lms = false;
	float net_mu = 0;
	switch(lms_mode){
		case lms_dsbl:
		default:
			//LMS disabled, output mic data to left output
			blocks_sinks_leftout(lms_mic);
			break;
		case lms_enbl:
			//LMS enabled, zero stepsize
			net_mu = 0;
			do_lms = true;
			break;
		case lms_updt:
			//LMS enabled, stepsize mu
			net_mu = lms_mu;
			do_lms = true;
			break;
	}

	/* As LMS filter is limited in length (shorter than the I/O buffers)
	 * make life easier by buffering the disturbance data for as long as the
	 * I/O buffers. We'll still have propogration delay in the channel and
	 * microphone downsampling filter, but this is much more modest than the
	 * LAB_LMS_IOBUF_LATENCY_BLKS*AUDIO_BLOCKSIZE elements in the I/O filters.
	 * Store data in the buffer here oldest-element first. */

	//First shuffle old data around
	{
		size_t i;
		for(i = 0; i < LAB_LMS_IOBUF_LATENCY_BLKS - 1; i++){
			arm_copy_f32(&lms_distbuf[(i + 1) * AUDIO_BLOCKSIZE], &lms_distbuf[i * AUDIO_BLOCKSIZE], AUDIO_BLOCKSIZE);
		}
	}

	//Finally add the newest data to the buffer
	arm_copy_f32(distdata, &lms_distbuf[NUMEL(lms_distbuf) - AUDIO_BLOCKSIZE], NUMEL(distdata));

	if(do_lms){
		my_lms(lms_distbuf, lms_mic, lms_output, lms_err, AUDIO_BLOCKSIZE, net_mu, lms_coeffs, lms_state, n_lms_taps);
		
		blocks_sinks_leftout(lms_err); // Send cleaned signal to left channel
		
		lms_err_buf_time  += (1.0f * AUDIO_BLOCKSIZE) / AUDIO_SAMPLE_RATE;

		//Add the first element of the error output to the logged error signal
		lms_err_buf[lms_err_buf_idx] = fabsf(lms_err[0]);
		lms_err_buf_idx = (lms_err_buf_idx+1) % NUMEL(lms_err_buf);	//Wrap err log when log is full
	}

	
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
	 * xhat(n) = h(n-1) * y(n) [note: here '*' implies the dot product]
	 * e(n) = x(n) - xhat(n)
	 * h(n) = h(n-1) + 2 * mu * y(n) * e(n)
	 * ----------------------------------------
	 * 
	 * The initialization step has already been taken care of.
	 * You now need to write code that performs the update step.
	 * 
	 * If you are unfamiliar with the C-language there is a Matlab
	 * implementation of the LMS algorithm in the 'matlab_lms' project 
	 * subdirectory. Run the 'test_lms.m' script to use it and draw a few
	 * plots. It's up to you to implement an equivalent to the my_lms.m
	 * function.
	 *
	 * Note that in the C implementation we won't split this function into
	 * two parts (doLms and my_lms) to save execution time. You'll need to
	 * correctly index lms_state and x directly without using an equivalent of
	 * the 'doLms' function.
	 * 
	 * ----------------------------------------
	 * 
	 * The following variables are accessible by you:
	 * block_size		input: length of the following vectors;
	 * 		y			input: the block-size'th most recent input signals (**you won't need to use this variable, see lms_state**)
	 * 		x			input: vector of "desired" signal
	 * 		xhat		output: vector of filter output
	 * 		e			output: vector of error output
	 * 
	 * lms_mu			input: step size of the LMS filter update
	 * 
	 * lms_coeffs		input/output: vector of filter coefficients (i.e. h) **STORED IN REVERSE ORDER**
	 * lms_taps			input: the number of elements in lms_coeffs (i.e. filter length)
	 * 
	 * By reverse order we mean that the true filter is given as
	 * h = [lms_coeffs[lms_taps-1], ..., lms_coeffs[1], lms_coeffs[0]]
	 * 
	 * lms_state		The most recent (block_size + lms_taps - 1) elements of y
	 * 
	 * The code that updates LMS state with the new elements in y is already
	 * implemented for you
	 * 
	 * Note: All signal vectors orderd with oldest element first, i.e.
	 * x = [x_(n-10), x_(n-9), ..., x_(n))]
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
	/* TODO: Add code from here...
	 *
	 * doing block_size lms update iterations, i.e. something like:
	 * 
	 * int n;
	 * for(n <range 0 - block_size>){ 		   //Which order should we loop n over? [0, 1, 2, ..., block_size]? [block_size, ..., 1, 0]?
	 *   float * y = &lms_state[<some index>]; //See documentation above for how to extract the relevant section from the long vector lms_state to create "y"
	 *   xhat[n] = lms_coeffs * y;             //here '*' implies the dot product. Either use arm_dot_prod_f32 or use a loop to compute xhat[n]
	 *   e[n] = x[n] - xhat[n];                //e[n] is a scalar, so do we need to do any looping here?
	 *   lms_coeffs += 2 * mu * y * e[n];      //Use some type of loop to update the vector lms_coeffs with the vector y multiplied by scalars 2, mu, e[n].
	 * }
	 * ...to here */
#endif

	/* Update lms state, ensure the lms_taps-1 first values correspond to the
	* lms_taps-1 last values of y, i.e.
	* [ y[end - lms_taps - 1], ..., y[end], ... <don't care values, will be filled with new y on next call> ...]
	*/
	arm_copy_f32( &y[block_size - (lms_taps-1)], lms_state, lms_taps-1);
};
