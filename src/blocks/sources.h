/** @brief Implements all source blocks */

#ifndef SOURCES_H_
#define SOURCES_H_

/** @brief Startup initialization for all blocks */
void blocks_sources_init(void);

/** @brief Updates all internally-generated time-varying signals
 * To be called by the backend subsystem only. */
void blocks_sources_update(void);

/** @brief Writes the AUDIO_BLOCKSIZE oldest y/x samples to sample_block.
 * Here, the ..._x and ..._y functions are related by Y(s) = X(s)*H(s), where h(n)
 * can be requested using the ..._get_h_... functions.
 * @param sample_block Pointer to array of AUDIO_BLOCKSIZE floats to write
 * oldest disturbance/x/y samples to. */
void blocks_sources_test_y(float * sample_block);
void blocks_sources_test_x(float * sample_block);

/** @brief Gets the number of elements of the simulated channel */
int blocks_sources_get_h_sim_len(void);

/** @brief Copies the simulated channel coefficients to a target vector.
 * NOTE: Must be of length blocks_sources_get_h_sim_len() or greater! */
void blocks_sources_get_h_sim(float * h_sim);

/** @brief Gets the AUDIO_BLOCKSIZE oldest unread microphone samples
 * @return Pointer to the AUDIO_BLOCKSIZE oldest unread microphone samples,
 * scaled to the range [-1, 1]. */
void blocks_sources_microphone(float * sample_block);

/** @brief Gets the AUDIO_BLOCKSIZE next samples from the stored waveform */
void blocks_sources_waveform(float * sample_block);

/** @brief Sets the desired frequency for the sine/cosine sources */
void blocks_sources_trig_setfreq(float frequency);

/** @brief Gets the AUDIO_BLOCKSIZE next samples for a sine of set frequency
 * Will default to a 1kHz signal if no frequency has been set. */
void blocks_sources_sin(float * sample_block);

/** @brief Gets the AUDIO_BLOCKSIZE next samples for a cosine of set frequency
 * Will default to a 1kHz signal if no frequency has been set. */
void blocks_sources_cos(float * sample_block);

/** @brief Gets a blocks of AUDIO_BLOCKSIZE zeros */
void blocks_sources_zeros(float * sample_block);

/** @brief Gets a blocks of AUDIO_BLOCKSIZE ones */
void blocks_sources_ones(float * sample_block);

#endif /* SOURCES_H_ */
