/** @file Contains all low-level hardware initialization and access not directly related to the microphone and speaker. */

#ifndef SRC_HW_BOARD_H_
#define SRC_HW_BOARD_H_

#include <stdbool.h>
#include <stdint.h>

/** @brief Enumerated type for designating a given status LED */
enum board_led_e {board_led_red = 0, board_led_orange, board_led_green, board_led_blue};

/** @brief Returns the initial random seed generated during startup.
 * Will be different on each power cycle, but constant for the duration of a
 * given power cycle. */
uint32_t board_get_randseed(void);

/** @brief Initializes all constant system peripherals */
void board_init(void);

/** @brief Gets the current filtered button state. */
bool board_get_btn(void);

/** @brief Queues a character of data in the USART transmit buffer, if space is available */
void board_usart_write(char data);

/** @brief Gets a character of data from the USART recieve buffer, if any is pending. Returns true if a character was available. */
bool board_get_usart_char(char * data);

/** @brief Systick handler; called by system time timer interrupt */
void SysTick_Handler(void);

/** @brief Set a given led to a given state */
void board_set_led(enum board_led_e led, bool state);

/** @brief Toggles a given LED's state */
void board_toggle_led(enum board_led_e led);

#endif /* SRC_HW_BOARD_H_ */
