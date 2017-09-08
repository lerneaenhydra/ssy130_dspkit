/** @brief Implements all sink blocks */

#ifndef SINKS_H_
#define SINKS_H_

/** @brief Copy AUDIO_BLOCKSIZE samples of data to the left output buffer */
void blocks_sinks_leftout(float * data);

/** @brief Copy AUDIO_BLOCKSIZE samples of data to the right output buffer */
void blocks_sinks_rightout(float * data);

/** @brief Returns a pointer to the buffer for the left output. */
float * blocks_sinks_leftout_ptr(void);
/** @brief Returns a pointer to the buffer for the right output */
float * blocks_sinks_rightout_ptr(void);

#endif /* SINKS_H_ */
