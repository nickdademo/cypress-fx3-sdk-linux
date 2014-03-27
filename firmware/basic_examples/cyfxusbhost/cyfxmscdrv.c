/*
 ## Cypress USB 3.0 Platform source file (cyfxmscdrv.c)
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

/* This file illustrates the MSC driver. */

/*
   A simple single interface USB BOT MSC device will be successfully 
   enumerated and storage parameters will be queried. The example shall read
   the last sector of data, save it in local buffers and write a fixed pattern
   to location. It will  then read and verify the data written. Once this is
   done, the original data will be written back to the storage.
   
   NOTE: If MSC device is unplugged before all transactions have completed
   or if the example encounters an error, the data on the drive might be
   corrupted.
*/

#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3usbhost.h"
#include "cyu3usbotg.h"
#include "cyu3utils.h"
#include "cyfxusbhost.h"

uint8_t glMscInEp = 0;          /* MSC IN endpoint. */
uint8_t glMscOutEp = 0;         /* MSC OUT endpoint. */

uint16_t glMscEpSize = 0;       /* MSC endpoint size. */
uint16_t glMscSectorSize = 0;   /* MSC sector size. */
uint32_t glMscCapacity = 0;     /* Size of the MSC device in sectors. */
uint8_t  glMscTestLun = 0;      /* LUN to be used for testing. */
uint32_t glMscTestSector = 0;   /* Test sector address. */
uint32_t glTimerCount = 0;      /* Counter which maintains the up time for host stack. */

CyBool_t glMscIsActive = CyFalse;

CyU3PDmaChannel glMscInCh;      /* IN EP channel for ingress data. */
CyU3PDmaChannel glMscOutCh;     /* OUT EP channel for egress data. */

uint8_t glMscCBW[CY_FX_MSC_CBW_CSW_BUFFER_SIZE] __attribute__ ((aligned (32)));   /* Buffer to store the current CBW. */
uint8_t glMscCSW[CY_FX_MSC_CBW_CSW_BUFFER_SIZE] __attribute__ ((aligned (32)));   /* Buffer to store the current CSW. */
uint8_t glMscSector[CY_FX_MSC_MAX_SECTOR_SIZE] __attribute__ ((aligned (32)));    /* Buffer for MSC operation. */

static void
CyFxMyFormatCBW (
        uint8_t *cbw,
        uint32_t tag,
        uint32_t xferLength,
        CyBool_t isRead,
        uint8_t lun,
        uint8_t rqtLength)
{
    CyU3PMemSet (cbw, 0, 31);
    cbw[0]  = 'U';
    cbw[1]  = 'S';
    cbw[2]  = 'B';
    cbw[3]  = 'C';
    cbw[4]  = (uint8_t)((tag      ) & 0xFF);
    cbw[5]  = (uint8_t)((tag >>  8) & 0xFF);
    cbw[6]  = (uint8_t)((tag >> 16) & 0xFF);
    cbw[7]  = (uint8_t)((tag >> 24) & 0xFF);
    cbw[8]  = (uint8_t)((xferLength      ) & 0xFF);
    cbw[9]  = (uint8_t)((xferLength >>  8) & 0xFF);
    cbw[10] = (uint8_t)((xferLength >> 16) & 0xFF);
    cbw[11] = (uint8_t)((xferLength >> 24) & 0xFF);
    cbw[12] = (isRead) ? 0x80 : 0;
    cbw[13] = lun;
    cbw[14] = rqtLength;
}

static void
CyFxMyFormatCmd (
        uint8_t *cbw,
        uint8_t opCode,
        uint32_t sectorAddr,
        uint16_t sectorCount)
{
    cbw[15] = opCode;
    cbw[17]  = (uint8_t)((sectorAddr >> 24) & 0xFF);
    cbw[18]  = (uint8_t)((sectorAddr >> 16) & 0xFF);
    cbw[19]  = (uint8_t)((sectorAddr >>  8) & 0xFF);
    cbw[20]  = (uint8_t)((sectorAddr      ) & 0xFF);
    cbw[22]  = (uint8_t)((sectorCount >>  8) & 0xFF);
    cbw[23]  = (uint8_t)((sectorCount      ) & 0xFF);
}

