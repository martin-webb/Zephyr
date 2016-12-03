#ifndef UTILS_OS_H_
#define UTILS_OS_H_

#include <stdbool.h>


typedef struct {
  const char* left;
  const char* right;
} ExtSplitPathComponents;


bool exists(const char* path);
const char* basename(const char* path); // NOTE: Caller owns memory
const char* dirname(const char* path); // NOTE: Caller owns memory
ExtSplitPathComponents splitext(const char* path); // NOTE: Caller owns memory
int mkdirp(const char* path);

#endif // UTILS_OS_H_
