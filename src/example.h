#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#include "config.h"

#if defined(SYSMODE_TEST1)
// Function triplet for the audio effects example
void example_test1_init(void);
void example_test1(void);
void example_test1_btnpress(void);				//!<- Function to call to switch between audio sources
#elif defined(SYSMODE_TEST2)
// Function pair for the microphone and disturbance example
void example_test2_init(void);
void example_test2(void);
#elif defined(SYSMODE_TEST3)
#define TEST3_PRINT_PERIOD_ms		(250)
// Function pair for the frequency shift example
void example_test3_init(void);
void example_test3(void);
#elif defined(SYSMODE_TEST4)
// Function triplet for the crude QPSK example
void example_test4_init(void);
void example_test4(void);
void example_test4_btnpress(void);	//!<- Function to call to trigger a transmission
#elif defined(SYSMODE_TEST5)
// Function pair for the time-domain frequency shift example
void example_test5_init(void);
void example_test5(void);
#elif defined(SYSMODE_RADAR)
// Function pair for the pulse-radar example
#define RADAR_V_PROP		(343)					//Speed of propogation in channel (i.e. for this example, the speed of sound in the air between speaker/microphone) [m/s]
#define RADAR_L_MAX			(20)					//Lower bound to maximum measurement distance [m]
#define RADAR_CHIRP_SIZE	(256)		//Radar chirp size [samples]. Must be no larger than AUDIO_BLOCKSIZE

//Example with very narrowband transmitted signal
#define RADAR_F_START		(2 * AUDIO_SAMPLE_RATE / RADAR_CHIRP_SIZE)				//Chirp initial frequency [Hz]
#define RADAR_F_STOP 		RADAR_F_START											//Chirp final frequency [Hz]


//Example with slightly increased bandwidth
//#define RADAR_F_START		(2 * AUDIO_SAMPLE_RATE / RADAR_CHIRP_SIZE)				//Chirp initial frequency [Hz]
//#define RADAR_F_STOP 		(RADAR_F_START * 2.0f)									//Chirp final frequency [Hz]

//Example with broadband chirp
//#define RADAR_F_START		(AUDIO_SAMPLE_RATE/16)									//Chirp initial frequency [Hz]
//#define RADAR_F_STOP 		(AUDIO_SAMPLE_RATE/4)									//Chirp final frequency [Hz]


//Length of buffer to store recieved samples in [samples]
//Choose length that is at least long enough for buffer and channel
//propogation delay, then round up size to nearest multiple of AUDIO_BLOCKSIZE.
#define RADAR_RX_SIZE  		(2 * AUDIO_BLOCKSIZE + AUDIO_BLOCKSIZE * CEILING(CEILING(RADAR_L_MAX * AUDIO_SAMPLE_RATE, RADAR_V_PROP), AUDIO_BLOCKSIZE) + RADAR_CHIRP_SIZE)
#define RADAR_DELAY_s		(1)						//Time to wait between successive radar chirps [s]
void example_radar_init(void);
void example_radar(void);
#elif defined(SYSMODE_FFT)
// Function pair for the frequency-domain-filtering example
void example_fft_init(void);
void example_fft(void);
void example_fft_btnpress(void);
#endif

#endif /* SRC_EXAMPLE_H_ */
