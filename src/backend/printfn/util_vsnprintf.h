#ifndef UTIL_VSNPRINTF_H_
#define UTIL_VSNPRINTF_H_

#include <stdarg.h>
#include <stddef.h>

int ap_vsnprintf(char *buf, size_t len, const char *format, va_list ap);

#endif /* SRC_BACKEND_PRINTFN_UTIL_VSNPRINTF_H_ */
