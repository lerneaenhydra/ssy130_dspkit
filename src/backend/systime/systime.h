/*
 * systime.h
 *
 *  Created on: Oct 30, 2013
 *      Author: desel-lock
 */

#ifndef SYSTIME_H_
#define SYSTIME_H_

#include <stdbool.h>
#include <limits.h>
#include <stdint.h>

/** @brief Define to the calling frequency of the ISR that calls systime_update(), expressed in microseconds.
 * WARNING; Must be no faster than 500kHz (2us)! */
#include "../../config.h"
#define SYSTIME_PERIOD_us BOARD_SYSTICK_FREQ_Hz

/** @brief Define to something that ensures code_ is executed atomically */
#include "../../util.h"
#ifndef ATOMIC
#define ATOMIC(code_) ATOMIC_BLOCK(ATOMIC_RESTORESTATE){code_;}
#endif

/** @brief Helper macro to convert milliseconds to microseconds */
#define MS2US(x)	((systime_t)((x*1ULL)*1000))

/** @brief Helper macro to convert seconds to microseconds */
#define S2US(x)		((systime_t)((MS2US(x)*1ULL)*1000))

/** @brief Systime type. Must be an unsigned integer type.
 * Timing values exactly correspond to microseconds, IE. systime_t foo = 100
 * corresponds to a time of 100us. Must be large enough to store the length
 * of times that should be able to be represented.
 * Maximum representable values vs. bit length;
 * 8-bit: 	255 us
 * 16-bit:	~65.535 ms
 * 32-bit:	~71.58 minutes
 *
 * NOTE: BE SURE TO UPDATE THE SYSTIME_T_MAX DEFINE IF CHANGING THIS PARAMETER!
 */
typedef uint32_t systime_t;
/** @brief *Must* be defined to the maximum value representable by systime_t */
#define SYSTIME_T_MAX UINT32_MAX

/** @brief Fundamental systime updating function.
 * Systime will increment by one per call. Should be called at a fixed period.
 * WARNING; Must be called no faster than 500kHz (2us)!
 */
void systime_update(void);

/** @brief The current system time.
 *	Note; will overflow when exceeding systime_t. */
systime_t systime_get(void);

/** @brief Returns the value that systime_get will return in us microseconds from now.
 * 	Note that us is far more granular than the speed systime_get is incremented.
 *  The returned value is the ceiling of the desired time relative to the
 *  achievable delay, ie the real delay will be *at least* us microseconds, and
 *  *at most* us + SYSTIME_PERIOD_us microseconds. Multiples of
 *  SYSTIME_PERIOD_us will be perfectly matched, so use these values where
 *  accuracy is important.
 *  As SYSTIME_PERIOD_us > 2 us can never overflow, IE all values of us will
 *  give valid delay values */
systime_t systime_get_delay(systime_t us);

/** @brief Adds a delay to an extisting delay.
 * See systime_get_delay for how to get the initial delay.
 * @param us_added	The number of additional microseconds to delay
 * @param old		The previous delay value.
 */
systime_t systime_add_delay(systime_t us_added, systime_t old);

/** @brief Returns nonzero if the current system time has passed systime_delay
 * for no more than SYSTIME_T_MAX/2 systicks ago. This allows for scheduling delays
 * up to SYSTIME_T_MAX/2 systicks in the future. */
bool systime_get_delay_passed(systime_t systime_delay);

#endif /* SYSTIME_H_ */
