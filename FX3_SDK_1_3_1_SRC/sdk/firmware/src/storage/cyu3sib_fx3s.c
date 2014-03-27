/*
 ## Cypress USB 3.0 Platform source file (cyu3sib_fx3s.c)
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

#include <cyu3regs.h>
#include <cyu3sib.h>
#include <cyu3sibpp.h>
#include <cyu3cardmgr.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3utils.h>
#include <cyu3error.h>
#include <cyu3protocol.h>
#include <cyu3vic.h>
#include <cyu3gpio.h>
#include <cyfx3_api.h>

typedef void (*CyU3PStorDriverFunction) (void);
extern void
CyU3PStorRegisterDriverEntry (
        CyU3PStorDriverFunction entry_p);

/* Summary:
   This file implements the Storage module functions. It provides wrapper functions
   for the card driver functionality.
 */

/* Retry values for the create and the erase partition functions. */
#define CY_U3P_SIB_PARTITION_RETRY_COUNT            (2)
#define CY_U3P_SIB_DMA_BUF_COUNT                    (2)

#define CY_U3P_SIB_SET_PWD                          (0x01)
#define CY_U3P_SIB_CLR_PWD                          (0x02)
#define CY_U3P_SIB_LOCK_CARD                        (0x04)
#define CY_U3P_SIB_FORCE_ERASE                      (0x08)

#define CY_U3P_SIB_CARD_LOCKED_BIT                  (1 << 25)
#define CY_U3P_SIB_CARD_LOCK_UNLOCK_STATUS_BIT      (1 << 24)

/************************************************************************************/
/********************************GLOBAL VARIABLES************************************/
/************************************************************************************/
extern CyU3PThread   glSibThread;               /* SIB Thread Handle */
extern CyU3PEvent    glSibEvent;                /* SIB Event Handle */
extern CyU3PSibIntfParams_t glSibIntfParams[];  /* Interface Control parameters */

CyU3PSibGlobalData   glSibDevInfo = {
    CyFalse,
    CyFalse,
    CyFalse,
    0,

    {1,                        1},
    {0,                        0},
    {0,                        0},
    {CyFalse,                  CyFalse},
    {CY_U3P_SIB_LOCATION_USER, CY_U3P_SIB_LOCATION_USER},
    {0,                        0},
    {0}
};

CyU3PSibCtxt_t       glSibCtxt[CY_U3P_SIB_NUM_PORTS];           /* SIB Context information for the ports */
CyU3PSibLunInfo_t    glSibLunInfo[CY_U3P_SIB_NUM_PORTS][CY_U3P_SIB_NUM_PARTITIONS];  /* Logical Unit information. */

/* A global buffer of size 512 bytes used for read/write during the initialization of the card. */
uint8_t  glSibBuffer[CY_U3P_SIB_BLOCK_SIZE]  __attribute__ ((aligned (32))) = {0};
/* This function is used to commit a pending read or a write transaction. */
CyU3PReturnStatus_t
CyU3PSibCommitReadWrite (
        uint8_t portId)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    if (!CyU3PIsSibActive ())
        return CY_U3P_ERROR_NOT_STARTED;
    if (portId > 1)
        return CY_U3P_ERROR_BAD_ARGUMENT;
    if ((glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_SD) && (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_MMC))
        return CY_U3P_ERROR_INVALID_DEV;

    SIB->sdmmc[portId].cs &= (~CY_U3P_SIB_SDMMC_CLK_DIS);

    if (CyU3PSibIsWrCommitPending (portId))
    {
        status = CyU3PCardMgrStopTransfer (portId);
        CyU3PSibClearWrCommitPending (portId);
        CyU3PSibSetOpenWrSize (portId, 0);
    }

    return status;
}

/* Summary:
   This function writes the Lock data structure with the appropriate
   fields set to the SD Card.
 */
static CyU3PReturnStatus_t
CyU3PSibWriteLockData (
        uint8_t portId,
        uint8_t *buffer,
        uint8_t len
        )
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    CyU3PDmaBuffer_t dmaBuffer;

    /* Set the block length bytes. */
    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD16_SET_BLOCKLEN,
            CY_U3P_SD_MMC_R1_RESP_BITS, len, 0);

    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    CyU3PSibSetBlockLen(portId, len);
    CyU3PSibSetNumBlocks(portId, 1);

    CyU3PSibSetActiveSocket(portId, CYU3P_SIB_INT_WRITE_SOCKET);

    /* Setup DMA for sending the data before sending the command. */
    dmaBuffer.buffer = buffer;
    dmaBuffer.size   = CY_U3P_SIB_BLOCK_SIZE;
    dmaBuffer.count  = len;
    dmaBuffer.status = 0;

    status = CyU3PDmaChannelSetupSendBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);

    /* Wait for the card to be in the ready state. */
    while ((SIB->sdmmc[portId].status & CY_U3P_SIB_DAT0_STAT) == 0);

    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD42_LOCK_UNLOCK, CY_U3P_SD_MMC_R1_RESP_BITS, 0, 0);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Write the block to the card */
    SIB->sdmmc[portId].cs |= CY_U3P_SIB_WRDCARD;
    status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCK_COMP);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);
        CyU3PSibResetSibCtrlr (portId);
    }

    return status;
}

CyU3PReturnStatus_t
CyU3PSibGetDeviceLock (
        uint8_t portId)
{
    CyU3PMutexGet (&glSibCtxt[portId].mutexLock, CYU3P_WAIT_FOREVER);

    if (CyU3PSibIsDeviceBusy (portId))
    {
        CyU3PMutexPut (&glSibCtxt[portId].mutexLock);
        return CY_U3P_ERROR_DEVICE_BUSY;
    }

    CyU3PSibSetDeviceBusy (portId);
    CyU3PMutexPut (&glSibCtxt[portId].mutexLock);

    return CY_U3P_SUCCESS;
}

/* Summary:
   This function fills up the lock card data structure with apt bits set
   so that the card can be locked/unlocked.
 */
CyU3PReturnStatus_t
CyU3PSibLockUnlockCard (
        uint8_t  portId,
        CyBool_t lockCard,
        CyBool_t clrPasswd,
        uint8_t *passwd,
        uint8_t  passwdLen
        )
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint8_t  *buffer = glSibBuffer;
    uint8_t   cmd = 0;
    uint32_t  temp = 0;
    uint32_t  prevIntrMask;

    if ((!passwd) || (!passwdLen))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE)
    {
        return CY_U3P_ERROR_INVALID_DEV;
    }

    if (lockCard)
    {
        if (glCardCtxt[portId].locked)
        {
            return CY_U3P_SUCCESS;
        }

        cmd = CY_U3P_SIB_LOCK_CARD;
    }
    else
    {
        if (clrPasswd)
        {
            cmd = CY_U3P_SIB_CLR_PWD;
        }
        else
        {
            if (!glCardCtxt[portId].locked)
            {
                return CY_U3P_SUCCESS;
            }
        }
    }

    if (CyU3PSibGetDeviceLock (portId) != CY_U3P_SUCCESS)
        return CY_U3P_ERROR_DEVICE_BUSY;

    /* Make sure there are no open write operations. */
    CyU3PSibCommitReadWrite (portId);

    /* Disable all interrupts from the S port until this transfer is complete. */
    prevIntrMask = SIB->sdmmc[portId].intr_mask;
    SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_SDMMC_INTR_MASK_DEFAULT;
    CyU3PSibClearIntr (portId);

    buffer[0] = cmd;
    buffer[1] = passwdLen;
    CyU3PMemCopy (&buffer[2], passwd, passwdLen);

    status = CyU3PSibWriteLockData (portId, buffer, (passwdLen + 2));
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PCardMgrCheckStatus (portId);
        if (status == CY_U3P_SUCCESS)
        {
            temp = SIB->sdmmc[portId].resp_reg0;
            if (temp & CY_U3P_SIB_CARD_LOCK_UNLOCK_STATUS_BIT)
            {
                status = CY_U3P_ERROR_CARD_LOCK_FAILURE;
                goto Exit;
            }

            if (lockCard)
            {
                if (!(temp & CY_U3P_SIB_CARD_LOCKED_BIT))
                {
                    status = CY_U3P_ERROR_CARD_LOCK_FAILURE;
                }
                else
                {
                    glCardCtxt[portId].locked = 1;
                }
            }
            else
            {
                if (temp & CY_U3P_SIB_CARD_LOCKED_BIT)
                {
                    status = CY_U3P_ERROR_CARD_LOCK_FAILURE;
                }
                else
                {
                    glCardCtxt[portId].locked = 0;
                    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_SD)
                        status = CyU3PCardMgrCompleteSDInit (portId);
                }
            }
        }
    }

Exit:
    /* Re-enable necessary interrupts. */
    CyU3PSibClearIntr (portId);
    SIB->sdmmc[portId].intr_mask = prevIntrMask;
    CyU3PSibClearDeviceBusy (portId);

    return status;
}

/* Summary:
   This function sets the password for the specified SD Card with an option to
   replace the password as well as lock the card while setting the password.
 */
CyU3PReturnStatus_t
CyU3PSibSetPasswd (
        uint8_t  portId,
        CyBool_t lockCard,
        uint8_t *passwd,
        uint8_t  passwdLen,
        uint8_t *newPasswd,
        uint8_t  newPasswdLen)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint8_t *buffer = glSibBuffer;
    uint32_t temp = 0;
    uint32_t prevIntrMask;

    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE)
    {
        return CY_U3P_ERROR_INVALID_DEV;
    }

    if (((passwdLen != 0) && (passwd == 0)) || ((newPasswdLen != 0) && (newPasswd == 0)))
        return CY_U3P_ERROR_BAD_ARGUMENT;

    if (CyU3PSibGetDeviceLock (portId) != CY_U3P_SUCCESS)
        return CY_U3P_ERROR_DEVICE_BUSY;

    /* Make sure there are no open write operations. */
    CyU3PSibCommitReadWrite (portId);

    /* Disable all interrupts from the S port until this transfer is complete. */
    prevIntrMask = SIB->sdmmc[portId].intr_mask;
    SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_SDMMC_INTR_MASK_DEFAULT;
    CyU3PSibClearIntr (portId);

    if (passwdLen)
    {
        buffer[1] = passwdLen;
        CyU3PMemCopy (&buffer[2], passwd, passwdLen);
    }

    if (newPasswdLen)
    {
        buffer[1] += newPasswdLen;
        CyU3PMemCopy (&buffer[2+passwdLen], newPasswd, newPasswdLen);
    }

    buffer[0] = CY_U3P_SIB_SET_PWD;
    if (lockCard)
    {
        buffer[0] |= CY_U3P_SIB_LOCK_CARD;
    }

    status = CyU3PSibWriteLockData (portId, buffer, (2 + passwdLen + newPasswdLen));
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PCardMgrCheckStatus (portId);
        if (status == CY_U3P_SUCCESS)
        {
            temp = SIB->sdmmc[portId].resp_reg0;

            if (lockCard)
            {
                if (!(temp & CY_U3P_SIB_CARD_LOCKED_BIT))
                {
                    status = CY_U3P_ERROR_CARD_LOCK_FAILURE;
                }
                else
                {
                    glCardCtxt[portId].locked = 1;
                }
            }

            if (temp & CY_U3P_SIB_CARD_LOCK_UNLOCK_STATUS_BIT)
            {
                status = CY_U3P_ERROR_CARD_LOCK_FAILURE;
            }
        }
    }
    else
        status = CY_U3P_ERROR_CARD_LOCK_FAILURE;

    /* Re-enable necessary interrupts. */
    CyU3PSibClearIntr (portId);
    SIB->sdmmc[portId].intr_mask = prevIntrMask;
    CyU3PSibClearDeviceBusy (portId);

    return status;
}

