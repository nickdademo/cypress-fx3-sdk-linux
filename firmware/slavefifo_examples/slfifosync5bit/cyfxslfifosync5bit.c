/*
## Cypress USB 3.0 Platform source file (cyfxslfifosync.c)
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

/* This file illustrates the Slave FIFO Synchronous mode example */

/*
This example illustrates the configuration and usage of the GPIF II
interface on the FX3 device to implement the asynchronous slave FIFO
5 bit protocol.

A master device that implements the Cypress defined Asynchronous Slave FIFO
5 bit protocol is required to perform data transfers with this application.

This application example does the following:

1. Configures the GPIF II interface to implement the Asynchronous Slave FIFO 
   5 Bit protocol.

2. Enumerates as a vendor specific USB device with maximum of 30 bulk endpoints
   (15-OUT and 15-IN) configurable with CY_FX_NUMBER_OF_ADDR_LINES.
	
3. The CY_FX_NUMBER_OF_ADDR_LINES defines the number of socket and the 
   number of bulk endpoints. The CY_FX_NUMBER_OF_ADDR_LINES can have a 
   value from 2 to 5. The number of socket and bulk endpoints are defined using the 
   equation 
   Number of socket = Number of bulk endpoints = 2^(CY_FX_NUMBER_OF_ADDR_LINES-1). 

4. Half of the sockets are used for receiving the data from master device 
   and other half is used for sending the data. Similarly the half of the 
   bulk endpoints is for sending the data through USB and other half is used for 
   receiving the data from USB.
	
   Following is the table of parameters that varies with respect to the 
   number of address lines
   -------------------------------------------------------------------
   |   CY_FX_NUMBER_OF_ADDR_LINES     |  2  |  3    |  4    |   5    |
   |   Receive Data Socket            |  1  | 1-3   | 1-7   | 1-15   |
   |   Send Data socket               |  3  | 5-7   | 9-15  | 17-31  |
   |   OUT Bulk Endpoints             |  1  | 1-3   | 1-7   | 1-15   | 
   |   IN Bulk Endpoints              |  81 | 81-83 | 81-87 | 81-8F  |
   -------------------------------------------------------------------		

5. Create AUTO DMA channels to enable the following data paths:
 a. All data received from the USB host through the X-OUT endpoint is
    forwarded to the master device on the slave port through socket
    2^(CY_FX_NUMBER_OF_ADDR_LINES-2) +X. 
    Example data received on 1-OUT endpoint will be sent from socket 17 at 
    the master device assuming CY_FX_NUMBER_OF_ADDR_LINES =5. Similarly data 
    received on 15-OUT point will be sent from socket 31 to the master device.
 b. All data received from the master device on the slave port through
    socket X is forwarded to the USB host through the X-IN endpoint. 
    Example data sent on 1-IN endpoint will be received at socket 1 at 
    the master device. Similarly data sent on 15-IN point will be received 
    at socket 15 to the master device.

6. The DMA buffer size for each channel is defined based on the USB speed. 64 for full
   speed, 512 for high speed and 1024 for super speed. CY_FX_SLFIFO_DMA_BUF_COUNT in the
   header file defines the number of DMA buffers per channel.

7. The constant CY_FX_SLFIFO_GPIF_16_32BIT_CONF_SELECT in the header file is used to
   select 16bit or 32bit GPIF data bus configuration.
*/

#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3uart.h"
#include "cyfxslfifosync.h"
#include "cyu3gpif.h"
#include "cyu3pib.h"
#include "pib_regs.h"

/* This file should be included only once as it contains
* structure definitions. Including it in multiple places
* can result in linker error. */
#include "cyfxgpif_syncsf.h"

CyU3PThread slFifoAppThread;		    /* Slave FIFO application thread structure */
CyU3PDmaChannel glChHandleSlFifoUtoP[15];   /* DMA Channel handle for U2P transfer. */
CyU3PDmaChannel glChHandleSlFifoPtoU[15];   /* DMA Channel handle for P2U transfer. */


/* Number of channels that need to be created */
#if (CY_FX_NUMBER_OF_ADDR_LINES==2)
uint8_t glNumberOfChannels = 2;
#endif
#if (CY_FX_NUMBER_OF_ADDR_LINES==3)
uint8_t glNumberOfChannels = 6;
#endif
#if (CY_FX_NUMBER_OF_ADDR_LINES==4)
uint8_t glNumberOfChannels = 14;
#endif
#if (CY_FX_NUMBER_OF_ADDR_LINES==5)
uint8_t glNumberOfChannels = 30;
#endif

