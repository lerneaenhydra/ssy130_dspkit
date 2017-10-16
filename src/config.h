/** @file Contains all compile-time constants and general system configuration */

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

#include "macro.h"
#include "backend/hw/board.h"

/** @brief Enumerates all possible system modes
 * 
 * Example projects. These are all fully-functional and located in example.h/.c
 * Refer to them as needed for examples of how to use the system.
 * 
 * SYSMODE_TEST1; will compile the project to apply one of several simple audio
 * DSP effects to either the internal waveform or the microphone input,
 * switching between effects depending on keys pressed on the keyboard and
 * between audio sources when pressing the user button.
 * SYSMODE_TEST2; will compile the project in a basic loop-back mode.
 * Will output the waveform to the left channel and output the microphone input
 * to the right channel. Can be used to verify the functionality of the entire
 * board and build environment and as a minimalist example of how to access the
 * microphone input and speaker outputs.
 * SYSMODE_TEST3; will compile the project to apply a frequency shift to the
 * microphone input and play back the result on both output channels. The
 * frequency shift is implemented using the FFT, where the size of the FFT is
 * set to AUDIO_BLOCKSIZE.
 * SYSMODE_TEST4; will compile the project to output a QPSK-modulated ASCII
 * bitstream on the left channel when the button is pressed and implement a ML
 * receiver on the microphone input, displaying received characters on the
 * terminal.
 * SYSMODE_RADAR; will compile the project for a pulse radar (i.e.
 * time-of-flight) distance-measurement example.
 * (See https://en.wikipedia.org/wiki/Radar#Transit_time)
 * SYSMODE_FFT; will compile the project for a simple filtering example, where
 * filtering is performed in the frequency domain rather than the naive
 * time-domain approach. For long enough block-sizes this is computationally
 * faster as a FIR filter of length N scales in computational complexity on the
 * order of N^2, while the FFT requires N*log(N). I.E. for large N eventually
 * the FFT approach is numerically faster.
 * 
 * 
 * Course projects. These are missing some lines of code (which you need to add).
 * 
 * SYSMODE_OFDM; will compile the project for the OFDM project.
 * Will call the lab_ofdm() function when AUDIO_BLOCKSIZE samples are
 * available from the microphone.
 * 
 * SYSMODE_LMS; will compile the project for the LMS adaptive filter project.
 * Will call the lab_dsp_lms() function when AUDIO_BLOCKSIZE samples are
 * available from the microphone.
 * 
 * Note; only one system mode may be defined at a time!
 * 
 */
//Fully functional example system modes
#define SYSMODE_TEST1
//#define SYSMODE_TEST2
//#define SYSMODE_TEST3
//#define SYSMODE_TEST4
//#define SYSMODE_RADAR
//#define SYSMODE_FFT

//Student project system modes
//#define SYSMODE_OFDM
//#define SYSMODE_LMS

#if (1 != defined(SYSMODE_TEST1) + defined(SYSMODE_TEST2) + defined(SYSMODE_TEST3) + defined(SYSMODE_TEST4) + defined(SYSMODE_LMS) + defined(SYSMODE_OFDM) + defined(SYSMODE_RADAR) + defined(SYSMODE_FFT))
#error Exactly one operation mode must be selected!
#endif

/******************************************************************************
 *
 * 			   The following parameters may be freely modified
 *
 *****************************************************************************/

/** @brief Define the waveform to load
 * Permissible values are of the format 'AUDIO_WAVEFORM_WAVEFORMx', where x may
 * be an integer that matches any of the files located in
 * /src/blocks/waveformx.h. For example, if there exists files waveform1.h/.c
 * through waveform6.h/.c, then AUDIO_WAVEFORM_WAVEFORM1 through 
 * AUDIO_WAVEFORM_WAVEFORM6 are permissibile */
#define AUDIO_WAVEFORM_WAVEFORM1

/** @brief Define this directive to include the assignment solutions. The
 * student code-base does not have the needed file and will generate a compiler
 * error if this directive is set. */
//#define MASTER_MODE

/** @brief Global audio sample rate [Hz]
 * Due to hardware limitations, the sample-rate must be evenly divisible by
 * 48kHz and no less than 16kHz. This gives a total of three valid sample-rates;
 * 48kHz, 24kHz, and 16kHz.
 * Note that higher sample rates will reduce the computational capacity
 * available as follows;
 * 	48kHz approx. 60% CPU time available for application-level code
 * 	24kHz approx. 80% CPU time available for application-level code
 * 	16kHz approx. 90% CPU time available for application-level code */