/* Summary:
   This function removes the password for the specified SD Card.
 */
CyU3PReturnStatus_t
CyU3PSibRemovePasswd (
        uint8_t portId,
        uint8_t *passwd,
        uint8_t passwdLen
        )
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint8_t *buffer = glSibBuffer;
    uint32_t prevIntrMask;

    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE)
    {
        return CY_U3P_ERROR_INVALID_DEV;
    }

    if (!passwdLen || !passwd)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (CyU3PSibGetDeviceLock (portId) != CY_U3P_SUCCESS)
        return CY_U3P_ERROR_DEVICE_BUSY;

    /* Make sure there are no open write operations. */
    CyU3PSibCommitReadWrite (portId);

    /* Disable all interrupts from the S port until this transfer is complete. */
    prevIntrMask = SIB->sdmmc[portId].intr_mask;
    SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_SDMMC_INTR_MASK_DEFAULT;
    CyU3PSibClearIntr (portId);

    buffer[0] = CY_U3P_SIB_CLR_PWD;
    buffer[1] = passwdLen;
    CyU3PMemCopy (&buffer[2], passwd, passwdLen);

    status = CyU3PSibWriteLockData (portId, buffer, (2 + passwdLen));
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PCardMgrCheckStatus (portId);
        if (status == CY_U3P_SUCCESS)
        {
            if (SIB->sdmmc[portId].resp_reg0 & CY_U3P_SIB_CARD_LOCK_UNLOCK_STATUS_BIT)
            {
                status = CY_U3P_ERROR_CARD_LOCK_FAILURE;
            }
            else
            {
                glCardCtxt[portId].locked = 0;
                if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_SD)
                    status = CyU3PCardMgrCompleteSDInit (portId);
            }
        }
    }
    else
        status = CY_U3P_ERROR_CARD_LOCK_FAILURE;

    /* Re-enable necessary interrupts. */
    CyU3PSibClearIntr (portId);
    SIB->sdmmc[portId].intr_mask = prevIntrMask;
    CyU3PSibClearDeviceBusy (portId);

    return status;
}

/* Summary:
   This function force erases the user content of the SD Card specified.
   The force erase would fail if the card's permanent write protect bit
   is set or if the card is unlocked.
   If force erase is successful then the card's temporaray write
   protect and the group write protect bits will be cleared and also the
   card will be unlocked.
 */
CyU3PReturnStatus_t
CyU3PSibForceErase (
        uint8_t portId
        )
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint8_t *buffer = glSibBuffer;
    uint32_t prevIntrMask;

    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE)
    {
        return CY_U3P_ERROR_INVALID_DEV;
    }

    if (CyU3PSibGetDeviceLock (portId) != CY_U3P_SUCCESS)
        return CY_U3P_ERROR_DEVICE_BUSY;

    /* Make sure there are no open write operations. */
    CyU3PSibCommitReadWrite (portId);

    /* Disable all interrupts from the S port until this transfer is complete. */
    prevIntrMask = SIB->sdmmc[portId].intr_mask;
    SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_SDMMC_INTR_MASK_DEFAULT;
    CyU3PSibClearIntr (portId);

    buffer[0] = CY_U3P_SIB_FORCE_ERASE;
    status = CyU3PSibWriteLockData (portId, buffer, 1);
    if (status == CY_U3P_SUCCESS)
    {
        while ((SIB->sdmmc[portId].status & CY_U3P_SIB_DAT0_STAT) == 0);
        status = CyU3PCardMgrCheckStatus (portId);
        if (status == CY_U3P_SUCCESS)
        {
            if (SIB->sdmmc[portId].resp_reg0 & CY_U3P_SIB_CARD_LOCK_UNLOCK_STATUS_BIT)
            {
                status = CY_U3P_ERROR_CARD_FORCE_ERASE;
            }
            else
            {
                glCardCtxt[portId].locked = 0;
                if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_SD)
                    status = CyU3PCardMgrCompleteSDInit (portId);
            }
        }
    }
    else
        status = CY_U3P_ERROR_CARD_FORCE_ERASE;

    /* Re-enable necessary interrupts. */
    CyU3PSibClearIntr (portId);
    SIB->sdmmc[portId].intr_mask = prevIntrMask;
    CyU3PSibClearDeviceBusy (portId);

    return status;
}

static CyU3PReturnStatus_t
DoSwitchCommand (
        uint8_t   portId,
        uint32_t  argument,
        uint32_t *resp_p)
{
    CyU3PReturnStatus_t status;
    uint8_t loc;

    /* Set the block length bytes. */
    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD6_SWITCH, CY_U3P_SD_MMC_R1B_RESP_BITS, argument, 0);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if ((status == CY_U3P_SUCCESS) && (resp_p != 0))
    {
        *resp_p = SIB->sdmmc[portId].resp_reg0;
    }

    /* If the PARTITION_CONFIG byte is being written, update the active partition information. */
    if ((argument & CY_U3P_MMC_SW_PARTCFG_USER_PARAM) == CY_U3P_MMC_SW_PARTCFG_USER_PARAM)
    {
        loc = ((argument >> 8) & 0x07);
        if (loc >= 4)
            loc = 0;
        CyU3PSibSetActivePartition (portId, (CyU3PSibLunLocation)loc);
    }

    /* Wait until the device stops signalling busy condition. */
    while ((SIB->sdmmc[portId].status & CY_U3P_SIB_DAT0_STAT) == 0);
    return status;
}

/* This function is used to erase the specified area of an SD Card. */
CyU3PReturnStatus_t
CyU3PSibEraseBlocks (
        uint8_t           portId,
        uint16_t          numEraseUnits,
        uint32_t          startAddr,
        CyU3PSibEraseMode mode)
{
    uint32_t endAddr;
    uint32_t lastAddr;
    uint32_t eraseSize;
    uint32_t eraseArg = 0;
    CyU3PReturnStatus_t status = CY_U3P_ERROR_INVALID_DEV;
    uint32_t tmp = CyU3PSibGetPartitionConfig(portId);

    uint8_t startCmd = CY_U3P_SD_CMD32_ERASE_WR_BLK_START;
    uint8_t stopCmd  = CY_U3P_SD_CMD33_ERASE_WR_BLK_END;

    if (!CyU3PIsSibActive ())
        return CY_U3P_ERROR_NOT_STARTED;
    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE)
        return CY_U3P_ERROR_INVALID_DEV;
    eraseSize = glCardCtxt[portId].eraseSize;
    if (eraseSize == 0)
        return CY_U3P_ERROR_NOT_SUPPORTED;

    lastAddr = glCardCtxt[portId].numBlks;
    CyU3PSibConvertAddr(portId, lastAddr);

    if (glCardCtxt[portId].highCapacity)
        eraseSize = CY_U3P_SIB_BYTECNT_TO_SECTORCNT (eraseSize);

    startAddr *= eraseSize;
    endAddr = startAddr + (numEraseUnits * eraseSize) - 1;

    /* Address validity check */
    if ((startAddr > lastAddr) || (endAddr > lastAddr))
    {
        status = CY_U3P_ERROR_INVALID_ADDR;
    }
    else
    {
        if (CyU3PSibGetDeviceLock (portId) != CY_U3P_SUCCESS)
            return CY_U3P_ERROR_DEVICE_BUSY;

        /* Make sure there are no open write operations. */
        CyU3PSibCommitReadWrite (portId);

        if (CY_U3P_SIB_LOCATION_USER != CyU3PSibGetActivePartition(portId))
        {
            status = DoSwitchCommand (portId, CY_U3P_MMC_SW_PARTCFG_USER_PARAM | tmp, 0);
            if (status != CY_U3P_SUCCESS)
            {
                CyU3PSibClearDeviceBusy (portId);
                return status;
            }

            CyU3PSibSetActivePartition (portId, CY_U3P_SIB_LOCATION_USER);
        }

        if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_MMC)
        {
            startCmd = CY_U3P_MMC_CMD35_ERASE_GROUP_START;
            stopCmd  = CY_U3P_MMC_CMD36_ERASE_GROUP_END;

            switch (mode)
            {
                case CY_U3P_SIB_ERASE_SECURE:
                    eraseArg = 0x80000000;
                    break;
                case CY_U3P_SIB_ERASE_TRIM_STEP1:
                    eraseArg = 0x80000001;
                    break;
                case CY_U3P_SIB_ERASE_TRIM_STEP2:
                    eraseArg = 0x80008000;
                    break;
                default:
                    break;
            }
        }

        if (eraseArg == 0x80008000)
        {
            /* Only erase command is to be used in case of Secure TRIM STEP2. */
            CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD38_ERASE, CY_U3P_SD_MMC_R1B_RESP_BITS, eraseArg, 0);
            status = CyU3PCardMgrWaitForCmdResp (portId);
            if (status == CY_U3P_SUCCESS)
            {
                /* Wait for the erase operation to complete. */
                status = CyU3PCardMgrCheckStatus (portId);
            }
        }
        else
        {
            /* Send start command and wait for a response. */
            CyU3PCardMgrSendCmd (portId, startCmd, CY_U3P_SD_MMC_R1_RESP_BITS, startAddr, 0);
            status = CyU3PCardMgrWaitForCmdResp (portId);
            if (status == CY_U3P_SUCCESS)
            {
                /* Send stop command and wait for a response. */
                CyU3PCardMgrSendCmd (portId, stopCmd, CY_U3P_SD_MMC_R1_RESP_BITS, endAddr, 0);
                status = CyU3PCardMgrWaitForCmdResp (portId);
                if (status == CY_U3P_SUCCESS)
                {
                    /* Initiate the write operation and wait for a response. */
                    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD38_ERASE, CY_U3P_SD_MMC_R1B_RESP_BITS, eraseArg, 0);
                    status = CyU3PCardMgrWaitForCmdResp (portId);
                    if (status == CY_U3P_SUCCESS)
                    {
                        /* No errors detected. Wait for the erase operation to complete. */
                        status = CyU3PCardMgrCheckStatus (portId);
                    }
                }
            }
        }

        CyU3PSibClearDeviceBusy (portId);
    }

    return status;
}

/* This function is used to re-initialize the card. */
CyU3PReturnStatus_t
CyU3PSibReInitCard (
                    uint8_t portId)
{
    CyU3PReturnStatus_t status;

    CyU3PSibCommitReadWrite (portId);
    CyU3PCardMgrDeInit (portId);

    status = CyU3PSibInitCard (portId);
    return status;
}

