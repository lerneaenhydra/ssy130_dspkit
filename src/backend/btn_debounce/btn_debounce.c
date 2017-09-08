#include "btn_debounce.h"
#include <stddef.h>

void btn_debounce_init(struct btn_debounce_s * const btn){
	BTN_DEBOUNCE_ATOMIC(										\
		btn->btn_state = btn_debounce_state_undefined;			\
		btn->integrator = btn->int_top;							\
	);
}

void btn_debounce_idle(struct btn_debounce_s * const btn, const bool rawstate){
	bool newstate = false;
	bool newstate_val;
	
	BTN_DEBOUNCE_ATOMIC(												\
		if(rawstate){													\
			if(btn->integrator < btn->int_top){							\
				btn->integrator++;										\
				if(btn->integrator == btn->int_top){					\
					if(btn->btn_state == btn_debounce_state_false){		\
						newstate = true;								\
						newstate_val = true;							\
					}													\
					btn->btn_state = btn_debounce_state_true;			\
				}														\
			}															\
		}else{															\
			if(btn->integrator){										\
				btn->integrator--;										\
				if(btn->integrator == 0){								\
					if(btn->btn_state == btn_debounce_state_true){		\
						newstate = true;								\
						newstate_val = false;							\
					}													\
					btn->btn_state = btn_debounce_state_false;			\
				}														\
			}															\
		}																\
	);
	
	if(btn->new_state_fun != NULL && newstate){
		btn->new_state_fun(newstate_val, btn);
	}
}

enum btn_debounce_state_e btn_debounce_get_state(struct btn_debounce_s * const btn){
	enum btn_debounce_state_e retval;
	BTN_DEBOUNCE_ATOMIC(retval = btn->btn_state);
	return retval;
}
