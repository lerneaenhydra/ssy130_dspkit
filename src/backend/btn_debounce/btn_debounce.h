/** @file Simple button debounce handler.
 * Allows for getting the current button state as well as defining a callback
 * function that is called when a new button state is detected.
 * The debouncing algorithm uses a saturated integrator to detect the button
 * state. When the idle function is called with a true input the integrator is
 * incremented, while a false input will decrement the integrator. If the
 * integrator reaches a defined top value while the stored button state is
 * false this is regarded as a button press event and the stored button state
 * will be set to true and an optional function pointer will be called.
 * Similarly, should the stored button state be true and the integrator reaches
 * zero this will be regarded as a button release event and the optional
 * function pointer will be called. The integrator is limited to lie in the
 * range of [0, int_top].
 * On initialization the button state is set to an undefined state, the
 * integrator is set to int_top/2 and when the button state stabilizes 
 * no function will be called.
 * Set the callback function pointer to NULL to disable calling a function on
 * a new debounced state.
 * Note that the debouncing time is set by the product of int_top and the idle
 * function call period. For example, with a 1 ms call period and a top value
 * of 50 this will give a minimum debounce time of 50 ms.
 * All functions are fully thread safe. Note that callback functions may not
 * safely directly access the elements of the instance structure. The pointer
 * to the button instance is solely intended to be used to identify which
 * button triggered the callback function in the event that many buttons share
 * a common callback function. */

#ifndef BTN_DEBOUNCE_H_
#define BTN_DEBOUNCE_H_

#include <stdbool.h>

/** @brief Define to a function that guarantees all function calls to the btn_debounce_xx functions will be atomic */
#include "macro.h"
#define BTN_DEBOUNCE_ATOMIC(x) ATOMIC(x)

/** @brief Storage type for a button debounce integrator */
typedef uint_fast32_t btn_debounce_integrator_t;

enum btn_debounce_state_e {btn_debounce_state_undefined = 0, btn_debounce_state_false, btn_debounce_state_true, BTN_DEBOUNCE_STATE_NUM};

/** @brief Memory storage element for a button debounce instance */
struct btn_debounce_s {
	enum btn_debounce_state_e btn_state;
	btn_debounce_integrator_t integrator;
	const btn_debounce_integrator_t int_top;
	void (* const new_state_fun)(const bool state, struct btn_debounce_s * const btn_instance);
};

/** @brief Initializes a button debounce object to an initial state
 * Will initialize the integrator to 50% and set the button state to the undefined state.
 * Be sure to set the int_top and new_state_fun parameters to their desired values at compile-time */
void btn_debounce_init(struct btn_debounce_s * const btn);

/** @brief Idle function to call at some constant period with the current button state */
void btn_debounce_idle(struct btn_debounce_s * const btn, const bool rawstate);

/** @brief Gets the current state of the a button instance */
enum btn_debounce_state_e btn_debounce_get_state(struct btn_debounce_s * const btn);

#endif /* BTN_DEBOUNCE_H_ */
