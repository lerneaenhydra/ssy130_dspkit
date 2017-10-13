/** @brief Contains all preprocessor macros */

#ifndef MACRO_H_
#define MACRO_H_

#include <stdint.h>

/** @brief Generate compile-time error when statement is true */
#define BUILD_BUG_ON(condition) 	((void)sizeof(char[1 - 2*!!(condition)]))

/** @brief Basic critical section implementation.
 * Disables all interrupts, executes some arbitrary code, and then restores
 * interrupts to their previous state */
#define CpuCriticalVar()  uint8_t cpuSR

#define CpuEnterCritical()              \
  do {                                  \
    asm (                               \
    "MRS   R0, PRIMASK\n\t"             \
    "CPSID I\n\t"                       \
    "STRB R0, %[output]"                \
    : [output] "=m" (cpuSR) :: "r0");   \
  } while(0)

#define CpuExitCritical()               \
  do{                                   \
    asm (                               \
    "ldrb r0, %[input]\n\t"             \
    "msr PRIMASK,r0;\n\t"               \
    ::[input] "m" (cpuSR) : "r0");      \
  } while(0)

/** @brief Generic atomic section macro */
#define ATOMIC(x) 	do{						\
						CpuCriticalVar();	\
						CpuEnterCritical();	\
						x;					\
						CpuExitCritical();	\
					}while(0)

/** @brief Textboot stringification macro. Allows for the conversion of a preprocessor statement to a string. */
#define xstr(s) str(s)
#define str(s) #s

/** @brief Simple macro for determining the number of elements in an array */
#define NUMEL(x)	(sizeof(x)/sizeof(x[0]))

/** @brief Mapping used to convert floating-point values in the range [-1, 1] to the representation used by the audio hardware
 * Note that we simply truncate the floating point values to convert to the fixed-point representation */
#define FtoI16(x)	((int16_t)((x) * INT16_MAX))

/** @brief Mapping used to convert the audio hardware input representation to the floating point range [-1, 1] */
#define I16toF(x)	(((x) * 1.0f)/ INT16_MAX)

/** @brief Evaluates as true for integers that are a nonzero power of two */
#define ISPOW2(val) (((val) != 0) && (((val) & (val-1)) == 0))

/** @brief Evaluates are true for integer values of x */
#define IS_INTEGER(x)	((x) == ((int32_t) (x)))

/** @brief Safely get the maximum of two values, evaluating a and b once only */
#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

/** @brief Safely get the absolute value of a value, evaluating the value once only */
#define ABS(x) 	\
	({ __typeof__ (x) _x = (x); \
		_x > 0 ? _x : -_x; })

/** @brief Safely get the minimum of two values, evaluating a and b once only */
#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
	 
/** @brief Saturate some numerical value to lie in range min <= x <= max */
#define SAT(x,min,max)	MIN(MAX(x, min), max)

/** @brief Returns ceil(num/den), i.e. the smallest integer N that satisfies N >= num/den */
#define CEILING(num,den) (((num) + (den) - 1) / (den))

#endif /* SRC_MACRO_H_ */