CyU3PReturnStatus_t
CyFxMscErrorRecovery (
        void)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Abort and reset the IN endpoint. */
    if (glMscInEp != 0)
    {
        CyU3PDmaChannelReset (&glMscInCh);
        CyU3PUsbHostEpAbort (glMscInEp);

        status = CyFxSendSetupRqt (0x02, CY_U3P_USB_SC_CLEAR_FEATURE,
                0, glMscInEp, 0, glEp0Buffer);
        if (status == CY_U3P_SUCCESS)
        {
            status = CyU3PUsbHostEpReset (glMscInEp);
        }
    }

    /* Abort and reset the OUT endpoint. */
    if ((status == CY_U3P_SUCCESS) && (glMscOutEp != 0))
    {
        CyU3PDmaChannelReset (&glMscOutCh);
        CyU3PUsbHostEpAbort (glMscOutEp);

        status = CyFxSendSetupRqt (0x02, CY_U3P_USB_SC_CLEAR_FEATURE,
                0, glMscOutEp, 0, glEp0Buffer);
        if (status == CY_U3P_SUCCESS)
        {
            status = CyU3PUsbHostEpReset (glMscOutEp);
        }
    }

    return status;
}

CyU3PReturnStatus_t
CyFxMscSendBuffer (
        uint8_t *buffer,
        uint16_t count)
{
    CyU3PDmaBuffer_t buf_p;
    CyU3PUsbHostEpStatus_t epStatus;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Setup the DMA for transfer. */
    buf_p.buffer = buffer;
    buf_p.count  = count;
    buf_p.size   = ((count + 0x0F) & ~0x0F);
    buf_p.status = 0;
    status = CyU3PDmaChannelSetupSendBuffer (&glMscOutCh, &buf_p);
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PUsbHostEpSetXfer (glMscOutEp,
                CY_U3P_USB_HOST_EPXFER_NORMAL, count);
    }
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PUsbHostEpWaitForCompletion (glMscOutEp, &epStatus,
                CY_FX_MSC_WAIT_TIMEOUT);
    }
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PDmaChannelWaitForCompletion (&glMscOutCh, CYU3P_NO_WAIT);
    }

    if (status != CY_U3P_SUCCESS)
    {
        CyFxMscErrorRecovery ();
    }

    return status;
}

CyU3PReturnStatus_t
CyFxMscRecvBuffer (
        uint8_t *buffer,
        uint16_t count)
{
    CyU3PDmaBuffer_t buf_p;
    CyU3PUsbHostEpStatus_t epStatus;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Setup the DMA for transfer. */
    buf_p.buffer = buffer;
    buf_p.count  = 0;
    buf_p.size   = ((count + 0x0F) & ~0x0F);
    buf_p.status = 0;
    status = CyU3PDmaChannelSetupRecvBuffer (&glMscInCh, &buf_p);
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PUsbHostEpSetXfer (glMscInEp,
                CY_U3P_USB_HOST_EPXFER_NORMAL, count);
    }
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PUsbHostEpWaitForCompletion (glMscInEp, &epStatus,
                CY_FX_MSC_WAIT_TIMEOUT);
    }
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PDmaChannelWaitForCompletion (&glMscInCh, CYU3P_NO_WAIT);
    }

    if (status != CY_U3P_SUCCESS)
    {
        CyFxMscErrorRecovery ();
    }

    return status;
}

