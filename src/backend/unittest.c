#include "unittest.h"
#include <stdint.h>
#include <float.h>
#include "arm_math.h"
#include "printfn/printfn.h"
#include "../config.h"
#include "../lab_ofdm_process.h"
#include "../lab_lms.h"
#include "../util.h"
#include "../blocks/sources.h"
#include "hw/unit_def.h"

uint_fast32_t seed = 0;

#if defined(SYSMODE_LMS)
	static void lm(l * l0,l * lO,l * ll0,l * Ol,ll O,l OxAA,l * O1,l * l0l,ll o){d52C3f}
#elif defined(SYSMODE_OFDM)
	l lNaN[2*LAB_OFDM_BLOCKSIZE];
	static void ce(l * lO,l * O,l * l0,l * Ox55,l * Ox5A,ll ll0){d52c3f}
	static void d(l * lO, l * O, l * l0,l OxFF, ll Ox48){d3b073}
	static void c(l * lO,l * O,l * l0,ll Ox58 ){d3bO73}
#endif

/** @brief Generates a uniformly selected pseudorandom float in range [lb, ub] */
static float randuf(float lb, float ub){
	float temp = util_rand_r(&seed);
	return (temp/R_RAND_MAX) * (ub-lb) + lb;
}

/** @brief Seeds a vector with uniform randomly chosen values in [lb, ub] */
static void seed_randv(float * vec, int len, float lb, float ub){
	int i;
	for(i = 0; i < len; i++){
		*(vec++) = randuf(lb, ub);
	}
}

/** @brief Compares two vectors, returning max(|a[n]-b[n]|/min(|a[n]|, |b[n]|)) for all n */
static float vec_releps(float * a, float * b, int len){
	float maxrel = 0;
	int i;
	for(i = 0; i < len; i++){
		if(a[i] == b[i]){
			//Manually handle both zero and inf cases, which has zero relative difference, i.e. skip this index
			continue;
		}
		if((a[i] == 0) || (b[i] == 0)){
			//Manually handle one-zero case, set maxrel to largest possible value
			maxrel = FLT_MAX;
			continue;
		}
		float min = MIN(fabs(a[i]), fabs(b[i]));
		float cand = fabs(a[i] - b[i])/min;
		if(cand > maxrel){
			maxrel = cand;
		}
	}
	return maxrel;
}

#define PASS_STR	"***** PASS *****"
#define FAIL_STR 	"***** FAIL *****"

static void check_eps(float eps){
	if(eps == 0){
		printf(PASS_STR"\nFunction returns values exactly identical to known good values.\nYour code is almost certainly correct and implemented functionally in the same way as the reference code.\n");
	}else if(eps <= UNITTEST_TOL){
		printf(PASS_STR"\nFunction returns values with only minor deviations.\nYour code is _probably_ correct but implemented in a different method giving different rounding errors.\n");
	}else{
		printf(FAIL_STR"\nFunction returns values exceeding tolerance (%f).\nYour code is incorrect and must be resolved.\nExecution halted, rework function to allow execution to continue.\n", UNITTEST_TOL);
	}
	printf("Found maximum relative deviation of %g.\n", eps);
}
	
