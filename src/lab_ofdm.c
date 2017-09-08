#include "lab_ofdm.h"
#include <stdbool.h>
#include "lab_ofdm_process.h"
#include "backend/arm_math.h"
#include "blocks/sources.h"
#include "blocks/sinks.h"
#include "blocks/misc.h"
#include "util.h"
#include "config.h"
#include "arm_math.h"
#include "backend/printfn/printfn.h"
#include "backend/systime/systime.h"

#if defined(SYSMODE_OFDM)

extern float volume; // declaration (it is defined elsewhere)

systime_t tx_timer = 0;
float tx_data[LAB_OFDM_TX_FRAME_SIZE];	//data to send
float envelope_data[10000];	//Stored data from envelope detection
struct misc_envelope_s env_s;
struct misc_queuedbuf_s queue_s;

systime_t block_timer = 0;
bool trig_enbl = true;
int sig_offset = 0;

static void lab_ofdm_trigstart_fun(void){
	board_set_led(board_led_blue, true);
	printf("Signal envelope detected!\n");
}

static void lab_ofdm_trigend_fun(void){
	board_set_led(board_led_blue, false);
}

void lab_ofdm_init(void){
	misc_envelope_init(&env_s, 1.0f, 1.0f, 100.0f, 0.0f, sig_offset, NUMEL(envelope_data), envelope_data, lab_ofdm_trigstart_fun, lab_ofdm_trigend_fun);
	misc_queuedbuf_init(&queue_s, envelope_data, NUMEL(envelope_data));
	lab_ofdm_process_init();
}

void lab_ofdm(void){
	float inp[AUDIO_BLOCKSIZE];
	blocks_sources_microphone(inp);

	if(systime_get_delay_passed(block_timer) && trig_enbl == false){
		trig_enbl = true;
//		printf("Envelope detection blanking period elapsed.\n");
	}

	misc_envelope_process(&env_s, inp, trig_enbl);

	if(misc_envelope_query_complete(&env_s)){ // True if a buffer is ready to process
		misc_envelope_ack_complete(&env_s);
		block_timer = systime_get_delay(1.5 * S2US(1.0f * NUMEL(envelope_data) / AUDIO_SAMPLE_RATE ));
		trig_enbl = false;
		lab_ofdm_process_rx(envelope_data); // Process data
	}

	char key;
	if(board_get_usart_char(&key)){ // Check if a key is pressed
		switch(key){
		default:
			printf("Invalid key pressed.\n");
			break;
		case '+':
			volume *= 2;
			printf("Increasing volume to %f \n",volume);
			break;
		case '-':
			volume /= 2;
			printf("Decreasing volume to %f \n",volume);
			break;
		case 'a':
			sig_offset += 10;
			printf("Sample offset adjusted to later  %d \n",env_s.sig_offset+10);
			lab_ofdm_init();
			break;
		case 'b':
			sig_offset -= 10;
			printf("Sample offset adjusted to earlier  %d \n",env_s.sig_offset-10);
			lab_ofdm_init();
			break;
		}
	}

	if(systime_get_delay_passed(tx_timer)){
		tx_timer = systime_get_delay(S2US(2));
		//is now time to generate data, write to tx_data
		lab_ofdm_process_tx(tx_data); // Create OFDM frame to send
		misc_queuedbuf_init(&queue_s, tx_data, NUMEL(tx_data)); // Add to queue
	}
	float out[AUDIO_BLOCKSIZE];
	misc_queuedbuf_process(&queue_s, out, NUMEL(out), 0.0f);
	blocks_sinks_leftout(out);
	blocks_sinks_rightout(out);
}

#endif