CyBool_t glIsApplnActive = CyFalse;      /* Whether the loopback application is active or not. */

/* Array of Procuder Endpoints */
const uint8_t CyFxProducerEndPoint[]=
{
    CY_FX_EP_PRODUCER_1,
    CY_FX_EP_PRODUCER_2,
    CY_FX_EP_PRODUCER_3,
    CY_FX_EP_PRODUCER_4,
    CY_FX_EP_PRODUCER_5,
    CY_FX_EP_PRODUCER_6,
    CY_FX_EP_PRODUCER_7,
    CY_FX_EP_PRODUCER_8,
    CY_FX_EP_PRODUCER_9,
    CY_FX_EP_PRODUCER_10,
    CY_FX_EP_PRODUCER_11,
    CY_FX_EP_PRODUCER_12,
    CY_FX_EP_PRODUCER_13,
    CY_FX_EP_PRODUCER_14,
    CY_FX_EP_PRODUCER_15
};

/* Array of Consumer Endpoints */
const uint8_t CyFxConsumerEndPoint[]=
{
    CY_FX_EP_CONSUMER_1,
    CY_FX_EP_CONSUMER_2,
    CY_FX_EP_CONSUMER_3,
    CY_FX_EP_CONSUMER_4,
    CY_FX_EP_CONSUMER_5,
    CY_FX_EP_CONSUMER_6,
    CY_FX_EP_CONSUMER_7,
    CY_FX_EP_CONSUMER_8,
    CY_FX_EP_CONSUMER_9,
    CY_FX_EP_CONSUMER_10,
    CY_FX_EP_CONSUMER_11,
    CY_FX_EP_CONSUMER_12,
    CY_FX_EP_CONSUMER_13,
    CY_FX_EP_CONSUMER_14,
    CY_FX_EP_CONSUMER_15

};

/* Array of Procuder Socket */
const uint16_t CyFxProducerSocket[] = 
{
    /* First 15 Value corresponds to the USB socket. Each value in the producer for the channel 
     * created for the glChHandleSlFifoUtoP */
    CY_FX_PRODUCER_1_USB_SOCKET,
    CY_FX_PRODUCER_2_USB_SOCKET, 
    CY_FX_PRODUCER_3_USB_SOCKET, 
    CY_FX_PRODUCER_4_USB_SOCKET, 
    CY_FX_PRODUCER_5_USB_SOCKET, 
    CY_FX_PRODUCER_6_USB_SOCKET, 
    CY_FX_PRODUCER_7_USB_SOCKET,
    CY_FX_PRODUCER_8_USB_SOCKET,
    CY_FX_PRODUCER_9_USB_SOCKET,
    CY_FX_PRODUCER_10_USB_SOCKET,
    CY_FX_PRODUCER_11_USB_SOCKET,
    CY_FX_PRODUCER_12_USB_SOCKET,
    CY_FX_PRODUCER_13_USB_SOCKET,
    CY_FX_PRODUCER_14_USB_SOCKET,
    CY_FX_PRODUCER_15_USB_SOCKET,
    
    /* Next 15 Value corresponds to the PPORT socket Each value in the producer for the channel 
     * created for the glChHandleSlFifoPtoU */
    CY_FX_PRODUCER_1_PPORT_SOCKET,
    CY_FX_PRODUCER_2_PPORT_SOCKET,
    CY_FX_PRODUCER_3_PPORT_SOCKET,
    CY_FX_PRODUCER_4_PPORT_SOCKET,
    CY_FX_PRODUCER_5_PPORT_SOCKET,
    CY_FX_PRODUCER_6_PPORT_SOCKET,
    CY_FX_PRODUCER_7_PPORT_SOCKET,
    CY_FX_PRODUCER_8_PPORT_SOCKET,
    CY_FX_PRODUCER_9_PPORT_SOCKET,
    CY_FX_PRODUCER_10_PPORT_SOCKET,
    CY_FX_PRODUCER_11_PPORT_SOCKET,
    CY_FX_PRODUCER_12_PPORT_SOCKET,
    CY_FX_PRODUCER_13_PPORT_SOCKET ,
    CY_FX_PRODUCER_14_PPORT_SOCKET,
    CY_FX_PRODUCER_15_PPORT_SOCKET
};

