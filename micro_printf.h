#ifndef MICRO_PRINTF_H_FILE_INCLUDED_
#define MICRO_PRINTF_H_FILE_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

int snprintf(char* buffer, unsigned buffer_len, const char* format_str, ...);

int vsnprintf(char* buffer, unsigned buffer_len, const char* format_str, va_list arg_ptr);

typedef int (*PrintfCallBack)(void* data, char character);

int cb_printf(PrintfCallBack callback, void* data, const char* format_str, ...);

int cb_vprintf(PrintfCallBack callback, void* data, const char* format_str, va_list arg_ptr);

#ifdef __cplusplus
}
#endif


#endif