/* This function is used to abort an ongoing read or a write transaction. */
CyU3PReturnStatus_t
CyU3PSibAbortTransaction (
        uint8_t portId)
{
    uint32_t tmp = 0;

    if (CyU3PSibIsDeviceBusy (portId))
    {
        CyU3PTimerStop (&glSibCtxt[portId].writeTimer);

        /* Reset the SIB to ensure that we can send the next command to the device. */
	CyU3PSibResetSibCtrlr (portId);

        /* Make sure a stop command is issued to the storage device. */
        CyU3PSibSetWrCommitPending (portId);
        CyU3PSibCommitReadWrite (portId);

        /* Sleep for about a millisecond */
        CyU3PThreadSleep (1);

        tmp = (SIB->sdmmc[portId].cs & CY_U3P_SIB_CS_REG_MASK);
        if (glSibCtxt[portId].isRead)
        {
            tmp |= CY_U3P_SIB_CLR_RDDCARD;
        }
        else
        {
            tmp |= CY_U3P_SIB_CLR_WRDCARD;
        }

        /* Clear the RDDCARD or WRDCARD bit if set. */
        SIB->sdmmc[portId].cs = tmp;

        /* Reset the SIB Controller. */
        CyU3PSibResetSibCtrlr (portId);
        CyU3PSibClearDeviceBusy (portId);
    }

    return CY_U3P_SUCCESS;
}

/* This function is used to abort any ongoing transactions on the specified port */
CyU3PReturnStatus_t
CyU3PSibAbortRequest (
        uint8_t portId)
{
    uint32_t flags = CY_U3P_SIB_EVT_ABORT;

    /* Check if any transfers are happening. */
    if (CyU3PSibIsDeviceBusy (portId))
    {
        CyU3PSibAbortTransaction (portId);

        if (portId == CY_U3P_SIB_PORT_0)
        {
            flags |= CY_U3P_SIB_EVT_PORT_0;
        }
        else
        {
            flags |= CY_U3P_SIB_EVT_PORT_1;
        }

        /* Send an Abort event to the storage thread. */
        CyU3PEventSet (&glSibEvent, flags, CYU3P_EVENT_OR);
    }

    return CY_U3P_SUCCESS;
}

/* Function to register the callback for the storage events. */
CyU3PReturnStatus_t
CyU3PSibRegisterCbk (
        CyU3PSibEvtCbk_t sibEvtCbk )
{
    CyU3PReturnStatus_t status = CY_U3P_ERROR_BAD_ARGUMENT;

    /* Validate the parameter received. */
    if (sibEvtCbk != NULL)
    {
        CyU3PSibSetEventCallback (sibEvtCbk);
        status = CY_U3P_SUCCESS;
    }

    return status;
}

/* This function validates the given address. This function checks if the given address lies within the
* start and end addresses of the unit. */
CyU3PReturnStatus_t
CyU3PSibIsAddrValid (
                     uint8_t portId,
                     uint8_t unitId,
                     uint16_t numBlocks,
                     uint32_t blkAddr)
{
    CyU3PReturnStatus_t status = CY_U3P_ERROR_INVALID_DEV;
    uint32_t endAddr;

    if (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_NONE)
    {
        if ((unitId < CY_U3P_SIB_NUM_PARTITIONS) && (CyU3PSibLunIsValid (portId, unitId)))
        {
            /* The block address should lie within the unit start and end address. Block start address + number
            of blocks should lie within the unit end address.
            */
            endAddr = CyU3PSibLunGetStartAddr (portId, unitId) + CyU3PSibLunGetNumBlocks (portId, unitId);
            if ((blkAddr >= CyU3PSibLunGetStartAddr(portId,unitId)) && ((blkAddr + numBlocks) <= endAddr))
            {
                status = CY_U3P_SUCCESS;
            }
            else
            {
                status = CY_U3P_ERROR_INVALID_ADDR;
            }
        }
        else
        {
            /* Invalid unit */
            status = CY_U3P_ERROR_INVALID_UNIT;
        }
    }

    return status;
}

/* This function updates the LUN structures with the information provided in the parameters. */
void
CyU3PSibUpdateLunInfo (
        uint8_t  portId,
        uint8_t  partNum,
        uint8_t  partType,
        uint8_t  location,
        uint32_t startAddr,
        uint32_t partSize)
{
    CyU3PSibLunInfo_t *pLunInfo = &glSibLunInfo[portId][partNum];

    pLunInfo->valid     = CyTrue;
    pLunInfo->startAddr = startAddr;
    pLunInfo->numBlocks = partSize;
    pLunInfo->blockSize = CY_U3P_SIB_BLOCK_SIZE;
    pLunInfo->location  = (CyU3PSibLunLocation)location;
    pLunInfo->type      = (CyU3PSibLunType)partType;
    if (glCardCtxt[portId].writeable)
        pLunInfo->writeable = 1;
    else
        pLunInfo->writeable = 0;
}


static CyU3PReturnStatus_t
CyU3PSibReadOneDataSector (
        uint8_t  portId,
        uint32_t address,
        uint8_t * buffer)
{
    CyU3PReturnStatus_t status;
    CyU3PDmaBuffer_t dmaBuffer;
    uint32_t param, intMask;

    status = CyU3PCardMgrCheckStatus (portId);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);
    CyU3PSibCommitReadWrite (portId);

    /* Mask out DAT3_CHANGE interrupt while doing a read operation. */
    intMask = SIB->sdmmc[portId].intr_mask;
    SIB->sdmmc[portId].intr_mask &= (~(CY_U3P_SIB_DAT3_CHANGE | CY_U3P_SIB_BLOCKS_RECEIVED));

    dmaBuffer.buffer = buffer;
    dmaBuffer.size   = CY_U3P_SIB_BLOCK_SIZE;
    dmaBuffer.count  = 0;
    dmaBuffer.status = 0;
    status = CyU3PDmaChannelSetupRecvBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);
        goto exitRead;
    }

    param = address;
    if (glCardCtxt[portId].highCapacity == CyFalse)
        param *= CY_U3P_SIB_BLOCK_SIZE;

    /* Wait for the card to come out of busy state. */
    while ((SIB->sdmmc[portId].status & CY_U3P_SIB_DAT0_STAT) == 0);
    CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD17_READ_SINGLE_BLOCK, CY_U3P_SD_MMC_R1_RESP_BITS, param,
            CY_U3P_SIB_RDDCARD);
    status = CyU3PCardMgrWaitForCmdResp (portId);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);
        goto exitRead;
    }

    /* Wait for the block received interrupt */
    status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCKS_RECEIVED);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);
        CyU3PSibResetSibCtrlr (portId);
    }

exitRead:
    CyU3PSibClearIntr (portId);
    SIB->sdmmc[portId].intr_mask = intMask;
    return status;
}

CyU3PReturnStatus_t
CyU3PSibReadOneDataSectorWrapper (
        uint8_t  portId,
        uint32_t blkAddr,
        uint8_t *buffer)
{

    CyU3PReturnStatus_t status;
    uint32_t prevIntrMask = 0;

    /* Disable all the interrupts */
    prevIntrMask = SIB->sdmmc[portId].intr_mask;
    SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_SDMMC_INTR_MASK_DEFAULT;
    CyU3PSibClearIntr (portId);
    
    status = CyU3PSibReadOneDataSector (portId, blkAddr, buffer);

    CyU3PSibClearIntr (portId);
    SIB->sdmmc[portId].intr_mask = prevIntrMask;

    return status;
}


static void
CyU3PSibRegisterMmcBootPartitions (
        uint8_t portId)
{
    CyU3PReturnStatus_t status;
    uint32_t size;
    uint8_t  mmc_vers, lun_idx = 0;
    uint32_t tmp;

    CyU3PCardMgrReadExtCsd (portId, glSibBuffer);

    mmc_vers = glSibBuffer[CY_U3P_MMC_ECSD_REVISION_LOC];
    if (mmc_vers < 3)
    {
        /* No boot partitions supported until MMC version 4.2. */
        return;
    }

    tmp = ((uint32_t)glSibBuffer[CY_U3P_MMC_ECSD_PARTCFG_LOC] << 8) & 0xF800;
    CyU3PSibSetPartitionConfig (portId, tmp);

    size = (uint32_t)glSibBuffer[CY_U3P_MMC_ECSD_BOOTSIZE_LOC];
    if (size != 0)
    {
        size = size * 128 * 2;              /* Convert to number of sectors. */

        /* Switch access to boot partition 1. */
        status = DoSwitchCommand (portId, (CY_U3P_MMC_SW_PARTCFG_BOOT1_PARAM | tmp), 0);
        if (status == CY_U3P_SUCCESS)
        {
            status = CyU3PSibReadOneDataSector (portId, 0, glSibBuffer);
            if (status == CY_U3P_SUCCESS)
            {
                if ( (((CyU3PSibGetPartitionConfig(portId) >> 8) & 0x38) == 0x08) && (glSibBuffer[0] == 'C') && (glSibBuffer[1] == 'Y'))
                    CyU3PSibUpdateLunInfo (portId, lun_idx, CY_U3P_SIB_LUN_BOOT, CY_U3P_SIB_LOCATION_BOOT1, 0, size);
                else
                    CyU3PSibUpdateLunInfo (portId, lun_idx, CY_U3P_SIB_LUN_DATA, CY_U3P_SIB_LOCATION_BOOT1, 0, size);

                glSibCtxt[portId].numBootLuns++;
                lun_idx++;
            }
        }

        /* Switch access to boot partition 2. */
        status = DoSwitchCommand (portId, (CY_U3P_MMC_SW_PARTCFG_BOOT2_PARAM | tmp), 0);
        if (status == CY_U3P_SUCCESS)
        {
            status = CyU3PSibReadOneDataSector (portId, 0, glSibBuffer);
            if (status == CY_U3P_SUCCESS)
            {
                if ((((CyU3PSibGetPartitionConfig(portId) >> 8) & 0x38) == 0x10) && (glSibBuffer[0] == 'C') && (glSibBuffer[1] == 'Y'))
                    CyU3PSibUpdateLunInfo (portId, lun_idx, CY_U3P_SIB_LUN_BOOT, CY_U3P_SIB_LOCATION_BOOT2, 0, size);
                else
                    CyU3PSibUpdateLunInfo (portId, lun_idx, CY_U3P_SIB_LUN_DATA, CY_U3P_SIB_LOCATION_BOOT2, 0, size);

                glSibCtxt[portId].numBootLuns++;
            }
        }
    }

    /* Make sure access is switched to User data area. */
    DoSwitchCommand (portId, (CY_U3P_MMC_SW_PARTCFG_USER_PARAM | tmp), 0);
}

/* This function checks if the card is partitioned. Refer bootloader spec or the USB 3.0 IROS for details
   regarding the partition header structure.
 */
