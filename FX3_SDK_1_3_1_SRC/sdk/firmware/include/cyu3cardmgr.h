/*
## Cypress USB 3.0 Platform header file (cyu3cardmgr.h)
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

#ifndef _INCLUDED_CYU3CARDMGR_H_
#define _INCLUDED_CYU3CARDMGR_H_

#include <cyu3os.h>
#include <cyu3types.h>
#include <cyu3sib.h>
#include <cyu3cardmgr_fx3s.h>

#include "cyu3externcstart.h"

/* Summary:
   This file contains the low level card driver related data structures.
*/

/* Cy partition signature */
#define CY_U3P_BOOT_PART_SIGN                           (0x42575943) /* "CYWB" */
#define CY_U3P_MMC_CARD_VER_4_4                          (44)

/* MMC card EXT_CSD byte offsets. */
#define CY_U3P_MMC_ECSD_REVISION_LOC                    (192)   /* EXT_CSD_REV */
#define CY_U3P_MMC_ECSD_PARTCFG_LOC                     (179)   /* PARTITION_CONFIG */
#define CY_U3P_MMC_ECSD_BOOTSIZE_LOC                    (226)   /* BOOT_SIZE_MULTI */

/* Clock divider values : Based on System Clock running at approx. 400 MHz. */
#define CY_U3P_CLOCK_DIVIDER_400                         (0x3FF)
#define CY_U3P_CLOCK_DIVIDER_20M                         (0x14)
#define CY_U3P_CLOCK_DIVIDER_26M                         (0x0F)
#define CY_U3P_CLOCK_DIVIDER_46M                         (0x08)
#define CY_U3P_CLOCK_DIVIDER_52M                         (0x07)
#define CY_U3P_CLOCK_DIVIDER_84M                         (0x04)
#define CY_U3P_CLOCK_DIVIDER_104M                        (0x03)

/* Post reset (CMD0) delay allowed for storage devices. */
#define CY_U3P_SIB_DEVICE_POSTRESET_DELAY               (5)

/* Timeout (in ms) for the wait for device readyness. */
#define CY_U3P_SIB_DEVICE_READY_TIMEOUT                 (0x3FF)

/* Storage driver polling delay in us. */
#define CY_U3P_SIB_POLLING_DELAY                        (5)

/* Storage driver command send delay. */
#define CY_U3P_SIB_SENDCMD_DELAY                        (10)

/* Delay required from hotplug event to handler start (in ms). */
#define CY_U3P_SIB_HOTPLUG_DELAY                        (100)

/* Block size of 512 bytes */
#define CY_U3P_SIB_BLOCK_SIZE                            (0x0200)

/* SD Spec 2.0 Sec 5.5 Driver Stage Register's default value */
#define CY_U3P_SIB_DSR_DEF_VAL                           (0x0404)

/* SIB CS Register mask */
#define CY_U3P_SIB_CS_REG_MASK                           (0xFFFFFF00)
#define CY_U3P_SIB_CS_CFG_MASK                           (0xFFFFC000)

/* Host Capacity Bit */
#define CY_U3P_SIB_HCS_BIT                               (0x40000000)
#define CY_U3P_SIB_CCS_BIT                               CY_U3P_SIB_HCS_BIT
#define CY_U3P_SIB_SEC_MODE_ADDR_BIT                     CY_U3P_SIB_HCS_BIT

/* SD 3.0 related bit fields of OCR register */
#define CY_U3P_SIB_XPC_BIT                               (0x10000000)
#define CY_U3P_SIB_S18R_BIT                              (0x01000000)
#define CY_U3P_SIB_S18A_BIT                              CY_U3P_SIB_S18R_BIT

/* 2.7v to 3.6v */
#define CY_U3P_SIB_DEF_VOLT_RANGE                        (0x00FFFF00)
#define CY_U3P_SIB_CARD_BUSY_BIT_POS                    (0x1F)

/* Clock stop duration while switching operating voltage. */
#define CY_U3P_SIB_SD_CMD11_CLKSTOP_DURATION            (10)

/* Delay from SD clock start that is required for SD card to drive the IOs high. */
#define CY_U3P_SIB_SD_CMD11_CLKACT_DURATION             (2)

/* Write data timeout value in clock ticks. */
#define CY_U3P_SIB_WRITE_DATA_TIMEOUT                    (5000)

/* Idle Time out value */
#define CY_U3P_SIB_IDLE_DATA_TIMEOUT                     (2000)

/* Convert a sector address/count value to a byte address/count. */
#define CY_U3P_SIB_SECTORCNT_TO_BYTECNT(n)              ((n) << 9)

