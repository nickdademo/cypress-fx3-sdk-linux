/*
 ## Cypress USB 3.0 Platform source file (cyfxusbhost.c)
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

/* This file defines the usb host mode application example. */

#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3usbhost.h"
#include "cyu3usbotg.h"
#include "cyu3utils.h"
#include "cyfxbulklpotg.h"

static CyBool_t glIsApplnActive = CyFalse;             /* Whether the application is active or not. */
static CyBool_t glIsPeripheralPresent = CyFalse;       /* Whether a remote peripheral is present or not. */

static uint8_t glSetupPkt[CY_FX_HOST_EP0_SETUP_SIZE] __attribute__ ((aligned (32)));  /* Setup packet buffer for host mode. */
static uint8_t glEp0Buffer[CY_FX_HOST_EP0_BUFFER_SIZE] __attribute__ ((aligned (32))); /* Buffer to send / receive data for EP0. */
static uint8_t glHostOutEp = 0;                        /* Interrupt endpoint used by the mouse application. */
static uint8_t glHostInEp = 0;                         /* Interrupt endpoint used by the mouse application. */
static uint16_t glHostEpSize = 0;                      /* Endpoint max. packet size. */

static uint32_t glTimerCount = 0;                      /* Counter which maintains the up time for host stack. */
static CyBool_t glIsHnpSupported = CyFalse;            /* Whether is supported by the remote device. */
static CyBool_t glIsHnp = CyFalse;                     /* Whether HNP is in progress. */
static CyBool_t glDoHnp = CyFalse;                     /* Complete the HNP process. */

static CyU3PDmaChannel glHostInCh;                     /* DMA channel for IN data. */
static CyU3PDmaChannel glHostOutCh;                    /* DMA channel for OUT data. */

static uint32_t glDMARxCount = 0;                      /* Host mode count of receive buffers. */
static uint32_t glDMATxCount = 0;                      /* Host mode count of transferred buffers. */

/* USB host stack EP transfer completion callback. */
static void
CyFxHostXferCb (uint8_t ep, CyU3PUsbHostEpStatus_t epStatus)
{
    /* Queue the next request if HNP is not in progress. */
    if ((ep != 0) && (!glIsHnp))
    {
        CyU3PUsbHostEpSetXfer (ep, CY_U3P_USB_HOST_EPXFER_NORMAL, glHostEpSize);
    }
}

/* USB host stack event callback function. */
static void
CyFxHostEventCb (CyU3PUsbHostEventType_t evType, uint32_t evData)
{
    /* This is connect / disconnect event. Log it so that the
     * application thread can handle it. */
    if (evType == CY_U3P_USB_HOST_EVENT_CONNECT)
    {
        CyU3PDebugPrint (4, "USB host mode: Remote peripheral detected.\r\n");
        glIsPeripheralPresent = CyTrue;
    }
    else
    {
        CyU3PDebugPrint (4, "USB host mode: Remote peripheral disconnected.\r\n");
        glIsPeripheralPresent = CyFalse;
        /* Indicate that this is a HNP role change. */
        if (glIsHnp)
        {
            glDoHnp = CyTrue;
        }
    }
}

/* DMA callback for the mouse application. */
static void
CyFxHostDmaCb (CyU3PDmaChannel *ch,
        CyU3PDmaCbType_t type,
        CyU3PDmaCBInput_t *input)
{
    if (type == CY_U3P_DMA_CB_PROD_EVENT)
    {
        /* Discard the current buffer to free it. Further
         * data integrity checks can be done before discard.*/
        CyU3PDmaChannelDiscardBuffer (ch);
        glDMARxCount++;
    }
    if (type == CY_U3P_DMA_CB_CONS_EVENT)
    {
        /* Commit another buffer. Since the data has all been
         * pre-filled, just a commit call can be done. */
        CyU3PDmaChannelCommitBuffer (ch, glHostEpSize, 0);
        glDMATxCount++;
    }
}