CyU3PReturnStatus_t
CyU3PSibCheckForPartition (
        uint8_t portId)
{
    uint32_t chkSum = 0;
    uint32_t temp = 0;
    uint32_t tmp = 0;
    uint8_t  blkCount;
    uint8_t  partType;

    uint32_t curAddr  = 0, startAddr = 0;
    uint32_t numBlks  = 0;
    uint8_t  lunCount = 0;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    glSibCtxt[portId].partition   = 0;
    glSibCtxt[portId].numBootLuns = 0;
    glSibCtxt[portId].numUserLuns = 0;

    /* Configure the block length and the number of blocks to be read. */
    CyU3PSibSetNumBlocks (portId, 1);
    CyU3PSibSetBlockLen (portId, CY_U3P_SIB_BLOCK_SIZE);

    /* Update the active socket number */
    CyU3PSibSetActiveSocket (portId, CYU3P_SIB_INT_READ_SOCKET);

    if (!glCardCtxt[portId].locked)
    {
        /* Check whether one or more boot partitions exist on the MMC device. */
        if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_MMC)
        {
            CyU3PSibRegisterMmcBootPartitions (portId);
            lunCount = glSibCtxt[portId].numBootLuns;
        }

        for (blkCount = 0; blkCount < CY_U3P_BOOT_PARTITION_NUM_BLKS; blkCount++)
        {
            tmp = curAddr + blkCount * 8;
            status = CyU3PSibReadOneDataSector (portId, tmp, glSibBuffer);

            /* Check if the CY signature is present. */
            temp = (*(uint32_t *)&glSibBuffer[0]);
            if (temp != CY_U3P_BOOT_PART_SIGN)
            {
                /* The Signatures don't match. Check in the next block. */
                continue;
            }

            /* Found the partition header. Compute the checksum over the partition header.
             * We are interested in the first 68 bytes of the partition header.
             * */
            CyU3PComputeChecksum ((uint32_t *)glSibBuffer, CY_U3P_SIB_VBP_CHECKSUM_LOCATION, &chkSum);

            /* Fetch this partition header's checksum value. The checksum value of the partition header
             * is a 4 byte value and is stored from the 68th byte onwards. Refer bootloader spec. for
             * further details.
             * */
            temp = (*(uint32_t *)&glSibBuffer[CY_U3P_SIB_VBP_CHECKSUM_LOCATION]);

            /* Check if the check sum matches */
            if (chkSum == temp)
            {
                if ((glSibCtxt[portId].numUserLuns + glSibCtxt[portId].numBootLuns) > CY_U3P_SIB_NUM_PARTITIONS)
                {
                    break;
                }

                /* Extract the partition size (LBA where the next partition starts). */
                temp     = (*(uint32_t *)&glSibBuffer[CY_U3P_SIB_VBP_PARTSIZE_LOCATION]);
                partType = glSibBuffer[CY_U3P_SIB_VBP_PARTTYPE_LOCATION];

                glSibCtxt[portId].numUserLuns++;

                numBlks   = temp;
                startAddr = curAddr + (CY_U3P_BOOT_PARTITION_NUM_BLKS << 3);
                CyU3PSibUpdateLunInfo (portId, lunCount++, partType, CY_U3P_SIB_LOCATION_USER, startAddr, numBlks);

                glSibCtxt[portId].partition = CyTrue;

                blkCount = 0;
                curAddr  = startAddr + numBlks;
            }
        }
    }

    if (status == CY_U3P_SUCCESS)
    {
        /* Treat the residual area on the storage device as a data partition. */
        if (curAddr < glCardCtxt[portId].numBlks)
        {
            glSibCtxt[portId].numUserLuns++;
            CyU3PSibUpdateLunInfo (portId, lunCount, CY_U3P_SIB_LUN_DATA, CY_U3P_SIB_LOCATION_USER, curAddr,
                    glCardCtxt[portId].numBlks - curAddr);
        }
    }

    return status;
}

/* Function to write the partition data to the card. In case of erase partition the buffer will be
   zeroed out otherwise it contains the partition information.
 */
static CyU3PReturnStatus_t
CyU3PSibWritePartitionData (
        uint8_t  portId,
        uint32_t blkAddr,
        uint8_t *buffer)
{
    CyU3PDmaBuffer_t dmaBuffer;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    uint32_t offset = 8;
    uint8_t  blkCount;
    
    dmaBuffer.buffer = buffer;
    dmaBuffer.size   = CY_U3P_SIB_BLOCK_SIZE;
    dmaBuffer.count  = CY_U3P_SIB_BLOCK_SIZE;
    dmaBuffer.status = 0;

    CyU3PSibSetNumBlocks (portId, 1);
    CyU3PSibSetBlockLen (portId, CY_U3P_SIB_BLOCK_SIZE);
    CyU3PSibSetActiveSocket (portId, CYU3P_SIB_INT_WRITE_SOCKET);

    if (glCardCtxt[portId].highCapacity == CyFalse)
    {
        /* Low capacity cards are byte addressed. */
        offset   = (CY_U3P_SIB_BLOCK_SIZE * 8);
        blkAddr *= CY_U3P_SIB_BLOCK_SIZE;
    }

    for (blkCount = 0; blkCount < CY_U3P_BOOT_PARTITION_NUM_BLKS; blkCount ++)
    {
        /* Wait for the card to come out of busy state. */
        while ((SIB->sdmmc[portId].status & CY_U3P_SIB_DAT0_STAT) == 0);
        CyU3PCardMgrSendCmd (portId, CY_U3P_SD_MMC_CMD24_WRITE_BLOCK, CY_U3P_SD_MMC_R1_RESP_BITS, blkAddr, 0);

        status = CyU3PCardMgrWaitForCmdResp (portId);
        if (status != CY_U3P_SUCCESS)
        {
            break;
        }

        /* Write the block to the card */
        status = CyU3PDmaChannelSetupSendBuffer (CyU3PSibGetDmaChannelHandle, &dmaBuffer);
        if (status != CY_U3P_SUCCESS)
        {
            break;
        }

        SIB->sdmmc[portId].cs |= CY_U3P_SIB_WRDCARD;

        /* Wait for the block complete interrupt */
        status = CyU3PCardMgrWaitForBlkXfer (portId, CY_U3P_SIB_BLOCK_COMP);
        if (status != CY_U3P_SUCCESS)
        {
            CyU3PSibResetSibCtrlr (portId);
            CyU3PDmaChannelReset (CyU3PSibGetDmaChannelHandle);
            break;
        }

        status = CyU3PCardMgrCheckStatus (portId);
        if (status != CY_U3P_SUCCESS)
        {
            break;
        }

        /* Point to the next address */
        blkAddr += offset;
    }

    if (status != CY_U3P_SUCCESS)
    {
        if (blkCount > 0)
        {
            /* In case of persistent failures then flag an error indicating that
             * the partition information couldn't be written/erased completely. */
            status = CY_U3P_ERROR_BAD_PARTITION;
        }
    }

    return status;
}

CyU3PReturnStatus_t
CyU3PSibWritePartitionDataWrapper (
        uint8_t  portId,
        uint32_t blkAddr,
        uint8_t *buffer)
{

    CyU3PReturnStatus_t status;
    uint32_t prevIntrMask = 0;

    /* Disable all the interrupts */
    prevIntrMask = SIB->sdmmc[portId].intr_mask;
    SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_SDMMC_INTR_MASK_DEFAULT;
    CyU3PSibClearIntr (portId);
    
    status = CyU3PSibWritePartitionData (portId, blkAddr, buffer);

    CyU3PSibClearIntr (portId);
    SIB->sdmmc[portId].intr_mask = prevIntrMask;

    return status;
}


/* This function partitions the user area of the card into two. */
CyU3PReturnStatus_t
CyU3PSibPartitionStorage (
        uint8_t   portId,
        uint8_t   partCount,
        uint32_t *partSizes_p,
        uint8_t  *partType_p)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint32_t blksReqd = 0;
    uint32_t prevIntrMask = 0;
    uint32_t startAddr = 0;
    uint32_t chkSum = 0;
    uint8_t  i, lun_idx;
    uint32_t tmp = CyU3PSibGetPartitionConfig(portId);

    if (!CyU3PIsSibActive ())
        return CY_U3P_ERROR_NOT_STARTED;
    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE)
        return CY_U3P_ERROR_INVALID_DEV;
    if (glCardCtxt[portId].locked)
        return CY_U3P_ERROR_CARD_LOCKED;
    if ((glSibIntfParams[portId].writeProtEnable == CyTrue) && (glCardCtxt[portId].writeable == CyFalse))
        return CY_U3P_ERROR_WRITE_PROTECTED;
    if (glSibCtxt[portId].partition)
        return CY_U3P_ERROR_ALREADY_PARTITIONED;
    if (partCount == 1)
        return CY_U3P_SUCCESS;  /* Nothing to do. */
    if ((partCount < 2) || (partCount > CY_U3P_SIB_NUM_PARTITIONS) || (partSizes_p == 0) || (partType_p == 0))
        return CY_U3P_ERROR_BAD_ARGUMENT;

    for (i = 0; i < partCount; i++)
    {
        if ((partType_p[i] != CY_U3P_SIB_LUN_BOOT) && (partType_p[i] != CY_U3P_SIB_LUN_DATA))
            return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    for (i = 0; i < (partCount - 1); i++)
    {
        if (partSizes_p[i] == 0)
            return CY_U3P_ERROR_BAD_ARGUMENT;
        blksReqd += partSizes_p[i];
    }

    /* We need partCount - 1 partition headers, each of which requires 64 block. */
    blksReqd += CY_U3P_BOOT_PARTITION_NUM_BLKS * 8 * partCount;
    if (blksReqd >= glCardCtxt[portId].numBlks)
        return CY_U3P_ERROR_INVALID_ADDR;

    if (CyU3PSibGetDeviceLock (portId) != CY_U3P_SUCCESS)
        return CY_U3P_ERROR_DEVICE_BUSY;

    /* Make sure there are no open write operations. */
    CyU3PSibCommitReadWrite (portId);

    if (CY_U3P_SIB_LOCATION_USER != CyU3PSibGetActivePartition(portId))
    {
        status = DoSwitchCommand (portId, CY_U3P_MMC_SW_PARTCFG_USER_PARAM | tmp, 0);
        if (status != CY_U3P_SUCCESS)
        {
            CyU3PSibClearDeviceBusy (portId);
            return status;
        }

        CyU3PSibSetActivePartition (portId, CY_U3P_SIB_LOCATION_USER);
    }

    /* Disable all the interrupts */
    prevIntrMask = SIB->sdmmc[portId].intr_mask;
    SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_SDMMC_INTR_MASK_DEFAULT;
    CyU3PSibClearIntr (portId);

    glSibCtxt[portId].numUserLuns = 0;
    lun_idx = glSibCtxt[portId].numBootLuns;

    for (i = 0; i < (partCount - 1); i++, lun_idx++)
    {
        /* Clear the buffer used to write the partition header, and then initialize it with the required fields. */
        CyU3PMemSet(glSibBuffer, 0, CY_U3P_SIB_BLOCK_SIZE);
        (*(uint32_t *)&glSibBuffer[0])  = CY_U3P_BOOT_PART_SIGN;
        (*(uint32_t *)&glSibBuffer[CY_U3P_SIB_VBP_PARTSIZE_LOCATION]) = partSizes_p[i];
        glSibBuffer[CY_U3P_SIB_VBP_PARTTYPE_LOCATION]                 = partType_p[i];

        status = CyU3PComputeChecksum ((uint32_t *)glSibBuffer, CY_U3P_SIB_VBP_CHECKSUM_LOCATION, &chkSum);
        if (status == CY_U3P_SUCCESS)
        {
            (*(uint32_t *)&glSibBuffer[CY_U3P_SIB_VBP_CHECKSUM_LOCATION]) = chkSum;

            status = CyU3PSibWritePartitionData (portId, startAddr, glSibBuffer);
            if (status == CY_U3P_SUCCESS)
            {
                glSibCtxt[portId].partition = CyTrue;
                startAddr = startAddr + (CY_U3P_BOOT_PARTITION_NUM_BLKS * 8);
                CyU3PSibUpdateLunInfo (portId, lun_idx, partType_p[i], CY_U3P_SIB_LOCATION_USER,
                        startAddr, partSizes_p[i]);

                glSibCtxt[portId].numUserLuns++;
                startAddr += partSizes_p[i];
            }
            else
            {
                /* Erase any partitions that have been created on the storage. */
                CyU3PSibClearDeviceBusy (portId);
                CyU3PSibRemovePartitions (portId);
            }
        }
    }

    if (status == CY_U3P_SUCCESS)
    {
        glSibCtxt[portId].numUserLuns++;
        CyU3PSibUpdateLunInfo (portId, lun_idx, CY_U3P_SIB_LUN_DATA, CY_U3P_SIB_LOCATION_USER, startAddr,
                glCardCtxt[portId].numBlks - startAddr);
    }
    else
    {
        glSibCtxt[portId].numUserLuns++;
        CyU3PSibUpdateLunInfo (portId, glSibCtxt[portId].numBootLuns, CY_U3P_SIB_LUN_DATA, CY_U3P_SIB_LOCATION_USER,
                0, glCardCtxt[portId].numBlks);
    }

    CyU3PSibClearIntr (portId);
    SIB->sdmmc[portId].intr_mask = prevIntrMask;

    CyU3PSibClearDeviceBusy (portId);
    return status;
}


