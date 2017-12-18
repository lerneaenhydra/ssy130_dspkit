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

/** @brief Datatype for storing deviation */
struct eps_res_s {
	bool pass;
	float releps;
	float abseps;
};

/** @brief Displays data on maximum relative/absolute deviation */
static void print_epsmsg(struct eps_res_s * res);

/** @brief Manages the result of a given deviation result, returning false if test failed */
static bool handle_epsres(struct eps_res_s * res);

/** @brief Generates a uniformly selected pseudorandom float in range [lb, ub] */
static float randuf(float lb, float ub);

/** @brief Seeds a vector with uniform randomly chosen values in [lb, ub] */
static void seed_randv(float * vec, int len, float lb, float ub);

/** @brief Compares two vectors, returning max(|a[n]-b[n]|/min(|a[n]|, |b[n]|)) for all n */
static float vec_releps(float * a, float * b, int len);

/** @brief Compares two vectors, returning max(|a[n]-b[n]|) for all n */
static float vec_abseps(float * a, float * b, int len);

/** @brief Initializes an eps_res_s type */
static void init_epsres(struct eps_res_s * res);

/** @brief Compares two vectors, updating result in a eps_res_s */
static void update_epsres(float * a, float * b, int len, float rel_lim, float abs_lim, struct eps_res_s * res);

#if defined(SYSMODE_LMS)
	static void lm(l * l0,l * lO,l * ll0,l * Ol,ll O,l OxAA,l * O1,l * l0l,ll o){d52C3f}
#elif defined(SYSMODE_OFDM)
	l lNaN[2*LAB_OFDM_BLOCKSIZE];
	static void ce(l * lO,l * O,l * l0,l * Ox55,l * Ox5A,ll ll0){d52c3f}
	static void d(l * lO, l * O, l * l0,l OxFF, ll Ox48){d3b073}
	static void c(l * lO,l * O,l * l0,ll Ox58 ){d3bO73}
#endif

static float randuf(float lb, float ub){
	float temp = util_rand_r(&seed);
	return (temp/R_RAND_MAX) * (ub-lb) + lb;
}

static void seed_randv(float * vec, int len, float lb, float ub){
	int i;
	for(i = 0; i < len; i++){
		*(vec++) = randuf(lb, ub);
	}
}

static float vec_releps(float * a, float * b, int len){
	float maxrel = 0;
	int i;
	for(i = 0; i < len; i++){
		if(a[i] == b[i]){
			//Manually handle both zero and inf cases, which has zero relative difference, i.e. skip this index
			continue;
		}
		if((a[i] == 0) || (b[i] == 0) ||
		(a[i] != a[i] || b[i] != b[i]) ){
			//Manually handle one-zero or any-NaN case, set maxrel to largest possible value
			maxrel = FLT_MAX;
			continue;
		}
		float min = MIN(fabsf(a[i]), fabsf(b[i]));
		float cand = fabsf(a[i] - b[i])/min;
		if(cand > maxrel){
			maxrel = cand;
		}
	}
	return maxrel;
}

static float vec_abseps(float * a, float * b, int len){
	float maxabs = 0;
	int i;
	for(i = 0; i < len; i++){
		float cand = fabsf(a[i] - b[i]);

		//Handle NaN case manually
		if(cand != cand){
			maxabs = FLT_MAX;
		}

		if(cand > maxabs){
			maxabs = cand;
		}
	}
	return maxabs;
}

static void init_epsres(struct eps_res_s * res){
	res->abseps = 0.0f;
	res->releps = 0.0f;
	res->pass = true;
}

static void update_epsres(float * a, float * b, int len, float rel_lim, float abs_lim, struct eps_res_s * res){
	res->releps = MAX(res->releps, vec_releps(a, b, len));
	res->abseps = MAX(res->abseps, vec_abseps(a, b, len));
	if(res->releps > rel_lim && res->abseps > abs_lim){
		res->pass = false;
	}
	//Else leave pass flag unchanged, defaults to true
}

