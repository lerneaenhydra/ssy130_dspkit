#include <stm32f4xx.h>
#include <math.h>
#include "main.h"
#include "config.h"
#include "backend/hw/board.h"
#include "backend/hw/headphone.h"
#include "backend/systime/systime.h"
#include "blocks/sources.h"
#include "blocks/sinks.h"
#include "backend/arm_math.h"
#include "util.h"
#include "backend/printfn/printfn.h"
#include "example.h"
#include "backend/unittest.h"
#if defined(SYSMODE_LMS)
#include "lab_lms.h"
#elif defined(SYSMODE_OFDM)
#include "lab_ofdm.h"
#endif
/**
 * This project is intended to act as an easy way of getting started with
 * real-time applied signal processing in hardware. All initialization and
 * backend handling is already implemented using a block-based structure.
 *
 * See the quick-state guide (included) for a quick description of the source
 * code structure and how to use the kit.
 */
systime_t idle_led_timer;

int main(void){
	//Generate a compiler error on invalid #define statements
	BUILD_BUG_ON(AUDIO_SAMPLE_RATE != 16000 && AUDIO_SAMPLE_RATE != 24000 && AUDIO_SAMPLE_RATE != 48000);
	BUILD_BUG_ON(AUDIO_VOLUME > 85 || AUDIO_VOLUME < 0);
	BUILD_BUG_ON(MIC_VOLUME > 100 || MIC_VOLUME < 0);
	BUILD_BUG_ON(!ISPOW2(OUTRAW_BUFFER_SAMPLES));
	BUILD_BUG_ON(!(SYS_SIGN == 1 || SYS_SIGN == -1));
	SystemInit();
	board_init();
	
	printf(DEBUG_STARTUPMSG);
	
	unittest_run();

	idle_led_timer = systime_get_delay(0);
	
#if defined(SYSMODE_TEST1)
	example_test1_init();
#elif defined(SYSMODE_TEST2)
	example_test2_init();
	example_test2_btnpress();	//Used to force printing a status message on startup
#elif defined(SYSMODE_TEST3)
	example_test3_init();
#elif defined(SYSMODE_TEST4)
	example_test4_init();
#elif defined(SYSMODE_LMS)
	lab_lms_init();
#elif defined(SYSMODE_OFDM)
	lab_ofdm_init();
#elif defined(SYSMODE_RADAR)
	example_radar_init();
#elif defined(SYSMODE_FFT)
	example_fft_init();
#else
#error invalid system mode setup!
#endif
	//Never returns; fills an incoming buffer with microphone samples and drives the stereo analog output
	WavePlayBack();
	//Return statement not strictly needed, included for formality.
	return 0;
}

void main_idle(void){
	/* Generic idle process. Is continuously called from a non-interrupt context
	 * when the callback function is completed. */
	//Toggle the idle LED occasionally
	if(systime_get_delay_passed(idle_led_timer)){
		idle_led_timer = systime_add_delay(MS2US(IDLE_LED_BLINK_PER_ms/2), idle_led_timer);
		board_toggle_led(IDLE_LED);
	}
}

void main_audio_callback(void){
	/* Called when AUDIO_BLOCKSIZE samples need to be supplied to the left and
	 * right audio sinks. At this point we can also safely read AUDIO_BLOCKSIZE
	 * samples from source blocks. Is called from a non-interrupt context. */
#if defined(SYSMODE_TEST1)
	example_test1();
#elif defined(SYSMODE_TEST2)
	example_test2();
#elif defined(SYSMODE_TEST3)
	example_test3();
#elif defined(SYSMODE_TEST4)
	example_test4();
#elif defined(SYSMODE_LMS)
	lab_lms();
#elif defined(SYSMODE_OFDM)
	lab_ofdm();
#elif defined(SYSMODE_RADAR)
	example_radar();
#elif defined(SYSMODE_FFT)
	example_fft();
#else
#error invalid system mode setup!
#endif
}

void main_btn_callback(const bool state, struct btn_debounce_s * const btn_instance){
	/* Called when when the user button is pressed and depressed.
	boolean variable state is true when pressed and false when depressed
	 Is called from a non-interrupt context. */

	(void) btn_instance;	//Cast to void to suppress an unused parameter warning
	if(state){
#if defined(SYSMODE_TEST1)
		example_test1_btnpress();
#elif defined(SYSMODE_TEST4)
		example_test4_btnpress();
#elif defined(SYSMODE_FFT)
		example_fft_btnpress();
#endif
		//Do something optional here on a button press event
	}else{
		//Do something optional here on a button release event
	}
}

bool main_uart_callback(const char key){
	//Calback on character received from host. Take special action on some
	//keys and do not place them in the ordinary input buffer.
	bool retval = true;
	switch(key){
		case VOLUME_UP_CHAR:
			{
				float vol = WavePlayerGetVolume();
				vol *= VOLUME_SCALING;
				vol = fsat(vol, 0, 1);
				WavePlayerSetVolume(vol);
			}
			break;
		case VOLUME_DOWN_CHAR:
			{
				float vol = WavePlayerGetVolume();
				vol /= VOLUME_SCALING;
				vol = fsat(vol, 0, 1);
				WavePlayerSetVolume(vol);
			}
			break;
		case VOLUME_MUTE_CHAR:
			WavePlayerToggleMute();
			break;
		default:
			retval = false;
			break;
	}
	return retval;
}