/* Conver a byte address/count value to a sector address/count. */
#define CY_U3P_SIB_BYTECNT_TO_SECTORCNT(n)              ((n) >> 9)

/* SD card Switch command parameters. */
#define CY_U3P_SD_SW_QUERY_FUNCTIONS                    (0x00FFFFF1)    /* Query device functions. */
#define CY_U3P_SD_SW_HIGHSP_PARAM                       (0x80FFFFF1)    /* High Speed mode. */
#define CY_U3P_SD_SW_UHS1_PARAM                         (0x80FFFFF2)    /* Select USH-1 mode. */

/* MMC Switch command parameters. */
#define CY_U3P_MMC_SW_PARTCFG_USER_PARAM                (0x03B30000)    /* Select User Data area. */
#define CY_U3P_MMC_SW_PARTCFG_BOOT1_PARAM               (0x03B30100)    /* Select Boot1 partition. */
#define CY_U3P_MMC_SW_PARTCFG_BOOT2_PARAM               (0x03B30200)    /* Select Boot2 partition. */

/* Byte locations in the virtual partition header. */
#define CY_U3P_SIB_VBP_PARTSIZE_LOCATION                (40)
#define CY_U3P_SIB_VBP_PARTTYPE_LOCATION                (47)
#define CY_U3P_SIB_VBP_CHECKSUM_LOCATION                (68)

#define CyU3PSibSetNumBlocks(portId, numBlks) \
    (SIB->sdmmc[(portId)].block_count = (numBlks))

/* Macro that sets the active socket for the port */
#define CyU3PSibSetActiveSocket(portId, sockNum) \
{\
    SIB->sdmmc[(portId)].cs = ((SIB->sdmmc[(portId)].cs & ~CY_U3P_SIB_SOCKET_MASK) | \
                               (CyU3PDmaGetSckNum((sockNum)) << CY_U3P_SIB_SOCKET_POS));\
}

/* Macro to reset the SIB Controller. */
#define CyU3PSibResetSibCtrlr(portId) \
{\
    uint32_t tmp321 = SIB->sdmmc[(portId)].mode_cfg; \
    SIB->sdmmc[(portId)].cs |= CY_U3P_SIB_RSTCONT; \
    while (SIB->sdmmc[(portId)].cs & CY_U3P_SIB_RSTCONT); \
    SIB->sdmmc[(portId)].mode_cfg = tmp321; \
}

/* Macro to clear all the interrupts on the given port */
#define CyU3PSibClearIntr(portId) \
{ \
    SIB->sdmmc[(portId)].intr = ~(CY_U3P_SIB_SDMMC_INTR_DEFAULT); \
}

/* Macro to convert the block address to byte address if the
 * card is a low capacity card. */
#define CyU3PSibConvertAddr(portId, blkAddr) \
{ \
    if (glCardCtxt[(portId)].highCapacity == CyFalse) \
    { \
        (blkAddr) <<= 9; \
    } \
}

/* Get the Number of function used in the SDIO */
#define CyU3PSdioNumberOfFunction(portId)                                               \
    ((SIB->sdmmc[(portId)].resp_reg0 >> 28) & 0x7)

/* Get the Memory present used in the SDIO */
#define CyU3PSdioMemoryPresent(portId)                                                  \
    ((SIB->sdmmc[(portId)].resp_reg0 >> 27) & 0x1)

/* Get the OCR register used in the SDIO */
#define CyU3PSdioOCRRegister(portId)                                                    \
    ((SIB->sdmmc[(portId)].resp_reg0 >> 8) & 0xFFFFFF)


/************************************************************************************/
/********************************DATATYPES*******************************************/
/************************************************************************************/

/* SD/MMC Card states */
typedef enum
{
    CY_U3P_SD_MMC_IDLE = 0,
    CY_U3P_SD_MMC_READY,
    CY_U3P_SD_MMC_IDENTIFICATION,
    CY_U3P_SD_MMC_STANDBY,
    CY_U3P_SD_MMC_TRANSFER,
    CY_U3P_SD_MMC_DATA,
    CY_U3P_SD_MMC_RECEIVE,
    CY_U3P_SD_MMC_PROGRAMMING,
    CY_U3P_SD_MMC_DISCONNECT,
    CY_U3P_MMC_BUS_TEST
} CyU3PSdMmcStates_t;

/* Enumeration of SD Card Versions */
typedef enum 
{
    CY_U3P_SD_CARD_VER_1_X = 0,
    CY_U3P_SD_CARD_VER_2_0,
    CY_U3P_SD_CARD_VER_3_0
} CyU3PSDCardVer_t;

