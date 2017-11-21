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


/*
*   Equalize the channel by multiplying with the conjugate of the channel
*  INP:
*   prxMes[] - complex vector with received data message in frequency domain (FD)
*   prxPilot[] - complex vector with received pilot in FD
*   ptxPilot[] - complex vector with transmitted pilot in FD
*   length  - number of complex OFDM symbols
*  OUT:
*   pEqualized[] - complex vector with equalized data message (Note: only phase
*   is equalized)
*   hhat_conj[] -  complex vector with estimated conjugated channel gain
*/
void ofdm_conj_equalize(float * prxMes, float * prxPilot, float * ptxPilot, float * pEqualized, float * hhat_conj, int length);

/*
* Converts a complex signal in the form of two vectors (pRe and pIm)
* into one float vector of size 2*length where the real and imaginary parts are
* interleaved. i.e [pCmplx[0]=pRe[0], pCmplx[1]=pIm[0],pCmplx[2]=pRe[1], pCmplx[3]=pIm[1]
* etc.
*/
void cnvt_re_im_2_cmplx(float * pRe, float * pIm, float * pCmplx, int length);

/*
* Demodulate a real signal (pSrc) into a complex signal (pRe and pPim)
* with modulation center frequency f and the signal length is length
*/
void ofdm_demodulate(float * pSrc, float * pRe, float * pIm, float f, int length);

#endif

#endif /* LAB_OFDM_PROCESS_H_ */
