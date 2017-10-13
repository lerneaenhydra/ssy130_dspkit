/**
 * @file         headphone.c
 * @version      1.0
 * @date         2015
 * @author       Christoph Lauer
 * @compiler     armcc
 * @copyright    Christoph Lauer engineering
 */
 
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "headphone.h"
#include "../../main.h"
#include "../../config.h"
#include "microphone.h"
#include "board.h"
#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio_codec.h"
#include "stm32f4xx_it.h"
#include "../systime/systime.h"
#include "../../blocks/sources.h"
#include "../printfn/printfn.h"
#include "blocks/sinks.h"

/*
 * We have three buffers: two output buffers used in a ping-pong arrangement, and an input
 * (microphone) circular buffer. Because the output buffers are written directly to the I2S
 * interface with DMA, they must be stereo. The microphone buffer is mono and its duration
 * is exactly 3 times the length of one of the ping-pong buffers. The idea is that during
 * normal operation, the microphone buffer will vary between about 1/3 full and 2/3 full
 * (with a 1/3 buffer margin on either side).
 */
#define MIC_BUFFER_SAMPLES (OUTRAW_BUFFER_SAMPLES * 3 / 2)

static int16_t buff0 [OUTRAW_BUFFER_SAMPLES], buff1 [OUTRAW_BUFFER_SAMPLES], micbuff [MIC_BUFFER_SAMPLES];

/** @brief For ease of use every call to fill_buffer will also fill this array
 * with the oldest pending data in the microphone buffer. This ensures that the
 * end-users don't need to worry about reading the microphone buffer at the
 * required period and also can address the microphone data with linear
 * addressing. */
float processed_micdata[AUDIO_BLOCKSIZE] = {0.0f};

//Head and tail indices for microphone buffer
static volatile uint16_t mic_head = 0;
static volatile uint16_t mic_tail = 0;

static volatile uint8_t next_buff = 0;	// next output buffer to write

//Data location to store new left and right data streams
float * WavePlayBackLeftData;
float * WavePlayBackRightData;

//Master volume scaling and mute state
float volume_gain = 1.0f;
int mute_state = 1;

static void fill_buffer (int16_t *buffer);

/** @brief Is set true if we get a buffer underflow during operation */
static bool buf_underflow = false;

/** @brief Initialize all playback */
static void WavePlayerInit(void);

void WaveRecorderCallback (int16_t *buffer, int num_samples){
	static int clip_timer  = 0;
	int i;
	bool clip = false;

	for(i = 0; i < num_samples; ++i) {
		const int16_t sample = *buffer++;
		if (sample < 0.99f * INT16_MIN || sample > 0.99f * INT16_MAX){
			clip = true;
		}
		micbuff[mic_head++] = SYS_SIGN * sample;	//Potentially reverse sign to maintain total signal chain polarity
	}
	mic_head = mic_head % MIC_BUFFER_SAMPLES;



	if(clip_timer){
		if (!--clip_timer){
			board_set_led(DEBUG_LED_CLIP, false);
		}
	}else if(clip){
		board_set_led(DEBUG_LED_CLIP, true);
	}

	if(clip){
		clip_timer = 50;
	}
}

void WavePlayBack(void){
	/* First, we start sampling internal microphone */
	WaveRecorderBeginSampling();

	/* Initialize wave player (Codec, DMA, I2C) */
	WavePlayerInit();

	/* Let the microphone data buffer get 2/3 full (which is 2 playback buffers) */
	while(mic_head < MIC_BUFFER_SAMPLES * 2 / 3){};

	/* Fill the second playback buffer (the first will just be zeros to start) */
	fill_buffer (buff1);

	/* Start audio playback on the first buffer (which is all zeros now) */
	Audio_MAL_Play((uint32_t)buff0, OUTRAW_BUFFER_SAMPLES * 2);
	next_buff = 1;

	bool exec_time = false;	//Used to ensure the execution time does not exceed the maximum processing time allowed per AUDIO_BLOCKSIZE samples

	/* This is the main loop of the program. We simply wait for a buffer to be exhausted
	* and then we refill it. The callback (which is triggered by DMA completion) actually
	* handles starting the next buffer playing, so we don't need to be worried about that
	* latency here.
	*/

	int16_t *nextbuf_ptr = buff0;
	int16_t next_buff_idx = 1;

	for(;;){
		exec_time = false;
		while (next_buff == next_buff_idx){
			main_idle();
			exec_time = true;
		}
		if(exec_time == false){
			//We spent too much time in main_idle. Send an error message and halt.
			halt_error(DEBUG_TIME_OVERFLOWMSG);
		}

		fill_buffer(nextbuf_ptr);

		if(next_buff_idx == 1){
			next_buff_idx = 0;
			nextbuf_ptr = buff1;
		}else{
			next_buff_idx = 1;
			nextbuf_ptr = buff0;
		}
	}
}

void WavePlayerSetVolume(float volume){
	volume_gain = SAT(volume, 0, 1);
}

float WavePlayerGetVolume(void){
	return volume_gain;
}

void WavePlayerToggleMute(void){
	mute_state = !mute_state;
}

static void WavePlayerInit(void){
	/* Initialize I2S interface */
	EVAL_AUDIO_SetAudioInterface(AUDIO_INTERFACE_I2S);

	/* Initialize the Audio codec and all related peripherals (I2S, I2C, IOExpander, IOs...) */
	EVAL_AUDIO_Init(OUTPUT_DEVICE_AUTO, AUDIO_VOLUME, AUDIO_SAMPLE_RATE);

	//Update the pointers to the output buffers
	WavePlayBackRightData = blocks_sinks_rightout_ptr();
	WavePlayBackLeftData = blocks_sinks_leftout_ptr();
}

void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t Size){
	if(!buf_underflow){
		if (next_buff == 0) {
			Audio_MAL_Play((uint32_t)buff0, OUTRAW_BUFFER_SAMPLES * 2);
			next_buff = 1;
		}else{
			Audio_MAL_Play((uint32_t)buff1, OUTRAW_BUFFER_SAMPLES * 2);
			next_buff = 0;
		}
	}
}
 
void EVAL_AUDIO_HalfTransfer_CallBack(uint32_t pBuffer, uint32_t Size){}

void EVAL_AUDIO_Error_CallBack(void* pData){
	for(;;){};	//Hang on error
}

uint16_t EVAL_AUDIO_GetSampleCallBack(void){
  return 0;
}

static void fill_buffer (int16_t *buffer){
	//Prepare the microphone data
	{
		int i;
		for(i = 0; i < AUDIO_BLOCKSIZE; i++){
			if(mic_tail != mic_head){
				processed_micdata[i] = (micbuff[mic_tail]*1.0f)/INT16_MAX;
				mic_tail = (mic_tail + 1 >= MIC_BUFFER_SAMPLES) ? 0 : mic_tail + 1;
			}else{
				processed_micdata[i] = 0.0f;
			}
		}
	}
	//Update all varying signals
	blocks_sources_update();
	
	//Calculate all output to generate.
	main_audio_callback();

	//Convert floating-point output to the machine representation
	int i = 0;
	while (i < AUDIO_BLOCKSIZE){
		//Note that it is critical that the left data is written first, followed by the right data, to ensure the channels are not reversed
		const float right_n = SAT((*(WavePlayBackRightData + i)) * volume_gain, -1.0f, 1.0f) * mute_state;
		const float left_n = SAT((*(WavePlayBackLeftData + i)) * volume_gain, -1.0f, 1.0f) * mute_state;

		*buffer++ = FtoI16(left_n);
		*buffer++ = FtoI16(right_n);

		i++;
	}
}