const uint16_t CyFxConsumerSocket[] = 
{
    /* First 15 Value corresponds to the PPORT socket. Each value in the Consumer for the channel 
     * created for the glChHandleSlFifoUtoP */
    CY_FX_CONSUMER_1_PPORT_SOCKET, 
    CY_FX_CONSUMER_2_PPORT_SOCKET,
    CY_FX_CONSUMER_3_PPORT_SOCKET,
    CY_FX_CONSUMER_4_PPORT_SOCKET,
    CY_FX_CONSUMER_5_PPORT_SOCKET,
    CY_FX_CONSUMER_6_PPORT_SOCKET,
    CY_FX_CONSUMER_7_PPORT_SOCKET,
    CY_FX_CONSUMER_8_PPORT_SOCKET,
    CY_FX_CONSUMER_9_PPORT_SOCKET,
    CY_FX_CONSUMER_10_PPORT_SOCKET,
    CY_FX_CONSUMER_11_PPORT_SOCKET,
    CY_FX_CONSUMER_12_PPORT_SOCKET,
    CY_FX_CONSUMER_13_PPORT_SOCKET ,
    CY_FX_CONSUMER_14_PPORT_SOCKET,
    CY_FX_CONSUMER_15_PPORT_SOCKET,

    /* Next 15 Value corresponds to the USB socket. Each value in the Consumer for the channel 
     * created for the glChHandleSlFifoPtoU */
    CY_FX_CONSUMER_1_USB_SOCKET,
    CY_FX_CONSUMER_2_USB_SOCKET, 
    CY_FX_CONSUMER_3_USB_SOCKET, 
    CY_FX_CONSUMER_4_USB_SOCKET, 
    CY_FX_CONSUMER_5_USB_SOCKET, 
    CY_FX_CONSUMER_6_USB_SOCKET, 
    CY_FX_CONSUMER_7_USB_SOCKET,
    CY_FX_CONSUMER_8_USB_SOCKET,
    CY_FX_CONSUMER_9_USB_SOCKET,
    CY_FX_CONSUMER_10_USB_SOCKET,
    CY_FX_CONSUMER_11_USB_SOCKET,
    CY_FX_CONSUMER_12_USB_SOCKET,
    CY_FX_CONSUMER_13_USB_SOCKET,
    CY_FX_CONSUMER_14_USB_SOCKET,
    CY_FX_CONSUMER_15_USB_SOCKET
};

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
CyFxSlFifoApplnDebugInit (void)
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

/* This function starts the slave FIFO loop application. This is called
* when a SET_CONF event is received from the USB host. The endpoints
* are configured and the DMA pipe is setup in this function. */
void
CyFxSlFifoApplnStart (
                      void)
{
    uint16_t size = 0;
    uint8_t i = 0;
    CyU3PEpConfig_t epCfg;
    CyU3PDmaChannelConfig_t dmaCfg;
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
    for(i=0;i<(glNumberOfChannels/2);i++)
    {
        apiRetStatus = CyU3PSetEpConfig(CyFxProducerEndPoint[i], &epCfg);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler (apiRetStatus);
        }

        /* Consumer endpoint configuration */
        apiRetStatus = CyU3PSetEpConfig(CyFxConsumerEndPoint[i], &epCfg);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler (apiRetStatus);
        }		
    }

    dmaCfg.size  = size;
    dmaCfg.count = CY_FX_SLFIFO_DMA_BUF_COUNT;
    dmaCfg.dmaMode = CY_U3P_DMA_MODE_BYTE;
    /* Enabling the callback for produce event. */
    dmaCfg.notification = 0;
    dmaCfg.cb = NULL;
    dmaCfg.prodHeader = 0;
    dmaCfg.prodFooter = 0;
    dmaCfg.consHeader = 0;
    dmaCfg.prodAvailCount = 0;
    /* Create a DMA AUTO channel for U2P transfer.
    * DMA size is set based on the USB speed. */
    for(i=0;i<(glNumberOfChannels/2);i++)
    {		
        dmaCfg.prodSckId = (CyU3PDmaSocketId_t)CyFxProducerSocket[i];
        dmaCfg.consSckId = (CyU3PDmaSocketId_t)CyFxConsumerSocket[i];
        apiRetStatus = CyU3PDmaChannelCreate (&glChHandleSlFifoUtoP[i],
            CY_U3P_DMA_TYPE_AUTO, &dmaCfg);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelCreate failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }
        apiRetStatus = CyU3PDmaChannelSetXfer (&glChHandleSlFifoUtoP[i], CY_FX_SLFIFO_DMA_TX_SIZE);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelSetXfer Failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }
    }

    for(i=0;i<(glNumberOfChannels/2);i++)
    {
        /* Create a DMA AUTO channel for P2U transfer. */
        dmaCfg.prodSckId = (CyU3PDmaSocketId_t)CyFxProducerSocket[i+15];
        dmaCfg.consSckId = (CyU3PDmaSocketId_t)CyFxConsumerSocket[i+15];		
        apiRetStatus = CyU3PDmaChannelCreate (&glChHandleSlFifoPtoU[i],
            CY_U3P_DMA_TYPE_AUTO, &dmaCfg);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelCreate failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }
        apiRetStatus = CyU3PDmaChannelSetXfer (&glChHandleSlFifoPtoU[i], CY_FX_SLFIFO_DMA_RX_SIZE);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelSetXfer Failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }
        CyU3PUsbFlushEp(CyFxProducerEndPoint[i]);
        CyU3PUsbFlushEp(CyFxConsumerEndPoint[i]);
    }

    /* Set DMA channel transfer size. */
    /* Update the status flag. */
    glIsApplnActive = CyTrue;
}

