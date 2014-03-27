/*
 ## Cypress USB 3.0 Platform source file (cyfxbulklplowlevel.c)
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

/* This file illustrates the bulkloop application example using socket and descriptor APIs. */

/*
   This examples illustrate a loopback mechanism between two USB bulk endpoints. The example
   comprises of vendor class USB enumeration descriptors with two bulk endpoints. A bulk OUT
   endpoint acts as the producer of data from the host. A bulk IN endpoint acts as the consumer
   of data to the host. The loopback is achieved with the help of a low level DMA descriptor and
   socket APIs.

   Data is received in the IN DMA buffer from the host through the producer endpoint. CPU waits
   for data from this channel and then copies the contents of the IN channel DMA buffer into the
   OUT channel DMA buffer. The CPU commits the DMA data transfer to the consumer endpoint which
   then gets transferred to the host.

   The DMA buffer size for each channel is defined based on the USB speed. 64 for full speed,
   512 for high speed and 1024 for super speed.

   NOTE: This example is made simple and does transfers at single buffer basis. This means that
   USB host can only send out a single USB packet in one transfer and has to read back before
   sending the next packet.
 */

#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3uart.h"
#include "cyu3socket.h"
#include "cyu3descriptor.h"
#include "sock_regs.h"
#include "cyfxbulklplowlevel.h"

CyU3PThread     BulkLpAppThread;	/* Bulk loop application thread structure */
CyBool_t glIsApplnActive = CyFalse;     /* Whether the loopback application is active or not. */

uint16_t glDscrIndex = 0xFFFF;          /* The index of the descriptor to be used for DMA transfers. */
uint16_t glDmaSize = 0;                 /* Size of the DMA descriptor. */
uint8_t *glDmaBuffer = NULL;            /* Pointer to buffer for DMA transfers. */
uint32_t glDMARxCount = 0;              /* Number of buffers received. */

/* Application Error Handler */
void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus    /* API return status */
        )
{
    /* Application failed with the error code apiRetStatus */

    /* Add custom debug or recovery actions here */

    /* Loop Indefinitely */
    for (;;)
    {
        /* Thread sleep : 100 ms */
        CyU3PThreadSleep (100);
    }
}

/* This function initializes the debug module. The debug prints
 * are routed to the UART and can be seen using a UART console
 * running at 115200 baud rate. */
void
CyFxBulkLpApplnDebugInit (void)
{
    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize the UART for printing debug messages */
    apiRetStatus = CyU3PUartInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set UART configuration */
    CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof (uartConfig));
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyFalse;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma = CyTrue;

    apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set the UART transfer to a really large value. */
    apiRetStatus = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Initialize the debug module. */
    apiRetStatus = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }
}

/* Configure the producer socket to read a buffer of data. 
 * This function does not wait for the data to be recieved. */
