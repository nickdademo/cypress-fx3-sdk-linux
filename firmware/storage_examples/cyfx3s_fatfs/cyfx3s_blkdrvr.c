/*
 ## Cypress USB 3.0 Platform source file (cyfx3s_blkdrvr.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2011,
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

#include <cyu3sib.h>
#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3error.h>

#include "cyfx3s_fatfs.h"
#include "diskio.h"		/* Common include file for FatFs and disk I/O layer */

#define CY_FX_FATFS_SIB_DONE_EVENT      (1 << 0)        /* SIB transfer done event. */

CyU3PSibDevInfo_t      DevInfo[2];
static DSTATUS         Stat[CY_U3P_SIB_NUM_PORTS] = {STA_NOINIT, STA_NOINIT};
static CyU3PSibDevType CardType[CY_U3P_SIB_NUM_PORTS] = {CY_U3P_SIB_DEV_NONE, CY_U3P_SIB_DEV_NONE};

void
CyFxFatfsAppSibCallback (
        uint8_t             portId,
        CyU3PSibEventType   evt,
        CyU3PReturnStatus_t status)
{
    if (evt == CY_U3P_SIB_EVENT_XFER_CPLT)
    {
        CyU3PEventSet (&glStorDriverEvent, CY_FX_FATFS_SIB_DONE_EVENT, CYU3P_EVENT_OR);
    }
}


