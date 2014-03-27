#ifndef CYFWSTORPROG_H_
#define CYFWSTORPROG_H_

#include "scsi.h"

#define CyFalse 0
#define CyTrue  1

typedef uint8_t CyBool;

typedef struct CyFx3PartitionElement
{
    uint8_t type;
    uint8_t location;
    uint16_t blockSize;
    uint32_t partitionSize;
     
} CyFx3PartitionElement_t, *CyFx3PartitionElement_p;

typedef struct CyFx3PartitionData
{
    uint8_t deviceType;
    uint8_t numPartitions;
    uint8_t reserved[2]; 
    uint32_t numBlocks;
    CyFx3PartitionElement_t partitionElement[4];
} CyFx3PartitionData_t, *CyFx3PartitionData_p;

typedef struct CyWbFwImage
{
    uint8_t *fwImage_p;     // Pointer to the firmware image (table of uint8_t values)
    uint32_t fwImageSize;   // Size of the firmware image in bytes
} CyWbFwImage_t;

#endif