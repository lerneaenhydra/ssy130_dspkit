/** @file Contains all code to be written for the LMS filter lab.
 * Each lab task has an associated pair of functions, one that is called during
 * the initialization phase, which allows for general initialization, and one
 * that is called every AUDIO_BLOCKSIZE samples that actually performs the
 * signal processing. */

#ifndef LAB_LMS_H_
#define LAB_LMS_H_

#define 	LAB_LMS_MU_INIT			(1.0e-3f)
#define		LAB_LMS_SINE_TONE_HZ	(440)

// Function pair for the LMS lab assignment
void lab_lms_init(void);
void lab_lms(void);

//Process one LMS iteration
void my_lms(float * y, float * x, float * xhat, float * e, int blockSize,
			float lms_mu, float * lms_coeffs, float * lms_state, int lms_taps);

#endif /* LAB_LMS_H_ */