/* This function erases the partitions on the card. */
CyU3PReturnStatus_t
CyU3PSibRemovePartitions (
        uint8_t portId)
{
    uint32_t prevIntrMask;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint32_t tmp = CyU3PSibGetPartitionConfig(portId);
    int i;

    if (!CyU3PIsSibActive ())
        return CY_U3P_ERROR_NOT_STARTED;
    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE)
        return CY_U3P_ERROR_INVALID_DEV;
    if (glCardCtxt[portId].locked)
        return CY_U3P_ERROR_CARD_LOCKED;
    if ((glSibIntfParams[portId].writeProtEnable == CyTrue) && (glCardCtxt[portId].writeable == CyFalse))
        return CY_U3P_ERROR_WRITE_PROTECTED;
    if (!glSibCtxt[portId].partition)
        return CY_U3P_ERROR_NOT_PARTITIONED;

    if (CyU3PSibGetDeviceLock (portId) != CY_U3P_SUCCESS)
        return CY_U3P_ERROR_DEVICE_BUSY;

    /* Make sure there are no open write operations. */
    CyU3PSibCommitReadWrite (portId);

    if (CY_U3P_SIB_LOCATION_USER != CyU3PSibGetActivePartition(portId))
    {
        status = DoSwitchCommand (portId, CY_U3P_MMC_SW_PARTCFG_USER_PARAM | tmp, 0);
        if (status != CY_U3P_SUCCESS)
        {
            CyU3PSibClearDeviceBusy (portId);
            return status;
        }

        CyU3PSibSetActivePartition (portId, CY_U3P_SIB_LOCATION_USER);
    }

    prevIntrMask = SIB->sdmmc[portId].intr_mask;
    SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_SDMMC_INTR_MASK_DEFAULT;
    CyU3PSibClearIntr (portId);

    /* Set all the bytes to zero. */
    CyU3PMemSet (glSibBuffer, 0, CY_U3P_SIB_BLOCK_SIZE);

    status = CyU3PSibWritePartitionData (portId, 0, glSibBuffer);
    if (status == CY_U3P_SUCCESS)
    {
        /* Update the LUN Data Structures. */
        for (i = 1; i < glSibCtxt[portId].numUserLuns; i++)
        {
            CyU3PMemSet ((uint8_t *)&(glSibLunInfo[portId][glSibCtxt[portId].numBootLuns + i]), 0,
                    sizeof(CyU3PSibLunInfo_t));
        }

        CyU3PSibUpdateLunInfo (portId, glSibCtxt[portId].numBootLuns, CY_U3P_SIB_LUN_DATA, CY_U3P_SIB_LOCATION_USER,
                0, glCardCtxt[portId].numBlks);

        /* Update the data structures */
        glSibCtxt[portId].partition   = 0;
        glSibCtxt[portId].numUserLuns = 1;
    }

    CyU3PSibClearIntr (portId);
    SIB->sdmmc[portId].intr_mask = prevIntrMask;

    CyU3PSibClearDeviceBusy (portId);
    return status;
}