CyU3PReturnStatus_t
CyFxMyDmaSetupReadBuffer ()
{
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaSocketConfig_t sck;

    /* First disable the socket before configuration. */
    CyU3PDmaSocketDisable (CY_FX_EP_PRODUCER_SOCKET);

    /* First configure the descriptor. */
    dscr.buffer = glDmaBuffer;
    /* Give only a valid pointer to the buffer. */
    if (dscr.buffer == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    /* Update the sync field so that the correct producer socket, 
     * produce event and produce interrupt is enabled. */
    dscr.sync   = ((CY_FX_EP_PRODUCER_SOCKET << CY_U3P_PROD_SCK_POS) |
            CY_U3P_EN_PROD_EVENT | CY_U3P_EN_PROD_INT);
    /* Since there is no consumer, update the consumer socket field with
     * invalid value. */
    dscr.sync  |= (CY_U3P_CPU_SOCKET_CONS);

    /* Since this is a single transfer, mark both the next descriptors
     * as invalid (0xFFFF). */
    dscr.chain  = 0xFFFFFFFF;
    /* Update the DMA buffer size. */
    dscr.size = (glDmaSize & CY_U3P_BUFFER_SIZE_MASK);
    /* Write the descriptor values to memory. */
    CyU3PDmaDscrSetConfig (glDscrIndex, &dscr);

    /* Now configure the socket. */
    /* Load the dscrChain field with the current descriptor index. */
    sck.dscrChain = glDscrIndex;
    /* Set for a single buffer transfer. */
    sck.xferSize = 1;
    sck.xferCount = 0;
    /* Set the socket to suspend when the transfer size is completed.
     * Enable the produce event handling. Also make the unit of count
     * as buffers instead of bytes. Then enable the socket. */
    sck.status = (CY_U3P_TRUNCATE | CY_U3P_SUSP_TRANS |
            CY_U3P_EN_PROD_EVENTS | CY_U3P_UNIT | CY_U3P_GO_ENABLE);
    /* Clear any pending interrupts. */
    sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
    /* Enable produce event interrupt. */
    sck.intrMask = (CY_U3P_PRODUCE_EVENT);
    /* Update the socket configuration. */
    CyU3PDmaSocketSetConfig (CY_FX_EP_PRODUCER_SOCKET, &sck);

    return CY_U3P_SUCCESS;
}

/* Configure the consumer socket to write a buffer of data. 
 * This function does not wait for the data to be sent out. */
CyU3PReturnStatus_t
CyFxMyDmaSetupWriteBuffer (uint16_t count)
{
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaSocketConfig_t sck;

    /* First disable the socket before configuration. */
    CyU3PDmaSocketDisable (CY_FX_EP_CONSUMER_SOCKET);

    /* First configure the descriptor. */
    dscr.buffer = glDmaBuffer;
    /* Give only a valid pointer to the buffer. */
    if (dscr.buffer == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    /* Update the sync field so that the correct consumer socket, 
     * consume event and consume interrupt is enabled. */
    dscr.sync   = (CY_FX_EP_CONSUMER_SOCKET | CY_U3P_EN_CONS_EVENT | CY_U3P_EN_CONS_INT);
    /* Since there is no producer, update the producer socket field with
     * invalid value. */
    dscr.sync  |= (CY_U3P_CPU_SOCKET_PROD << CY_U3P_PROD_SCK_POS);

    /* Since this is a single transfer, mark both the next descriptors
     * as invalid (0xFFFF). */
    dscr.chain  = 0xFFFFFFFF;
    /* Update the DMA buffer size. */
    dscr.size = (glDmaSize & CY_U3P_BUFFER_SIZE_MASK);
    /* Load the count field. */
    dscr.size |= (count << CY_U3P_BYTE_COUNT_POS);
    /* Mark the buffer as occupied. */
    dscr.size |= CY_U3P_BUFFER_OCCUPIED;
    /* Write the descriptor values to memory. */
    CyU3PDmaDscrSetConfig (glDscrIndex, &dscr);

    /* Now configure the socket. */
    /* Load the dscrChain field with the current descriptor index. */
    sck.dscrChain = glDscrIndex;
    /* Set for a single buffer transfer. */
    sck.xferSize = 1;
    sck.xferCount = 0;
    /* Set the socket to suspend when the transfer size is completed.
     * Enable the consume event handling. Also make the unit of count
     * as buffers instead of bytes. Then enable the socket. */
    sck.status = (CY_U3P_TRUNCATE | CY_U3P_SUSP_TRANS |
            CY_U3P_EN_CONS_EVENTS | CY_U3P_UNIT | CY_U3P_GO_ENABLE);
    /* Clear any pending interrupts. */
    sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
    /* Enable consume event interrupt. */
    sck.intrMask = (CY_U3P_CONSUME_EVENT);
    /* Update the socket configuration. */
    CyU3PDmaSocketSetConfig (CY_FX_EP_CONSUMER_SOCKET, &sck);

    return CY_U3P_SUCCESS;
}

/* DMA socket event callback function. */
void
CyFxMyDmaSocketCb (
        uint16_t sckId,
        uint32_t status)
{
    uint16_t count = 0;
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaSocketConfig_t sck;

    if (sckId == CY_FX_EP_PRODUCER_SOCKET)
    {
        /* Increment the receive count by 1. */
        glDMARxCount++;

        /* Data has been received from the USB host. Read the
         * descriptor to find the amount of data received. */
        CyU3PDmaDscrGetConfig (glDscrIndex, &dscr);
        count = (dscr.size >> CY_U3P_BYTE_COUNT_POS);

        /* Send the data back to USB host. */
        CyFxMyDmaSetupWriteBuffer (count);
    }
    else
    {
        /* Data has been sent back to the USB host.
         * Queue another read. */
        CyFxMyDmaSetupReadBuffer ();
    }

    /* Clear the socket interrupts. */
    CyU3PDmaSocketGetConfig (sckId, &sck);
    sck.intr = status;
    CyU3PDmaSocketSetConfig (sckId, &sck);
}

/* This function starts the bulk loop application. This is called
 * when a SET_CONF event is received from the USB host. The endpoints
 * are configured and the DMA pipe is setup in this function. */
void
CyFxBulkLpApplnStart (
        void)
{
    uint16_t size = 0;
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();

    /* First identify the usb speed. Once that is identified,
     * create a DMA channel and start the transfer on this. */

    /* Based on the Bus Speed configure the endpoint packet size */
    switch (usbSpeed)
    {
        case CY_U3P_FULL_SPEED:
            size = 64;
            break;

        case CY_U3P_HIGH_SPEED:
            size = 512;
            break;

        case  CY_U3P_SUPER_SPEED:
            size = 1024;
            break;

        default:
            CyU3PDebugPrint (4, "Error! Invalid USB speed.\n");
            CyFxAppErrorHandler (CY_U3P_ERROR_FAILURE);
            break;
    }

    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyTrue;
    epCfg.epType = CY_U3P_USB_EP_BULK;
    epCfg.burstLen = 1;
    epCfg.streams = 0;
    epCfg.pcktSize = size;

    /* Producer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);

    /* Allocate a free DMA descriptor for socket configuration. */
    apiRetStatus = CyU3PDmaDscrGet (&glDscrIndex);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaDscrGet failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Allocate a DMA buffer for the data transfer. */
    glDmaBuffer = CyU3PDmaBufferAlloc (size);
    if (glDmaBuffer == NULL)
    {
        CyU3PDebugPrint (4, "CyU3PDmaBufferAlloc failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Disable the sockets before configuring. */
    CyU3PDmaSocketDisable (CY_FX_EP_PRODUCER_SOCKET);
    CyU3PDmaSocketDisable (CY_FX_EP_CONSUMER_SOCKET);

    /* Save the size of buffer. */
    glDmaSize = size;

    /* Register DMA socket event callback function. */
    CyU3PDmaSocketRegisterCallback (CyFxMyDmaSocketCb);

    /* Configure the socket producer socket to transmit one buffer of data. */
    CyFxMyDmaSetupReadBuffer ();

    /* Update the flag so that the application thread is notified of this. */
    glIsApplnActive = CyTrue;
}

/* This function stops the bulk loop application. This shall be called whenever
 * a RESET or DISCONNECT event is received from the USB host. The endpoints are
 * disabled and the DMA pipe is destroyed by this function. */
void
CyFxBulkLpApplnStop (
        void)
{
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Update the flag so that the application thread is notified of this. */
    glIsApplnActive = CyFalse;

    /* Disable the DMA sockets. */
    CyU3PDmaSocketDisable (CY_FX_EP_PRODUCER_SOCKET);
    CyU3PDmaSocketDisable (CY_FX_EP_CONSUMER_SOCKET);

    /* Load invalid values. */
    glDmaBuffer = NULL;
    glDscrIndex = 0;
    glDmaSize = 0;

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);

    /* Disable endpoints. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyFalse;

    /* Producer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }
}

/* Callback to handle the USB setup requests. */
CyBool_t
CyFxBulkLpApplnUSBSetupCB (
        uint32_t setupdat0, /* SETUP Data 0 */
        uint32_t setupdat1  /* SETUP Data 1 */
    )
{
    /* Fast enumeration is used. Only requests addressed to the interface, class,
     * vendor and unknown control requests are received by this function.
     * This application does not support any class or vendor requests. */

    uint8_t  bRequest, bReqType;
    uint8_t  bType, bTarget;
    uint16_t wValue, wIndex;
    CyBool_t isHandled = CyFalse;

    /* Decode the fields from the setup request. */
    bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bType    = (bReqType & CY_U3P_USB_TYPE_MASK);
    bTarget  = (bReqType & CY_U3P_USB_TARGET_MASK);
    bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue   = ((setupdat0 & CY_U3P_USB_VALUE_MASK)   >> CY_U3P_USB_VALUE_POS);
    wIndex   = ((setupdat1 & CY_U3P_USB_INDEX_MASK)   >> CY_U3P_USB_INDEX_POS);

    if (bType == CY_U3P_USB_STANDARD_RQT)
    {
        /* Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
         * requests here. It should be allowed to pass if the device is in configured
         * state and failed otherwise. */
        if ((bTarget == CY_U3P_USB_TARGET_INTF) && ((bRequest == CY_U3P_USB_SC_SET_FEATURE)
                    || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) && (wValue == 0))
        {
            if (glIsApplnActive)
                CyU3PUsbAckSetup ();
            else
                CyU3PUsbStall (0, CyTrue, CyFalse);

            isHandled = CyTrue;
        }

        /* CLEAR_FEATURE request for endpoint is always passed to the setup callback
         * regardless of the enumeration model used. When a clear feature is received,
         * the previous transfer has to be flushed and cleaned up. This is done at the
         * protocol level. Since this is just a loopback operation, there is no higher
         * level protocol. So flush the EP memory and reset the DMA channel associated
         * with it. If there are more than one EP associated with the channel reset both
         * the EPs. The endpoint stall and toggle / sequence number is also expected to be
         * reset. Return CyFalse to make the library clear the stall and reset the endpoint
         * toggle. Or invoke the CyU3PUsbStall (ep, CyFalse, CyTrue) and return CyTrue.
         * Here we are clearing the stall. */
        if ((bTarget == CY_U3P_USB_TARGET_ENDPT) && (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)
                && (wValue == CY_U3P_USBX_FS_EP_HALT))
        {
            if ((wIndex == CY_FX_EP_PRODUCER) || (wIndex == CY_FX_EP_CONSUMER))
            {
                if (glIsApplnActive)
                {
                    CyU3PDmaSocketDisable (CY_FX_EP_PRODUCER_SOCKET);
                    CyU3PDmaSocketDisable (CY_FX_EP_CONSUMER_SOCKET);
                    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
                    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);
                    CyU3PUsbResetEp (CY_FX_EP_PRODUCER);
                    CyU3PUsbResetEp (CY_FX_EP_CONSUMER);
                    CyU3PUsbStall (wIndex, CyFalse, CyTrue);
                    CyFxMyDmaSetupReadBuffer ();

                    CyU3PUsbAckSetup ();
                    isHandled = CyTrue;
                }
            }
        }
    }

    return isHandled;
}

/* This is the callback function to handle the USB events. */
void
CyFxBulkLpApplnUSBEventCB (
    CyU3PUsbEventType_t evtype, /* Event type */
    uint16_t            evdata  /* Event data */
    )
{
    switch (evtype)
    {
        case CY_U3P_USB_EVENT_SETCONF:
            /* Stop the application before re-starting. */
            if (glIsApplnActive)
            {
                CyFxBulkLpApplnStop ();
            }
            /* Start the loop back function. */
            CyFxBulkLpApplnStart ();
            break;

        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_DISCONNECT:
            /* Stop the loop back function. */
            if (glIsApplnActive)
            {
                CyFxBulkLpApplnStop ();
            }
            break;

        default:
            break;
    }
}

/* Callback function to handle LPM requests from the USB 3.0 host. This function is invoked by the API
   whenever a state change from U0 -> U1 or U0 -> U2 happens. If we return CyTrue from this function, the
   FX3 device is retained in the low power state. If we return CyFalse, the FX3 device immediately tries
   to trigger an exit back to U0.

   This application does not have any state in which we should not allow U1/U2 transitions; and therefore
   the function always return CyTrue.
 */
CyBool_t
CyFxBulkLpApplnLPMRqtCB (
        CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}

/* This function initializes the USB Module, sets the enumeration descriptors.
 * This function does not start the bulk streaming and this is done only when
 * SET_CONF event is received. */
void
CyFxBulkLpApplnInit (void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Start the USB functionality. */
    apiRetStatus = CyU3PUsbStart();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PUsbStart failed to Start, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* The fast enumeration is the easiest way to setup a USB connection,
     * where all enumeration phase is handled by the library. Only the
     * class / vendor requests need to be handled by the application. */
    CyU3PUsbRegisterSetupCallback(CyFxBulkLpApplnUSBSetupCB, CyTrue);

    /* Setup the callback to handle the USB events. */
    CyU3PUsbRegisterEventCallback(CyFxBulkLpApplnUSBEventCB);

    /* Register a callback to handle LPM requests from the USB 3.0 host. */
    CyU3PUsbRegisterLPMRequestCallback(CyFxBulkLpApplnLPMRqtCB);

    /* Set the USB Enumeration descriptors */

    /* Super speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB30DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* High speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB20DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* BOS descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, NULL, (uint8_t *)CyFxUSBBOSDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set configuration descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Device qualifier descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, NULL, (uint8_t *)CyFxUSBDeviceQualDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device qualifier descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Super speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBSSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set configuration descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* High speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBHSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Other Speed Descriptor failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Full speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBFSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Configuration Descriptor failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 0 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 1 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 2 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Connect the USB Pins with super speed operation enabled. */
    apiRetStatus = CyU3PConnectState(CyTrue, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Connect failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }
}

/* Entry function for the BulkLpAppThread. */
void
BulkLpAppThread_Entry (
        uint32_t input)
{
    /* Initialize the debug module */
    CyFxBulkLpApplnDebugInit();

    /* Initialize the bulk loop application */
    CyFxBulkLpApplnInit();

    for (;;)
    {
        CyU3PThreadSleep (1000);
        if (glIsApplnActive)
        {
            CyU3PDebugPrint (6, "Data tracker: Number of buffers received: %d\n", glDMARxCount);
        }
    }
}

/* Application define function which creates the threads. */
void
CyFxApplicationDefine (
        void)
{
    void *ptr = NULL;
    uint32_t retThrdCreate = CY_U3P_SUCCESS;

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_BULKLP_THREAD_STACK);

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (&BulkLpAppThread,           /* Bulk loop App Thread structure */
                          "21:Bulk_loop_MANUAL_IN_OUT",            /* Thread ID and Thread name */
                          BulkLpAppThread_Entry,                   /* Bulk loop App Thread Entry function */
                          0,                                       /* No input parameter to thread */
                          ptr,                                     /* Pointer to the allocated thread stack */
                          CY_FX_BULKLP_THREAD_STACK,               /* Bulk loop App Thread stack size */
                          CY_FX_BULKLP_THREAD_PRIORITY,            /* Bulk loop App Thread priority */
                          CY_FX_BULKLP_THREAD_PRIORITY,            /* Bulk loop App Thread priority */
                          CYU3P_NO_TIME_SLICE,                     /* No time slice for the application thread */
                          CYU3P_AUTO_START                         /* Start the Thread immediately */
                          );

    /* Check the return code */
    if (retThrdCreate != 0)
    {
        /* Thread Creation failed with the error code retThrdCreate */

        /* Add custom recovery or debug actions here */

        /* Application cannot continue */
        /* Loop indefinitely */
        while(1);
    }
}

/*
 * Main function
 */
int
main (void)
{
    CyU3PIoMatrixConfig_t io_cfg;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Initialize the device */
    status = CyU3PDeviceInit (NULL);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Initialize the caches. Enable both Instruction and Data Caches. */
    status = CyU3PDeviceCacheControl (CyTrue, CyTrue, CyTrue);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Configure the IO matrix for the device. On the FX3 DVK board, the COM port 
     * is connected to the IO(53:56). This means that either DQ32 mode should be
     * selected or lppMode should be set to UART_ONLY. Here we are choosing
     * UART_ONLY configuration. */
    io_cfg.isDQ32Bit = CyFalse;
    io_cfg.s0Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.s1Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.useUart   = CyTrue;
    io_cfg.useI2C    = CyFalse;
    io_cfg.useI2S    = CyFalse;
    io_cfg.useSpi    = CyFalse;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_UART_ONLY;
    /* No GPIOs are enabled. */
    io_cfg.gpioSimpleEn[0]  = 0;
    io_cfg.gpioSimpleEn[1]  = 0;
    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0;
    status = CyU3PDeviceConfigureIOMatrix (&io_cfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* This is a non returnable call for initializing the RTOS kernel */
    CyU3PKernelEntry ();

    /* Dummy return to make the compiler happy */
    return 0;

handle_fatal_error:

    /* Cannot recover from this error. */
    while (1);
}

/* [ ] */

