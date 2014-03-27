/*
 ## Cypress USB 3.0 Platform source file (cyfxmousedrv.c)
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

/* This file illustrates the mouse driver. */

/*
   A simple single interface USB HID mouse will be successfully enumerated
   and the current offset will be printed via the UART debug logs.

   We support only single interface with interface class = HID(0x03),
   interface sub class = Boot (0x01) and interface protocol = Mouse (0x02).
   This example supports only 4 byte input reports with the following format:
        BYTE0: Bitmask for each of the button present.
        BYTE1: Signed movement in X direction.
        BYTE2: Signed movement in Y direction.
        BYTE3: Signed movement in scroll wheel.
   Further types can be implemented by decoding the HID descriptor.
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

uint8_t glHostMouseEp = 0;                      /* Interrupt endpoint used by the mouse application. */

CyU3PDmaChannel glHostMouseCh;                  /* DMA channel for mouse application. */

/* DMA callback for the mouse application. */
void
CyFxMouseDmaCb (CyU3PDmaChannel *ch,
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

/* Initalizes the mouse driver. */
CyU3PReturnStatus_t
CyFxMouseDriverInit ()
{
    uint16_t length, size, interval;
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

    /* Save the required information. */
    glHostMouseEp = glEp0Buffer[29];
    size = CY_U3P_MAKEWORD(glEp0Buffer[32], glEp0Buffer[31]);
    interval = glEp0Buffer[33];

    /* Set the new configuration. */
    status = CyFxSendSetupRqt (0x00, CY_U3P_USB_SC_SET_CONFIGURATION, 1, 0, 0, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    /* Set the report mode to idle so that report is sent only
     * when there is active data. */
    status = CyFxSendSetupRqt (0x21, 0x0A, 0, 0, 0, glEp0Buffer);
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
    dmaCfg.cb = CyFxMouseDmaCb;
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

    CyU3PDebugPrint (4, "USB HID Mouse driver initialized successfully.\r\n");
    return CY_U3P_SUCCESS;

app_error:
    CyU3PDmaChannelDestroy (&glHostMouseCh);
    if (glHostMouseEp != 0)
    {
        CyU3PUsbHostEpRemove (glHostMouseEp);
        glHostMouseEp = 0;
    }

enum_error:
    return CY_U3P_ERROR_FAILURE;
}

/* Disables the mouse driver. */
void
CyFxMouseDriverDeInit ()
{
    /* Destroy the DMA channel. */
    CyU3PDmaChannelDestroy (&glHostMouseCh);
    if (glHostMouseEp != 0)
    {
        CyU3PUsbHostEpRemove (glHostMouseEp);
        glHostMouseEp = 0;
    }
}

/* [ ] */