/* Enumeration of card bus width */
typedef enum
{
    CY_U3P_CARD_BUS_WIDTH_1_BIT = 0,
    CY_U3P_CARD_BUS_WIDTH_4_BIT,
    CY_U3P_CARD_BUS_WIDTH_8_BIT
} CyU3PCardBusWidth_t;

/* Enumeration of SD Card Registers */
typedef enum
{
    CY_U3P_SIB_SD_REG_OCR = 0,  /* OCR Register */
    CY_U3P_SIB_SD_REG_CID,      /* CID Register */
    CY_U3P_SIB_SD_REG_CSD       /* CSD Register */
} CyU3PSibSDRegs_t;

/* Enumeration of card operating mode */
typedef enum
{
    CY_U3P_SIB_SD_CARD_ID_400 = 0, /* SD 400 KHz */
    CY_U3P_SIB_SDSC_25,         /* SDSC */
    CY_U3P_SIB_SDHC_DS_25,      /* SDHC Default Speed Mode 0 - 25MHz */
    CY_U3P_SIB_SDHC_HS_50,      /* SDHC 25MB/sec 0 - 50MHz */
    CY_U3P_SIB_UHS_I_DS,        /* Default Speed up to 25MHz 3.3V signaling */
    CY_U3P_SIB_UHS_I_HS,        /* High Speed up to 50MHz 3.3V signaling */
    CY_U3P_SIB_UHS_I_SDR12,     /* SDR up to 25MHz 1.8V signaling */
    CY_U3P_SIB_UHS_I_SDR25,     /* SDR up to 50MHz 1.8V signaling */
    CY_U3P_SIB_UHS_I_SDR50,     /* SDR up to 100MHz 1.8V signaling */
    CY_U3P_SIB_UHS_I_SDR104,    /* SDR up to 208MHz 1.8V signaling */
    CY_U3P_SIB_UHS_I_DDR50      /* DDR up to 50MHz 1.8V signaling */
} CyU3PCardOpMode_t;

/* Card context structure. */
typedef struct CyU3PCardCtxt
{
    uint8_t cidRegData[CY_U3P_SD_REG_CID_LEN]; /* CID Register data */
    uint8_t csdRegData[CY_U3P_SD_REG_CSD_LEN]; /* CSD Register data */
    uint32_t ocrRegData;       /* OCR Register data */

    uint32_t numBlks;       /* Current block size setting for the device. */
    uint32_t eraseSize;     /* The erase unit size in bytes for this device. */
    uint32_t dataTimeOut;   /* Timeout value for data transactions */

    uint16_t blkLen;        /* Current block size setting for the device. */
    uint16_t cardRCA;       /* Relative card address */
    uint16_t ccc;           /* Card command classes */

    uint8_t cardType;       /* Type of storage device connected on the S port. (Can be none if
                               no device detected).*/
    uint8_t removable;      /* Indicates if the storage device is a removable device. */
    uint8_t writeable;      /* Whether the storage device is write enabled. */
    uint8_t locked;         /* Identifies whether the storage device is password locked. */
    uint8_t ddrMode;        /* Whether DDR clock mode is being used for this device. */
    uint8_t clkRate;        /* Current operating clock frequency for the device.*/
    uint8_t opVoltage;      /* Current operating voltage setting for the device. */
    uint8_t busWidth;       /* Current bus width setting for the device. */

    uint8_t cardVer;        /* Card version */
    uint8_t highCapacity;   /* Indicates if the card is a high capacity card. */
    uint8_t uhsIOpMode;     /* SD 3.0 Operating modes */
    uint8_t cardSpeed;      /* Card speed information */
#ifdef CYU3P_STORAGE_SDIO_SUPPORT
    CyBool_t isMemoryPresent; /* Memory is present in SDIO */
    uint8_t numberOfFunction; /* Number of Function present in SDIO */
    uint8_t CCCRVersion;      /* Version number of CCCR register in SDIO */ 
    uint8_t sdioVersion;      /* SDIO Version  */
    uint8_t cardCapability;   /* Sdio Card Capability  */
    uint32_t addrCIS;         /* Address of CIS */
    uint16_t manufactureId;   /* Manufacture ID */
    uint16_t manufactureInfo; /* Manufacture information */	 
    uint16_t fnBlkSize [8];   /* Array to store block size from SetBlockSize*/

    uint8_t uhsSupport;
    uint8_t supportsAsyncIntr;
#endif /* CYU3P_STORAGE_SDIO_SUPPORT */

} CyU3PCardCtxt_t;

