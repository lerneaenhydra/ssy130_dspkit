/** @brief Implements all generator functions.
 * These functions differ from the source blocks in that they are stateless
 * and are primarily intended to be used as utility functions. */

#ifndef GEN_H_
#define GEN_H_

#include <stdint.h>

/** @brief Writes a vector of sinusoidal data with configurable frequency and phase.
 * @param frequency The desired frequency
 * @param phase The phase of the first sample
 * @param dest Destination to write to
 * @param len The number of elements in dest */
void blocks_gen_sin(float frequency, float phase, float * dest, int_fast32_t len);

/** @brief Writes a vector of cosine data with configurable frequency and phase.
 * @param frequency The desired frequency
 * @param phase The phase of the first sample
 * @param dest Destination to write to
 * @param len The number of elements in dest */
void blocks_gen_cos(float frequency, float phase, float * dest, int_fast32_t len);

/** @brief Generates a random (human-readable) string with up to strlen non-null elements
 * @param str Pointer to char array to write result to
 * @param strlen Length of char array
 * @param seed Pointer to RNG seed */
void blocks_gen_str(char * str, int_fast32_t strlen, uint_fast32_t * seed);

#endif /* SRC_BLOCKS_GEN_H_ */
