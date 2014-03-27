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
#include "cyfxusbotg.h"

static CyBool_t glIsApplnActive = CyFalse;             /* Whether the application is active or not. */
static CyBool_t glIsPeripheralPresent = CyFalse;       /* Whether a remote peripheral is present or not. */

static uint8_t glSetupPkt[CY_FX_HOST_EP0_SETUP_SIZE] __attribute__ ((aligned (32)));  /* Setup packet buffer for host mode. */
static uint8_t glEp0Buffer[CY_FX_HOST_EP0_BUFFER_SIZE] __attribute__ ((aligned (32)));/* Buffer to send / receive data for EP0. */
static uint8_t glHostMouseEp = 0;                      /* Interrupt endpoint used by the mouse application. */

static CyU3PDmaChannel glHostMouseCh;                  /* DMA channel for mouse application. */

/* USB host stack EP transfer completion callback. */
static void
CyFxHostXferCb (uint8_t ep, CyU3PUsbHostEpStatus_t epStatus)
{
    /* This callback is not used in this current application. */
}

/* USB host stack event callback function. */
static void
CyFxHostEventCb (CyU3PUsbHostEventType_t evType, uint32_t evData)
{
    /* This is connect / disconnect event. Log it so that the
     * application thread can handle it. */
    if (evType == CY_U3P_USB_HOST_EVENT_CONNECT)
    {
        glIsPeripheralPresent = CyTrue;
    }
    else
    {
        glIsPeripheralPresent = CyFalse;
    }
}

