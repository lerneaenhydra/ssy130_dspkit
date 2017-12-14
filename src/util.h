/** @brief Various utility functions used throughout the project */

/** @brief Critical section macros, taken from https://mcuoneclipse.com/2014/01/26/entercritical-and-exitcritical-why-things-are-failing-badly/ */
#ifndef UTIL_H_
#define UTIL_H_

#include <stdint.h>
#include <stddef.h>

#define R_RAND_MAX	0xFFFF

/** @brief Prints the values in a vector of floats, formatted with MATLAB syntax */
void print_vector_f(char * name, float * vals, int_fast32_t len);

/** @brief Prints the values in a vector of floats representing complex numbers.
 * Assumes the numbers are stored as [real0, imag0, real1, imag1, ...] */
void print_cplx_vector_f(char * prefix, float * vals, int_fast32_t len);

/** @brief Prints the values in a vector of integers, formatted with MATLAB syntax */
void print_vector_i(char * prefix, int32_t * vals, int_fast32_t len);

/** @brief Converts a vector of length len with floating point values to integers with an optional scaling parameter */
void vector_f2i(float * vals_f, int32_t * vals_i, int_fast32_t len, float scale);

/** @brief Compures the mean average of a vector of floats */
float vector_mean(float * vals, int_fast32_t len);

/** @brief Prints an error message and enters an infinite loop blinking the board led's */
void halt_error(char * errmsg);

/** @brief Gets the random seed generated at startup from psuedo-random hardware */
uint_fast32_t util_get_seed(void);

/** @brief Outputs a random integer in the range [0, R_RAND_MAX], given a random seed location */
uint_fast16_t util_rand_r(uint_fast32_t * const seed);

/** @brief Outputs a random integer in the range [minval,maxval], given a random seed location */
uint_fast16_t util_rand_range(uint_fast16_t minval, uint_fast16_t maxval, uint_fast32_t * const seed);

/** @brief Fills a vector with samples from the standard distribution.
 * @param mu    Mean
 * @param sigma Standard deviation
 * @param seed  Pointer to seed
 * @param res   Pointer to result array
 * @param len   Number of elements in result vector */
void util_randN(const float mu, const float sigma, uint_fast32_t * const seed, float * res, size_t len);

#endif /* UTIL_H_ */