/* This function is a wrapper for the card initialization function. */
CyU3PReturnStatus_t
CyU3PSibInitCard (
        uint8_t portId)
{
    CyU3PGpioSimpleConfig_t cfg;
    CyU3PReturnStatus_t     status;
    uint32_t                intMask = 0;

    /* If a GPIO is being used to control low voltage operation, drive it high to select 3.3 V operation. */
    if (glSibIntfParams[portId].lowVoltage)
    {
        cfg.outValue    = !glSibIntfParams[portId].lvGpioState;         /* Start off with 3.3 V setting. */
        cfg.driveLowEn  = CyTrue;
        cfg.driveHighEn = CyTrue;
        cfg.inputEn     = CyFalse;
        cfg.intrMode    = CY_U3P_GPIO_NO_INTR;
        status = CyU3PGpioSetSimpleConfig (glSibIntfParams[portId].voltageSwGpio, &cfg);
        if (status != CY_U3P_SUCCESS)
            return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    /* If a device reset GPIO is present, reset the device at startup. */
    if (glSibIntfParams[portId].resetGpio != 0xFF)
    {
        cfg.outValue    = glSibIntfParams[portId].rstActHigh;
        cfg.driveLowEn  = CyTrue;
        cfg.driveHighEn = CyTrue;
        cfg.inputEn     = CyFalse;
        cfg.intrMode    = CY_U3P_GPIO_NO_INTR;
        status = CyU3PGpioSetSimpleConfig (glSibIntfParams[portId].resetGpio, &cfg);
        if (status != CY_U3P_SUCCESS)
            return CY_U3P_ERROR_INVALID_SEQUENCE;

        CyU3PGpioSetValue (glSibIntfParams[portId].resetGpio, !glSibIntfParams[portId].rstActHigh);
    }

    /* If the card detect scheme for the port is based on GPIO, enable this pin as an input. */
    if ((portId == 0) && (glSibIntfParams[portId].cardDetType == CY_U3P_SIB_DETECT_GPIO))
    {
        cfg.outValue    = CyFalse;
        cfg.driveLowEn  = CyFalse;
        cfg.driveHighEn = CyFalse;
        cfg.inputEn     = CyTrue;
        cfg.intrMode    = CY_U3P_GPIO_NO_INTR;
        status = CyU3PGpioSetSimpleConfig (CY_U3P_SIB0_DETECT_GPIO, &cfg);
        if (status != CY_U3P_SUCCESS)
            return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    /* If write protection is to be enabled, enable the control pin as an input. */
    if (glSibIntfParams[portId].writeProtEnable)
    {
        cfg.outValue    = CyFalse;
        cfg.driveLowEn  = CyFalse;
        cfg.driveHighEn = CyFalse;
        cfg.inputEn     = CyTrue;
        cfg.intrMode    = CY_U3P_GPIO_NO_INTR;
        status = CyU3PGpioSetSimpleConfig ((portId ? CY_U3P_SIB1_WRPROT_GPIO : CY_U3P_SIB0_WRPROT_GPIO), &cfg);
        if (status != CY_U3P_SUCCESS)
            return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    status = CyU3PCardMgrInit (portId);
    if (status == CY_U3P_SUCCESS)
    {
        if (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_SDIO)
        {
            status = CyU3PSibCheckForPartition (portId);	
	    if (status != CY_U3P_SUCCESS)
	    {
	        CyU3PDebugPrint (4, "Partition Check Not Successfull : %d\r\n", status);
	    }
        }

        CyU3PSibSetActivePartition (portId, CY_U3P_SIB_LOCATION_USER);

        intMask = (CY_U3P_SIB_CRC16_ERROR | CY_U3P_SIB_BLOCKS_RECEIVED |
            CY_U3P_SIB_RD_DATA_TIMEOUT | CY_U3P_SIB_BLOCK_COMP);
        if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_SDIO)
        {
            intMask |= CY_U3P_SIB_SDIO_INTR;
        }
    }
    else
        /* Finding no card attached is not an error. */
        status = CY_U3P_SUCCESS;

    /* Ensure that all the interrupt bits are cleared */
    CyU3PSibClearIntr (portId);

    /* Enable the card detection interrupt */
    if (glSibIntfParams[portId].cardDetType == CY_U3P_SIB_DETECT_DAT_3)
    {
        intMask |= CY_U3P_SIB_DAT3_CHANGE;
    }
    else
    {
        if (glSibIntfParams[portId].cardDetType == CY_U3P_SIB_DETECT_GPIO)
        {
            intMask |= CY_U3P_SIB_CARD_DETECT;
        }
    }

    SIB->sdmmc[portId].intr_mask = intMask;
    return status;
}

/* The write data timer call back function. Invoked when the timer is run down
   indicating that the write data operation couldn't be completed.
 */
void
CyU3PSibWriteTimerCbk(
        uint8_t portId)
{
    CyU3PSibAbortTransaction (portId);

    if (CyU3PSibGetEventCallback != 0)
    {
        CyU3PSibGetEventCallback (portId, CY_U3P_SIB_EVENT_XFER_CPLT, CY_U3P_ERROR_TIMEOUT);
    }

    CyU3PSibReInitCard (portId);
    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE)
    {
        if (CyU3PSibGetEventCallback != 0)
            CyU3PSibGetEventCallback (portId, CY_U3P_SIB_EVENT_REMOVE, CY_U3P_SUCCESS); 
    }
}

/* Timer callback function for port 0 */
void CyU3PSibPort0WriteTimerCbk(
        uint32_t param)
{
    CyU3PEventSet (&glSibEvent, CY_U3P_SIB_TIMER0_EVT, CYU3P_EVENT_OR);
}

/* Timer callback function for port 1 */
void CyU3PSibPort1WriteTimerCbk(
        uint32_t param)
{
    CyU3PEventSet (&glSibEvent, CY_U3P_SIB_TIMER1_EVT, CYU3P_EVENT_OR);
}

/* Initializes the sib context variables. Also statically assigns the sockets on
   the SIB side.
 */
void
CyU3PSibInitCtxt (
        uint8_t portId)
{
    if (portId == CY_U3P_SIB_PORT_0)
    {
        glSibCtxt[portId].writeTimerCbk = &CyU3PSibPort0WriteTimerCbk;
    }
    else
    {
        glSibCtxt[portId].writeTimerCbk = &CyU3PSibPort1WriteTimerCbk;
    }

    /* Ensure that all the interrupt bits are cleared and masked */
    CyU3PSibClearIntr (portId);
    SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_SDMMC_INTR_MASK_DEFAULT;
}

CyU3PReturnStatus_t
CyU3PSibSetIntfParams (
        uint8_t portId,
        CyU3PSibIntfParams_t *intfParams)
{
    if ((portId >= 2) || (intfParams == 0))
        return CY_U3P_ERROR_BAD_ARGUMENT;

    if (intfParams->resetGpio != 0xFF)
    {
        if ((CyU3PIsGpioValid (intfParams->resetGpio) == CyFalse) ||
            (CyU3PIsGpioSimpleIOConfigured (intfParams->resetGpio) == CyFalse))
            return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (intfParams->voltageSwGpio != 0xFF)
    {
        if ((CyU3PIsGpioValid (intfParams->voltageSwGpio) == CyFalse) ||
            (CyU3PIsGpioSimpleIOConfigured (intfParams->voltageSwGpio) == CyFalse))
            return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if ((intfParams->lowVoltage) && (intfParams->voltageSwGpio == 0xFF))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (intfParams->cardDetType == CY_U3P_SIB_DETECT_GPIO)
    {
        if ((portId != 0) || (CyU3PIsGpioSimpleIOConfigured (CY_U3P_SIB0_DETECT_GPIO) == CyFalse))
            return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (intfParams->writeProtEnable)
    {
        if (portId == 0)
            if (CyU3PIsGpioSimpleIOConfigured (CY_U3P_SIB0_WRPROT_GPIO) == CyFalse)
                return CY_U3P_ERROR_BAD_ARGUMENT;
        if (portId == 1)
            if (CyU3PIsGpioSimpleIOConfigured (CY_U3P_SIB1_WRPROT_GPIO) == CyFalse)
                return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (intfParams->maxFreq < CY_U3P_SIB_FREQ_20MHZ)
        return CY_U3P_ERROR_BAD_ARGUMENT;

    glSibIntfParams[portId] = *intfParams;
    return CY_U3P_SUCCESS;
}

/* Summary:
   This function creates the read and write DMA channels used for storage device initialization.
 */
static CyU3PReturnStatus_t
CyU3PSibCreateChannels (
        void)
{
    CyU3PReturnStatus_t status;
    CyU3PDmaChannelConfig_t dmaCfg;

    /* One buffer of 512 bytes is sufficient for this DMA channel. */
    dmaCfg.size           = CY_U3P_SIB_BLOCK_SIZE;
    dmaCfg.count          = 1;
    dmaCfg.prodAvailCount = 0;
    dmaCfg.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.prodHeader     = 0;
    dmaCfg.prodFooter     = 0;
    dmaCfg.consHeader     = 0;
    dmaCfg.notification   = 0;
    dmaCfg.cb             = NULL;
    dmaCfg.prodSckId      = (CyU3PDmaSocketId_t)CYU3P_SIB_INT_READ_SOCKET;
    dmaCfg.consSckId      = (CyU3PDmaSocketId_t)CYU3P_SIB_INT_WRITE_SOCKET;
    status = CyU3PDmaChannelCreate (CyU3PSibGetDmaChannelHandle, CY_U3P_DMA_TYPE_AUTO, &dmaCfg);
    if (status != CY_U3P_SUCCESS)
    {
        status = CY_U3P_ERROR_SIB_INIT;
    }

    return status;
}

/* Summary:
   This function destroys the read and the write dma channels used for storage initialization.
 */
static CyU3PReturnStatus_t
CyU3PSibDestroyChannels (
        void)
{
    return (CyU3PDmaChannelDestroy (CyU3PSibGetDmaChannelHandle));
}

/* Function to selectively initialize the cards on a port basis */
CyU3PReturnStatus_t
CyU3PSibInit (
        uint8_t portId )
{
    CyU3PReturnStatus_t status;

    if ((portId == 0) && (!CyU3PSibIsS0Enabled ()))
        return CY_U3P_ERROR_NOT_CONFIGURED;
    if ((portId == 1) && (!CyU3PSibIsS1Enabled ()))
        return CY_U3P_ERROR_NOT_CONFIGURED;

    /* Check if already initialized */
    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE)
    {
        /* Initialize the sib context structure and clear the interrupt register. */
        CyU3PSibInitCtxt (portId);

        status = CyU3PMutexCreate (&glSibCtxt[portId].mutexLock, CYU3P_NO_INHERIT);
        if(status != CY_U3P_SUCCESS)
        {
            return CY_U3P_ERROR_SIB_INIT;
        }

        /* Create a write data timer and keep it in a inactive state until the setup write
         * is invoked. */
        status = CyU3PTimerCreate (&glSibCtxt[portId].writeTimer, glSibCtxt[portId].writeTimerCbk, 0,
                CY_U3P_SIB_WRITE_DATA_TIMEOUT, 0, CYU3P_NO_ACTIVATE);
        if (status != CY_U3P_SUCCESS)
        {
            CyU3PMutexDestroy (&glSibCtxt[portId].mutexLock);
            return CY_U3P_ERROR_SIB_INIT;
        }

        CyU3PSibInitCard (portId);
    }

    return CY_U3P_SUCCESS;
}

/* Function to selectively de-initialize on a port basis. Deallocates
   the resources allocated during the initialization. */
void
CyU3PSibDeInit (
        uint8_t portId )
{
    CyU3PSibCommitReadWrite (portId);
    CyU3PCardMgrDeInit (portId);

    /* Reset the Logical Unit Information Structure */
    CyU3PMemSet ((uint8_t *)&glSibLunInfo[portId], 0, sizeof(CyU3PSibLunInfo_t));

    CyU3PMutexDestroy (&glSibCtxt[portId].mutexLock);
    CyU3PTimerDestroy (&glSibCtxt[portId].writeTimer);
}

static void
CyU3PSibSocketInit (
                    void)
{
    uint8_t i;

    for (i = 0; i < CY_U3P_DMA_SIB_NUM_SCK; i++)
    {
        SIB->sck[i].status = CY_U3P_SIB_SCK_STATUS_DEFAULT;
        SIB->sck[i].intr = ~CY_U3P_SIB_SCK_INTR_DEFAULT;
        SIB->sck[i].intr_mask = CY_U3P_SIB_SCK_INTR_MASK_DEFAULT;
    }
}

static void
CyU3PSibInitVars (
        void)
{
    CyU3PMemSet ((uint8_t *)&glSibCtxt, 0, sizeof (glSibCtxt));
    CyU3PMemSet ((uint8_t *)&glCardCtxt, 0, sizeof (glCardCtxt));
    CyU3PMemSet ((uint8_t *)&glSibLunInfo, 0, sizeof (glSibLunInfo));
    CyU3PMemSet (glSibBuffer, 0, CY_U3P_SIB_BLOCK_SIZE);
    CyU3PSibSetEventCallback (0);
}

/* This function handles the card insertion or removal event. */
static void
CyU3PSibHandleCardInsRemEvt (
        uint8_t  portId,
        CyBool_t inserted)
{
    if (inserted)
    {
        /* Card has been inserted. Handle the event. */
        CyU3PSibInitCard (portId);
    }
    else
    {
        /* Card has been removed. Handle the event and abort any on-going transaction. */
        CyU3PTimerStop (&glSibCtxt[portId].writeTimer);
        CyU3PSibAbortTransaction (portId);
        CyU3PSibCommitReadWrite (portId);
        CyU3PCardMgrDeInit (portId);
    }
}

/* Handles the SIB events. The interrupt handler sets event indicating the port that has experienced
   the interrupt. This function checks the interrupt bits to see what interrupt has occured and handles
   accordingly.
 */
void
CyU3PSibEvtHandler (
        uint8_t  portId,
        uint32_t flags )
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint32_t            sibIntr;
    uint32_t            temp = 0;

    CyBool_t sendCardInsEvt  = CyFalse;
    CyBool_t sendCardRemEvt  = CyFalse;
    CyBool_t sendRDerrorEvt  = CyFalse;
    CyBool_t sendCRC16Evt    = CyFalse;
    CyBool_t sendSdioEvt     = CyFalse;
    CyBool_t sendXferCpltEvt = CyFalse;

    if (flags & CY_U3P_SIB_EVT_ABORT)
    {
        /* Handle the abort event */
        CyU3PSibClearIntr (portId);
	CyU3PSibEnableCoreIntr (portId);
        return;
    }

    /* Identify the interrupts to be handled. */
    sibIntr = (SIB->sdmmc[portId].intr & SIB->sdmmc[portId].intr_mask);
    if ((sibIntr & CY_U3P_SIB_CARD_DETECT) > 0)
    {
        if (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_NONE)
        {
            /* Insert a delay to ensure that the card is fully removed from the socket. */
            CyU3PThreadSleep (CY_U3P_SIB_HOTPLUG_DELAY);
            status = CyU3PCardMgrCheckStatus (portId);
            if (status != CY_U3P_SUCCESS)
            {
                temp = 0;
                sendCardRemEvt = CyTrue;
                CyU3PSibHandleCardInsRemEvt (portId, temp);
            }
        }
        else
        {
	    CyU3PSibHandleCardInsRemEvt (portId, 1);
            if (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_NONE)
            {
                temp = 1;
                sendCardInsEvt = CyTrue;
            }
        }

        goto EndOfIntrHandling;
    }

    if ((sibIntr & CY_U3P_SIB_SDIO_INTR) > 0)
    {
        if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_SDIO)
        {
            sendSdioEvt = CyTrue;
        }
    }

    if ((sibIntr & CY_U3P_SIB_DAT3_CHANGE) > 0)
    {

	if (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_NONE)
	{
            /* Insert a delay to ensure that the card is fully removed from the socket. */
            CyU3PThreadSleep (CY_U3P_SIB_HOTPLUG_DELAY);

            status = CyU3PCardMgrCheckStatus (portId);
            if (status != CY_U3P_SUCCESS)
            {
                sendCardRemEvt = CyTrue;
                CyU3PSibHandleCardInsRemEvt (portId, 0);
            }
	}
        else
        {
	    CyU3PSibHandleCardInsRemEvt (portId, 1);
            if (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_NONE)
            {
                sendCardInsEvt = CyTrue;
            }
        }

        goto EndOfIntrHandling;
    }

    if (((sibIntr & CY_U3P_SIB_RD_DATA_TIMEOUT) > 0) || ((sibIntr & CY_U3P_SIB_RD_END_DATA_ERROR) > 0))
    {
        if (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_SDIO)
        {
            sendRDerrorEvt = CyTrue;
            status = CY_U3P_ERROR_TIMEOUT;
	    CyU3PSibAbortTransaction (portId);
            CyU3PSibReInitCard (portId);
	    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE)
		sendCardRemEvt = CyTrue;
        }

        goto EndOfIntrHandling;
    }

    if ((sibIntr & CY_U3P_SIB_CRC16_ERROR) > 0)
    {
        sendCRC16Evt = CyTrue;
	status = CY_U3P_ERROR_CRC;

	CyU3PTimerStop (&glSibCtxt[portId].writeTimer);
	CyU3PSibAbortTransaction (portId);

	status = CyU3PCardMgrCheckStatus (portId);
        if (status != CY_U3P_SUCCESS)
        {               
            sendCardRemEvt = CyTrue;	    
	    sendCRC16Evt   = CyFalse;
        }
        else
        {
            CyU3PSibReInitCard (portId);
	    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE)
	    {
		sendCardRemEvt = CyTrue;
		sendCRC16Evt   = CyFalse;
	    }
        }
	CyU3PDebugPrint (4, "CRC Error\r\n");
        
        goto EndOfIntrHandling;
    }

    if ((sibIntr & CY_U3P_SIB_BLOCKS_RECEIVED) > 0)
    {
        sendXferCpltEvt = CyTrue;
        status = CyU3PCardMgrStopTransfer (portId);
        goto EndOfIntrHandling;
    }

    if ((sibIntr & CY_U3P_SIB_BLOCK_COMP) > 0)
    {
        sendXferCpltEvt = CyTrue;
        CyU3PTimerStop (&glSibCtxt[portId].writeTimer);
        if (!CyU3PSibIsWrCommitPending (portId))
            status = CyU3PCardMgrStopTransfer (portId);
    }

EndOfIntrHandling:
    CyU3PSibClearDeviceBusy (portId);

    if(sibIntr & CY_U3P_SIB_SDIO_INTR)
    {
        SIB->sdmmc[portId].intr_mask |= CY_U3P_SIB_SDIO_INTR;
    }

    /* Re-enable DAT3 change interrupt if DAT3 card detect is used. */
    SIB->sdmmc[portId].intr = sibIntr | CY_U3P_SIB_DAT3_CHANGE;
    if (glSibIntfParams[portId].cardDetType == CY_U3P_SIB_DETECT_DAT_3)
    {
        SIB->sdmmc[portId].intr_mask |= CY_U3P_SIB_DAT3_CHANGE;
    }

    CyU3PSibEnableCoreIntr (portId);

    if (CyU3PSibGetEventCallback != 0)
    {
        if ((sendCRC16Evt) || (sendRDerrorEvt))
        {
            CyU3PSibGetEventCallback (portId, CY_U3P_SIB_EVENT_DATA_ERROR,
                    ((sendCRC16Evt) ? CY_U3P_ERROR_CRC : CY_U3P_ERROR_TIMEOUT));
        }

        if (sendCardInsEvt)
        {
            CyU3PSibGetEventCallback (portId, CY_U3P_SIB_EVENT_INSERT, CY_U3P_SUCCESS);
        }

        if (sendCardRemEvt)
        {
            CyU3PSibGetEventCallback (portId, CY_U3P_SIB_EVENT_REMOVE, CY_U3P_SUCCESS);
        }

        if (sendXferCpltEvt)
        {
            CyU3PSibGetEventCallback (portId, CY_U3P_SIB_EVENT_XFER_CPLT, status);
        }

        if (sendSdioEvt)
        {
            CyU3PSibGetEventCallback (portId, CY_U3P_SIB_EVENT_SDIO_INTR, CY_U3P_SUCCESS);
        }
    }
}

