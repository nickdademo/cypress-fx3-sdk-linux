/*
 ## Cypress USB 3.0 Platform source file (cyfxuac.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2012-2013,
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

/* This file illustrates USB Audio Class application example (streaming audio data from the SPI Flash) */

/*
   This sample application implements a USB audio class Driver with the help of the appropriate USB 
   enumeration descriptors. With these descriptors, the FX3 device enumerates as a USB Audio Class 
   device on the USB host. The application implementes the USB Device Class Definition for Audio 
   Devices Release 1.0

   The sample application illustrates USB Audio Class application example (streaming audio data from
   the SPI Flash). The application pulls out the audio data stored in the SPI Flash and streams it over
   the Isochronous endpoint to the USB Host. The application supports stereo channel, PCM data with a
   48 KHz sampling frequncy, and 16-bits per sample. The audio streaming is accomplished by having two
   separate DMA MANUAL channels. The SPI Flash to CPU DMA MANUAL_IN channel is used to read audio data
   from the SPI flash. The firmware then breaks up the data received such that it fits into the USB 
   Isochronous packets. The CPU to USB DMA MANUAL_OUT channel is used to push audio data onto the USB
   Host.

   In the case of USB High Speed and Super Speed the isochronous service interval is set to 4 frames 
   with 96 bytes of audio data transferred every 4 microframes.

   In the case of USB Full Speed 192 bytes of audio data are sent every frame.
 */

#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyfxuac.h"
#include "cyu3usb.h"
#include "cyu3spi.h"
#include "cyu3uart.h"
#include "cyu3utils.h"

/* Setup data field : Request */
#define CY_U3P_USB_REQUEST_MASK                         (0x0000FF00)
#define CY_U3P_USB_REQUEST_POS                          (8)

/* Iscochronous Transfer length */
#define CY_FX3_ISO_XFER_LEN                             (96)
/* Give a timeout value of 5s for any flash programming. */
#define CY_FX_USB_SPI_TIMEOUT                           (5000)
/* Size in pages of the Audio sample to be streamed from the SPI Flash. */
#define CY_FX3_AUDIO_SAMPLE_SIZE_IN_PAGES               (0x3CD)

CyU3PThread uacThread;                /* Thread structure */

CyBool_t glIsApplnActive = CyFalse;   /* Whether the loopback application is active or not. */
CyU3PDmaChannel glUacStreamHandle;    /* DMA Channel Handle  */
CyU3PDmaChannel glUacSpiRxHandle;     /* SPI Rx channel handle */

uint16_t glSpiPageSize = 0x100;  /* SPI Page size to be used for transfers. */

/* Application error handler */
void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus    /* API return status */
        )
{
    /* Application failed with the error code apiRetStatus */

    /* Add custom debug or recovery actions here */

    /* Loop indefinitely */
    for (;;)
    {
        /* Thread sleep : 100 ms */
        CyU3PThreadSleep (100);
    }
}

/* This function initializes the debug module for the UAC application */
void
CyFxUacApplnDebugInit (void)
{
    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize the UART for printing debug messages */
    apiRetStatus = CyU3PUartInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set UART Configuration */
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyFalse;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma = CyTrue;

    /* Set the UART configuration */
    apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set the UART transfer */
    apiRetStatus = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Initialize the debug application */
    apiRetStatus = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }
}

/* This function starts the audio streaming application. It is called
 * when there is a SET_INTERFACE event for alternate interface 1. */
CyU3PReturnStatus_t
CyFxUacApplnStart (void)
{
    CyU3PEpConfig_t epCfg;
    CyU3PDmaChannelConfig_t dmaCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Audio streaming endpoint configuration */
    epCfg.enable = CyTrue;
    epCfg.epType = CY_U3P_USB_EP_ISO;
    epCfg.pcktSize = CY_FX3_ISO_XFER_LEN;
    epCfg.isoPkts = 1;
    epCfg.burstLen = 1;
    epCfg.streams = 0;

    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_ISO_AUDIO, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error Code = %d\n", apiRetStatus);
        return apiRetStatus;
    }

    /* Create a DMA Manual OUT channel for streaming data */
    /* Audio streaming Channel is not active till a stream request is received */
    dmaCfg.size = CY_FX3_ISO_XFER_LEN;
    dmaCfg.count = CY_FX_UAC_STREAM_BUF_COUNT;
    dmaCfg.prodSckId = CY_U3P_CPU_SOCKET_PROD;
    dmaCfg.consSckId = CY_FX_EP_AUDIO_CONS_SOCKET;
    dmaCfg.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.cb = NULL;
    dmaCfg.prodHeader = 0;
    dmaCfg.prodFooter = 0;
    dmaCfg.consHeader = 0;
    dmaCfg.prodAvailCount = 0;
    apiRetStatus = CyU3PDmaChannelCreate (&glUacStreamHandle, CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelCreate failed, error code = %d\n",apiRetStatus);
        return apiRetStatus;
    }

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp (CY_FX_EP_ISO_AUDIO);

    apiRetStatus = CyU3PDmaChannelSetXfer (&glUacStreamHandle, 0);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelSetXfer failed, error code = %d\n", apiRetStatus);
        return apiRetStatus;
    }

    /* Update the flag so that the application thread is notified of this. */
    glIsApplnActive = CyTrue;
    return CY_U3P_SUCCESS;
}

