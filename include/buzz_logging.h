#ifndef BUZZ_LOGGER_H
#define BUZZ_LOGGER_H

#include <stdarg.h>

typedef enum LOG_LEVEL_E {
   BUZZ_ERROR = 0,
   BUZZ_WARN,
   BUZZ_INFO,
   BUZZ_DEBUG
} LOG_LEVEL;

void logger(LOG_LEVEL level, const char* message, ...);

void set_log_level(const char * level);

#endif