#define PASS_STR	"******************** PASS ********************"
#define FAIL_STR 	"******************** FAIL ********************"

static void print_epsmsg(struct eps_res_s * res){
	printf("Permissible maximum relative deviation: %e\n", UNITTEST_RELTOL);
	printf("       Your maximum relative deviation: %e\n", res->releps);
	printf("Permissible maximum absolute deviation: %e\n", UNITTEST_ABSTOL);
	printf("       Your maximum absolute deviation: %e\n", res->abseps);
	printf("Your function may exceed one, but not both, thresholds\n");
}

static bool handle_epsres(struct eps_res_s * res){
	bool retval = true;
	print_epsmsg(res);
	if(res->pass){
		if(res->releps == 0 && res->abseps == 0){
			printf(PASS_STR"\nFunction returns values exactly identical to known good values.\nYour code is almost certainly correct and implemented functionally in the same way as the reference code.\n");
		}else{
			printf(PASS_STR"\nFunction returns values minor deviations.\nYour code is _probably_ correct but implemented in a different method giving different rounding errors.\n");
		}	
	}else{
		printf(FAIL_STR"\nFunction returns values exceeding tolerance.\nYour code is incorrect and must be resolved.\nExecution halted, rework function to allow execution to continue.\n");
		retval = false;
	}
	return retval;
}
	
void unittest_run(void){
	//Suppress unused parameter warnings
	(void) seed_randv;
	(void) vec_releps;
	(void) handle_epsres;
	(void) update_epsres;
	(void) init_epsres;
	
	bool pass = true;

	seed = board_get_randseed();
	printf("Testing functions with reference input parameters\n");
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
		
	int i;
	for(i = 1; i <= UNITTEST_PASSES_LMS; i++){
		//Get the next x and y samples from the simulated channel
		blocks_sources_test_y(y);
		blocks_sources_test_x(x);
		
		my_lms(y, x, xhat_test, e_test, AUDIO_BLOCKSIZE, LAB_LMS_MU_INIT, lms_coeffs_test, lms_state_test, lms_taps);
		lm(y, x, xhat_ref, e_ref, AUDIO_BLOCKSIZE, LAB_LMS_MU_INIT, lms_coeffs_ref, lms_state_ref, lms_taps);
	}

	/* Compare all filter outputs:  lms_coeffs, xhat, e */

	struct eps_res_s res_h, res_x, res_e;
	init_epsres(&res_h);
	init_epsres(&res_x);
	init_epsres(&res_e);

	update_epsres(lms_coeffs_ref, lms_coeffs_test, NUMEL(lms_coeffs_ref), UNITTEST_RELTOL, UNITTEST_ABSTOL, &res_h);
	update_epsres(xhat_ref, xhat_test, NUMEL(xhat_ref), UNITTEST_RELTOL, UNITTEST_ABSTOL, &res_x);
	update_epsres(e_ref, e_test, NUMEL(e_ref), UNITTEST_RELTOL, UNITTEST_ABSTOL, &res_e);
	
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
		printf("%10.7f", h_ref[i]);
		if(i < NUMEL(h_ref) - 1){
			printf(", ");
		}
	}
	printf("\n");
	
	printf("Reference estimate of simulated channel; ");
	for(i = 0; i < NUMEL(h_est_ref); i++){
		printf("%10.7f", h_est_ref[i]);
		if(i < NUMEL(h_est_ref) - 1){
			printf(", ");
		}
	}
	printf("\n");
	
	printf("Your estimate of simulated channel;      ");
	for(i = 0; i < NUMEL(h_est_test); i++){
		printf("%10.7f", h_est_test[i]);
		if(i < NUMEL(h_est_test) - 1){
			printf(", ");
		}
	}
	printf("\n\n");
	
	printf("Comparing generated h_hat:\n");
	pass &= handle_epsres(&res_h);
	printf("\n\n");
	printf("Comparing generated x_hat:\n");
	pass &= handle_epsres(&res_x);
	printf("\n\n");
	printf("Comparing generated e:\n");
	pass &= handle_epsres(&res_e);
	printf("\n\n");
	
