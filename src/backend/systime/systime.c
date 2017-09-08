#include "systime.h"
#include <limits.h>

static volatile systime_t systime = 0;

void systime_update(void){
	systime_t systime_shadow;
	ATOMIC(systime_shadow = systime);
	systime_shadow++;
	systime_shadow = systime_shadow%SYSTIME_T_MAX;
	ATOMIC(systime = systime_shadow);
}

systime_t systime_get(void){
	volatile systime_t retval;
	ATOMIC(retval = systime);
	return retval;
}

systime_t systime_get_delay(systime_t us){
	volatile systime_t systime_shadow;
	ATOMIC(systime_shadow =  systime);
	return ((systime_shadow + ((systime_t)(us / SYSTIME_PERIOD_us)) + 1)%SYSTIME_T_MAX);
}

systime_t systime_add_delay(systime_t us_added, systime_t old){
	return (old + ((systime_t) (us_added / SYSTIME_PERIOD_us)) + 1)%SYSTIME_T_MAX;
}

bool systime_get_delay_passed(systime_t systime_delay){
	bool retval = false;
	volatile systime_t systime_shadow;
	ATOMIC(systime_shadow  = systime);
	systime_t delta = systime_shadow - systime_delay;
	if(delta < (SYSTIME_T_MAX/2)){	//As systime_t is unsigned delta will progress as ... SYSTIME_T_MAX - 1, SYSTIME_T_MAX, 0, 1 ... when the delay decreases past zero
		retval = true;
	}
	return retval;
}
