/*
 ## Cypress FX3S Example Application Source File (cyfxgpiftostorage.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2012,
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

/* This file illustrates the use of the FX3S APIs to transfer data from the GPIF port to the
   storage devices connected to FX3S. */

#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3error.h>
#include <cyu3pib.h>
#include <cyu3gpif.h>
#include <cyu3mbox.h>
#include <cyu3uart.h>
#include <cyu3sib.h>
#include <cyu3gpio.h>
#include <cyu3utils.h>

#include "cyfxgpif_asyncadmux.h"
#include "cyfxgpiftostorage.h"

#define CYFXAPP_STACK_SIZE      (0x800)                 /* Thread stack size is 2 KB. */
#define CYFXAPP_THREAD_PRIORITY (8)                     /* Application thread priority. */

#define CYFXAPP_RQT_EVENT       (1 << 0)                /* Event indicating a new control request. */
#define CYFXAPP_SIB_DONE_EVENT  (1 << 1)                /* Event indicating storage transfer completion. */

static CyU3PThread fxAppThread;	                        /* Application thread structure */
static CyU3PEvent  fxAppEvent;                          /* Application Event group */
static CyU3PMbox   fxAppMbox;                           /* Structure used to receive mailbox requests. */

static CyU3PDmaChannel fxAppWriteChannel;               /* DMA channel for writing to storage device. */
static CyU3PDmaChannel fxAppReadChannel;                /* DMA channel for reading from storage device. */
static uint16_t        fxAppDevBlkSize = 512;           /* Block (sector) size for the storage devices. */
static CyU3PReturnStatus_t fxAppXferStatus;             /* SIB transfer status obtained through the callback. */

/*
 * Main function
 */
