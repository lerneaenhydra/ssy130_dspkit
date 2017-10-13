/* @file All miscellaneous blocks that do not belong to any of the other block categories */

#ifndef MISC_H_
#define MISC_H_

#include <stdint.h>
#include <stdbool.h>

/** @brief Memory element for envelope detection functions
 * The envelope detection functions allow for detecting the presence of a
 * signal with a sudden increase in squared amplitude. On detection the input
 * signal will be stored to a target buffer of arbitrary length.
 */
struct misc_envelope_s {
	int_fast32_t tot_siglen;		//!<- Length of destination buffer
	int_fast32_t sig_offset;		//!<- Number of samples to offset the final signal by relative to
									//!< the trigger. Negative values will store samples before trigger,
									//!< while positive values will store samples after the trigger.

	int_fast32_t processed_siglen;	//!<- Current number of elements written to destination buffer
	bool trigd;						//!<- If true, a trigger signal has been detected
	float * outdata;				//!<- Pointer to destination buffer
	float filt_k;					//!<- First order IIR filter coefficient
	float filtmem;					//!<- IIR filter memory element
	float thrs;						//!<- Threshold for signal detection
	void (*trigfun_start)(void);	//!<- Pointer to function to call on envelope detection
	void (*trigfun_end)(void);		//!<- Pointer to function to call when target buffer full
};

/** @brief Initializes an envelope detector
 * @param s 			The envelope detector to set up
 * @param amb_fc 		The ambient noise floor lowpass filter's cutoff frequency [Hz]
 * @param inpmax 		The maximum amplitude of the input signal
 * @param thrs			The detection threshold. A sample whose squared value exceeds the
 * 						product of the threshold and the squared filtered input will trigger
 * 						data collection.
 * @param fillval		Value to set all elements in outdata to on initialization.
 * @param sig_offset	Sample detection offset. If set to zero will start sampling immediately
 * 						on exceeding the trigger threshold. Negative values will keep this many
 * 						samples before the trigger and positive values will delay storing data
 * 						by this amount. Note that a naive implementation is used for storing
 * 						pre-threshold data, so execution time increases linearly with negative
 * 						values whose magnitude increases!
 * 						NOTE; Must be in range -tot_siglen < sig_offset < INT_FAST32_MAX!
 * @param tot_siglen	The number of samples to store when a signal is detected
 * @param outdata 		Pointer to float array to write detected signal to. Must be tot_siglen elements in length!
 * @param trigfun_start	Pointer to function to call when envelope detection triggered. Set to NULL to disable.
 * @param trigfun_end	Pointer to function to call when output buffer full. Set to NULL to disable. */
void misc_envelope_init(struct misc_envelope_s * const s,
		const float amb_fc,
		const float inpmax,
		const float thrs,
		const float fillval,
		const int_fast32_t sig_offset,
		const int_fast32_t tot_siglen,
		float * const outdata,
		void (* const trigfun_start)(void),
		void (* const trigfun_end)(void));

/** @brief Processes raw data, setting flags and filtering data as needed
 * @param s			The envelope detector to use
 * @param inp 		Pointer to AUDIO_BLOCKSIZE samples of data
 * @param trig_enbl	Set true to allow detection of a signal, false to only update the ambient noise filter */
void misc_envelope_process(struct misc_envelope_s * const s, float * const inp, bool trig_enbl);

/** @brief Returns true if an envelope structure s has detected and sampled a signal */
bool misc_envelope_query_complete(struct misc_envelope_s * const s);

/** @brief Resets the detection flag in an envelope structure s and allows detection of a new signal */
void misc_envelope_ack_complete(struct misc_envelope_s * const s);

/** @brief Adds data inp to a linear buffer buf, removing the oldest inp elements.
 * Note that data is stored in an order such that buf[0] will contain the oldest
 * data, while buf[inplen-1] will contain the newest. This implies that buf will
 * retain the order of data in inp. */
void misc_inpbuf_add(float * const buf, const int_fast32_t buflen, float * const inp, const int_fast32_t inplen);

/** @brief Memory element for queued buffer.
 * The queued buffer allows for easily outputting a block of samples that is
 * not equal to AUDIO_BLOCKSIZE. */
struct misc_queuedbuf_s {
	float * data;				//!<- Pointer to block of data
	int_fast32_t len;			//!<- Length of block of data
	int_fast32_t curr_idx;		//!<- Last sample from block of data that has been output
};

/** @brief Initializes/resets a given queued buffer
 * @param s		The buffer to initialize
 * @param buf	Pointer to buffer of data to queue
 * @param len	The length of the buffer */
void misc_queuedbuf_init(struct misc_queuedbuf_s * const s, float * const buf, const int_fast32_t len);

/** @brief Processes the current queued output.
 * Copies up to outlen elements from the queued output to out, padding with
 * padval should the queued output not be full.
 * @param s			The buffer to use
 * @param out		Pointer to block of data to copy contents of buffer to
 * @param outlen	Length of destination buffer
 * @param padval	Padding value to use should there be less than outlen elements stored in the buffer
 */
void misc_queuedbuf_process(struct misc_queuedbuf_s * const s, float * out, const int_fast32_t outlen, const float padval);

#endif /* MISC_H_ */