/* This function stops the slave FIFO loop application. This shall be called
* whenever a RESET or DISCONNECT event is received from the USB host. The
* endpoints are disabled and the DMA pipe is destroyed by this function. */
void
CyFxSlFifoApplnStop (
                     void)
{
    CyU3PEpConfig_t epCfg;
    uint8_t i=0;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Update the flag. */
    glIsApplnActive = CyFalse;

    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyFalse;
    /* Flush the endpoint memory */
    for(i=0;i<(glNumberOfChannels/2);i++)
    {
        CyU3PUsbFlushEp(CyFxProducerEndPoint[i]);
        CyU3PUsbFlushEp(CyFxConsumerEndPoint[i]);
        CyU3PDmaChannelDestroy (&glChHandleSlFifoUtoP[i]);
        CyU3PDmaChannelDestroy (&glChHandleSlFifoPtoU[i]);

        /* Producer endpoint configuration. */
        apiRetStatus = CyU3PSetEpConfig(CyFxProducerEndPoint[i], &epCfg);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler (apiRetStatus);
        }

        /* Consumer endpoint configuration. */
        apiRetStatus = CyU3PSetEpConfig(CyFxConsumerEndPoint[i], &epCfg);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler (apiRetStatus);
        }

    }    
}

/* Callback to handle the USB setup requests. */
CyBool_t
CyFxSlFifoApplnUSBSetupCB (
                           uint32_t setupdat0,
                           uint32_t setupdat1
                           )
{
    /* Fast enumeration is used. Only requests addressed to the interface, class,
    * vendor and unknown control requests are received by this function.
    * This application does not support any class or vendor requests. */

    uint8_t  bRequest, bReqType;
    uint8_t  bType, bTarget,i;
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
            if (glIsApplnActive)
            {
                for(i=0;i<(glNumberOfChannels/2);i++)
                {
                    if (wIndex == CyFxProducerEndPoint[i])
                    {
                        CyU3PDmaChannelReset (&glChHandleSlFifoUtoP[i]);
                        CyU3PUsbFlushEp(CyFxProducerEndPoint[i]);
                        CyU3PUsbResetEp (CyFxProducerEndPoint[i]);
                        CyU3PDmaChannelSetXfer (&glChHandleSlFifoUtoP[i], CY_FX_SLFIFO_DMA_TX_SIZE);
                        break;
                    }
                    if (wIndex == CyFxConsumerEndPoint[i])
                    {
                        CyU3PDmaChannelReset (&glChHandleSlFifoPtoU[i]);
                        CyU3PUsbFlushEp(CyFxConsumerEndPoint[i]);
                        CyU3PUsbResetEp (CyFxConsumerEndPoint[i]);
                        CyU3PDmaChannelSetXfer (&glChHandleSlFifoPtoU[i], CY_FX_SLFIFO_DMA_RX_SIZE);
                        break;
                    }
                }

                CyU3PUsbStall (wIndex, CyFalse, CyTrue);

                CyU3PUsbAckSetup ();
                isHandled = CyTrue;
            }
        }
    }
    return isHandled;
}

