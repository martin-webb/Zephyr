#ifndef HDMATRANSFER_H_
#define HDMATRANSFER_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  GENERAL,
  HBLANK
} HDMATransferType;

typedef struct
{
  HDMATransferType type;
  bool isActive;
  uint16_t length;
  uint16_t nextSourceAddr;
  uint16_t nextDestinationAddr;
} HDMATransfer;

#endif // HDMATRANSFER_H_