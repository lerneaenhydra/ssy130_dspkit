/** @file Contains all code to be written for the OFDM lab.
 * Each lab task has an associated pair of functions, one that is called during
 * the initialization phase, which allows for general initialization, and one
 * that is called every AUDIO_BLOCKSIZE samples that actually performs the
 * signal processing. */

#ifndef LAB_OFDM_H_
#define LAB_OFDM_H_

#include "config.h"

#if defined(SYSMODE_OFDM)

// Function pair for the OFDM lab assignment
void lab_ofdm_init(void);
void lab_ofdm(void);

#endif

#endif /* LAB_LMS_H_ */
