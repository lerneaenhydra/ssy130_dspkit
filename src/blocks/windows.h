/** @file Generators for various window functions */

#ifndef WINDOWS_H_
#define WINDOWS_H_

#include <stdint.h>

/** @brief Generates the coefficients for a blackman window
 * Writes the coefficients corresponding to a window of the form
 * w(n) = a0 - a1*cos(2*pi*n/(N-1)) + a2*cos(4*pi*n/(N-1))
 * where a0 = (1-a)/2, a1 = 1/2, and a2 = a/2 for a = 0.16 and N is the length
 * of the window.
 * @param coeffs	Array of len floats
 * @param len		The length of the window */
void windows_blackman(float * coeffs, uint_fast32_t len);

#endif /* SRC_WINDOWS_H_ */