extern CyU3PCardCtxt_t glCardCtxt[CY_U3P_SIB_NUM_PORTS];

/*****************************************************************************/
/*******************************FUNCTION PROTOTYPES***************************/
/*****************************************************************************/

/* Summary
 * Initialize a SD/MMC/SDIO/CE-ATA storage device attached to FX3.
 *
 * Description
 * Initialize a SD/MMC/SDIO/CE-ATA storage device attached to FX3.
 *
 * Parameters
 * port_id: indicates the storage port to which card is connected.
 *
 * Return value
 * CY_U3P_SUCCESS
 * CY_U3P_ERROR_TIMEOUT
 */
extern CyU3PReturnStatus_t
CyU3PCardMgrInit (
                  uint8_t portId);

/* Summary 
 * Deinitializes the card manager.
 * Description 
 * Deinitializes the card manager for the specified port.
 *
 * Parameters
 * port_id: indicates the storage port to which card is connected.
 *
 * Return value
 * None
 */
extern void
CyU3PCardMgrDeInit (
                    uint8_t portId);

/* Summary
 * Function to set the sib clock frequency.
 *
 * Description
 * This function is used to set the sib core clock.
 *
 * Parameters
 * portId       : indicates the core clock which has to be modified.
 * clkDivider   : the new clock divider value
 * 
 * Return value:
 * None
 */
void
CyU3PCardMgrSetClockFreq (
                          uint8_t portId,
                          uint16_t clkDivider);

/* Summary
 * Initiate a write data request.
 *
 * Description
 * Initiates a data write request by setting up the sib registers.
 *
 * Parameters
 * portId       : indicates the storage port to which card is connected.
 * unit         : Indicates the logical unit number to which the write is to be done.
 * socket       : Indicates the s-port socket to be used for this transaction.
 * numReadBlks  : Number of blocks to be written.
 * blkAddr      : Start address of the write operation
 * Return value:
 * CY_U3P_SUCCESS
 * CY_U3P_ERROR_INVALID_ADDR
 */
extern CyU3PReturnStatus_t
CyU3PCardMgrSetupWrite (
                        uint8_t portId, 
                        uint8_t unit,
                        uint8_t socket,
                        uint16_t numWriteBlks,
                        uint32_t blkAddr);

/* Summary
 * Initiate a read data request.
 *
 * Description
 * Initiates a data read request by setting up the sib registers.
 *
 * Parameters
 * portId       : indicates the storage port to which card is connected.
 * unit         : Indicates the logical unit number from which the data is to be read. 
 * socket       : Indicates the s-port socket to be used for this transaction.
 * numReadBlks  : Number of blocks to be read.
 * blkAddr      : Start address of the read operation
 * Return value:
 * CY_U3P_SUCCESS
 * CY_U3P_ERROR_INVALID_ADDR
 */
extern CyU3PReturnStatus_t
CyU3PCardMgrSetupRead (
                       uint8_t portId, 
                       uint8_t unit,
                       uint8_t socket,
                       uint16_t numReadBlks,
                       uint32_t blkAddr);

extern CyU3PReturnStatus_t
CyU3PCardMgrContinueReadWrite (
        uint8_t  isRead,                /* CyTrue = Read operation, CyFalse = Write operation. */
        uint8_t  portId,                /* Port to do the operation on. */
        uint8_t  socket,                /* Socket to use for data transfer. */
        uint16_t numBlks                /* Number of blocks (sectors) to transfer. */
        );

/* Summary
 * Initiate a stop transmission command request.
 *
 * Description
 * This function is used to send a stop transmission command to the card to
 * terminate the data flow.
 *
 * Parameters
 * portId       : indicates the storage port to which card is connected.
 *
 * Return value:
 * CY_U3P_SUCCESS
 * CY_U3P_ERROR_TIMEOUT
 */
extern CyU3PReturnStatus_t
CyU3PCardMgrStopTransfer (
                          uint8_t portId );

/* Summary
   Read the CSD register content for a specified SD/MMC device.
 */
void
CyU3PCardMgrGetCSD (
        uint8_t  portId,                /* Port number. */
        uint8_t *csd_buffer             /* Return buffer to be filled with CSD data. */
        );


/* Summary
   Read the Extended CSD register from an MMC card.
 */
extern CyU3PReturnStatus_t
CyU3PCardMgrReadExtCsd (
        uint8_t  portId,                /* Port number. */
        uint8_t *buffer_p               /* Buffer to read the register data into. Should be able to hold 512 bytes. */
        );