/* This function stops the audio streaming. It is called from the USB event
 * handler, when there is a reset / disconnect or SET_INTERFACE for alternate
 * interface 0. */
void
CyFxUacApplnStop (void)
{
    CyU3PEpConfig_t epCfg;

    /* Update the flag so that the application thread is notified of this. */
    glIsApplnActive = CyFalse;

    /* Reset the SPI Read Data channel Handle */
    CyU3PDmaChannelReset (&glUacSpiRxHandle);

    /* Abort and destroy the Audio streaming channel */
    CyU3PDmaChannelDestroy (&glUacStreamHandle);

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp (CY_FX_EP_ISO_AUDIO);

    /* Disable the audio streaming endpoint. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyFalse;

    CyU3PSetEpConfig (CY_FX_EP_ISO_AUDIO, &epCfg);
}

/* This is the Callback function to handle the USB Events */
static void
CyFxUacApplnUsbEventCbk (
    CyU3PUsbEventType_t evtype, /* Event type */
    uint16_t            evdata  /* Event data */
    )
{
    uint8_t interface = 0, altSetting = 0;

    switch (evtype)
    {
        case CY_U3P_USB_EVENT_SETINTF:
             /* Start the audio streamer application if the
              * interface requested was 1. If not, stop the
              * streamer. */
            interface = CY_U3P_GET_MSB(evdata);
            altSetting = CY_U3P_GET_LSB(evdata);

            if ((altSetting == CY_FX_UAC_STREAM_INTERFACE) &&
                    (interface == 1))
            {
                /* Stop the application before re-starting. */
                if (glIsApplnActive)
                {
                    CyFxUacApplnStop ();
                }
                CyFxUacApplnStart ();
                break;
            }
            /* Fall-through. */

        case CY_U3P_USB_EVENT_SETCONF:
        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_DISCONNECT:
            /* Stop the audio streamer application. */
            if (glIsApplnActive)
            {
                CyFxUacApplnStop ();
            }
            break;

        default:
            break;
    }
}

/* Callback to handle the USB Setup Requests and UAC Class requests */
static CyBool_t
CyFxUacApplnUSBSetupCB (
        uint32_t setupdat0, /* SETUP Data 0 */
        uint32_t setupdat1  /* SETUP Data 1 */
    )
{
    uint8_t  bRequest, bReqType;
    uint8_t  bType, bTarget;
    uint16_t wValue;
    CyBool_t isHandled = CyFalse;

    /* Fast enumeration is used. Only requests addressed to the interface, class,
     * vendor and unknown control requests are received by this function. 
     * */

    /* Decode the fields from the setup request. */
    bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bType    = (bReqType & CY_U3P_USB_TYPE_MASK);
    bTarget  = (bReqType & CY_U3P_USB_TARGET_MASK);
    bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue   = ((setupdat0 & CY_U3P_USB_VALUE_MASK)   >> CY_U3P_USB_VALUE_POS);

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
    }

    /* Check for UAC Class Requests */
    if (bType == CY_U3P_USB_CLASS_RQT)
    {
        while (bType == bType)
        {
            CyU3PBusyWait (10);
        }
    }

    return isHandled;
}

/* Callback function to handle LPM requests from the USB 3.0 host. This function is invoked by the API
 * whenever a state change from U0 -> U1 or U0 -> U2 happens. If we return CyTrue from this function, 
 * the FX3 device is retained in the low power state. If we return CyFalse, the FX3 device immediately 
 * tries to trigger an exit back to U0.
 * This application does not have any state in which we should not allow U1/U2 transitions; and therefore
 * the function always return CyTrue.
 * */
CyBool_t
CyFxUacApplnLPMRqtCbk (
        CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}