int
main (void)
{
    CyU3PReturnStatus_t status;
    CyU3PIoMatrixConfig_t io_cfg;

    /* Initialize the device */
    status = CyU3PDeviceInit (0);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Initialize the caches. Enable both instruction and data caches. */
    status = CyU3PDeviceCacheControl (CyTrue, CyFalse, CyFalse);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Configure the IO matrix for the device.
     * S0 port is enabled in 8 bit mode.
     * S1 port is enabled in 4 bit mode.
     * UART is enabled on remaining pins of the S1 port.
     */
    io_cfg.isDQ32Bit        = CyFalse;
    io_cfg.s0Mode           = CY_U3P_SPORT_8BIT;
    io_cfg.s1Mode           = CY_U3P_SPORT_4BIT;
    io_cfg.gpioSimpleEn[0]  = 0;
    io_cfg.gpioSimpleEn[1]  = 0x02103800;               /* IOs 43, 44, 45, 52 and 57 are chosen as GPIO. */
    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0;
    io_cfg.useUart          = CyTrue;
    io_cfg.useI2C           = CyFalse;
    io_cfg.useI2S           = CyFalse;
    io_cfg.useSpi           = CyFalse;
    io_cfg.lppMode          = CY_U3P_IO_MATRIX_LPP_UART_ONLY;
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

/* Application error handler */
void
CyFxGpifAppErrorHandler (
        CyU3PReturnStatus_t status)
{

    /* Application failed with the error code status */

    /* Add custom debug or recovery actions here */

    /* Loop Indefinitely */
    for (;;)
    {
        /* Thread Sleep : 100 ms */
        CyU3PThreadSleep (100);
    }
}

/* This function initializes the Debug Module. */
void
CyFxGpifAppDebugInit ()
{

    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t status;

    /* Initialize the UART for printing debug messages */
    status = CyU3PUartInit ();
    if (status != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyFxGpifAppErrorHandler (status);
    }

    /* Set UART Configuration */
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit  = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity   = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyFalse;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma    = CyTrue;

    /* Set the UART configuration */
    status = CyU3PUartSetConfig (&uartConfig, NULL);
    if (status != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyFxGpifAppErrorHandler(status);
    }

    /* Set the UART transfer */
    status = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (status != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyFxGpifAppErrorHandler(status);
    }

    /* Initialize the Debug application */
    status = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (status != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyFxGpifAppErrorHandler(status);
    }

    CyU3PDebugPreamble (CyFalse);
}

/* Callback for receiving notification of mailbox messages from the external processor. */
static void
CyFxAppMboxCallback (
        CyBool_t isNewMsg)
{
    CyU3PReturnStatus_t status;

    /* Only incoming messages are handled here. Outgoing messages are queued directly from the thread. */
    if (isNewMsg)
    {
        /* Read the request into the global buffer and send an event to the thread. */
        status = CyU3PMboxRead (&fxAppMbox);
        if (status == CY_U3P_SUCCESS)
            CyU3PEventSet (&fxAppEvent, CYFXAPP_RQT_EVENT, CYU3P_EVENT_OR);
    }
}

void
CyFxGpifAppSibCallback (
        uint8_t             portId,
        CyU3PSibEventType   evt,
        CyU3PReturnStatus_t status)
{
    CyU3PMbox evtMbox;

    if (evt == CY_U3P_SIB_EVENT_XFER_CPLT)
    {
        fxAppXferStatus = status;
        CyU3PEventSet (&fxAppEvent, CYFXAPP_SIB_DONE_EVENT, CYU3P_EVENT_OR);
    }

    if (evt == CY_U3P_SIB_EVENT_INSERT)
    {
        evtMbox.w0 = CY_U3P_MAKEDWORD (CYFXSTORRESP_INSERT_EVT, portId, 0, 0);
        evtMbox.w1 = 0;
        CyU3PMboxWrite (&evtMbox);
    }

    if (evt == CY_U3P_SIB_EVENT_REMOVE)
    {
        evtMbox.w0 = CY_U3P_MAKEDWORD (CYFXSTORRESP_REMOVE_EVT, portId, 0, 0);
        evtMbox.w1 = 0;
        CyU3PMboxWrite (&evtMbox);
    }
}

static CyU3PReturnStatus_t
CyFxGpifAppCreateDmaChannels (
        void)
{
    CyU3PDmaChannelConfig_t dmaConfig;
    CyU3PReturnStatus_t status;

    /* Create DMA channels for the read and write data paths. */
    dmaConfig.size           = CYFXSTORAPP_DMA_BUFSIZE;
    dmaConfig.count          = CYFXSTORAPP_DMA_BUFCOUNT;
    dmaConfig.prodSckId      = (CyU3PDmaSocketId_t)(CY_U3P_PIB_SOCKET_0 | CYFXSTORAPP_GPIF_WRSOCK);
    dmaConfig.consSckId      = (CyU3PDmaSocketId_t)(CY_U3P_SIB_SOCKET_0 | CYFXSTORAPP_SIB_WRSOCK);
    dmaConfig.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    dmaConfig.notification   = 0;
    dmaConfig.cb             = 0;
    dmaConfig.prodHeader     = 0;
    dmaConfig.prodFooter     = 0;
    dmaConfig.consHeader     = 0;
    dmaConfig.prodAvailCount = 0;
    status = CyU3PDmaChannelCreate (&fxAppWriteChannel, CY_U3P_DMA_TYPE_AUTO, &dmaConfig);
    if (status != CY_U3P_SUCCESS)
        return status;

    dmaConfig.prodSckId      = (CyU3PDmaSocketId_t)(CY_U3P_SIB_SOCKET_0 | CYFXSTORAPP_SIB_RDSOCK);
    dmaConfig.consSckId      = (CyU3PDmaSocketId_t)(CY_U3P_PIB_SOCKET_0 | CYFXSTORAPP_GPIF_RDSOCK);
    status = CyU3PDmaChannelCreate (&fxAppReadChannel, CY_U3P_DMA_TYPE_AUTO, &dmaConfig);
    if (status != CY_U3P_SUCCESS)
        CyU3PDmaChannelDestroy (&fxAppWriteChannel);

    return status;
}

static void
CyFxGpifAppDestroyDmaChannels (
        void)
{
    CyU3PDmaChannelDestroy (&fxAppWriteChannel);
    CyU3PDmaChannelDestroy (&fxAppReadChannel);
}

void
CyFxGpifAppRqtHandler (
        void)
{
    CyU3PReturnStatus_t status;
    CyU3PMbox rspMbox;
    uint8_t rqtCode;
    uint8_t params[7];

    /* Break the request data into bytes. */
    rqtCode   = CY_U3P_DWORD_GET_BYTE3 (fxAppMbox.w0);  /* The request code is the MSB of w0. */
    params[0] = CY_U3P_DWORD_GET_BYTE2 (fxAppMbox.w0);
    params[1] = CY_U3P_DWORD_GET_BYTE1 (fxAppMbox.w0);
    params[2] = CY_U3P_DWORD_GET_BYTE0 (fxAppMbox.w0);
    params[3] = CY_U3P_DWORD_GET_BYTE3 (fxAppMbox.w1);
    params[4] = CY_U3P_DWORD_GET_BYTE2 (fxAppMbox.w1);
    params[5] = CY_U3P_DWORD_GET_BYTE1 (fxAppMbox.w1);
    params[6] = CY_U3P_DWORD_GET_BYTE0 (fxAppMbox.w1);

    switch (rqtCode)
    {
    case CYFXSTORRQT_INIT:
        {
            /* SIB start request. No parameters are used for this request. */
            status = CyU3PSibStart ();

            /* SIB start has succeeded. We can create the DMA channels for data transfer at this stage. */
            if (status == CY_U3P_SUCCESS)
            {
                status = CyFxGpifAppCreateDmaChannels ();

                /* Register a callback for SIB events. */
                CyU3PSibRegisterCbk (CyFxGpifAppSibCallback);
            }

            rspMbox.w0 = (CYFXSTORRESP_STATUS << 24) | status;
            rspMbox.w1 = 0;
            CyU3PMboxWrite (&rspMbox);
        }
        break;

    case CYFXSTORRQT_DEINIT:
        {
            /* SIB start request. No parameters are used for this request. */
            CyU3PSibStop ();

            /* SIB stop is complete. We can tear down the DMA channels at this stage. */
            CyFxGpifAppDestroyDmaChannels ();

            rspMbox.w0 = (CYFXSTORRESP_STATUS << 24);
            rspMbox.w1 = 0;
            CyU3PMboxWrite (&rspMbox);
        }
        break;

    case CYFXSTORRQT_PORTCFG:
        {
            CyU3PSibIntfParams_t intfCfg;

            /* Populate the structure with data from the mailbox request. */
            intfCfg.rstActHigh      = ((params[1] & 0x01) == 0x01) ? 1 : 0;
            intfCfg.writeProtEnable = ((params[1] & 0x02) == 0x02) ? 1 : 0;
            intfCfg.lowVoltage      = ((params[1] & 0x04) == 0x04) ? 1 : 0;
            intfCfg.useDdr          = ((params[1] & 0x08) == 0x08) ? 1 : 0;
            intfCfg.resetGpio       = params[2];
            intfCfg.cardDetType     = params[3];
            intfCfg.voltageSwGpio   = params[4];
            intfCfg.lvGpioState     = CyFalse;
            intfCfg.maxFreq         = CY_U3P_SIB_FREQ_104MHZ;        /* No S port clock limitation. */
            intfCfg.cardInitDelay   = 0;                             /* No SD/MMC initialization delay. */

            status = CyU3PSibSetIntfParams (params[0], &intfCfg);

            rspMbox.w0 = (CYFXSTORRESP_STATUS << 24) | status;
            rspMbox.w1 = 0;
            CyU3PMboxWrite (&rspMbox);
        }
        break;

    case CYFXSTORRQT_QUERYDEV:
        {
            CyU3PSibDevInfo_t devInfo;

            status = CyU3PSibQueryDevice (params[0], &devInfo);
            if (status == CY_U3P_SUCCESS)
            {
                /* Only some of the device info fields are being returned here, so as to restrict the
                   response to 8 bytes. This can be expanded by breaking up the response into multiple
                   messages.
                   */
                rspMbox.w0 = CY_U3P_MAKEDWORD (CYFXSTORRESP_DEVDATA, (uint8_t)devInfo.cardType,
                        (uint8_t)devInfo.numUnits, (uint8_t)devInfo.writeable);
                rspMbox.w1 = (uint32_t)devInfo.blkLen;

                fxAppDevBlkSize = devInfo.blkLen;
            }
            else
            {
                rspMbox.w0 = (CYFXSTORRESP_STATUS << 24) | status;
                rspMbox.w1 = 0;
            }

            CyU3PMboxWrite (&rspMbox);
        }
        break;

    case CYFXSTORRQT_QUERYUNIT:
        {
            CyU3PSibLunInfo_t unitInfo;

            status = CyU3PSibQueryUnit (params[0], params[1], &unitInfo);
            if (status == CY_U3P_SUCCESS)
            {
                /* Only some of the device info fields are being returned here, so as to restrict the
                   response to 8 bytes. This can be expanded by breaking up the response into multiple
                   messages.
                   */
                rspMbox.w0 = CY_U3P_MAKEDWORD (CYFXSTORRESP_UNITDATA, (uint8_t)unitInfo.valid,
                        (uint8_t)unitInfo.location, (uint8_t)unitInfo.type);
                rspMbox.w1 = unitInfo.numBlocks;
            }
            else
            {
                rspMbox.w0 = (CYFXSTORRESP_STATUS << 24) | status;
                rspMbox.w1 = 0;
            }

            CyU3PMboxWrite (&rspMbox);
        }
        break;

    case CYFXSTORRQT_READ:
        {
            uint32_t flag;

            status = CyU3PDmaChannelSetXfer (&fxAppReadChannel, params[3] * fxAppDevBlkSize);
            if (status == CY_U3P_SUCCESS)
            {
                status = CyU3PSibReadWriteRequest (CyTrue, params[0], params[1], params[2],
                        fxAppMbox.w1, CYFXSTORAPP_SIB_RDSOCK);
                if (status == CY_U3P_SUCCESS)
                {
                    status = CyU3PEventGet (&fxAppEvent, CYFXAPP_SIB_DONE_EVENT, CYU3P_EVENT_OR_CLEAR,
                            &flag, CYFXSTORAPP_XFER_TIMEOUT);
                    if (status == CY_U3P_SUCCESS)
                    {
                        if (fxAppXferStatus == CY_U3P_SUCCESS)
                        {
                            status = CyU3PDmaChannelWaitForCompletion (&fxAppReadChannel, CYFXSTORAPP_XFER_TIMEOUT);
                        }
                        else
                            status = fxAppXferStatus;
                    }

                    if (status != CY_U3P_SUCCESS)
                    {
                        CyU3PSibAbortRequest (params[0]);
                        CyU3PDmaChannelReset (&fxAppReadChannel);
                    }
                }
            }

            rspMbox.w0 = (CYFXSTORRESP_STATUS << 24) | status;
            rspMbox.w1 = 0;
            CyU3PMboxWrite (&rspMbox);
        }
        break;

    case CYFXSTORRQT_WRITE:
        {
            uint32_t flag;

            status = CyU3PDmaChannelSetXfer (&fxAppWriteChannel, params[3] * fxAppDevBlkSize);
            if (status == CY_U3P_SUCCESS)
            {
                status = CyU3PSibReadWriteRequest (CyFalse, params[0], params[1], params[2],
                        fxAppMbox.w1, CYFXSTORAPP_SIB_WRSOCK);
                if (status == CY_U3P_SUCCESS)
                {
                    status = CyU3PEventGet (&fxAppEvent, CYFXAPP_SIB_DONE_EVENT, CYU3P_EVENT_OR_CLEAR,
                            &flag, CYFXSTORAPP_XFER_TIMEOUT);
                    if (status == CY_U3P_SUCCESS)
                    {
                        if (fxAppXferStatus == CY_U3P_SUCCESS)
                        {
                            status = CyU3PDmaChannelWaitForCompletion (&fxAppWriteChannel, CYFXSTORAPP_XFER_TIMEOUT);
                        }
                        else
                            status = fxAppXferStatus;
                    }

                    if (status != CY_U3P_SUCCESS)
                    {
                        CyU3PSibAbortRequest (params[0]);
                        CyU3PDmaChannelReset (&fxAppWriteChannel);
                    }
                }
            }

            rspMbox.w0 = (CYFXSTORRESP_STATUS << 24) | status;
            rspMbox.w1 = 0;
            CyU3PMboxWrite (&rspMbox);
        }
        break;

    case CYFXSTORRQT_ECHO:
        {
            rspMbox.w0 = (CYFXSTORRESP_ECHO << 24) | (fxAppMbox.w0 & 0xFFFFFF);
            rspMbox.w1 = fxAppMbox.w1;
            CyU3PMboxWrite (&rspMbox);
        }
        break;

    default:
        {
            /* Unsupported command. */
            rspMbox.w0 = (CYFXSTORRESP_STATUS << 24) | CY_U3P_ERROR_CMD_NOT_SUPPORTED;
            rspMbox.w1 = 0;
            CyU3PMboxWrite (&rspMbox);
        }
        break;
    }
}

void
CyFxGpifAppInit (
        void)
{
    CyU3PReturnStatus_t status;
    CyU3PGpioClock_t    gpioClock;
    CyU3PPibClock_t     pibClock;

    /* GPIO module needs to be initialized before SIB is initialized. This is required because
       GPIOs are used in the SIB code.
     */
    gpioClock.fastClkDiv = 2;
    gpioClock.slowClkDiv = 16;
    gpioClock.simpleDiv  = CY_U3P_GPIO_SIMPLE_DIV_BY_2;
    gpioClock.clkSrc     = CY_U3P_SYS_CLK;
    gpioClock.halfDiv    = 0;
    status = CyU3PGpioInit (&gpioClock, NULL);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "GPIO Init failed, code=%d\r\n", status);
        CyFxGpifAppErrorHandler (status);
    }

    /* Initialize PIB and load the GPIF waveform. */
    pibClock.clkDiv      = 2;
    pibClock.clkSrc      = CY_U3P_SYS_CLK;
    pibClock.isHalfDiv   = CyFalse;
    pibClock.isDllEnable = CyFalse;

    status = CyU3PPibInit (CyTrue, &pibClock);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (2, "Error: PIB Init failed with code %d\r\n", status);
        CyFxGpifAppErrorHandler (status);
    }

    status = CyU3PGpifLoad (&Async_Admux_CyFxGpifConfig);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (2, "Error: GPIF load failed with code %d\r\n", status);
        CyFxGpifAppErrorHandler (status);
    }

    /* Register a callback that will receive notifications of incoming requests. */
    CyU3PMboxInit (CyFxAppMboxCallback);
}

