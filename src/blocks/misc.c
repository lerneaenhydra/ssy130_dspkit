#include "misc.h"
#include <math.h>
#include "config.h"
#include "arm_math.h"

void misc_envelope_init(struct misc_envelope_s * const s,
		const float amb_fc,
		const float inpmax,
		const float thrs,
		const float fillval,
		const int_fast32_t sig_offset,
		const int_fast32_t tot_siglen,
		float * const outdata,
		void (* const trigfun_start)(void),
		void (* const trigfun_end)(void)){
	/* https://en.wikipedia.org/wiki/Low-pass_filter#Discrete-time_realization
	 * gives the analytic solution for a first-order IIR filter as;
	 * y[n+1] = x[n] * k + y[n] * [1-k] where
	 * k = 2 * pi * (f_c / f_s) / (2 * pi * (f_c/f_s) + 1), where f_c is the
	 * desired cutoff frequency and f_s is the sample frequency. */

	int_fast32_t net_offset = sig_offset;
	if(net_offset < -(tot_siglen-1)){
		net_offset = -(tot_siglen-1);
	}

	s->filt_k = M_TWOPI * (amb_fc / (1.0f * AUDIO_SAMPLE_RATE)) / (M_TWOPI * (amb_fc / (1.0f * AUDIO_SAMPLE_RATE)) + 1);
	s->tot_siglen = tot_siglen;
	s->sig_offset = net_offset;
	s->trigd = false;

	/** If we're going to save data from before the trigger set up the buffer
	 * as if we've already received sig_offset samples. If we wish to delay the
	 * start of data collection, set the processed signal length to a negative
	 * amount and use this to count the number of samples we want to skip. */
	s->processed_siglen = -net_offset;

	s->outdata = outdata;
	s->filtmem = inpmax * inpmax;
	s->thrs = thrs;
	s->trigfun_start = trigfun_start;
	s->trigfun_end = trigfun_end;

	arm_fill_f32(fillval, outdata, tot_siglen);
}

void misc_envelope_process(struct misc_envelope_s * const s, float * const inp, bool trig_enbl){
	int_fast32_t i;
	const float filt_k = s->filt_k;
	for(i = 0; i < AUDIO_BLOCKSIZE; i++){
		/* First, apply the ambient noise filter to the input. We will use the
		 * raw input squared as the input to both the ambient noise filter and
		 * for determining when we have detected a signal. */
		const float inp2 = inp[i] * inp[i];

		s->filtmem = filt_k * inp2 + (1 - filt_k) * s->filtmem;

		/* If we're set up to generate a negative sample offset (IE. store
		 * data before trigger), update the buffer contents */
		if(s->sig_offset < 0 && s->trigd == false){
			arm_copy_f32(&s->outdata[1], &s->outdata[0], -s->sig_offset);
			s->outdata[-s->sig_offset] = inp[i];
		}

		/* Now, check if the current sample exceeds the filtered signal by the
		 * required threshold and if so set the triggered flag */
		if(inp2 > s->thrs * s->filtmem && s->processed_siglen == -s->sig_offset){
			if(s->trigfun_start != NULL){
				s->trigfun_start();
			}
			s->trigd = true;
		}

		/* If triggered, process data */
		if(s->trigd){
			if(s->processed_siglen < 0){
				s->processed_siglen++;
			}else if(s->processed_siglen != s->tot_siglen){
				s->outdata[s->processed_siglen++] = inp[i];
			}

			if(s->processed_siglen == s->tot_siglen && s->trigfun_end != NULL){
				s->trigfun_end();
			}
		}
	}
}

bool misc_envelope_query_complete(struct misc_envelope_s * const s){
	return s->processed_siglen == s->tot_siglen;
}

void misc_envelope_ack_complete(struct misc_envelope_s * const s){
	s->processed_siglen = -s->sig_offset;
	s->trigd = false;
}

void misc_inpbuf_add(float * const buf, const int_fast32_t buflen, float * const inp, const int_fast32_t inplen){
	if(inplen >= buflen){
		//Input length at least as long as buffer, simply fill entire buffer with input
		//(though there's no real reason to use this utility function)
		arm_copy_f32(inp, buf, buflen);
	}else{
		//Buffer will contain at least one old sample
		
		//First handle samples of buf that are not an even multiple of inplen
		const int_fast32_t rem = buflen % inplen;
		if(rem != 0){
			const size_t start = rem + inplen - rem;
			const size_t end = 0;
			//printf("REM: Copying %d bytes starting from index %d to index %d. Total buffer length %d.\n", rem, start, end, buflen);
			arm_copy_f32(&buf[start], &buf[end], rem);	
		}
		
		//Now handle all remaining samples. Guaranteed to be a multiple of inplen
		int_fast32_t i;
		for(i = rem + inplen; i <= buflen - inplen; i+=inplen){
			const size_t start = i;
			const size_t end = i - inplen;
			//printf("STD: Copying %d bytes starting from index %d to index %d. Total buffer length %d.\n", inplen, start, end, buflen);
			arm_copy_f32(&buf[start], &buf[end], inplen);
		}
		
		//Now we can simply copy inp to the start of buf
		const size_t start = (buflen - 1) - (inplen - 1);
		arm_copy_f32(inp, &buf[start], inplen);
		//printf("NEW: Adding %d bytes starting from index %d. Total buffer length %d.\n", inplen, start, buflen);
	}
}

void misc_queuedbuf_init(struct misc_queuedbuf_s * const s, float * const buf, const int_fast32_t len){
	s->curr_idx = 0;
	s->len = len;
	s->data = buf;
}

void misc_queuedbuf_process(struct misc_queuedbuf_s * const s, float * out, const int_fast32_t outlen, const float padval){
	// Copy from the queued buffer to the target

	const int_fast32_t queued_elems_left = s->len - s->curr_idx;
	int_fast32_t queued_elems_copy = MIN(queued_elems_left, outlen);
	arm_copy_f32(&s->data[s->curr_idx], out, queued_elems_copy);
	
	//printf("%d elems left. %d elems to copy\n", queued_elems_left, queued_elems_copy);

	/* We've now copied up to outlen elements from the queued buffer, update
	 * the number of remaining elements and possibly pad with padval. */

	s->curr_idx += queued_elems_copy;

	const int_fast32_t pad_elems = outlen - queued_elems_copy;
	
	//printf("%d curr idx, %d elems to pad\n\n", s->curr_idx, pad_elems);

	if(pad_elems > 0){
		arm_fill_f32(padval, &out[queued_elems_left], pad_elems);
	}
}