void
CyU3PStorDriverEntry (
        void)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint32_t flag = 0;
    uint32_t mask = (CY_U3P_SIB_EVT_PORT_0 | CY_U3P_SIB_EVT_PORT_1 | CY_U3P_SIB_EVT_ABORT |
            CY_U3P_SIB_TIMER0_EVT | CY_U3P_SIB_TIMER1_EVT);

    /* The main storage thread loop */
    while (1)
    {
        /* Check if any events have occured. */
        status = CyU3PEventGet (&glSibEvent, mask, CYU3P_EVENT_OR_CLEAR, &flag, CYU3P_WAIT_FOREVER);
        if (status != CY_U3P_SUCCESS)
        {
            continue;
        }

        /* Reset the bits that we are not interested in. */
        flag &= mask;

        if (flag & CY_U3P_SIB_TIMER0_EVT)
        {
            CyU3PSibWriteTimerCbk (0);
        }

        if (flag & CY_U3P_SIB_TIMER1_EVT)
        {
            CyU3PSibWriteTimerCbk (1);
        }

        if (flag & CY_U3P_SIB_EVT_PORT_0)
        {
            CyU3PSibEvtHandler (0, flag);
        }

        if (flag & CY_U3P_SIB_EVT_PORT_1)
        {
            CyU3PSibEvtHandler (1, flag);
        }
    }
}

/* Starts the storage module. Initializes the cards on a per port basis if found. */
CyU3PReturnStatus_t
CyU3PSibStart (
        void)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Verify that the part supports the Storage ports. Otherwise, going ahead with the rest of the code
       will cause errors. */
    if (!CyFx3DevIsSib0Supported ())
        return CY_U3P_ERROR_NOT_SUPPORTED;

    /* Check if the SIB Stack is up and running */
    if (CyU3PIsSibActive ())
        return CY_U3P_SUCCESS;

    CyU3PSibInitVars ();
    CyU3PStorRegisterDriverEntry (CyU3PStorDriverEntry);

    /* Start the storage ports at the default frequency of 400 KHz. */
    CyU3PCardMgrSetClockFreq (0, CY_U3P_CLOCK_DIVIDER_400);
    CyU3PCardMgrSetClockFreq (1, CY_U3P_CLOCK_DIVIDER_400);

    /* Power the SIB block on. */
    CyFx3SibPowerOn ();

    if (CyFx3DevIOIsSib0Configured ())
        CyU3PSibSetS0Enabled ();
    else
        CyU3PSibClearS0Enabled ();

    if (CyFx3DevIOIsSib1Configured ())
        CyU3PSibSetS1Enabled ();
    else
        CyU3PSibClearS1Enabled ();

    /* Ensure that IO configurations are correct. */
    if ((!CyU3PSibIsS0Enabled ()) && (!CyU3PSibIsS1Enabled ()))
    {
        /* No storage ports have been enabled. */
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    /* Make sure all sockets are disabled and socket interrupts cleared. */
    CyU3PSibSocketInit ();

    /* Enable the SIB DMA Interrupt */
    CyU3PVicEnableInt (CY_U3P_VIC_SIB_DMA_VECTOR);

    /* Enable both the SIB interrupts */
    CyU3PVicEnableInt (CY_U3P_VIC_SIB0_CORE_VECTOR);
    CyU3PVicEnableInt (CY_U3P_VIC_SIB1_CORE_VECTOR);

    status = CyU3PSibCreateChannels ();
    if (status != CY_U3P_SUCCESS)
        return status;

    if (CyU3PSibIsS0Enabled ())
    {
        status = CyU3PSibInit (0);
        if (status == CY_U3P_ERROR_SIB_INIT)
            return status;
    }

    if (CyU3PSibIsS1Enabled ())
    {
        status = CyU3PSibInit (1);
        if (status == CY_U3P_ERROR_SIB_INIT)
        {
            if (CyU3PSibIsS0Enabled ())
                CyU3PSibDeInit (0);
            return status;
        }
    }

    if (status == CY_U3P_SUCCESS)
        CyU3PSetSibActive ();
    return status;
}

/* Stops the storage module. Deallocates the resources. */
void
CyU3PSibStop (
              void )
{
    CyU3PSibDeInit (0);
    CyU3PSibDeInit (1);

    CyU3PSibDestroyChannels ();

    /* Power off the SIB Block. */
    CyFx3SibPowerOff ();

    /* Disable the sib clocks */
    GCTL->sib0_core_clk &= ~CY_U3P_GCTL_SIBCLK_CLK_EN;
    GCTL->sib1_core_clk &= ~CY_U3P_GCTL_SIBCLK_CLK_EN;

    /* Disable storage interrupts. */
    CyU3PVicDisableInt (CY_U3P_VIC_SIB_DMA_VECTOR);
    CyU3PVicDisableInt (CY_U3P_VIC_SIB0_CORE_VECTOR);
    CyU3PVicDisableInt (CY_U3P_VIC_SIB1_CORE_VECTOR);

    CyU3PClearSibActive ();
}

/* Handles the read/write request. */
CyU3PReturnStatus_t
CyU3PSibReadWriteRequest (
        CyBool_t  isRead,
        uint8_t   portId,
        uint8_t   unitId,
        uint16_t  numBlks,
        uint32_t  blkAddr,
        uint8_t   socket
        )
{
    CyU3PReturnStatus_t status = CY_U3P_ERROR_INVALID_DEV;
    CyBool_t continuedOp = CyFalse;
    uint32_t tmp = CyU3PSibGetPartitionConfig(portId);

    if (!CyU3PIsSibActive ())
        return CY_U3P_ERROR_NOT_STARTED;
    if ((portId > 1) || (unitId > CY_U3P_SIB_NUM_PARTITIONS))
        return CY_U3P_ERROR_BAD_ARGUMENT;
    if (glCardCtxt[portId].locked)
        return CY_U3P_ERROR_CARD_LOCKED;
    if ((!isRead) && (glSibIntfParams[portId].writeProtEnable == CyTrue) && (glCardCtxt[portId].writeable == CyFalse))
        return CY_U3P_ERROR_WRITE_PROTECTED;

    /* Update the block address to an absolute device specific sector address and check for validity. */
    blkAddr += CyU3PSibLunGetStartAddr (portId, unitId);
    status =  CyU3PSibIsAddrValid (portId, unitId, numBlks, blkAddr);
    if (status != CY_U3P_SUCCESS)
        return status;

    if (CyU3PSibGetDeviceLock (portId) != CY_U3P_SUCCESS)
        return CY_U3P_ERROR_DEVICE_BUSY;

    /* Make sure that the DAT3 change interrupt is masked. */
    SIB->sdmmc[portId].intr_mask &= ~CY_U3P_SIB_DAT3_CHANGE;

    if (CyU3PSibLunGetLocation(portId,unitId) != CyU3PSibGetActivePartition(portId))
    {
        tmp |= (CyU3PSibLunGetLocation(portId, unitId) << 8);
        CyU3PSibCommitReadWrite (portId);

        status = DoSwitchCommand (portId, CY_U3P_MMC_SW_PARTCFG_USER_PARAM | tmp, 0);
        if (status != CY_U3P_SUCCESS)
        {
            /* Re-enable DAT3 change interrupt if DAT3 card detect is used. */
            if (glSibIntfParams[portId].cardDetType == CY_U3P_SIB_DETECT_DAT_3)
            {
                SIB->sdmmc[portId].intr_mask |= CY_U3P_SIB_DAT3_CHANGE;
            }

            CyU3PSibClearDeviceBusy (portId);
            return status;
        }

        CyU3PSibSetActivePartition (portId, CyU3PSibLunGetLocation(portId, unitId));
    }

    if (CyU3PSibIsWrCommitPending (portId))
    {
        if ((isRead) || (CyU3PSibGetNextWrAddress(portId) != blkAddr))
            CyU3PSibCommitReadWrite (portId);
        else
            continuedOp = CyTrue;
    }

    if (isRead)
    {
        status = CyU3PCardMgrSetupRead (portId, unitId, socket, numBlks, blkAddr);
    }
    else
    {
        /* Decide whether to commit the write at the end of this call or not. */
        CyU3PSibSetOpenWrSize (portId, (CyU3PSibGetOpenWrSize(portId) + numBlks));
        if (CyU3PSibGetOpenWrSize(portId) < CyU3PSibGetWrCommitSize(portId))
        {
            CyU3PSibSetWrCommitPending (portId);
            CyU3PSibSetNextWrAddress (portId, (blkAddr + numBlks));
        }
        else
        {
            CyU3PSibClearWrCommitPending (portId);
            CyU3PSibSetOpenWrSize (portId, 0);
        }

        if (continuedOp)
            status = CyU3PCardMgrContinueReadWrite (CyFalse, portId, socket, numBlks);
        else
            status = CyU3PCardMgrSetupWrite (portId, unitId, socket, numBlks, blkAddr);
    }

    if (status != CY_U3P_SUCCESS)
    {
        CyU3PSibCommitReadWrite (portId);

        /* Re-enable DAT3 change interrupt if DAT3 card detect is used. */
        if (glSibIntfParams[portId].cardDetType == CY_U3P_SIB_DETECT_DAT_3)
        {
            /* Re-enable the DAT3 Change interrupt mask. */
            SIB->sdmmc[portId].intr_mask |= CY_U3P_SIB_DAT3_CHANGE;
        }

        CyU3PSibReInitCard (portId);
        CyU3PSibClearDeviceBusy (portId);
    }

    return status;
}