#if defined(SYSMODE_TEST1) || defined(SYSMODE_TEST2)
/** @brief Sample-rate for examples that are relatively computationally lightweight */
#define AUDIO_SAMPLE_RATE 			(48000)
#elif defined(SYSMODE_TEST3) || defined(SYSMODE_FFT)
/** @brief Sample-rate for examples that are relatively computationally heavy */
#define AUDIO_SAMPLE_RATE			(24000)
#elif defined(SYSMODE_TEST4) || defined(SYSMODE_RADAR) || defined(SYSMODE_LMS) || defined(SYSMODE_OFDM)
/** @brief For test 4, the sample rate must be a power of two in order to
 * generate a sinusoid that perfectly fits in AUDIO_BLOCKTIME. For the LMS and 
OFDM labs lots of computational time is needed and we don't have any real sample 
rate requirements. */
#define AUDIO_SAMPLE_RATE			(16000)
#else
#error Invalid system mode selected
#endif

/** @brief Block size for each speaker callback function.
 * Speaker callbacks will expect this many samples to be present on each call.
 * Larger values will increase the net audio path latency, but result in lower
 * task-switching overhead. Reasonable values range from 64 to 1024 with 256
 * being a reasonable default.
 * For speed, this must be a power of two. */
#if defined(SYSMODE_TEST1) || defined(SYSMODE_TEST2)
/** @brief Arbitrarily select a 256-sample block size */
#define AUDIO_BLOCKSIZE				(256)
#elif defined(SYSMODE_FFT)
/** @brief Select a 192-sample block size  to fit 64 length filter and
* 256 samples fft  */
#define AUDIO_BLOCKSIZE				(256)
#elif defined(SYSMODE_TEST3)
/** @brief Force a large block size for the FFT example in order to have narrow
 * frequency bins and reduce the AM effect from windowing. */
#define AUDIO_BLOCKSIZE				(1024)
#elif defined(SYSMODE_TEST4)
/** @brief In test 4, the symbol length is directly set by the block size.
 * 256 samples gives a reasonable (but relatively long) symbol length. */
#define AUDIO_BLOCKSIZE				(256)
#elif defined(SYSMODE_RADAR)
/** @brief For the radar example, keep the block size shortish to reduce the
 * latency caused by the audio buffers. The maximum chirp length must fit
 * within one block, setting a lower bound to the value */
#define AUDIO_BLOCKSIZE				(256)
#elif defined(SYSMODE_LMS)
/** @brief Arbitrarily select a 256-sample block size for the LMS lab. This value is not particularly critical */
#define AUDIO_BLOCKSIZE				(256)
#elif defined(SYSMODE_OFDM)
/** @brief Arbitrarily select a large block size as this gives us a longer time
 * to perform signal processing calculations before the microphone/output DMA
 * buffers need to be handled */
#define AUDIO_BLOCKSIZE				(1024)
#else
#error Invalid system mode selected
#endif

/** @brief Microphone prefilter low-pass cutoff frequency.
 * Set to zero to disable and slightly reduce the computational load. */
#define MIC_LP_FC					(0)
/** @brief Microphone prefilter high-pass cutoff frequency.
 * Set to zero to disable and slightly reduce the computational load.
 * Recommended to keep at 20-50Hz. */
#define MIC_HP_FC					(20)

/** @brief Character to input to increase the output volume */
#define VOLUME_UP_CHAR				'+'
/** @brief Character to input to decrease the output volume */
#define VOLUME_DOWN_CHAR			'-'
/** @brief Character to input to toggle the output mute */
#define VOLUME_MUTE_CHAR			'0'
/** @brief Amount to scale volume control by for each volume up/down event */
#define VOLUME_SCALING				(1.2f)

/** @brief Number of columns in all terminal plots */
#define PLOT_COLS					(120)
/** @brief Number of rows in all terminal plots */
#define PLOT_ROWS					(40)

/** @brief Master logarithmic output volume (amplitude) set in DAC.
 * Valid range are integers 0 - 85.
 * Value must be <= 85 to ensure output clipping does not occur with input
 * signals in the range [-1, 1]. Output amplitude is proportional to
 * 10^AUDIO_VOLUME.
 * The user can control the output amplitude using the  */
#define AUDIO_VOLUME				(70)

