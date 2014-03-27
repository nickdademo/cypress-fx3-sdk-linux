/*
 ## Cypress USB 3.0 Platform header file (cyu3sibpp.h)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2013,
 ##  All Rights Reserved
 ##  UNPUBLISHED, LICENSED SOFTWARE.
 ##
 ##  CONFIDENTIAL AND PROPRIETARY INFORMATION
 ##  WHICH IS THE PROPERTY OF CYPRESS.
 ##
 ##  Use of this file is governed
 ##  by the license agreement included in the file
 ##
 ##     <install>/license/license.txt
 ##
 ##  where <install> is the Cypress software
 ##  installation root directory path.
 ##
 ## ===========================
*/

#ifndef _INCLUDED_CYU3SIBPP_H_
#define _INCLUDED_CYU3SIBPP_H_

#include "cyu3sib.h"
#include "cyu3externcstart.h"

/************************************************************************************/
/********************************DATATYPES*******************************************/
/************************************************************************************/

/* OS related defines */
#define CY_U3P_SIB_STACK_SIZE               (0x800) /* 1K stack for SIB Thread */
#define CY_U3P_SIB_THREAD_PRIORITY          (4)     /* SIB thread priority = 4 */

/* SIB Event macros. */
#define CY_U3P_SIB_EVT_PORT_POS             (5)
#define CY_U3P_SIB_EVT_PORT_0               (1 << 5)
#define CY_U3P_SIB_EVT_PORT_1               (1 << 6)
#define CY_U3P_SIB_EVT_ABORT                (1 << 7)
#define CY_U3P_SIB_TIMER0_EVT		    (1 << 15)
#define CY_U3P_SIB_TIMER1_EVT	            (1 << 16)

/* Partition related macros */
#define CY_U3P_PARTITION_TYPE_DATA_AREA     0xDA
#define CY_U3P_PARTITION_TYPE_AP_BOOT       0xAB
#define CY_U3P_PARTITION_TYPE_BENICIA_BOOT  0xBB
#define CY_U3P_BOOT_PARTITION_NUM_BLKS      0x08

/* Enumeration of the device partitions */
typedef enum
{
    CY_U3P_SIB_DEV_PARTITION_0 = 0, /* Device partition 0 */
    CY_U3P_SIB_DEV_PARTITION_1,     /* Device partition 1 */
    CY_U3P_SIB_DEV_PARTITION_2,     /* Device partition 2 */
    CY_U3P_SIB_DEV_PARTITION_3,     /* Device partition 3 */
    CY_U3P_SIB_NUM_PARTITIONS       /* Maximum paritions allowed */
} CyU3PSibDevPartition;

/* Macros to enable or disable the SIB Core interrupts. */
#define CyU3PSibEnableCoreIntr(portId) \
{\
    if ((portId) == CY_U3P_SIB_PORT_0) \
    { \
        CyU3PVicEnableInt (CY_U3P_VIC_SIB0_CORE_VECTOR); \
    } \
    else \
    { \
        CyU3PVicEnableInt (CY_U3P_VIC_SIB1_CORE_VECTOR); \
    } \
}

#define CyU3PSibDisableCoreIntr(portId) \
{\
    if ((portId) == CY_U3P_SIB_PORT_0) \
    { \
        CyU3PVicDisableInt (CY_U3P_VIC_SIB0_CORE_VECTOR); \
    } \
    else \
    { \
        CyU3PVicDisableInt (CY_U3P_VIC_SIB1_CORE_VECTOR); \
    } \
}

typedef struct CyU3PSibGlobalData
{
    CyBool_t            isActive;
    CyBool_t            s0Enabled;
    CyBool_t            s1Enabled;
    CyU3PSibEvtCbk_t    sibEvtCbk;

    uint32_t            wrCommitSize[CY_U3P_SIB_NUM_PORTS];
    uint32_t            nextWrAddress[CY_U3P_SIB_NUM_PORTS];
    uint32_t            openWrSize[CY_U3P_SIB_NUM_PORTS];
    CyBool_t            wrCommitPending[CY_U3P_SIB_NUM_PORTS];
    CyU3PSibLunLocation activePartition[CY_U3P_SIB_NUM_PORTS];
    uint32_t            partConfig[CY_U3P_SIB_NUM_PORTS];
    CyU3PDmaChannel     sibDmaChannel;
} CyU3PSibGlobalData;

extern CyU3PSibGlobalData glSibDevInfo;

