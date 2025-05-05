/*
请不要修改此文件
*/

#include "logger.h"
#include <stdarg.h>
#include <stdio.h>

#define VPRINTF                                                                \
  do {                                                                         \
    va_list args;                                                              \
    va_start(args, format);                                                    \
    vprintf(format, args);                                                     \
    va_end(args);                                                              \
  } while (0)

void fs_debug(const char *format, ...) {
#if LOG_LEVEL <= LOG_DEBUG
  VPRINTF;
#endif
}

void fs_info(const char *format, ...) {
#if LOG_LEVEL <= LOG_INFO
  VPRINTF;
#endif
}

void fs_important(const char *format, ...) {
#if LOG_LEVEL <= LOG_IMPORTANT
  VPRINTF;
#endif
}

void fs_warning(const char *format, ...) {
#if LOG_LEVEL <= LOG_WARNING
  VPRINTF;
#endif
}

void fs_error(const char *format, ...) {
#if LOG_LEVEL <= LOG_ERROR
  VPRINTF;
#endif
}