CyU3PReturnStatus_t
CyFxMscCheckCSW (
        uint8_t *csw)
{
    /* Check if the CSW signature is correct. */
    if ((csw[0] != 'U') || (csw[1] != 'S') ||
            (csw[2] != 'B') || (csw[3] != 'S'))
    {
        return CY_U3P_ERROR_FAILURE;
    }

    /* Check if there is any error returned. */
    if (csw[12] != 0)
    {
        return CY_U3P_ERROR_FAILURE;
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyFxMscTestUnitReady (uint8_t lun)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Format and send the request. */
    CyFxMyFormatCBW (glMscCBW, 1, 0, CyFalse, lun, 6);
    CyFxMyFormatCmd (glMscCBW, 0, 0, 0);
    status = CyFxMscSendBuffer (glMscCBW, 31);
    if (status == CY_U3P_SUCCESS)
    {
        status = CyFxMscRecvBuffer (glMscCSW, 13);
    }
    if (status == CY_U3P_SUCCESS)
    {
        status = CyFxMscCheckCSW (glMscCSW);
    }

    return status;
}

CyU3PReturnStatus_t
CyFxMscReadCapacity (uint8_t lun)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Format and send the request. */
    CyFxMyFormatCBW (glMscCBW, 1, 8, CyTrue, lun, 10);
    CyFxMyFormatCmd (glMscCBW, 0x25, 0, 0);
    status = CyFxMscSendBuffer (glMscCBW, 31);
    if (status == CY_U3P_SUCCESS)
    {
        status = CyFxMscRecvBuffer (glMscSector, 8);
    }
    if (status == CY_U3P_SUCCESS)
    {
        status = CyFxMscRecvBuffer (glMscCSW, 13);
    }
    if (status == CY_U3P_SUCCESS)
    {
        status = CyFxMscCheckCSW (glMscCSW);
    }

    /* Decode the capacity. */
    if (status == CY_U3P_SUCCESS)
    {
        glMscSectorSize = CY_U3P_MAKEWORD (glMscSector[6], glMscSector[7]);
        if ((glMscSectorSize > CY_FX_MSC_MAX_SECTOR_SIZE) ||
                (glMscSector[4] != 0) || (glMscSector[5] != 0))
        {
            glMscSectorSize = 0;
            status = CY_U3P_ERROR_NOT_SUPPORTED;
        }
    }
    if (status == CY_U3P_SUCCESS)
    {
        glMscCapacity = CY_U3P_MAKEDWORD (glMscSector[0], glMscSector[1],
                glMscSector[2], glMscSector[3]);
    }

    return status;
}

CyU3PReturnStatus_t
CyFxMscReadSectors (
        uint8_t lun,
        uint32_t addr,
        uint16_t count,
        uint8_t *buffer)
{
    uint16_t size = count * glMscSectorSize;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    if ((count * glMscSectorSize) > (uint16_t)0xFFFF)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Format and send the request. */
    CyFxMyFormatCBW (glMscCBW, 1, size, CyTrue, lun, 10);
    CyFxMyFormatCmd (glMscCBW, 0x28, addr, count);
    status = CyFxMscSendBuffer (glMscCBW, 31);
    if (status == CY_U3P_SUCCESS)
    {
        status = CyFxMscRecvBuffer (buffer, size);
    }
    if (status == CY_U3P_SUCCESS)
    {
        status = CyFxMscRecvBuffer (glMscCSW, 13);
    }
    if (status == CY_U3P_SUCCESS)
    {
        status = CyFxMscCheckCSW (glMscCSW);
    }

    return status;
}

CyU3PReturnStatus_t
CyFxMscWriteSectors (
        uint8_t lun,
        uint32_t addr,
        uint16_t count,
        uint8_t *buffer)
{
    uint16_t size = count * glMscSectorSize;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    if ((count * glMscSectorSize) > (uint16_t)0xFFFF)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Format and send the request. */
    CyFxMyFormatCBW (glMscCBW, 1, size, CyFalse, lun, 10);
    CyFxMyFormatCmd (glMscCBW, 0x2A, addr, count);
    status = CyFxMscSendBuffer (glMscCBW, 31);
    if (status == CY_U3P_SUCCESS)
    {
        status = CyFxMscSendBuffer (buffer, size);
    }
    if (status == CY_U3P_SUCCESS)
    {
        status = CyFxMscRecvBuffer (glMscCSW, 13);
    }
    if (status == CY_U3P_SUCCESS)
    {
        status = CyFxMscCheckCSW (glMscCSW);
    }

    return status;
}