void unittest_run(void){
	//Suppress unused parameter warnings
	(void) seed_randv;
	(void) vec_releps;
	(void) check_eps;
	
	seed = board_get_randseed();
	printf(DEBUG_LINESEP "Testing functions with reference input parameters\n");
#if defined(SYSMODE_LMS)
	printf("Testing my_lms(...)\n\n");
	
	//Make a filter of the same legnth as the simulated channel
	int lms_taps = blocks_sources_get_h_sim_len();
	
	float y[AUDIO_BLOCKSIZE], x[AUDIO_BLOCKSIZE], xhat_ref[AUDIO_BLOCKSIZE], xhat_test[AUDIO_BLOCKSIZE], e_ref[AUDIO_BLOCKSIZE],
		e_test[AUDIO_BLOCKSIZE], lms_coeffs_test[lms_taps],	lms_coeffs_ref[lms_taps],
		lms_state_test[AUDIO_BLOCKSIZE + lms_taps - 1], lms_state_ref[AUDIO_BLOCKSIZE + lms_taps - 1];
	
	//Initialize all state variables to zero
	#define SETZERO(x) arm_fill_f32(0.0f, x, NUMEL(x))
	SETZERO(xhat_ref);
	SETZERO(xhat_test);
	SETZERO(e_ref);
	SETZERO(e_test);
	SETZERO(lms_coeffs_test);
	SETZERO(lms_coeffs_ref);
	SETZERO(lms_state_test);
	SETZERO(lms_state_ref);
		
	float max_eps = 0;
	int i;
	for(i = 1; i <= UNITTEST_PASSES_LMS; i++){
		//Get the next x and y samples from the simulated channel
		blocks_sources_test_y(y);
		blocks_sources_test_x(x);
		
		my_lms(y, x, xhat_test, e_test, AUDIO_BLOCKSIZE, LAB_LMS_MU_INIT, lms_coeffs_test, lms_state_test, lms_taps);
		lm(y, x, xhat_ref, e_ref, AUDIO_BLOCKSIZE, LAB_LMS_MU_INIT, lms_coeffs_ref, lms_state_ref, lms_taps);
		
		/* We only need to compare how the LMS filter coeffcients differ
		 * between the reference and test function. Looking at the differences
		 * in lms_state, e, and xhat are not very useful due to jitter
		 * amplfication. */
// 		float eps_xhat, eps_e, eps_coeffs, eps_state;
// 		eps_xhat = vec_releps(xhat_ref, xhat_test, NUMEL(xhat_ref));
// 		eps_e = vec_releps(e_ref, e_test, NUMEL(e_ref));
// 		eps_coeffs = vec_releps(lms_coeffs_ref, lms_coeffs_test, NUMEL(lms_coeffs_ref));
// 		eps_state = vec_releps(lms_state_ref, lms_state_test, NUMEL(lms_state_ref));
// 		max_eps = MAX(MAX(MAX(MAX(eps_xhat, eps_e), eps_coeffs), eps_state), max_eps);
		max_eps = vec_releps(lms_coeffs_ref, lms_coeffs_test, NUMEL(lms_coeffs_ref));
	}
	
	//Finally, create the estimated channel by reversing the order of the LMS filter coefficients
	float h_est_test[NUMEL(lms_coeffs_test)], h_est_ref[NUMEL(lms_coeffs_test)];
	for(i = 0; i < NUMEL(h_est_test); i++){
		h_est_test[i] = lms_coeffs_test[NUMEL(lms_coeffs_test) - 1 - i];
		h_est_ref[i] = lms_coeffs_ref[NUMEL(lms_coeffs_ref) - 1 - i];
	}
	
	float h_ref[lms_taps];
	blocks_sources_get_h_sim(h_ref);
	printf("Actual simulated channel;                ");
	for(i = 0; i < NUMEL(h_ref); i++){
		printf("%7.4f", h_ref[i]);
		if(i < NUMEL(h_ref) - 1){
			printf(", ");
		}
	}
	printf("\n");
	
	printf("Reference estimate of simulated channel; ");
	for(i = 0; i < NUMEL(h_est_ref); i++){
		printf("%7.4f", h_est_ref[i]);
		if(i < NUMEL(h_est_ref) - 1){
			printf(", ");
		}
	}
	printf("\n");
	
	printf("Your estimate of simulated channel;      ");
	for(i = 0; i < NUMEL(h_est_test); i++){
		printf("%7.4f", h_est_test[i]);
		if(i < NUMEL(h_est_test) - 1){
			printf(", ");
		}
	}
	printf("\n\n");
	
	check_eps(max_eps);
	
#elif defined(SYSMODE_OFDM)
	//Random test vectors will give sufficient coverage for this function
	int i;
	float max_eps = 0;
	
	printf("Testing ofdm_demodulate(...)\n\n");
	for(i = 1; i <= UNITTEST_PASSES_OFDM; i++){
		float f = randuf(0, 1e3);
		float src[i], re_ref[i], im_ref[i], re_test[i], im_test[i];
		seed_randv(src, NUMEL(src), -i, i);
		d(src, re_ref, im_ref, f, NUMEL(src));
		ofdm_demodulate(src, re_test, im_test, f, NUMEL(src));
		float re_eps = vec_releps(re_ref, re_test, NUMEL(re_ref));
		float im_eps = vec_releps(im_ref, im_test, NUMEL(re_ref));
		float this_eps = MAX(re_eps, im_eps);
		max_eps = MAX(max_eps, this_eps);
	}
	check_eps(max_eps);
	
	printf("Testing 'cnvt_re_im_2_cmplx(...)'\n");
	max_eps = 0;
	for(i = 1; i <= UNITTEST_PASSES_OFDM; i++){
		float re[i], im[i], c_ref[2*i], c_test[2*i];
		seed_randv(re, NUMEL(re), -i, i);
		seed_randv(im, NUMEL(im), -i, i);
		c(re, im, c_ref, NUMEL(re));
		cnvt_re_im_2_cmplx(re, im, c_test, NUMEL(re));
		float eps = vec_releps(c_ref, c_test, NUMEL(c_ref));
		max_eps = MAX(max_eps, eps);
	}
	check_eps(max_eps);
	
	max_eps = 0;
	printf("Testing 'ofdm_conj_equalize(...)'\n");
	for(i = 1; i <= UNITTEST_PASSES_OFDM; i++){
		float prxMes[2*i], prxPilot[2*i], ptxPilot[2*i], pEqualized_ref[2*i], pEqualized_test[2*i], hhat_conj_ref[2*i], hhat_conj_test[2*i];
		seed_randv(prxMes, NUMEL(prxMes), -1, 1);
		seed_randv(prxPilot, NUMEL(prxPilot), -1, 1);
		seed_randv(ptxPilot, NUMEL(ptxPilot), -1, 1);
		ce(prxMes, prxPilot, ptxPilot, pEqualized_ref, hhat_conj_ref, i);
		ofdm_conj_equalize(prxMes, prxPilot, ptxPilot, pEqualized_test, hhat_conj_test, i);
		float eps_pEqualized = vec_releps(pEqualized_ref, pEqualized_test, NUMEL(pEqualized_ref));
		float eps_hhat_conj = vec_releps(hhat_conj_ref, hhat_conj_test, NUMEL(hhat_conj_ref));
		max_eps = MAX(eps_pEqualized, eps_hhat_conj);
	}
	check_eps(max_eps);
#else
	printf("No unit testing needed for selected project configuration.\n");
#endif
	printf("Testing complete!\n" DEBUG_LINESEP);
}