/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS
disk_status (
	BYTE drv			/* Drive number */
        )
{
    CyU3PReturnStatus_t status;

    if (drv >= CY_U3P_SIB_NUM_PORTS)
        return STA_NOINIT;

    /* Check if a device is connected on the selected port. */
    status = CyU3PSibQueryDevice ((uint8_t)drv, &DevInfo[drv]);
    if (status == CY_U3P_SUCCESS)
    {
        Stat[drv]     = 0;
        CardType[drv] = DevInfo[drv].cardType;

        /* If the device type is not SD/MMC, or if no valid partitions are found; report no disk found. */
        if ((DevInfo[drv].numUnits == 0) ||
                ((CardType[drv] != CY_U3P_SIB_DEV_SD) && (CardType[drv] != CY_U3P_SIB_DEV_MMC)))
            Stat[drv] = STA_NODISK;
        if (DevInfo[drv].writeable == 0)
            Stat[drv] = STA_PROTECT;
    }
    else
        Stat[drv] = STA_NODISK;

    return Stat[drv];
}

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/
DSTATUS
disk_initialize (
        BYTE drv		/* Physical drive number */
        )
{	
    return (disk_status (drv));
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
		BYTE drv,			/* Physical drive nmuber (0) */
		BYTE *buff,			/* Pointer to the data buffer to store read data */
		DWORD sector,			/* Start sector number (LBA) */
		BYTE count			/* Sector count (1..128) */
		)
{	
    CyU3PReturnStatus_t status;
    CyU3PDmaBuffer_t    BufIN_t;
    uint32_t            evStat;

    /* No storage device found. */
    if ((Stat[drv] == STA_NOINIT) || (Stat[drv] == STA_NODISK))
        return RES_NOTRDY;

    /* FX3S cannot handle DMA buffers larger that 65535 bytes. */
    if (count < 1 || count > 127)
        return RES_PARERR;

    BufIN_t.buffer = (uint8_t *) buff;
    BufIN_t.count  = 0;
    BufIN_t.size   = 512 * count;
    BufIN_t.status = 0;

    status = CyU3PDmaChannelSetupRecvBuffer ( &glChHandleStorIn, &BufIN_t);
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PSibReadWriteRequest (CyTrue, (uint8_t) drv, 0, (uint16_t)count, (uint32_t)sector, (uint8_t) CY_U3P_SIB_SOCKET_1);
        if (status != CY_U3P_SUCCESS)
        {
            /* Abort the DMA Channel */
            CyU3PDmaChannelReset (&glChHandleStorIn);
            return RES_ERROR;
        }
    }

    /* Wait until we get the SIB transfer done event. */
    status = CyU3PEventGet (&glStorDriverEvent, CY_FX_FATFS_SIB_DONE_EVENT, CYU3P_EVENT_OR_CLEAR, &evStat,
            CYU3P_WAIT_FOREVER);
    if (status == CY_U3P_SUCCESS)
        status = CyU3PDmaChannelWaitForRecvBuffer (&glChHandleStorIn,  &BufIN_t, CYU3P_WAIT_FOREVER);

    if (status != CY_U3P_SUCCESS)
    {
        CyU3PSibAbortRequest ((uint8_t)drv);
        CyU3PDmaChannelReset (&glChHandleStorIn);
    }

    return status ? RES_ERROR : RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
		BYTE drv,			/* Physical drive nmuber (0) */
		const BYTE *buff,		/* Pointer to the data to be written */
		DWORD sector,			/* Start sector number (LBA) */
		BYTE count			/* Sector count (1..128) */
		)
{
    CyU3PReturnStatus_t status;
    CyU3PDmaBuffer_t    BufOUT_t;
    uint32_t            evStat;

    /* No storage device found. */
    if ((Stat[drv] == STA_NOINIT) || (Stat[drv] == STA_NODISK))
        return RES_NOTRDY;

    /* FX3S cannot handle DMA buffers larger that 65535 bytes. */
    if (count < 1 || count > 127)
        return RES_PARERR;

    /* Check whether the volume is write protected. */
    if (Stat[drv] == STA_PROTECT)
        return RES_WRPRT;

    BufOUT_t.buffer = (uint8_t *)buff;
    BufOUT_t.count  = (uint16_t)count * 512;
    BufOUT_t.size   = (uint16_t)count * 512;
    BufOUT_t.status = 0;

    status = CyU3PSibReadWriteRequest (CyFalse, (uint8_t) drv, 0, (uint16_t)count, (uint32_t)sector, (uint8_t)CY_U3P_SIB_SOCKET_0);
    if (status != CY_U3P_SUCCESS)
    {
        /* Abort the DMA Channel */
        CyU3PDmaChannelReset (&glChHandleStorOut);
        return RES_ERROR;
    }

    status = CyU3PDmaChannelSetupSendBuffer (&glChHandleStorOut,  &BufOUT_t);
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PEventGet (&glStorDriverEvent, CY_FX_FATFS_SIB_DONE_EVENT, CYU3P_EVENT_OR_CLEAR, &evStat,
                CYU3P_WAIT_FOREVER);
        if (status == CY_U3P_SUCCESS)
            status = CyU3PDmaChannelWaitForCompletion (&glChHandleStorOut, CYU3P_WAIT_FOREVER);
    }

    if (status != CY_U3P_SUCCESS)
    {
        CyU3PSibAbortRequest ((uint8_t)drv);
        CyU3PDmaChannelReset (&glChHandleStorOut);
    }

    return status ? RES_ERROR : RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
		BYTE drv,		/* Physical drive nmuber (0) */
		BYTE ctrl,		/* Control code */
		void *buff		/* Buffer to send/receive control data */
		)
{
    CyU3PReturnStatus_t status;
    CyU3PSibLunInfo_t   unitInfo;
    DRESULT res;

    disk_status (drv);
    if ((Stat[drv] == STA_NOINIT) || (Stat[drv] == STA_NODISK))
        return RES_NOTRDY;

    res = RES_ERROR;
    switch (ctrl) {
        case CTRL_SYNC :		/* Make sure that no pending write process */
            CyU3PSibCommitReadWrite ((uint8_t)drv);
            res = RES_OK;
            break;

        case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
            /* Query the 0th volume on the storage device to find the capacity. */
            status = CyU3PSibQueryUnit ((uint8_t)drv, 0, &unitInfo);
            if ((status != CY_U3P_SUCCESS) || (unitInfo.valid == 0))
                return RES_ERROR;

            *(DWORD *)buff = unitInfo.numBlocks;
            res = RES_OK;
            break;

        case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
            *(DWORD*)buff = 128;
            res = RES_OK;
            break;

        default:
            res = RES_PARERR;
    }

    return res;
}