/* Wait for the status response from the SPI flash. */
CyU3PReturnStatus_t
CyUacFxSpiWaitForStatus (
        void)
{
    uint8_t buf[2], rd_buf[2];
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Wait for status response from SPI flash device. */
    do
    {
        buf[0] = 0x06;  /* Write enable command. */

        CyU3PSpiSetSsnLine (CyFalse);
        status = CyU3PSpiTransmitWords (buf, 1);
        CyU3PSpiSetSsnLine (CyTrue);
        if (status != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (2, "SPI WR_ENABLE command failed\n\r");
            return status;
        }

        buf[0] = 0x05;  /* Read status command */

        CyU3PSpiSetSsnLine (CyFalse);
        status = CyU3PSpiTransmitWords (buf, 1);
        if (status != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (2, "SPI READ_STATUS command failed\n\r");
            CyU3PSpiSetSsnLine (CyTrue);
            return status;
        }

        status = CyU3PSpiReceiveWords (rd_buf, 2);
        CyU3PSpiSetSsnLine (CyTrue);
        if(status != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (2, "SPI status read failed\n\r");
            return status;
        }

    } while ((rd_buf[0] & 1)|| (!(rd_buf[0] & 0x2)));

    return CY_U3P_SUCCESS;
}

/* SPI read / write for programmer application. */
CyU3PReturnStatus_t
CyFxUacSpiTransfer (
        uint16_t  pageAddress,
        uint16_t  byteCount,
        uint8_t  *buffer,
        CyBool_t  isRead)
{
    CyU3PDmaBuffer_t buf_p;
    uint8_t location[4];
    uint32_t byteAddress = 0;
    uint16_t pageCount = (byteCount / glSpiPageSize);
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    if (byteCount == 0)
    {
        return CY_U3P_SUCCESS;
    }
    if ((byteCount % glSpiPageSize) != 0)
    {
        pageCount ++;
    }

    buf_p.buffer = buffer;
    buf_p.status = 0;

    byteAddress  = pageAddress * glSpiPageSize;
    CyU3PDebugPrint (2, "SPI access - addr: 0x%x, size: 0x%x, pages: 0x%x.\r\n",
            byteAddress, byteCount, pageCount);

    while (pageCount != 0)
    {
        location[1] = (byteAddress >> 16) & 0xFF;       /* MS byte */
        location[2] = (byteAddress >> 8) & 0xFF;
        location[3] = byteAddress & 0xFF;               /* LS byte */

        if (isRead)
        {
            location[0] = 0x03; /* Read command. */

            buf_p.size  = glSpiPageSize;
            buf_p.count = glSpiPageSize;

            status = CyUacFxSpiWaitForStatus ();
            if (status != CY_U3P_SUCCESS)
                return status;

            CyU3PSpiSetSsnLine (CyFalse);
            status = CyU3PSpiTransmitWords (location, 4);
            if (status != CY_U3P_SUCCESS)
            {
                CyU3PDebugPrint (2, "SPI READ command failed\r\n");
                CyU3PSpiSetSsnLine (CyTrue);
                return status;
            }

            CyU3PSpiSetBlockXfer (0, glSpiPageSize);

            status = CyU3PDmaChannelSetupRecvBuffer (&glUacSpiRxHandle,
                    &buf_p);
            if (status != CY_U3P_SUCCESS)
            {
                CyU3PSpiSetSsnLine (CyTrue);
                return status;
            }
            status = CyU3PDmaChannelWaitForCompletion (&glUacSpiRxHandle,
                    CY_FX_USB_SPI_TIMEOUT);
            if (status != CY_U3P_SUCCESS)
            {
                CyU3PSpiSetSsnLine (CyTrue);
                return status;
            }

            CyU3PSpiSetSsnLine (CyTrue);
            CyU3PSpiDisableBlockXfer (CyFalse, CyTrue);
        }
        else 
        {
            /* Note: 
             * Implement the write to SPI logic here if required.
             * */
        }

        /* Update the parameters */
        byteAddress  += glSpiPageSize;
        buf_p.buffer += glSpiPageSize;
        pageCount --;
    }
    return CY_U3P_SUCCESS;
}