/* DMA callback for the mouse application. */
static void
CyFxHostDmaCb (CyU3PDmaChannel *ch,
        CyU3PDmaCbType_t type,
        CyU3PDmaCBInput_t *input)
{
    uint8_t *buf = NULL;

    if (type == CY_U3P_DMA_CB_PROD_EVENT)
    {
        if (input->buffer_p.count < 4)
        {
            CyU3PDebugPrint (4, "Unknown mouse input.\r\n");
            return;
        }
        /* Print the current mouse event. This example supports only
         * 4 byte input reports with the following format:
         *      BYTE0: Bitmask for each of the button present.
         *      BYTE1: Signed movement in X direction.
         *      BYTE2: Signed movement in Y direction.
         *      BYTE3: Signed movement in scroll wheel. */
        buf = input->buffer_p.buffer;
        CyU3PDebugPrint (4, "Mouse event: X = %d, Y = %d, scroll = %d, BMask = 0x%x.\r\n",
                (int8_t)buf[1], (int8_t)buf[2], (int8_t)buf[3], (uint8_t)buf[0]);

        /* Discard the current buffer to free it. */
        CyU3PDmaChannelDiscardBuffer (ch);
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
    uint16_t length, size, interval;
    CyU3PUsbHostEpStatus_t epStatus;
    CyU3PReturnStatus_t status;
    CyU3PUsbHostEpConfig_t epCfg;
    CyU3PDmaChannelConfig_t dmaCfg;

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
        CyU3PUsbHostEpRemove (0);
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

    /* Identify if this is an HID mouse that can be supported. If
     * the device cannot be supported, just disable the port and
     * wait for a new device to be attached. We support only single
     * interface with interface class = HID(0x03),
     * interface sub class = Boot (0x01) 
     * and interface protocol = Mouse (0x02). */
    if ((glEp0Buffer[5] != 1) || (glEp0Buffer[14] != 0x03) ||
            (glEp0Buffer[15] != 0x01) || (glEp0Buffer[16] != 0x02) ||
            (glEp0Buffer[28] != CY_U3P_USB_ENDPNT_DESCR))
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
        goto enum_error;
    }

    /* Save the required information. */
    glHostMouseEp = glEp0Buffer[29];
    size = CY_U3P_MAKEWORD(glEp0Buffer[32], glEp0Buffer[31]);
    interval = glEp0Buffer[33];

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

    /* Set the report mode to idle so that report is sent only
     * when there is active data. */
    CyFxFormatSetupRqt (glSetupPkt, 0x21, 0x0A, 0, 0, 0);
    status = CyU3PUsbHostSendSetupRqt (glSetupPkt, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
    status = CyU3PUsbHostEpWaitForCompletion (0, &epStatus,
            CY_FX_HOST_EP0_WAIT_TIMEOUT);
#if 0 /* It does not matter even if the request gets stalled. */
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
#endif

    /* Initialize the HID mouse. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof(epCfg));
    epCfg.type = CY_U3P_USB_EP_INTR;
    epCfg.mult = 1;
    /* Start off with 8 byte EP0 packet size. */
    epCfg.maxPktSize = size;
    interval = (1 << (interval - 1)) / 8;
    if (interval > 255)
    {
        interval = 255;
    }
    epCfg.pollingRate = interval;
    /* Since DMA buffer sizes can only be multiple of 16 bytes and
     * also since this is an interrupt endpoint where the max data
     * packet size is same as the maxPktSize field, the fullPktSize
     * has to be a multiple of 16 bytes. */
    size = ((size + 0x0F) & ~0x0F);
    epCfg.fullPktSize = size;
    /* Since the IN token has to be sent out continously, it
     * is easier to enable the stream mode. Otherwise we will
     * have to maintain the timing. */
    epCfg.isStreamMode = CyTrue;
    status = CyU3PUsbHostEpAdd (glHostMouseEp, &epCfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    /* Create a DMA channel for this EP. */
    CyU3PMemSet ((uint8_t *)&dmaCfg, 0, sizeof(dmaCfg));
    dmaCfg.size = size;
    dmaCfg.count = CY_FX_HOST_DMA_BUF_COUNT;
    dmaCfg.prodSckId = (CyU3PDmaSocketId_t)(CY_U3P_UIB_SOCKET_PROD_0 + (0x0F & glHostMouseEp));
    dmaCfg.consSckId = CY_U3P_CPU_SOCKET_CONS;
    dmaCfg.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.notification = CY_U3P_DMA_CB_PROD_EVENT;
    dmaCfg.cb = CyFxHostDmaCb;
    dmaCfg.prodHeader = 0;
    dmaCfg.prodFooter = 0;
    dmaCfg.consHeader = 0;
    dmaCfg.prodAvailCount = 0;
    status = CyU3PDmaChannelCreate (&glHostMouseCh, CY_U3P_DMA_TYPE_MANUAL_IN, &dmaCfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto app_error;
    }

    /* Enable EP transfer. In stream mode, the transfer size should be zero. */
    status = CyU3PUsbHostEpSetXfer (glHostMouseEp, CY_U3P_USB_HOST_EPXFER_NORMAL, 0);
    if (status != CY_U3P_SUCCESS)
    {
        goto app_error;
    }

    /* Set for infinite transfer. */
    status = CyU3PDmaChannelSetXfer (&glHostMouseCh, 0);
    if (status != CY_U3P_SUCCESS)
    {
        goto app_error;
    }

    glIsApplnActive = CyTrue;
    CyU3PDebugPrint (4, "USB HID mouse driver initialized.\r\n");
    return;

app_error:
    CyU3PDmaChannelDestroy (&glHostMouseCh);
    if (glHostMouseEp != 0)
    {
        CyU3PUsbHostEpRemove (glHostMouseEp);
        glHostMouseEp = 0;
    }

enum_error:
    /* Remove EP0. and disable the port. */
    CyU3PUsbHostEpRemove (0);
    CyU3PUsbHostPortDisable ();
    CyU3PDebugPrint (4, "Application start failed with error: %d.\r\n", status);
}

/* This function disables the mouse driver application. */
static void
CyFxApplnStop ()
{
    /* Destroy the DMA channel. */
    CyU3PDmaChannelDestroy (&glHostMouseCh);
    if (glHostMouseEp != 0)
    {
        CyU3PUsbHostEpRemove (glHostMouseEp);
        glHostMouseEp = 0;
    }

    /* Remove EP0. and disable the port. */
    CyU3PUsbHostEpRemove (0);
    CyU3PUsbHostPortDisable ();

    /* Clear state variables. */
    glIsApplnActive = CyFalse;

    CyU3PDebugPrint (4, "USB HID mouse driver disabled.\r\n");
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
}

/* This function will be called periodically from
 * the application thread. If there is a change in
 * the peripheral status, this function will start
 * or stop the application accordingly. */
void
CyFxUsbHostDoWork ()
{
    static CyBool_t isPresent = CyFalse;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    if (isPresent != glIsPeripheralPresent)
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

        /* Update the state variable. */
        isPresent = glIsPeripheralPresent;
    }
}

/* [ ] */