CyU3PReturnStatus_t
CyFxMscDriverInit ()
{
    uint8_t maxLun = 0, i, retry;
    uint16_t length, size, offset;
    CyU3PReturnStatus_t status;
    CyU3PUsbHostEpConfig_t epCfg;
    CyU3PDmaChannelConfig_t dmaCfg;

    /* Read first four bytes of configuration descriptor to determine
     * the total length. */
    status = CyFxSendSetupRqt (0x80, CY_U3P_USB_SC_GET_DESCRIPTOR,
            (CY_U3P_USB_CONFIG_DESCR << 8), 0, 4, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    /* Identify the length of the data received. */
    length = CY_U3P_MAKEWORD(glEp0Buffer[3], glEp0Buffer[2]);
    if (length > CY_FX_HOST_EP0_BUFFER_SIZE)
    {
        goto enum_error;
    }

    /* Read the full configuration descriptor. */
    status = CyFxSendSetupRqt (0x80, CY_U3P_USB_SC_GET_DESCRIPTOR,
            (CY_U3P_USB_CONFIG_DESCR << 8), 0, length, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    /* Identify the EP characteristics. */
    offset = 0;
    while (offset < length)
    {
        if (glEp0Buffer[offset + 1] == CY_U3P_USB_ENDPNT_DESCR)
        {
            if (glEp0Buffer[offset + 3] != CY_U3P_USB_EP_BULK)
            {
                goto enum_error;
            }

            /* Retreive the information. */
            glMscEpSize = CY_U3P_MAKEWORD(glEp0Buffer[offset + 5],
                    glEp0Buffer[offset + 4]);
            if (glEp0Buffer[offset + 2] & 0x80)
            {
                glMscInEp = glEp0Buffer[offset + 2];
            }
            else
            {
                glMscOutEp = glEp0Buffer[offset + 2];
            }
        }

        /* Advance to next descriptor. */
        offset += glEp0Buffer[offset];
    }

    /* Set the new configuration. */
    status = CyFxSendSetupRqt (0x00, CY_U3P_USB_SC_SET_CONFIGURATION,
            1, 0, 0, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    /* Identify the number of LUNs available.
     * We will use only the first available LUN. */
    status = CyFxSendSetupRqt (0xA1, CY_FX_MSC_GET_MAX_LUN,
            0, 0, 1, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
    maxLun = glEp0Buffer[0];

    /* Add the IN endpoint. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof(epCfg));
    epCfg.type = CY_U3P_USB_EP_BULK;
    epCfg.mult = 1;
    epCfg.maxPktSize = glMscEpSize;
    epCfg.pollingRate = 0;
    /* Since DMA buffer sizes can only be multiple of 16 bytes and
     * also since this is an interrupt endpoint where the max data
     * packet size is same as the maxPktSize field, the fullPktSize
     * has to be a multiple of 16 bytes. */
    size = ((glMscEpSize + 0x0F) & ~0x0F);
    epCfg.fullPktSize = size;
    epCfg.isStreamMode = CyFalse;
    status = CyU3PUsbHostEpAdd (glMscInEp, &epCfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    /* Add the OUT EP. */
    status = CyU3PUsbHostEpAdd (glMscOutEp, &epCfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    /* Create a DMA channel for IN EP. */
    CyU3PMemSet ((uint8_t *)&dmaCfg, 0, sizeof(dmaCfg));
    dmaCfg.size = glMscEpSize;
    dmaCfg.count = 0;
    dmaCfg.prodSckId = (CyU3PDmaSocketId_t)(CY_U3P_UIB_SOCKET_PROD_0 + (0x0F & glMscInEp));
    dmaCfg.consSckId = CY_U3P_CPU_SOCKET_CONS;
    dmaCfg.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.notification = 0;
    dmaCfg.cb = NULL;
    dmaCfg.prodHeader = 0;
    dmaCfg.prodFooter = 0;
    dmaCfg.consHeader = 0;
    dmaCfg.prodAvailCount = 0;
    status = CyU3PDmaChannelCreate (&glMscInCh, CY_U3P_DMA_TYPE_MANUAL_IN, &dmaCfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto app_error;
    }

    /* Create a DMA channel for OUT EP. */
    dmaCfg.prodSckId = CY_U3P_CPU_SOCKET_PROD;
    dmaCfg.consSckId = (CyU3PDmaSocketId_t)(CY_U3P_UIB_SOCKET_CONS_0 + (0x0F & glMscOutEp));
    status = CyU3PDmaChannelCreate (&glMscOutCh, CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaCfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto app_error;
    }

    for (retry = 0; retry < CY_FX_MSC_MAX_RETRY; retry++)
    {
        /* Give some time for the device to initialize. */
        CyU3PThreadSleep (1000);
        for (i = 0; i <= maxLun; i++)
        {
            /* Check if the unit is ready. */
            status = CyFxMscTestUnitReady (maxLun);
            if (status == CY_U3P_SUCCESS)
            {
                glMscTestLun = i;
                break;
            }
        }

        if (status == CY_U3P_SUCCESS)
        {
            break;
        }
    }
    if (status != CY_U3P_SUCCESS)
    {
        glMscTestLun = 0;
        goto app_error;
    }

    /* Read the device capacity. */
    status = CyFxMscReadCapacity (glMscTestLun);
    if (status != CY_U3P_SUCCESS)
    {
        goto app_error;
    }

    glMscIsActive = CyTrue;
    return status;

app_error:
    CyU3PDmaChannelDestroy (&glMscInCh);
    if (glMscInEp != 0)
    {
        CyU3PUsbHostEpRemove (glMscInEp);
        glMscInEp = 0;
    }
    CyU3PDmaChannelDestroy (&glMscOutCh);
    if (glMscOutEp != 0)
    {
        CyU3PUsbHostEpRemove (glMscOutEp);
        glMscOutEp = 0;
    }

    glMscEpSize = 0;
    glMscSectorSize = 0;
    glMscCapacity = 0;
    glMscTestLun = 0;
    glMscTestSector = 0;
    glTimerCount = 0;

enum_error:
    return CY_U3P_ERROR_FAILURE;
}

void
CyFxMscDriverDeInit ()
{
    /* Clear all variables. */
    glMscIsActive = CyFalse;
    glMscEpSize = 0;
    glMscSectorSize = 0;
    glMscCapacity = 0;
    glMscTestLun = 0;
    glMscTestSector = 0;
    glTimerCount = 0;

    CyU3PDmaChannelDestroy (&glMscInCh);
    if (glMscInEp != 0)
    {
        CyU3PUsbHostEpRemove (glMscInEp);
        glMscInEp = 0;
    }
    CyU3PDmaChannelDestroy (&glMscOutCh);
    if (glMscOutEp != 0)
    {
        CyU3PUsbHostEpRemove (glMscOutEp);
        glMscOutEp = 0;
    }
}

void
CyFxMscDriverDoWork ()
{
    uint32_t upTime = 0;
    uint16_t index = 0;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
#ifdef CY_FX_MSC_ENABLE_WRITE_TEST
    uint16_t temp;
    uint8_t pattern = 0;
#endif /* CY_FX_MSC_ENABLE_WRITE_TEST */

    /* Host polling interval. */
    upTime = glTimerCount * CY_FX_HOST_POLL_INTERVAL;
    glTimerCount++;
    if ((glMscIsActive) && ((upTime % CY_FX_MSC_TEST_INTERVAL) == 0))
    {
        CyU3PDebugPrint (4, "Starting MSC read / write tests.\r\n");
        /* Write the sectors to memory. */
        for (index = 0; index < CY_FX_MSC_TEST_SECTOR_COUNT; index++)
        {
            if ((glMscTestSector >= (glMscCapacity - CY_FX_MSC_TEST_SECTOR_COUNT))
                    || (glMscTestSector < CY_FX_MSC_TEST_SECTOR_OFFSET))
            {
                glMscTestSector = CY_FX_MSC_TEST_SECTOR_OFFSET;
            }

#ifdef CY_FX_MSC_ENABLE_WRITE_TEST
            /* Pre-fill the buffer with known pattern. */
            pattern = (uint8_t)(glMscTestSector + index);
            CyU3PMemSet (glMscSector, pattern, glMscSectorSize);
            CyU3PDebugPrint (6, "Writing to sector: %d.\r\n", glMscSector);
            status = CyFxMscWriteSectors (glMscTestLun, glMscTestSector, 1, glMscSector);
            if (status != CY_U3P_SUCCESS)
            {
                break;
            }
#endif /* CY_FX_MSC_ENABLE_WRITE_TEST */

            /* Read back and verify. */
            CyU3PMemSet (glMscSector, 0, glMscSectorSize);
            CyU3PDebugPrint (6, "Reading from sector: %d.\r\n", glMscSector);
            status = CyFxMscReadSectors (glMscTestLun, glMscTestSector, 1, glMscSector);
            if (status != CY_U3P_SUCCESS)
            {
                break;
            }
#ifdef CY_FX_MSC_ENABLE_WRITE_TEST
            for (temp = 0; temp < glMscSectorSize; temp++)
            {
                if (glMscSector[temp] != pattern)
                {
                    CyU3PDebugPrint (6, "Compare failed for sector: %d at byte %d.\r\n", glMscSector, temp);
                    status = CY_U3P_ERROR_FAILURE;
                    break;
                }
            }
            if (status != CY_U3P_SUCCESS)
            {
                break;
            }
#endif /* CY_FX_MSC_ENABLE_WRITE_TEST */

            /* Increment the test sector. */
            glMscTestSector++;
        }

        /* If an error was encountered, stop the test. */
        if (status == CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "Completed MSC read / write tests.\r\n");
        }
        else
        {
            CyU3PDebugPrint (4, "MSC test failed with status: %d.\r\n", status);
            glMscIsActive = CyFalse;
        }
    }
}

/* [ ] */

