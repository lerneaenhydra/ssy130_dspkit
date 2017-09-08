/** @brief Rudimentary wrapper for the ap_snprintfn functions.
 * Adds support for printf-like functionality. */
#include "util_vsnprintf.h"
#include <stdarg.h>
#include "../hw/board.h"
#include "../../config.h"


/**
 * @brief  Transmit a char, if you want to use printf(),
 *         you need implement this function
 *
 * @param  pStr	Storage string.
 * @param  c    Character to write.
 */
static void PrintChar(char c){
	if(c == '\n'){
		board_usart_write('\r');
	}
	board_usart_write(c);
}


/**
 * @brief  Outputs a formatted string on the DEBUG stream, using a variable number of
 *         arguments.
 *
 * @param  pFormat  Format string.
 */
signed int printfn(const char *pFormat, ...){
    va_list ap;
    signed int result;

    /* Forward call to vprintf */
    va_start(ap, pFormat);
    char outbuf[PRINTF_MAX_STRING_SIZE];
    result = ap_vsnprintf(outbuf, PRINTF_MAX_STRING_SIZE, pFormat, ap);
    va_end(ap);

    char * strptr = outbuf;
    while(*strptr != 0){
    	PrintChar(*(strptr++));
    }

    return result;
}

/**
 * @brief  Outputs a formatted string to a buffer, using a variable number of
 *         arguments. Output will always be null-terminated
 *
 * @param buf		Buffer to write to
 * @param len 		Length of buffer to write to
 * @param  pFormat  Format string.
 * @return The number of written characters (excluding null-terminator)
 */
signed int snprintfn(char *buf, size_t len, const char *pFormat, ...){
    va_list ap;
    signed int result;

    /* Forward call to vprintf */
    va_start(ap, pFormat);
    result = ap_vsnprintf(buf, len, pFormat, ap);
    va_end(ap);

	*(buf + len - 1) = '\0';
	
    return result;
}

/* --------------------------------- End Of File ------------------------------ */