#elif defined(SYSMODE_OFDM)

	struct eps_res_s res;
	init_epsres(&res);

	//Random test vectors will (hopefully) give sufficient coverage for this function
	int i;

	printf("\nTesting 'ofdm_demodulate(...)'\n");
	for(i = 1; i <= UNITTEST_PASSES_OFDM; i++){
		float f = randuf(-1e3, 1e3);
		float src[i], re_ref[i], im_ref[i], re_test[i], im_test[i];
		arm_fill_f32(0.0f, re_test, NUMEL(re_test));
		arm_fill_f32(0.0f, im_test, NUMEL(im_test));
		seed_randv(src, NUMEL(src), -i, i);
		d(src, re_ref, im_ref, f, NUMEL(src));
		ofdm_demodulate(src, re_test, im_test, f, NUMEL(src));
		update_epsres(re_ref, re_test, NUMEL(re_ref), UNITTEST_RELTOL, UNITTEST_ABSTOL, &res);
		update_epsres(im_ref, im_test, NUMEL(im_ref), UNITTEST_RELTOL, UNITTEST_ABSTOL, &res);
	}
	pass &= handle_epsres(&res);
	
	printf("\nTesting 'cnvt_re_im_2_cmplx(...)'\n");
	init_epsres(&res);
	for(i = 1; i <= UNITTEST_PASSES_OFDM; i++){
		float re[i], im[i], c_ref[2*i], c_test[2*i];
		arm_fill_f32(0.0f, c_test, NUMEL(c_test));
		seed_randv(re, NUMEL(re), -i, i);
		seed_randv(im, NUMEL(im), -i, i);
		c(re, im, c_ref, NUMEL(re));
		cnvt_re_im_2_cmplx(re, im, c_test, NUMEL(re));
		update_epsres(c_ref, c_test, NUMEL(c_ref), UNITTEST_RELTOL, UNITTEST_ABSTOL, &res);
	}
	pass &= handle_epsres(&res);
	
	printf("\nTesting 'ofdm_conj_equalize(...)'\n");
	init_epsres(&res);
	for(i = 1; i <= UNITTEST_PASSES_OFDM; i++){
		float prxMes[2*i], prxPilot[2*i], ptxPilot[2*i], pEqualized_ref[2*i], pEqualized_test[2*i], hhat_conj_ref[2*i], hhat_conj_test[2*i];
		seed_randv(prxMes, NUMEL(prxMes), -1, 1);
		seed_randv(prxPilot, NUMEL(prxPilot), -1, 1);
		seed_randv(ptxPilot, NUMEL(ptxPilot), -1, 1);
		arm_fill_f32(0.0f, pEqualized_test, NUMEL(pEqualized_test));
		arm_fill_f32(0.0f, hhat_conj_test, NUMEL(hhat_conj_test));
		ce(prxMes, prxPilot, ptxPilot, pEqualized_ref, hhat_conj_ref, i);
		ofdm_conj_equalize(prxMes, prxPilot, ptxPilot, pEqualized_test, hhat_conj_test, i);
		update_epsres(pEqualized_ref, pEqualized_test, NUMEL(pEqualized_ref), UNITTEST_RELTOL, UNITTEST_ABSTOL, &res);
		update_epsres(hhat_conj_ref, hhat_conj_test, NUMEL(hhat_conj_ref), UNITTEST_RELTOL, UNITTEST_ABSTOL, &res);
	}

	pass &= handle_epsres(&res);

#else
	printf("No unit testing needed for selected project configuration.\n");
#endif
	printf("Testing complete!\n" DEBUG_LINESEP);

	//Halt on failure
	if(!pass){
		for(;;){};
	}
}