static void
GpifAppSendReadyEvent (
        void)
{
    CyU3PMbox rspMbox;

    rspMbox.w0 = (CYFXSTORRESP_READY << 24);
    rspMbox.w1 = 0;
    CyU3PMboxWrite (&rspMbox);
}

/*
 * Entry function for the application thread.
 */
void
GpifAppThreadEntry (
        uint32_t input)
{
    CyU3PReturnStatus_t status;
    uint32_t evMask = CYFXAPP_RQT_EVENT;
    uint32_t evStat;

    /* Initialize the Debug Module */
    CyFxGpifAppDebugInit ();

    /* Initialize the other blocks used by the application. */
    CyFxGpifAppInit ();

    /* Send an event to the external processor to indicate that the firmware is now ready. */
    GpifAppSendReadyEvent ();

    for (;;)
    {
        status = CyU3PEventGet (&fxAppEvent, evMask, CYU3P_EVENT_OR_CLEAR, &evStat, CYU3P_WAIT_FOREVER);
        if (status == CY_U3P_SUCCESS)
        {
            if (evStat & CYFXAPP_RQT_EVENT)
            {
                CyFxGpifAppRqtHandler ();
            }
        }
    }
}

/*
 * Create the application thread and other OS objects required for this example.
 */
void
CyFxApplicationDefine (
        void)
{
    void *ptr;
    uint32_t txstatus;

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CYFXAPP_STACK_SIZE);
    if (ptr == 0)
        goto error;

    /* Event structure used by this application. */
    txstatus = CyU3PEventCreate (&fxAppEvent);
    if (txstatus != 0)
        goto error;

    /* Create the thread for the application */
    txstatus = CyU3PThreadCreate (&fxAppThread, "25:GpifToStorage App",
            GpifAppThreadEntry, 0, ptr, CYFXAPP_STACK_SIZE, CYFXAPP_THREAD_PRIORITY, CYFXAPP_THREAD_PRIORITY,
            CYU3P_NO_TIME_SLICE, CYU3P_AUTO_START);
    if (txstatus != 0)
        goto error;

    return;

error:
    /* Application cannot continue. Loop indefinitely. */
    while (1);
}

/*[]*/

