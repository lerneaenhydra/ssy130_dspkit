#include "cbuf.h"
#include <stddef.h>
#include "macro.h"

void cbuf_new(cbuf_s * const cb, cbuf_elem_t * const pool, const cbuf_idx_t elems){
	if(pool == NULL || cb == NULL || elems <= 0){
		return;
	}
	cb->buf = pool;

	// Initialize cbuf size and pointers
	cb->elems = elems;
	cb->head = 0;
	cb->tail = 0;
}

cbuf_idx_t cbuf_elems_used(cbuf_s * const cb){
	cbuf_idx_t elems_used = 0;
	if(cb == NULL){
		return elems_used;
	}

	ATOMIC(elems_used = (cb->head - cb->tail + cb->elems) % cb->elems);

	return elems_used;
}

cbuf_idx_t cbuf_elems_free(cbuf_s * const cb){
	cbuf_idx_t elems_free = 0;
	if(cb == NULL){
		return elems_free;
	}

	ATOMIC(elems_free = cb->elems - ((cb->head - cb->tail + cb->elems) % cb->elems) - 1);

	return elems_free;
}

void cbuf_flush(cbuf_s * const cb){
	if(cb == NULL){
		return;
	}
	ATOMIC(				\
		cb->head = 0; 	\
		cb->tail = 0; 	\
	);
}

bool cbuf_write(cbuf_s * const cb, cbuf_elem_t const data){
	bool retval = false;
	if(cb == NULL){
		return retval;
	}

	// If buffer is not full, write data to it
	ATOMIC(												\
		if((cb->head + 1) % cb->elems != cb->tail){		\
			cb->buf[cb->head] = data;					\
			cb->head = (cb->head + 1) % cb->elems;		\
			retval = true;								\
		}												\
	);

	return retval;
}

bool cbuf_read(cbuf_s * const cb, cbuf_elem_t * const data){
	bool retval = false;
	if(cb == NULL || data == NULL){
		return retval;
	}

	//If buffer does not contain any data, do not attempt a read
	ATOMIC(												\
		if(cb->head != cb->tail){						\
			*data = cb->buf[cb->tail];					\
			cb->tail = (cb->tail + 1) % cb->elems;		\
			retval = true;								\
		}												\
	);

	return retval;
}

bool cbuf_peek(cbuf_s * const cb, cbuf_elem_t * const data){
	bool retval = false;
	if(cb == NULL || data == NULL){
		return retval;
	}

	//If buffer does not contain any data, do not attempt a read
	ATOMIC(												\
		if(cb->head != cb->tail){						\
			*data = cb->buf[cb->tail];					\
			retval = true;								\
		}												\
	);

	return retval;
}