/* Helper function to format the setup packet. */
static void
CyFxFormatSetupRqt (uint8_t *ptr,
                  uint8_t type,
                  uint8_t request,
                  uint16_t value,
                  uint16_t index,
                  uint16_t length)
{
    ptr[0] = type;
    ptr[1] = request;
    ptr[2] = CY_U3P_GET_LSB(value);
    ptr[3] = CY_U3P_GET_MSB(value);
    ptr[4] = CY_U3P_GET_LSB(index);
    ptr[5] = CY_U3P_GET_MSB(index);
    ptr[6] = CY_U3P_GET_LSB(length);
    ptr[7] = CY_U3P_GET_MSB(length);
}

/* This function initializes the mouse driver application. */
static void
CyFxApplnStart ()
{
    uint16_t length, size, offset;
    CyU3PDmaBuffer_t buf_p;
    CyU3PReturnStatus_t status;
    CyU3PUsbHostEpConfig_t epCfg;
    CyU3PDmaChannelConfig_t dmaCfg;
    CyU3PUsbHostEpStatus_t epStatus;

    /* Add EP0 to the scheduler. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof(epCfg));
    epCfg.type = CY_U3P_USB_EP_CONTROL;
    epCfg.mult = 1;
    /* Start off with 8 byte EP0 packet size. */
    epCfg.maxPktSize = 8;
    epCfg.pollingRate = 0;
    epCfg.fullPktSize = 8;
    epCfg.isStreamMode = CyFalse;
    status = CyU3PUsbHostEpAdd (0, &epCfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    CyU3PThreadSleep (100);
    /* Get the device descriptor. */
    CyFxFormatSetupRqt (glSetupPkt, 0x80, CY_U3P_USB_SC_GET_DESCRIPTOR,
            (CY_U3P_USB_DEVICE_DESCR << 8), 0, 8);
    status = CyU3PUsbHostSendSetupRqt (glSetupPkt, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
    status = CyU3PUsbHostEpWaitForCompletion (0, &epStatus,
            CY_FX_HOST_EP0_WAIT_TIMEOUT);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    /* Identify the EP0 packet size and update the scheduler. */
    if (glEp0Buffer[7] != 8)
    {
        status = CyU3PUsbHostEpRemove (0);
        if (status != CY_U3P_SUCCESS)
        {
            goto enum_error;
        }
        /* Update the correct size. */
        epCfg.maxPktSize = glEp0Buffer[7];
        epCfg.fullPktSize = glEp0Buffer[7];
        status = CyU3PUsbHostEpAdd (0, &epCfg);
        if (status != CY_U3P_SUCCESS)
        {
            goto enum_error;
        }
    }

    /* Read the full device descriptor. */
    CyFxFormatSetupRqt (glSetupPkt, 0x80, CY_U3P_USB_SC_GET_DESCRIPTOR,
            (CY_U3P_USB_DEVICE_DESCR << 8), 0, 18);
    status = CyU3PUsbHostSendSetupRqt (glSetupPkt, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
    status = CyU3PUsbHostEpWaitForCompletion (0, &epStatus,
            CY_FX_HOST_EP0_WAIT_TIMEOUT);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    /* Check for the VID and PID of the attached peripheral. */
    if ((CY_U3P_MAKEWORD(glEp0Buffer[9], glEp0Buffer[8]) != CY_FX_HOST_PERIPHERAL_VID) ||
            (CY_U3P_MAKEWORD(glEp0Buffer[11], glEp0Buffer[10]) != CY_FX_HOST_PERIPHERAL_PID))
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
        goto enum_error;
    }

    /* Check for device class, sub-class and protocol all of which has to be zero. */
    if ((glEp0Buffer[4] != 0) || (glEp0Buffer[5] != 0) || (glEp0Buffer[6] != 0))
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
        goto enum_error;
    }

    /* Set the peripheral device address. */
    CyFxFormatSetupRqt (glSetupPkt, 0x00, CY_U3P_USB_SC_SET_ADDRESS,
            CY_FX_HOST_PERIPHERAL_ADDRESS, 0, 0);
    status = CyU3PUsbHostSendSetupRqt (glSetupPkt, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
    status = CyU3PUsbHostEpWaitForCompletion (0, &epStatus,
            CY_FX_HOST_EP0_WAIT_TIMEOUT);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
    status = CyU3PUsbHostSetDeviceAddress (CY_FX_HOST_PERIPHERAL_ADDRESS);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    /* Read the OTG descriptor to identify its characteristics.
     * Do this only if we are not already in role change. */
    glIsHnpSupported = CyFalse;
    if (!CyU3POtgIsHnpEnabled ())
    {
        CyFxFormatSetupRqt (glSetupPkt, 0x80, CY_U3P_USB_SC_GET_DESCRIPTOR,
                (CY_U3P_USB_OTG_DESCR << 8), 0, 5);
        status = CyU3PUsbHostSendSetupRqt (glSetupPkt, glEp0Buffer);
        if (status != CY_U3P_SUCCESS)
        {
            goto enum_error;
        }
        status = CyU3PUsbHostEpWaitForCompletion (0, &epStatus,
                CY_FX_HOST_EP0_WAIT_TIMEOUT);
        /* If the device does not support OTG, the request will be stalled. */
        if ((status == CY_U3P_SUCCESS) && (glEp0Buffer[2] & 0x02))
        {
            /* Let the device know that the host is HNP capable. */
            CyFxFormatSetupRqt (glSetupPkt, 0x00, CY_U3P_USB_SC_SET_FEATURE,
                    CY_U3P_USB2_OTG_A_HNP_SUPPORT, 0, 0);
            status = CyU3PUsbHostSendSetupRqt (glSetupPkt, glEp0Buffer);
            if (status != CY_U3P_SUCCESS)
            {
                goto enum_error;
            }
            status = CyU3PUsbHostEpWaitForCompletion (0, &epStatus,
                    CY_FX_HOST_EP0_WAIT_TIMEOUT);

        }
        if (status == CY_U3P_SUCCESS)
        {
            glIsHnpSupported = CyTrue;
        }
    }

    /* Read first four bytes of configuration descriptor to determine
     * the total length. */
    CyFxFormatSetupRqt (glSetupPkt, 0x80, CY_U3P_USB_SC_GET_DESCRIPTOR,
            (CY_U3P_USB_CONFIG_DESCR << 8), 0, 4);
    status = CyU3PUsbHostSendSetupRqt (glSetupPkt, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
    status = CyU3PUsbHostEpWaitForCompletion (0, &epStatus,
            CY_FX_HOST_EP0_WAIT_TIMEOUT);
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
    CyFxFormatSetupRqt (glSetupPkt, 0x80, CY_U3P_USB_SC_GET_DESCRIPTOR,
            (CY_U3P_USB_CONFIG_DESCR << 8), 0, length);
    status = CyU3PUsbHostSendSetupRqt (glSetupPkt, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
    status = CyU3PUsbHostEpWaitForCompletion (0, &epStatus,
            CY_FX_HOST_EP0_WAIT_TIMEOUT);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    /* Check if the device can be supported. Number of interfaces should be 1,
     * the intreface class must be vendor (0xFF) and the subclass and protocol
     * must be zero. The number of endpoints must be two. Also the endpoints
     * should be bulk. */
    if ((glEp0Buffer[5] != 1) || (glEp0Buffer[14] != 0xFF) ||
            (glEp0Buffer[15] != 0x00) || (glEp0Buffer[16] != 0x00) ||
            (glEp0Buffer[13] != 2))
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
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
                status = CY_U3P_ERROR_NOT_SUPPORTED;
                goto enum_error;
            }

            /* Retreive the information. */
            glHostEpSize = CY_U3P_MAKEWORD(glEp0Buffer[offset + 5],
                    glEp0Buffer[offset + 4]);
            if (glEp0Buffer[offset + 2] & 0x80)
            {
                glHostInEp = glEp0Buffer[offset + 2];
            }
            else
            {
                glHostOutEp = glEp0Buffer[offset + 2];
            }
        }

        /* Advance to next descriptor. */
        offset += glEp0Buffer[offset];
    }

    /* If there is any error in the configuration abort. */
    if ((glHostOutEp == 0) || (glHostInEp == 0))
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
        goto enum_error;
    }

    /* Set the new configuration. */
    CyFxFormatSetupRqt (glSetupPkt, 0x00, CY_U3P_USB_SC_SET_CONFIGURATION, 1, 0, 0);
    status = CyU3PUsbHostSendSetupRqt (glSetupPkt, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
    status = CyU3PUsbHostEpWaitForCompletion (0, &epStatus,
            CY_FX_HOST_EP0_WAIT_TIMEOUT);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    /* Initialize the loopback application. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof(epCfg));
    epCfg.type = CY_U3P_USB_EP_BULK;
    epCfg.mult = 1;
    epCfg.maxPktSize = glHostEpSize;
    epCfg.pollingRate = 0;
    size = ((glHostEpSize + 0x0F) & ~0x0F);
    epCfg.fullPktSize = glHostEpSize;
    epCfg.isStreamMode = CyFalse;
    status = CyU3PUsbHostEpAdd (glHostOutEp, &epCfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
    status = CyU3PUsbHostEpAdd (glHostInEp, &epCfg);
    if (status != CY_U3P_SUCCESS)
    {
        glHostInEp = 0;
        goto app_error;
    }

    /* Reset counter to zero. */
    glDMARxCount = 0;
    glDMATxCount = 0;

    /* Create a DMA channels for IN and OUT directions. */
    CyU3PMemSet ((uint8_t *)&dmaCfg, 0, sizeof(dmaCfg));
    dmaCfg.size = size;
    dmaCfg.count = CY_FX_HOST_DMA_BUF_COUNT;
    dmaCfg.prodHeader = 0;
    dmaCfg.prodFooter = 0;
    dmaCfg.consHeader = 0;
    dmaCfg.prodAvailCount = 0;

    dmaCfg.prodSckId = (CyU3PDmaSocketId_t)(CY_U3P_UIB_SOCKET_PROD_0 + (0x0F & glHostInEp));
    dmaCfg.consSckId = CY_U3P_CPU_SOCKET_CONS;
    dmaCfg.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.notification = CY_U3P_DMA_CB_PROD_EVENT;
    dmaCfg.cb = CyFxHostDmaCb;
    status = CyU3PDmaChannelCreate (&glHostInCh, CY_U3P_DMA_TYPE_MANUAL_IN, &dmaCfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto app_error;
    }

    dmaCfg.prodSckId = CY_U3P_CPU_SOCKET_PROD;
    dmaCfg.consSckId = (CyU3PDmaSocketId_t)(CY_U3P_UIB_SOCKET_CONS_0 + (0x0F & glHostOutEp));
    dmaCfg.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.notification = CY_U3P_DMA_CB_CONS_EVENT;
    dmaCfg.cb = CyFxHostDmaCb;
    status = CyU3PDmaChannelCreate (&glHostOutCh, CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaCfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto app_error;
    }

    /* Set infinite transfer on both the channels. */
    status = CyU3PDmaChannelSetXfer (&glHostInCh, 0);
    if (status != CY_U3P_SUCCESS)
    {
        goto app_error;
    }
    status = CyU3PDmaChannelSetXfer (&glHostOutCh, 0);
    if (status != CY_U3P_SUCCESS)
    {
        goto app_error;
    }

    /* Pre-load all OUT buffers with fixed data. */
    for (offset = 0; offset < CY_FX_HOST_DMA_BUF_COUNT; offset++)
    {
        status = CyU3PDmaChannelGetBuffer (&glHostOutCh, &buf_p, CYU3P_NO_WAIT);
        if (status != CY_U3P_SUCCESS)
        {
            goto app_error;
        }
        CyU3PMemSet (buf_p.buffer, (uint8_t)CY_FX_HOST_DATA_BYTE, glHostEpSize);
        status = CyU3PDmaChannelCommitBuffer (&glHostOutCh, glHostEpSize, 0);
        if (status != CY_U3P_SUCCESS)
        {
            goto app_error;
        }
    }

    /* Queue a single read and single write request. */
    status = CyU3PUsbHostEpSetXfer (glHostInEp, CY_U3P_USB_HOST_EPXFER_NORMAL, glHostEpSize);
    if (status != CY_U3P_SUCCESS)
    {
        goto app_error;
    }
    status = CyU3PUsbHostEpSetXfer (glHostOutEp, CY_U3P_USB_HOST_EPXFER_NORMAL, glHostEpSize);
    if (status != CY_U3P_SUCCESS)
    {
        goto app_error;
    }

    glTimerCount = 0;
    glIsApplnActive = CyTrue;
    glIsHnp = CyFalse;
    glDoHnp = CyFalse;

    if (glIsHnpSupported)
    {
        CyU3PDebugPrint (4, "USB bulk loopback host mode operation started with HNP enabled.\r\n");
    }
    else
    {
        CyU3PDebugPrint (4, "USB bulk loopback host mode operation started with HNP disabled.\r\n");
    }

    return;

app_error:
    CyU3PDmaChannelDestroy (&glHostInCh);
    if (glHostInEp != 0)
    {
        CyU3PUsbHostEpRemove (glHostInEp);
        glHostInEp = 0;
    }
    CyU3PDmaChannelDestroy (&glHostOutCh);
    if (glHostOutEp != 0)
    {
        CyU3PUsbHostEpRemove (glHostOutEp);
        glHostOutEp = 0;
    }

enum_error:
    /* Remove EP0. and disable the port. */
    CyU3PUsbHostEpRemove (0);
    glHostEpSize = 0;
    CyU3PUsbHostPortDisable ();
    CyU3PDebugPrint (4, "Host mode application start failed with error: %d.\r\n", status);
}

/* This function disables the mouse driver application. */
static void
CyFxApplnStop ()
{
    /* Destroy the DMA channel. */
    CyU3PDmaChannelDestroy (&glHostInCh);
    if (glHostInEp != 0)
    {
        CyU3PUsbHostEpRemove (glHostInEp);
        glHostInEp = 0;
    }
    CyU3PDmaChannelDestroy (&glHostOutCh);
    if (glHostOutEp != 0)
    {
        CyU3PUsbHostEpRemove (glHostOutEp);
        glHostOutEp = 0;
    }

    /* Remove EP0. and disable the port. */
    CyU3PUsbHostEpRemove (0);
    glHostEpSize = 0;
    CyU3PUsbHostPortDisable ();

    /* Clear state variables. */
    glIsApplnActive = CyFalse;
    glTimerCount = 0;
    glIsHnp = CyFalse;
    glDoHnp = CyFalse;
    glIsHnpSupported = CyFalse;
    /* Reset counter to zero. */
    glDMARxCount = 0;
    glDMATxCount = 0;

    CyU3PDebugPrint (4, "Host mode application stopped.\r\n");
}

/* This function initializes the USB host stack. */
void
CyFxUsbHostStart ()
{
    CyU3PUsbHostConfig_t hostCfg;
    CyU3PReturnStatus_t status;

    hostCfg.ep0LowLevelControl = CyFalse;
    hostCfg.eventCb = CyFxHostEventCb;
    hostCfg.xferCb = CyFxHostXferCb;
    status = CyU3PUsbHostStart (&hostCfg);
    if (status != CY_U3P_SUCCESS)
    {
        return;
    }

    CyU3PDebugPrint (4, "USB host stack initialized.\r\n");
}

/* This function disables the USB host stack. */
void
CyFxUsbHostStop ()
{
    /* Stop host mode application if running. */
    if (glIsApplnActive)
    {
        CyFxApplnStop ();
    }

    /* Stop the host module. */
    CyU3PUsbHostStop ();

    CyU3PDebugPrint (4, "USB host stack disabled.\r\n");
}

/* This function will be called periodically from
 * the application thread. If there is a change in
 * the peripheral status, this function will start
 * or stop the application accordingly. */
void
CyFxUsbHostDoWork ()
{
    uint32_t upTime = 0, epStatus = 0;
    static CyBool_t isPresent = CyFalse;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    static uint32_t rxCount = 0;
    static uint32_t txCount = 0;

    if (isPresent != glIsPeripheralPresent)
    {
        if (glDoHnp)
        {
            /* Complete the HNP. */
            CyFxHostHnp ();
            glDoHnp = CyFalse;
        }
        else
        {
            /* Stop previously started application. */
            if (glIsApplnActive)
            {
                CyFxApplnStop ();
            }

            /* If a peripheral got connected, then enumerate
             * and start the application. */
            if (glIsPeripheralPresent)
            {
                status = CyU3PUsbHostPortEnable ();
                if (status == CY_U3P_SUCCESS)
                {
                    CyFxApplnStart ();
                }
            }
        }

        /* Update the state variable. */
        isPresent = glIsPeripheralPresent;
    }

    if (glIsApplnActive)
    {
        if ((rxCount != glDMARxCount) || (txCount != glDMATxCount))
        {
            CyU3PDebugPrint (4, "Host mode: transferred %d buffers, received %d buffers.\r\n",
                    glDMATxCount, glDMARxCount);

            rxCount = glDMARxCount;
            txCount = glDMATxCount;
        }
        /* OTG status polling. */
        upTime = glTimerCount * CY_FX_OTG_POLL_INTERVAL;
        glTimerCount++;
        if ((glIsHnpSupported) && ((upTime % CY_FX_OTG_STATUS_POLL_INTERVAL) == 0)
            && (!CyU3POtgIsHnpEnabled ()) && (!glIsHnp) && (!glDoHnp))
        {
            /* Get the OTG status. */
            CyFxFormatSetupRqt (glSetupPkt, 0x80, CY_U3P_USB_SC_GET_STATUS,
                    0, CY_U3P_USB_OTG_STATUS_SELECTOR, 1);
            status = CyU3PUsbHostSendSetupRqt (glSetupPkt, glEp0Buffer);
            if (status == CY_U3P_SUCCESS)
            {
                status = CyU3PUsbHostEpWaitForCompletion (0, &epStatus,
                        CY_FX_HOST_EP0_WAIT_TIMEOUT);
            }
            /* Check if the session request flag is set. */
            if ((status == CY_U3P_SUCCESS) && (glEp0Buffer[0] & 0x01))
            {
                /* Initiate the HNP process. */
                CyFxFormatSetupRqt (glSetupPkt, 0x00, CY_U3P_USB_SC_SET_FEATURE,
                        CY_U3P_USB2_OTG_B_HNP_ENABLE, 0, 0);
                status = CyU3PUsbHostSendSetupRqt (glSetupPkt, glEp0Buffer);
                if (status == CY_U3P_SUCCESS)
                {
                    status = CyU3PUsbHostEpWaitForCompletion (0, &epStatus,
                            CY_FX_HOST_EP0_WAIT_TIMEOUT);
                }
                if (status == CY_U3P_SUCCESS)
                {
                    glIsHnp = CyTrue;
                    /* Stop all active transfers. */
                    CyU3PDmaChannelDestroy (&glHostInCh);
                    if (glHostInEp != 0)
                    {
                        CyU3PUsbHostEpRemove (glHostInEp);
                        glHostInEp = 0;
                    }
                    CyU3PDmaChannelDestroy (&glHostOutCh);
                    if (glHostOutEp != 0)
                    {
                        CyU3PUsbHostEpRemove (glHostOutEp);
                        glHostOutEp = 0;
                    }

                    /* Remove EP0. and disable the port. */
                    CyU3PUsbHostEpRemove (0);
                    glHostEpSize = 0;

                    /* Suspend the USB bus and wait for the device to disconnect. */
                    status = CyU3PUsbHostPortSuspend ();
                }
            }
            if (status != CY_U3P_SUCCESS)
            {
                glIsHnp = CyFalse;
                glIsHnpSupported = CyFalse;
                CyU3PUsbHostEpAbort (0);
                CyU3PDebugPrint (4, "HNP disabled due to GET_STATUS failure.\r\n");
            }
        }
    } 
}

/* [ ] */