/* Macros to GET/SET the global data structure fields. */
#define CyU3PIsSibActive()                      (glSibDevInfo.isActive)
#define CyU3PSetSibActive()                     (glSibDevInfo.isActive = CyTrue)
#define CyU3PClearSibActive()                   (glSibDevInfo.isActive = CyFalse)

#define CyU3PSibIsS0Enabled()                   (glSibDevInfo.s0Enabled)
#define CyU3PSibSetS0Enabled()                  (glSibDevInfo.s0Enabled = CyTrue)
#define CyU3PSibClearS0Enabled()                (glSibDevInfo.s0Enabled = CyFalse)

#define CyU3PSibIsS1Enabled()                   (glSibDevInfo.s1Enabled)
#define CyU3PSibSetS1Enabled()                  (glSibDevInfo.s1Enabled = CyTrue)
#define CyU3PSibClearS1Enabled()                (glSibDevInfo.s1Enabled = CyFalse)

#define CyU3PSibSetEventCallback(cb)            (glSibDevInfo.sibEvtCbk = (cb))
#define CyU3PSibGetEventCallback                (glSibDevInfo.sibEvtCbk)

#define CyU3PSibIsWrCommitPending(port)         (glSibDevInfo.wrCommitPending[(port)])
#define CyU3PSibSetWrCommitPending(port)        (glSibDevInfo.wrCommitPending[(port)] = CyTrue)
#define CyU3PSibClearWrCommitPending(port)      (glSibDevInfo.wrCommitPending[(port)] = CyFalse)

#define CyU3PSibGetWrCommitSize(port)           (glSibDevInfo.wrCommitSize[(port)])
#define CyU3PSibSetWrCommitSize(port,size)      (glSibDevInfo.wrCommitSize[(port)] = (size))

#define CyU3PSibGetNextWrAddress(port)          (glSibDevInfo.nextWrAddress[(port)])
#define CyU3PSibSetNextWrAddress(port,addr)     (glSibDevInfo.nextWrAddress[(port)] = (addr))

#define CyU3PSibGetOpenWrSize(port)             (glSibDevInfo.openWrSize[(port)])
#define CyU3PSibSetOpenWrSize(port,size)        (glSibDevInfo.openWrSize[(port)] = (size))

#define CyU3PSibGetActivePartition(port)        (glSibDevInfo.activePartition[(port)])
#define CyU3PSibSetActivePartition(port,loc)    (glSibDevInfo.activePartition[(port)] = (loc))

#define CyU3PSibGetPartitionConfig(port)        (glSibDevInfo.partConfig[(port)])
#define CyU3PSibSetPartitionConfig(port,cfg)    (glSibDevInfo.partConfig[(port)] = (cfg))

#define CyU3PSibGetDmaChannelHandle             (&(glSibDevInfo.sibDmaChannel))

#define CYU3P_SIB_INT_READ_SOCKET               (CY_U3P_SIB_SOCKET_6)
#define CYU3P_SIB_INT_WRITE_SOCKET              (CY_U3P_SIB_SOCKET_7)

/** \def CY_U3P_DMA_SIB_NUM_SCK
    \brief Number of storage sockets supported by FX3S.
 */
#define CY_U3P_DMA_SIB_NUM_SCK                          (8)

/* Socket number range for stoarge */
#define CY_U3P_DMA_SIB_MIN_CONS_SCK     (0)
#define CY_U3P_DMA_SIB_MAX_CONS_SCK                     (CY_U3P_DMA_SIB_NUM_SCK - 1)
#define CY_U3P_DMA_SIB_MIN_PROD_SCK     (0)
#define CY_U3P_DMA_SIB_MAX_PROD_SCK                     (CY_U3P_DMA_SIB_NUM_SCK - 1)

/* Macros to access the glSibLunInfo structure. */
#define CyU3PSibLunIsValid(port,lun)                    (glSibLunInfo[(port)][(lun)].valid != 0)
#define CyU3PSibLunGetStartAddr(port,lun)               (glSibLunInfo[(port)][(lun)].startAddr)
#define CyU3PSibLunGetNumBlocks(port,lun)               (glSibLunInfo[(port)][(lun)].numBlocks)
#define CyU3PSibLunGetLocation(port,lun)                (glSibLunInfo[(port)][(lun)].location)

#define CyU3PSibLunSetValid(port,lun)                   (glSibLunInfo[(port)][(lun)].valid = 1)
#define CyU3PSibLunClearValid(port,lun)                 (glSibLunInfo[(port)][(lun)].valid = 0)