/* SPI initialization for application. */
CyU3PReturnStatus_t
CyFxUacSpiInit (uint16_t pageLen)
{
    CyU3PSpiConfig_t spiConfig;
    CyU3PDmaChannelConfig_t dmaConfig;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Start the SPI module and configure the master. */
    status = CyU3PSpiInit ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Start the SPI master block. Run the SPI clock at 8MHz
     * and configure the word length to 8 bits. Also configure
     * the slave select using FW. */
    CyU3PMemSet ((uint8_t *)&spiConfig, 0, sizeof(spiConfig));
    spiConfig.isLsbFirst = CyFalse;
    spiConfig.cpol       = CyTrue;
    spiConfig.ssnPol     = CyFalse;
    spiConfig.cpha       = CyTrue;
    spiConfig.leadTime   = CY_U3P_SPI_SSN_LAG_LEAD_HALF_CLK;
    spiConfig.lagTime    = CY_U3P_SPI_SSN_LAG_LEAD_HALF_CLK;
    spiConfig.ssnCtrl    = CY_U3P_SPI_SSN_CTRL_FW;
    spiConfig.clock      = 10000000;
    spiConfig.wordLen    = 8;

    status = CyU3PSpiSetConfig (&spiConfig, NULL);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Create the DMA channels for SPI write and read. */
    CyU3PMemSet ((uint8_t *)&dmaConfig, 0, sizeof(dmaConfig));
    dmaConfig.size           = pageLen;
    /* No buffers need to be allocated as this channel
     * will be used only in override mode. */
    dmaConfig.count          = 0;
    dmaConfig.prodAvailCount = 0;
    dmaConfig.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    dmaConfig.prodHeader     = 0;
    dmaConfig.prodFooter     = 0;
    dmaConfig.consHeader     = 0;
    dmaConfig.notification   = 0;
    dmaConfig.cb             = NULL;

    /* Channel to read from SPI flash. */
    dmaConfig.prodSckId = CY_U3P_LPP_SOCKET_SPI_PROD;
    dmaConfig.consSckId = CY_U3P_CPU_SOCKET_CONS;
    status = CyU3PDmaChannelCreate (&glUacSpiRxHandle,
            CY_U3P_DMA_TYPE_MANUAL_IN, &dmaConfig);

    if (status == CY_U3P_SUCCESS)
    {
        glSpiPageSize = pageLen;
    }

    return status;
}

/* This function initializes the USB Module, creates event group,
   sets the enumeration descriptors, configures the Endpoints and
   configures the DMA module for the UAC Application */
void
CyFxUacApplnInit (void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize the SPI interface for flash of page size 256 bytes. */
    apiRetStatus = CyFxUacSpiInit (0x100);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "Failed to Initialize the SPI Block, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Start the USB functionality */
    apiRetStatus = CyU3PUsbStart ();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Function Failed to Start, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* The fast enumeration is the easiest way to setup a USB connection,
     * where all enumeration phase is handled by the library. Only the
     * class / vendor requests need to be handled by the application. */
    CyU3PUsbRegisterSetupCallback (CyFxUacApplnUSBSetupCB, CyTrue);

    /* Setup the callback to handle the USB events */
    CyU3PUsbRegisterEventCallback (CyFxUacApplnUsbEventCbk);

    /* Register a callback to handle LPM requests from the USB 3.0 host. */
    CyU3PUsbRegisterLPMRequestCallback (CyFxUacApplnLPMRqtCbk);
    
    /* Set the USB Enumeration descriptors */

    /* Super speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUsb30DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* High speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_HS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUsb20DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* BOS descriptor */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_BOS_DESCR, NULL, (uint8_t *)CyFxUsbBOSDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set configuration descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Device qualifier descriptor */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_DEVQUAL_DESCR, NULL, (uint8_t *)CyFxUsbDeviceQualDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device qualifier descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Super speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUsbSSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set configuration descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* High speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_HS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUsbHSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Other Speed Descriptor failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Full speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_FS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUsbFSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Configuration Descriptor failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* String descriptor 0 */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUsbStringLangIDDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* String descriptor 1 */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUsbManufactureDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* String descriptor 2 */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUsbProductDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Connect the USB pins and enable super speed operation */
    apiRetStatus = CyU3PConnectState (CyTrue, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB connect failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }
}

