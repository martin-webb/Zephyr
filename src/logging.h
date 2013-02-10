#ifndef LOGGING_H_
#define LOGGING_H_

enum LogLevel
{
  LogLevelDebug,
  LogLevelInfo,
  LogLevelWarning,
  LogLevelError,
  LogLevelCritical
};

int debug(const char* format, ...);
int info(const char* format, ...);
int warning(const char* format, ...);
int error(const char* format, ...);
int critical(const char* format, ...);

#endif LOGGING_H_