void
CyU3PSibSetBlockLen (
        uint8_t  portId,
        uint16_t blkLen)
{
    SIB->sdmmc[portId].blocklen = blkLen;
}

CyU3PReturnStatus_t
CyU3PSibVendorAccess (
        uint8_t  portId,
        uint8_t  cmd,
        uint32_t cmdArg,
        uint8_t  respLen,
        uint8_t  *respData,
        uint16_t numBlks,
        CyBool_t isRead,
        uint8_t  socket
                       )
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint32_t prevIntr = 0;
    uint32_t flags = 0;

    if ((portId >= 2) || (respData == NULL))
    {
        /* Parameters are not valid. */
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (CyU3PSibGetDeviceLock (portId) != CY_U3P_SUCCESS)
        return CY_U3P_ERROR_DEVICE_BUSY;

    /* Make sure there are no open write operations. */
    CyU3PSibCommitReadWrite (portId);

    /* Disable the interrupts for now. */
    prevIntr = SIB->sdmmc[portId].intr_mask;
    SIB->sdmmc[portId].intr_mask = 0;
    SIB->sdmmc[portId].intr = SIB->sdmmc[portId].intr;

    if (numBlks > 0)
    {
        CyU3PSibSetNumBlocks (portId, numBlks);
        /* Configure the DMA Channels for the Xfer */
        if (isRead)
        {
            flags = CY_U3P_SIB_RDDCARD;
        }

        /* Setup the channel and the socket for the transfer */
        CyU3PSibSetActiveSocket (portId, socket);
    }

    /* Wait for the card to be in the ready state. */
    while ((SIB->sdmmc[portId].status & CY_U3P_SIB_DAT0_STAT) == 0);

    CyU3PCardMgrSendCmd (portId, cmd, respLen, cmdArg, flags);
    status = CyU3PCardMgrWaitForCmdResp (portId);

    /* Number of response bytes = Requested number of bytes + 1 (Start, Xmit bits and command index bits) */
    respLen = (respLen / 8) + ((respLen % 8) ? 1 : 0) + 3;

    respData[2] = SIB->sdmmc[portId].resp_idx;
    cmdArg = CY_U3P_SIB_SDMMC_RESP_REG0(portId);
    CyU3PMemCopy (&respData[3], (uint8_t *)&cmdArg, 4);
    cmdArg = CY_U3P_SIB_SDMMC_RESP_REG1(portId);
    CyU3PMemCopy (&respData[7], (uint8_t *)&cmdArg, 4);
    cmdArg = CY_U3P_SIB_SDMMC_RESP_REG2(portId);
    CyU3PMemCopy (&respData[11], (uint8_t *)&cmdArg, 4);
    cmdArg = CY_U3P_SIB_SDMMC_RESP_REG3(portId);
    CyU3PMemCopy (&respData[15], (uint8_t *)&cmdArg, 4);
    cmdArg = CY_U3P_SIB_SDMMC_RESP_REG4(portId);
    CyU3PMemCopy (&respData[19], (uint8_t *)&cmdArg, 4);

    if (numBlks > 0)
    {
        if (status == CY_U3P_SUCCESS)
        {
            if (!isRead)
            {
                /* Write the block to the card */
                SIB->sdmmc[portId].cs |= CY_U3P_SIB_WRDCARD;
            }

            status = CyU3PCardMgrWaitForBlkXfer (portId, isRead ? CY_U3P_SIB_BLOCKS_RECEIVED : CY_U3P_SIB_BLOCK_COMP);
            if (status != CY_U3P_SUCCESS)
            {
                CyU3PSibResetSibCtrlr (portId);
            }
        }

        if (status == CY_U3P_SUCCESS)
        {
            CyU3PCardMgrStopTransfer (portId);
        }
    }

    /* Re-enable the interrupts */
    SIB->sdmmc[portId].intr = SIB->sdmmc[portId].intr;
    SIB->sdmmc[portId].intr_mask = prevIntr;

    CyU3PSibClearDeviceBusy (portId);
    return status;
}

CyU3PReturnStatus_t
CyU3PSibQueryDevice (
        uint8_t            portId,
        CyU3PSibDevInfo_t *devInfo_p)
{
    uint32_t clkSetting;

    if (!CyU3PIsSibActive ())
        return CY_U3P_ERROR_NOT_STARTED;

    if ((portId > 1) || (devInfo_p == 0))
        return CY_U3P_ERROR_BAD_ARGUMENT;

    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE)
        return CY_U3P_ERROR_INVALID_DEV;

    if (portId == 0)
        clkSetting = GCTL->sib0_core_clk;
    else
        clkSetting = GCTL->sib1_core_clk;

    devInfo_p->clkRate   = (glSysClkFreq / ((clkSetting & CY_U3P_GCTL_SIBCLK_DIV_MASK) + 1)) ;
    devInfo_p->cardType  = (CyU3PSibDevType)glCardCtxt[portId].cardType;
    devInfo_p->numBlks   = glCardCtxt[portId].numBlks;
    devInfo_p->eraseSize = glCardCtxt[portId].eraseSize;
    devInfo_p->blkLen    = glCardCtxt[portId].blkLen;
    devInfo_p->ccc       = glCardCtxt[portId].ccc;
    devInfo_p->removable = glCardCtxt[portId].removable;
    devInfo_p->writeable = glCardCtxt[portId].writeable;
    devInfo_p->locked    = glCardCtxt[portId].locked;
    devInfo_p->ddrMode   = glCardCtxt[portId].ddrMode;
    devInfo_p->opVoltage = glCardCtxt[portId].opVoltage;
    devInfo_p->busWidth  = glCardCtxt[portId].busWidth;
    devInfo_p->numUnits  = glSibCtxt[portId].numBootLuns + glSibCtxt[portId].numUserLuns;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PSibQueryUnit (
        uint8_t            portId,
        uint8_t            unitId,
        CyU3PSibLunInfo_t *unitInfo_p)
{
    /* Perform error checks. */
    if (!CyU3PIsSibActive ())
        return CY_U3P_ERROR_NOT_STARTED;
    if ((portId > 1) || (unitInfo_p == 0))
        return CY_U3P_ERROR_BAD_ARGUMENT;
    if (glCardCtxt[portId].cardType == CY_U3P_SIB_DEV_NONE)
        return CY_U3P_ERROR_INVALID_DEV;
    if (unitId >= (glSibCtxt[portId].numBootLuns + glSibCtxt[portId].numUserLuns))
        return CY_U3P_ERROR_INVALID_UNIT;

    /* Copy the unit information to the output structure. */
    *unitInfo_p = glSibLunInfo[portId][unitId];
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PSibGetCSD (
	    uint8_t  portId,
	    uint8_t *csd_buffer)
{
    if (!CyU3PIsSibActive ())
        return CY_U3P_ERROR_NOT_STARTED;
    if ((portId > 1) || (csd_buffer == 0))
        return CY_U3P_ERROR_BAD_ARGUMENT;
    if ((glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_MMC) && (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_SD))
        return CY_U3P_ERROR_INVALID_DEV;

    CyU3PCardMgrGetCSD (portId, csd_buffer);
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PSibGetMMCExtCsd (
        uint8_t  portId,
        uint8_t *buffer_p)
{
    CyU3PReturnStatus_t status;
    uint32_t prevIntrMask;

    if (!CyU3PIsSibActive ())
        return CY_U3P_ERROR_NOT_STARTED;
    if ((portId > 1) || (buffer_p == 0))
        return CY_U3P_ERROR_BAD_ARGUMENT;
    if (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_MMC)
        return CY_U3P_ERROR_INVALID_DEV;

    if (CyU3PSibGetDeviceLock (portId) != CY_U3P_SUCCESS)
        return CY_U3P_ERROR_DEVICE_BUSY;

    /* Make sure there are no open write operations. */
    CyU3PSibCommitReadWrite (portId);

    /* Disable all interrupts from the S port until this transfer is complete. */
    prevIntrMask = SIB->sdmmc[portId].intr_mask;
    SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_SDMMC_INTR_MASK_DEFAULT;
    CyU3PSibClearIntr (portId);

    status = CyU3PCardMgrReadExtCsd (portId, buffer_p);

    /* Re-enable necessary interrupts. */
    CyU3PSibClearIntr (portId);
    SIB->sdmmc[portId].intr_mask = prevIntrMask;
    CyU3PSibClearDeviceBusy (portId);

    return status;
}

CyU3PReturnStatus_t
CyU3PSibSendSwitchCommand (
        uint8_t   portId,
        uint32_t  argument,
        uint32_t *resp_p)
{
    CyU3PReturnStatus_t status;
    uint32_t prevIntrMask;

    if (!CyU3PIsSibActive ())
        return CY_U3P_ERROR_NOT_STARTED;
    if (portId > 1)
        return CY_U3P_ERROR_BAD_ARGUMENT;
    if (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_MMC)
        return CY_U3P_ERROR_INVALID_DEV;

    if (CyU3PSibGetDeviceLock (portId) != CY_U3P_SUCCESS)
        return CY_U3P_ERROR_DEVICE_BUSY;

    /* Make sure there are no open write operations. */
    CyU3PSibCommitReadWrite (portId);
  /* Disable all interrupts from the S port until this transfer is complete. */
    prevIntrMask = SIB->sdmmc[portId].intr_mask;
    SIB->sdmmc[portId].intr_mask = CY_U3P_SIB_SDMMC_INTR_MASK_DEFAULT;
    CyU3PSibClearIntr (portId);

    status = DoSwitchCommand (portId, argument, resp_p);

    /* Re-enable necessary interrupts. */
    CyU3PSibClearIntr (portId);
    SIB->sdmmc[portId].intr_mask = prevIntrMask;
    glSibCtxt[portId].inUse = CyFalse;

    CyU3PSibClearDeviceBusy (portId);
    return status;
}

CyU3PReturnStatus_t
CyU3PSibSetWriteCommitSize (
        uint8_t  portId,
        uint32_t numSectors)
{
    if (!CyU3PIsSibActive ())
        return CY_U3P_ERROR_NOT_STARTED;
    if ((portId > 1) || (numSectors == 0))
        return CY_U3P_ERROR_BAD_ARGUMENT;

    CyU3PSibSetWrCommitSize (portId, numSectors);
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PSibGetCardStatus (
        uint8_t   portId,
        uint32_t *status_p)
{
    CyU3PReturnStatus_t status;

    if (!CyU3PIsSibActive ())
        return CY_U3P_ERROR_NOT_STARTED;
    if (portId > 1)
        return CY_U3P_ERROR_BAD_ARGUMENT;
    if ((glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_MMC) && (glCardCtxt[portId].cardType != CY_U3P_SIB_DEV_SD))
        return CY_U3P_ERROR_INVALID_DEV;

    if (CyU3PSibGetDeviceLock (portId) != CY_U3P_SUCCESS)
        return CY_U3P_ERROR_DEVICE_BUSY;

    status = CyU3PCardMgrCheckStatus (portId);
    if (status == CY_U3P_SUCCESS)
    {
        if (status_p != 0)
            *status_p = SIB->sdmmc[portId].resp_reg0;
    }

    CyU3PSibClearDeviceBusy (portId);
    return status;
}

/*[]*/