/* Entry function for the UAC application thread. */
void
CyFxUacAppThread_Entry (
        uint32_t input)
{
    uint8_t spiBuffer [2048];
    uint16_t pageAddress = 0;
    CyU3PDmaBuffer_t dmaBuffer;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    CyBool_t partialBuf = CyFalse;
    uint16_t dataCount = 0;
    uint16_t dataCopyOffset = 0;
    uint16_t offset = 0;

    /* Initialize the Debug Module */
    CyFxUacApplnDebugInit();

    /* Initialize the UAC Application */
    CyFxUacApplnInit();

    for (;;)
    {
        /* Audio streaming logic */
        while (glIsApplnActive)
        {
            if (dataCount == 0)
            {
                dataCount = 8 * glSpiPageSize;
                /* Read one page of data from SPI Flash */
                status = CyFxUacSpiTransfer (pageAddress, dataCount, spiBuffer, CyTrue);
                if (status != CY_U3P_SUCCESS)
                {
                    CyFxAppErrorHandler (status);
                }

                offset = 0;
                pageAddress += 8;
            }

            if (!partialBuf)
            {
                if (dataCount)
                {
                    /* If there is data to be copied then try and get a USB buffer. */
                    status = CyU3PDmaChannelGetBuffer (&glUacStreamHandle, &dmaBuffer, 1000);
                    if (status != CY_U3P_SUCCESS)
                    {
                        CyU3PThreadSleep (100);
                        continue;
                    }
                }

                if (dataCount / CY_FX3_ISO_XFER_LEN)
                {
                    CyU3PMemCopy (dmaBuffer.buffer, &spiBuffer[offset], CY_FX3_ISO_XFER_LEN);
                    dataCount -= CY_FX3_ISO_XFER_LEN;
                    offset += CY_FX3_ISO_XFER_LEN;
                }
                else
                {
                    if (dataCount)
                    {
                        partialBuf = CyTrue;
                        dataCopyOffset = dataCount;
                        dataCount = 0;

                        /* Store the data. */
                        CyU3PMemCopy (dmaBuffer.buffer, &spiBuffer[offset], dataCopyOffset);
                    }
                    continue;
                }
            }
            else
            {
                partialBuf = CyFalse;
                CyU3PMemCopy (&dmaBuffer.buffer[dataCopyOffset], &spiBuffer[offset], (CY_FX3_ISO_XFER_LEN - dataCopyOffset));

                /* Update the offset from which to start copying the data */
                offset = CY_FX3_ISO_XFER_LEN - dataCopyOffset;
                dataCount -= offset;
            }

            /* Commit the buffer for transfer */
            status = CyU3PDmaChannelCommitBuffer (&glUacStreamHandle, CY_FX3_ISO_XFER_LEN, 0);
            if (status != CY_U3P_SUCCESS)
            {
                CyU3PThreadSleep (100);
                continue;
            }

            if (pageAddress >= CY_FX3_AUDIO_SAMPLE_SIZE_IN_PAGES)
                pageAddress = 0;
        }

        /* There is a streamer error. Flag it. */
        if ((status != CY_U3P_SUCCESS) && (glIsApplnActive))
        {
            CyU3PDebugPrint (4, "UAC streamer error. Code %d.\n", status);
            CyFxAppErrorHandler (status);
        }
        
        /* Sleep for sometime as audio streamer is idle. */
        CyU3PThreadSleep (100);
    } /* End of for(;;) */
}

/* Application define function which creates the threads. */
void
CyFxApplicationDefine (
        void)
{
    void *ptr = NULL;
    uint32_t retThrdCreate = CY_U3P_SUCCESS;

    /* Allocate the memory for the thread and create the thread */
    ptr = CyU3PMemAlloc (CY_FX_UAC_APP_THREAD_STACK);
    retThrdCreate = CyU3PThreadCreate (&uacThread,        /* UAC Thread structure */
                           "31:UAC_App_Thread",           /* Thread Id and name */
                           CyFxUacAppThread_Entry,        /* UAC Application Thread Entry function */
                           0,                             /* No input parameter to thread */
                           ptr,                           /* Pointer to the allocated thread stack */
                           CY_FX_UAC_APP_THREAD_STACK,    /* UAC Application Thread stack size */
                           CY_FX_UAC_APP_THREAD_PRIORITY, /* UAC Application Thread priority */
                           CY_FX_UAC_APP_THREAD_PRIORITY, /* Pre-emption threshold */
                           CYU3P_NO_TIME_SLICE,           /* No time slice for the application thread */
                           CYU3P_AUTO_START               /* Start the Thread immediately */
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
    CyU3PMemSet ((uint8_t *)&io_cfg, 0, sizeof(io_cfg));
    io_cfg.isDQ32Bit = CyFalse;
    io_cfg.s0Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.s1Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.useUart   = CyTrue;
    io_cfg.useI2C    = CyFalse;
    io_cfg.useI2S    = CyFalse;
    io_cfg.useSpi    = CyTrue;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_DEFAULT;
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