/* Summary
 * Function to send a SD/MMC Command to the card.
 *
 * Description
 * This function sends the SD/MMC command to the card by setting up
 * the corresponding registers in the SIB block.
 *
 * Parameters
 * portId       : Indicates the storage port to which card is connected.
 * cmd          : Command to be sent.
 * respLen      : Length in bytes of the response to this command.
 * cmdArg       : Argument to the command.
 * flags        : Additional information to the command being sent.
 * Return value : None
 */
extern void
CyU3PCardMgrSendCmd (
                     uint8_t portId,
                     uint8_t cmd,
                     uint8_t respLen,
                     uint32_t cmdArg,
                     uint32_t flags );

/* Summary
 * Function to wait for a SIB related interrupt.
 *
 * Description
 * This function is used to wait for a SIB related completion interrupt, or until
 * an error or timeout is detected.
 *
 * Parameters
 * portId       : Indicates the storage port to which card is connected.
 * intr         : Type of interrupt to wait for.
 * timeout      : Timeout for the polling loop. A delay of 5us is applied in each loop iteration.
 *
 * Return value
 * CY_U3P_SUCCESS
 * CY_U3P_ERROR_CRC
 * CY_U3P_ERROR_TIMEOUT
 */
extern CyU3PReturnStatus_t
CyU3PCardMgrWaitForInterrupt (
        uint8_t  portId,
        uint32_t intr,
        uint32_t timeout);

/* Wrapper macro to wait for a storage device response. */
#define CyU3PCardMgrWaitForCmdResp(port)        \
    (CyU3PCardMgrWaitForInterrupt((port),CY_U3P_SIB_RCVDRES,0x3FFFF))

/* Wrapper macro to wait for data block transfer completion. */
#define CyU3PCardMgrWaitForBlkXfer(port,intr)   \
    (CyU3PCardMgrWaitForInterrupt((port),(intr),glCardCtxt[(port)].dataTimeOut))

/* Summary
 * Function to check if the card is ready to accept the data.
 *
 * Description
 * This function is used to check if the card is ready to accept the data.
 *
 * Parameters
 * portId       : Indicates the storage port to which card is connected.
 *
 * Return value : 
 * CY_U3P_SUCCESS
 * CY_U3P_ERROR_TIMEOUT
 */
extern CyU3PReturnStatus_t
CyU3PCardMgrCheckStatus (
                         uint8_t portId );

/* Summary
   Complete the initialization of an SD card after it has been unlocked.
 */
extern CyU3PReturnStatus_t
CyU3PCardMgrCompleteSDInit (
        uint8_t portId                  /* Port on which the card is connected. */
        );

/* Define Benicia SDIO methods. */
#ifdef CYU3P_STORAGE_SDIO_SUPPORT

/* Summary
 * Function enables or disables the interrupt for the specified IO function. 
 *
 * Description
 * This function is used enable or disable the interrupt of the IO function
 *
 * Parameters
 * portId        : Indicates the storage port to which card is connected.
 * funcNo        : Function number of the sdio 
 * enable	 : Enable or disble interrupt
 * 
 * Return value  : 
 * CY_U3P_SUCCESS
 * CY_U3P_ERROR_BAD_ARGUMENT
 */
extern CyU3PReturnStatus_t
CyU3PSdioGlobalInterruptControl (
        uint8_t portId,
        uint8_t funcNo,
        CyBool_t enable);

/* Summary
 * Function initialises the sdio function. 
 *
 * Description
 * This function is initialises the sdio function.
 *
 * Parameters
 * portId         : Indicates the storage port to which card is connected. 
 * funcNo	  : Function number of the sdio to be sent
 * 
 * Return value   : 
 * CY_U3P_SUCCESS
 * CY_U3P_ERROR_BAD_ARGUMENT
 * CY_U3P_ERROR_TIMEOUT
 */
extern CyU3PReturnStatus_t 
CyU3PSdioFunctionInit (
        uint8_t portId,
        uint8_t funcNo);

/* Summary
 * Function de-initialises the specified sdio function. 
 *
 * Description
 * This function is de-initialises the specified sdio function.
 *
 * Parameters
 * portId         : Indicates the storage port to which card is connected.
 * funcNo	  : Function number of the sdio to be sent
 * 
 * Return value : 
 * CY_U3P_SUCCESS
 * CY_U3P_ERROR_BAD_ARGUMENT
 */
extern CyU3PReturnStatus_t
CyU3PCardMgrSdioFuncDeInit (
        uint8_t portId,
        uint8_t funcNo);

#endif /* CYU3P_STORAGE_SDIO_SUPPORT */

#include "cyu3externcend.h"

#endif /* _INCLUDED_CYU3CARDMGR_H_ */

/*[]*/