/** @brief Microphone gain.
 * Valid range is integers 0 - 100 */
#define MIC_VOLUME					(100)

/** @brief Set to 1 or -1 to control the net sign through the speakers and microphone */
#define SYS_SIGN					(-1)

/******************************************************************************
 *
 * The following parameters may be freely changed, but are set to (hopefully)
 * reasonable defaults that shound't need to be modified.
 *
 *****************************************************************************/

/** @brief Number of test loops to apply to each function to test for the OFDM-functions */
#define UNITTEST_PASSES_OFDM		(150)

/** @brief Number of LMS iterations to apply when testing the LMS functions */
#define UNITTEST_PASSES_LMS			(313)

/** @brief Maximum tolerable deviation between reference and student functions */
#define UNITTEST_TOL				(1.0e-3)

/** @brief Debug USART interface baud rate */
#define DEBUG_USART_BAUDRATE		(921600ULL)

/** @brief Number of elements for debug USART transmit buffer */
#define DEBUG_USART_TX_BUF_LEN		(8192)
/** @brief Number of elements for debug USART recieve buffer */
#define DEBUG_USART_RX_BUF_LEN		(16)

#define DEBUG_LINESEP				"-------------------------------------------------------------------------------\n"

/** @brief Message to print at startup */
#define DEBUG_STARTUPMSG 							\
"\n\n\n\n\n"										\
"  ____ ______   ___ _____  ___  \n"				\
" / ___/ ___\\ \\ / / |___ / / _ \\ \n"				\
" \\___ \\___ \\\\ V /| | |_ \\| | | |\n"			\
"  ___) |__) || | | |___) | |_| |\n"				\
" |____/____/ |_| |_|____/_\\___/ \n"				\
" |  _ \\ ___ _ __   | |/ (_) |_  \n"				\
" | | | / __| '_ \\  | ' /| | __| \n"				\
" | |_| \\__ \\ |_) | | . \\| | |_  \n"				\
" |____/|___/ .__/  |_|\\_\\_|\\__| \n"				\
"           |_|"									\
"\n\nCompiled on " __DATE__ " at " __TIME__			\
"\n\nSystem setup;"																		\
"\n\tSample rate           "xstr(AUDIO_SAMPLE_RATE)" Hz"								\
"\n\tBlocksize;            "xstr(AUDIO_BLOCKSIZE)" samples"								\
"\n\tMic. lowpass  cutoff; "xstr(MIC_LP_FC)" Hz"										\
"\n\tMic. highpass cutoff; "xstr(MIC_HP_FC)" Hz"										\
"\n\nUse the "xstr(VOLUME_UP_CHAR)" and "xstr(VOLUME_DOWN_CHAR)" keys respectively to change the speaker volume during operation."\
"\nUse the "xstr(VOLUME_MUTE_CHAR)" key to mute/unmute the output.\n"\
DEBUG_LINESEP

/** @brief Error message to print on a processing time overrun */
#define DEBUG_TIME_OVERFLOWMSG		"\nERROR! Execution time exceeded maximum permissible time per sample! Halting =(\n"

/** @brief Error message to print on a microphone buffer overflow */
#define DEBUG_MICBUG_OVERFLOWMSG	"\nERROR! Microphone input buffer overflow! Halting =(\n"

/** @brief Systick timer frequency */
#define BOARD_SYSTICK_FREQ_Hz		(1000)

/** Maximum string size allowed for printf family [bytes]. */
#define PRINTF_MAX_STRING_SIZE		(4096)

/** @brief LED to use for clipped input indication */
#define DEBUG_LED_CLIP				(board_led_red)

/** @brief Period for idle LED indicator */
#define IDLE_LED_BLINK_PER_ms		(500)

/** @brief The button debounce interval.
 * The button must be kept at this value for at least this time to be regarded
 * as stable */
#define DEBOUNCE_TIME_MIN_ms		(50)

/** @brief LED to use for system-alive indiation */
#define IDLE_LED					(board_led_green)

/** @brief The time duration each block corresponds to */
#define AUDIO_BLOCKTIME_s 			((AUDIO_BLOCKSIZE*1.0f)/(AUDIO_SAMPLE_RATE*1.0f))

/** @brief Number of samples (left and right) for each output DMA buffer */
#define OUTRAW_BUFFER_SAMPLES 		(AUDIO_BLOCKSIZE*2)

#endif /* SRC_CONFIG_H_ */
