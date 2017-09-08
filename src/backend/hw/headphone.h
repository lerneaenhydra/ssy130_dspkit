/**
 * @file         headphone.h
 * @version      1.0
 * @date         2015
 * @author       Christoph Lauer
 * @compiler     armcc
 * @copyright    Christoph Lauer engineering
 */
#ifndef __HEADPHONE_H
#define __HEADPHONE_H

#include <stdint.h>
#include "../../config.h"

/*
 * This function is called by the wav recorder (i.e. microphone sampler) when PCM samples from the
 * microphone are ready. Here we store them into the circular microphone data buffer and check for
 * possibly clipped values (which we use to flash the red LED as a warning). Note that we assume
 * that the data will be removed from the buffer in time, so we do not check the mic_tail pointer
 * for buffer overrun.
 */
void WaveRecorderCallback (int16_t *buffer, int num_samples);
void WavePlayBack();

/** @brief The oldest unread microphone data, filled before main_audio_callback is called */
extern float processed_micdata[AUDIO_BLOCKSIZE];

/** @brief Pointers to locations to write AUDIO_BLOCKSIZE samples to before main_audio_callback terminates */
extern float * WavePlayBackLeftData;
extern float * WavePlayBackRightData;

/** @brief Adds AUDIO_BLOCKSIZE samples of left and right data to the output.
 * To be called only in the main_audio_callback context (by the sink functions) */
void WavePlayerAddSamples(float * leftdata, float * rightdata);

/** @brief Sets a new volume scaling in range [0, 1] */
void WavePlayerSetVolume(float volume);

/** @brief Gets the current volume scaling */
float WavePlayerGetVolume(void);

/** @brief Toggles the current system mute state */
void WavePlayerToggleMute(void);

/*
 * Callbacks implementation:
 * the callbacks prototypes are defined in the stm324xg_eval_audio_codec.h file
 * and their implementation should be done in the user code if they are needed.
 * Below some examples of callback implementations.
 */
 
/*
 * @brief  Calculates the remaining file size and new position of the pointer.
 * Called when the previous DMA playback buffer is completed. Here we simply
 * start playing the other buffer and signal the main loop that it can refill
 * the buffer we just played.
 */
void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t Size);

/*
 * @brief  Manages the DMA Half Transfer complete interrupt.
 * Generally this interrupt routine is used to load the buffer when 
 * a streaming scheme is used: When first Half buffer is already transferred load 
 * the new data to the first half of buffer while DMA is transferring data from 
 * the second half. And when Transfer complete occurs, load the second half of 
 * the buffer while the DMA is transferring from the first half
 */
void EVAL_AUDIO_HalfTransfer_CallBack(uint32_t pBuffer, uint32_t Size);

void EVAL_AUDIO_Error_CallBack(void* pData);
uint16_t EVAL_AUDIO_GetSampleCallBack(void);

#endif