/* This is the callback function to handle the USB events. */
void
CyFxSlFifoApplnUSBEventCB (
                           CyU3PUsbEventType_t evtype,
                           uint16_t            evdata
                           )
{
    switch (evtype)
    {
    case CY_U3P_USB_EVENT_SETCONF:
        /* Stop the application before re-starting. */
        if (glIsApplnActive)
        {
            CyFxSlFifoApplnStop ();
        }
        /* Start the loop back function. */
        CyFxSlFifoApplnStart ();
        break;

    case CY_U3P_USB_EVENT_RESET:
    case CY_U3P_USB_EVENT_DISCONNECT:
        /* Stop the loop back function. */
        if (glIsApplnActive)
        {
            CyFxSlFifoApplnStop ();
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
CyFxApplnLPMRqtCB (
                   CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}

/* This function initializes the GPIF interface and initializes
* the USB interface. */
void
CyFxSlFifoApplnInit (void)
{
    CyU3PPibClock_t pibClock;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize the p-port block. */
    pibClock.clkDiv = 2;
    pibClock.clkSrc = CY_U3P_SYS_CLK;
    pibClock.isHalfDiv = CyFalse;
    /* Disable DLL for sync GPIF */
    pibClock.isDllEnable = CyFalse;
    apiRetStatus = CyU3PPibInit(CyTrue, &pibClock);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "P-port Initialization failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Load the GPIF configuration for Slave FIFO sync mode. */
    apiRetStatus = CyU3PGpifLoad (&Sync_Slave_Fifo_5Bit_CyFxGpifConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PGpifLoad failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Start the state machine. */
    apiRetStatus = CyU3PGpifSMStart (SYNC_SLAVE_FIFO_5BIT_RESET, SYNC_SLAVE_FIFO_5BIT_ALPHA_RESET);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PGpifSMStart failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

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
    CyU3PUsbRegisterSetupCallback(CyFxSlFifoApplnUSBSetupCB, CyTrue);

    /* Setup the callback to handle the USB events. */
    CyU3PUsbRegisterEventCallback(CyFxSlFifoApplnUSBEventCB);

    /* Register a callback to handle LPM requests from the USB 3.0 host. */
    CyU3PUsbRegisterLPMRequestCallback(CyFxApplnLPMRqtCB);    

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

/* Entry function for the slFifoAppThread. */
void
SlFifoAppThread_Entry (
                       uint32_t input)
{
    /* Initialize the debug module */
    CyFxSlFifoApplnDebugInit();

    /* Initialize the slave FIFO application */
    CyFxSlFifoApplnInit();

    for (;;)
    {
        CyU3PThreadSleep (1000);        
    }
}

/* Application define function which creates the threads. */
void
CyFxApplicationDefine (
                       void)
{
    void *ptr = NULL;
    uint32_t retThrdCreate = CY_U3P_SUCCESS;

    /* Allocate the memory for the thread */
    ptr = CyU3PMemAlloc (CY_FX_SLFIFO_THREAD_STACK);

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (&slFifoAppThread,           /* Slave FIFO app thread structure */
        "21:Slave_FIFO_Sync_5Bit",                    /* Thread ID and thread name */
        SlFifoAppThread_Entry,                   /* Slave FIFO app thread entry function */
        0,                                       /* No input parameter to thread */
        ptr,                                     /* Pointer to the allocated thread stack */
        CY_FX_SLFIFO_THREAD_STACK,               /* App Thread stack size */
        CY_FX_SLFIFO_THREAD_PRIORITY,            /* App Thread priority */
        CY_FX_SLFIFO_THREAD_PRIORITY,            /* App Thread pre-emption threshold */
        CYU3P_NO_TIME_SLICE,                     /* No time slice for the application thread */
        CYU3P_AUTO_START                         /* Start the thread immediately */
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
    CyU3PSysClockConfig_t clockConfig;

    /* When the GPIF data bus is configured as 32-bits wide and running at 100 MHz (synchronous),
       the FX3 system clock has to be set to a frequency greater than 400 MHz. */
#if (CY_FX_SLFIFO_GPIF_16_32BIT_CONF_SELECT == 0)
    clockConfig.setSysClk400  = CyFalse;
#else
    clockConfig.setSysClk400  = CyTrue;
#endif
    clockConfig.cpuClkDiv     = 2;
    clockConfig.dmaClkDiv     = 2;
    clockConfig.mmioClkDiv    = 2;
    clockConfig.useStandbyClk = CyFalse;
    clockConfig.clkSrc        = CY_U3P_SYS_CLK;

    status = CyU3PDeviceInit (&clockConfig);
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
    * UART_ONLY configuration for 16 bit slave FIFO configuration and setting
    * isDQ32Bit for 32-bit slave FIFO configuration. */
    io_cfg.s0Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.s1Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.useUart   = CyTrue;
    io_cfg.useI2C    = CyFalse;
    io_cfg.useI2S    = CyFalse;
    io_cfg.useSpi    = CyFalse;
#if (CY_FX_SLFIFO_GPIF_16_32BIT_CONF_SELECT == 0)
    io_cfg.isDQ32Bit = CyFalse;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_UART_ONLY;
#else
    io_cfg.isDQ32Bit = CyTrue;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_DEFAULT;
#endif
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

