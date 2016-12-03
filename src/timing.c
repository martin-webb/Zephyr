#include "timing.h"

#include <sys/time.h>

uint64_t currentTimeMillis()
{
  struct timeval time;
  gettimeofday(&time, NULL);
  return ((uint64_t)time.tv_sec * 1000) + ((uint64_t)time.tv_usec / 1000);
}

uint64_t currentTimeMicros()
{
  struct timeval time;
  gettimeofday(&time, NULL);
  return ((uint64_t)time.tv_sec * 1000000) + ((uint64_t)time.tv_usec);
}