#define CyU3PSibLunSetStartAddr(port,lun,addr)          (glSibLunInfo[(port)][(lun)].startAddr = (addr))
#define CyU3PSibLunSetNumBlocks(port,lun,cnt)           (glSibLunInfo[(port)][(lun)].numBlocks = (cnt))
#define CyU3PSibLunSetLocation(port,lun,loc)            (glSibLunInfo[(port)][(lun)].location = (loc))

/* Macros to access the glSibCtxt structure. */
#define CyU3PSibIsDeviceBusy(port)                      (glSibCtxt[(port)].inUse)
#define CyU3PSibSetDeviceBusy(port)                     (glSibCtxt[(port)].inUse = CyTrue)
#define CyU3PSibClearDeviceBusy(port)                   (glSibCtxt[(port)].inUse = CyFalse)

/* Summary
   SIB Socket IDs.

   Description
   This is a software representation of all the SIB sockets on the device.
 */
typedef enum
{
    CY_U3P_SIB_SOCKET0  = 0x0200,
    CY_U3P_SIB_SOCKET1,
    CY_U3P_SIB_SOCKET2,
    CY_U3P_SIB_SOCKET3,
    CY_U3P_SIB_SOCKET4 = 0x0204,               /* S-port socket number 4. */
    CY_U3P_SIB_SOCKET5,                        /* S-port socket number 5. */
    CY_U3P_SIB_SOCKET_6,                        /* S-port socket number 6. */
    CY_U3P_SIB_SOCKET_7                         /* S-port socket number 7. */
} CyU3PSibSocketId_t;

/* Summary: SIB Context structure.
   This structure holds the SIB Context information. An instance of this structure will
   be created for each port.
 */
typedef struct CyU3PSibCtxt
{
    uint8_t             isRead;         /**< Indicates if its a read or a write operation. */
    volatile uint8_t    inUse;          /**< Variable to indicate if the card is being used. */
    uint8_t             partition;      /**< Flag indicating if the device is partitioned. */
    uint8_t             numBootLuns;    /**< Number of Logical UNits containing boot code. */
    uint8_t             numUserLuns;    /**< Number of user accessible logical units on the device. */
    CyU3PSibSocketId_t  sibRdSockId;    /**< S-Port socket id to be used for read transfers */
    CyU3PSibSocketId_t  sibWrSockId;    /**< S-Port socket id to be used for write transfers */
    CyU3PTimer          writeTimer;     /**< Sib Write Timeout Timer */
    CyU3PTimerCb_t      writeTimerCbk;  /**< SIB Timer callback function */
    CyU3PMutex          mutexLock;      /**< Mutex lock for the port */
    CyU3PDmaChannel     rdChHandle;     /**< Internal read channel handle */
    CyU3PDmaChannel     wrChHandle;     /**< Internal write channel handle */
} CyU3PSibCtxt_t;

/************************************************************************************/
/******************************* GLOBAL VARIABLES ***********************************/
/************************************************************************************/

extern CyU3PThread          glSibThread;          /* SIB thread */
extern CyU3PEvent           glSibEvent;           /* SIB Event Handle */
extern CyU3PSibCtxt_t       glSibCtxt[CY_U3P_SIB_NUM_PORTS];            /* Storage device/port information. */
extern CyU3PSibIntfParams_t glSibIntfParams[CY_U3P_SIB_NUM_PORTS];      /* Per-port interface parameters. */
extern CyU3PSibLunInfo_t    glSibLunInfo[CY_U3P_SIB_NUM_PORTS][CY_U3P_SIB_NUM_PARTITIONS];
extern CyU3PSibEvtCbk_t     glSibEvtCbk;                                /* SIB event callback function */

/** \brief Register information about a storage partition.

    **Description**\n
    This function stores information about a storage partition in the driver
    data structures.
 */
extern void
CyU3PSibUpdateLunInfo (
        uint8_t  portId,                        /**< Port on which storage device is connected. */
        uint8_t  partNum,                       /**< Partition number which is being updated. */
        uint8_t  partType,                      /**< Type of storage partition. */
        uint8_t  location,                      /**< Location of this storage partition (boot area or data area). */
        uint32_t startAddr,                     /**< Start address for this storage partition. */
        uint32_t partSize                       /**< Size of the partition in sectors. */
        );

/** \brief Initialize the storage device connected on the specified storage port.

    **Description**\n
    Initialize the storage device connected on the specified storage port.
 */
extern CyU3PReturnStatus_t
CyU3PSibInitCard (
        uint8_t portId                          /**< Port on which the device should be initialized. */
        );

#include "cyu3externcend.h"

#endif /* _INCLUDED_CYU3SIBPP_H_ */

/*[]*/

