#include "logging.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

const char* const LOG_LEVEL_NAMES[] =
{
  "DEBUG",
  "INFO",
  "WARNING",
  "ERROR",
  "CRITICAL"
};

int logImpl(enum LogLevel logLevel, const char* format, va_list args)
{
  struct timeval t;
  gettimeofday(&t, NULL);

  struct tm* localTime = localtime(&t.tv_sec);
  unsigned int milliseconds = t.tv_usec / 1000;

  // NOTE: Time format is 23 characters: YYYY/MM/DD HH:MM:SS.sss but only 19 can be filled in by strftime, the remaining 4 for milliseconds are handled later
  char timeStringBuffer[19 + 1]; // Don't forget the null terminator
  strftime(timeStringBuffer, 19 + 1, "%Y/%m/%d %H:%M:%S", localTime);

  switch (logLevel) {
    case LogLevelDebug:
    case LogLevelInfo:
    case LogLevelWarning:
      printf("[%s.%03u][%s] ", timeStringBuffer, milliseconds, LOG_LEVEL_NAMES[logLevel]);
      return vprintf(format, args);
      break;
    case LogLevelError:
    case LogLevelCritical:
      fprintf(stderr, "[%s.%03u][%s] ", timeStringBuffer, milliseconds, LOG_LEVEL_NAMES[logLevel]);
      return vfprintf(stderr, format, args);
      break;
    default:
      fprintf(stderr, "%s: Unknown log level %d\n", __func__, logLevel);
      exit(EXIT_FAILURE);
  }
}

int debug(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  int chars = logImpl(LogLevelDebug, format, args);
  va_end(args);

  return chars;
}

int info(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  int chars = logImpl(LogLevelInfo, format, args);
  va_end(args);

  return chars;
}

int warning(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  int chars = logImpl(LogLevelWarning, format, args);
  va_end(args);

  return chars;
}

int error(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  int chars = logImpl(LogLevelError, format, args);
  va_end(args);

  return chars;
}

int critical(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  int chars = logImpl(LogLevelCritical, format, args);
  va_end(args);

  return chars;
}
