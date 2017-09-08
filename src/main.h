/*
 * main.h
 *
 *  Created on: Jun 14, 2016
 *      Author: stm32dev
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdint.h>
#include <stdbool.h>

#include "backend/btn_debounce/btn_debounce.h"

/** @brief Generic idle process that is run when not in interrupt context */
void main_idle(void);

/** @brief Called when AUDIO_BLOCKSIZE elements are required for the outputs.
 * When called AUDIO_BLOCKSIZE samples must be supplied to the
 * blocks_sinks_leftout and blocks_sinks_rightout functions. */
void main_audio_callback(void);

/** @brief Called on a button state change */
void main_btn_callback(const bool state, struct btn_debounce_s * const btn_instance);

/** @brief Called on UART character received
 * @return true if the key was "absorbed" (acted on) globally and should not
 * be sent to the general UART receive buffer */
bool main_uart_callback(const char key);

#endif /* MAIN_H_ */
