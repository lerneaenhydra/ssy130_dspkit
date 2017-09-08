/** @file Stand-alone printf implementation. Note function is appended with an
 * 'n' character to ensure that collisions with stdio's printf are avoided. */

#ifndef PRINTFN_H_
#define PRINTFN_H_
#include <stddef.h>

/**
 * @brief  Outputs a formatted string on the DBGU stream, using a variable number of
 *         arguments.
 *
 * @param  pFormat  Format string.
 */
signed int printfn(const char *pFormat, ...);

/**
 * @brief  Outputs a formatted string to a buffer, using a variable number of
 *         arguments. Output will always be null-terminated
 *
 * @param buf		Buffer to write to
 * @param len 		Length of buffer to write to
 * @param  pFormat  Format string.
 * @return The number of written characters (excluding null-terminator)
 */
signed int snprintfn(char *buf, size_t len, const char *pFormat, ...);

#endif /* PRINTFN_H_ */
