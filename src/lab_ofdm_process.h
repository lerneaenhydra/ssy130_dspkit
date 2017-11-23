#ifndef LAB_OFDM_PROCESS_H_
#define LAB_OFDM_PROCESS_H_

#include "config.h"
#include <stdbool.h>

#if defined(SYSMODE_OFDM)

extern char message[];
extern char rec_message[];
extern char stat_message[];

#define LAB_OFDM_CYCLIC_PREFIX_SIZE 	(32) /* Complex */
#define LAB_OFDM_BLOCKSIZE 				(64) /* Complex. Note; must be aligned with FFT size! */
#define LAB_OFDM_UPSAMPLE_RATE 			(8)  /* Same as downsample rate */
#define LAB_OFDM_NUM_MESSAGE 			(2)
#define LAB_OFDM_CHAR_MESSAGE_SIZE 		(LAB_OFDM_BLOCKSIZE / 4)
#define LAB_OFDM_BLOCK_W_CP_SIZE   		(LAB_OFDM_CYCLIC_PREFIX_SIZE + LAB_OFDM_BLOCKSIZE) /* Complex */
#define LAB_OFDM_BB_FRAME_SIZE   		(LAB_OFDM_NUM_MESSAGE * LAB_OFDM_BLOCK_W_CP_SIZE) /* Complex */
#define LAB_OFDM_TX_FRAME_SIZE   		((LAB_OFDM_BB_FRAME_SIZE)*(LAB_OFDM_UPSAMPLE_RATE)) /* Complex */
#define LAB_OFDM_FFT_FLAG 				(0)
#define LAB_OFDM_IFFT_FLAG 				(1)
#define LAB_OFDM_DO_BITREVERSE 			(1)
#define LAB_OFDM_FILTER_LENGTH 			(64)
#define LAB_OFDM_CENTER_FREQUENCY 		(4000.0f)

void lab_ofdm_process_qpsk_encode(char * pMessage, float * pDst, int Mlen);
void lab_ofdm_process_qpsk_decode(float * pSrc, char * pMessage,  int Mlen);
void lab_ofdm_process_set_randpilot(bool randpilot_enbl);
void lab_ofdm_process_tx(float * tx_data);
void lab_ofdm_process_rx(float * rx_data);
void lab_ofdm_process_init(void);


/* Equalize channel by multiplying with the conjugate of the channel
* Essentially, we want to create an estimate of the channel using the
* transmitted (ptxPilot) and recieved (prxPilot) pilot signals.
*
* There are two things you need to do here;
* 1) Estimate the conjugate of the channel
* 2) Equalize the recieved message by multiplying with the previous results
*
* Note that we only need to estimate the phase of the channel!
* (We could, at higher computational cost, estimate the phase and magnitude of
* the channel by letting \hat{H[k]} = prxPilot[k]/ptxPilot[k], for
* k = [0,..,length-1]). We need to estimate the channel phase without using a
* (computationally expensive) divide operation.
* 
* INP:
*  prxMes[] - complex vector with received data message in frequency domain (FD)
*  prxPilot[] - complex vector with received pilot in FD
*  ptxPilot[] - complex vector with transmitted pilot in FD
*  length  - number of complex OFDM symbols
* OUT:
*  pEqualized[] - complex vector with equalized data message (Note: only phase
*  is equalized)
*  hhat_conj[] -  complex vector with estimated conjugated channel gain
*/
void ofdm_conj_equalize(float * prxMes, float * prxPilot, float * ptxPilot, float * pEqualized, float * hhat_conj, int length);

/*
* Converts a complex signal represented as pRe + sqrt(-1) * pIm into an
* interleaved real-valued vector of size 2*length
* I.e. pCmplx = [pRe[0], pIm[0], pRe[1], pIm[1], ... , pRe[length-1], pIm[length-1]]
*/
void cnvt_re_im_2_cmplx(float * pRe, float * pIm, float * pCmplx, int length);

/*
* Demodulate a real signal (pSrc) into a complex signal (pRe and pPim)
* with modulation center frequency f and length 'length'.
*/
void ofdm_demodulate(float * pSrc, float * pRe, float * pIm, float f, int length);

#endif

#endif /* LAB_OFDM_PROCESS_H_ */
