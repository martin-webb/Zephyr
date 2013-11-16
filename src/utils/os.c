#include "os.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

bool exists(const char* path)
{
  return access(path, F_OK) == 0;
}

const char* basename(const char* path)
{
  size_t pathLength = strlen(path);
  size_t endPos = pathLength - 1;
  while (path[endPos] != '/') {
    endPos--;
  }
  size_t baseLength = pathLength - endPos - 1;

  char* d = (char*)malloc((baseLength + 1) * sizeof(char));
  assert(d);

  strncpy(d, &(path[endPos + 1]), baseLength);
  d[baseLength] = '\0';

  return d;
}

const char* dirname(const char* path)
{
  size_t endPos = strlen(path) - 1;
  while (path[endPos] != '/') {
    endPos--;
  }

  char* d = (char*)malloc((endPos + 1) * sizeof(char));
  assert(d);

  strncpy(d, path, endPos);
  d[endPos] = '\0';

  return d;
}

ExtSplitPathComponents splitext(const char* path)
{
  size_t pathLength = strlen(path);
  size_t dotPos = -1;
  for (int i = pathLength - 1; i > 0; i--) { // NOTE: Don't include the first character ("i > 0" not "i >= 0") to account for dot-files
    if (path[i] == '.') {
      dotPos = i;
      break;
    }
  }

  ExtSplitPathComponents extSplitPathComponents;
  char* left;
  char* right;

  if (dotPos != -1) {
    size_t leftLength = dotPos;
    left = (char*)malloc((dotPos + 1) * sizeof(char));
    assert(left);

    size_t rightLength = pathLength - dotPos + 1;
    right = (char*)malloc((rightLength + 1) * sizeof(char));
    assert(right);

    strncpy(left, &(path[0]), leftLength);
    left[leftLength] = '\0';

    strncpy(right, &(path[dotPos]), rightLength);
    right[rightLength] = '\0';
  } else {
    left = (char*)malloc((pathLength + 1) * sizeof(char));
    assert(left);

    right = NULL;

    strncpy(left, path, pathLength);
    left[pathLength] = '\0';
  }

  extSplitPathComponents.left = left;
  extSplitPathComponents.right = right;

  return extSplitPathComponents;
}

int mkdirp(const char* path)
{
  int lastMkdirRet = 0;
  int length = strlen(path);

  char pathCopy[length + 1];
  strncpy(pathCopy, path, length);
  pathCopy[length] = '\0';

  for (int i = 1; i < length; i++) {
    if ((pathCopy[i] == '/') || (i == length - 1)) {
      if (!(i == length - 1)) {
        pathCopy[i] = '\0';
      }
      lastMkdirRet = mkdir(pathCopy, S_IRWXU | S_IRGRP | S_IROTH);
      if (!(i == length - 1)) {
        pathCopy[i] = '/';
      }
    }
  }

  return lastMkdirRet;
}