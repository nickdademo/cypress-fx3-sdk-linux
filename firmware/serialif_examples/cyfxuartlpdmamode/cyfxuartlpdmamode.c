/*
 ## Cypress USB 3.0 Platform source file (cyfxuartlpdmamode.c)
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

/* This file implements a simple UART (DMA mode) loopback application example */

/*
   This example illustrates a loopback mechanism between the UART RX and UART TX
   using DMA MANUAL channel. The loopback is achieved with the help of a DMA MANUAL
   channel. DMA MANUAL channel is created between the UART producer socket and the
   UART consumer socket.

   Upon every reception of data in the DMA buffer from the host, the CPU is signalled
   using DMA callbacks. The CPU then commits the DMA buffer received so that the data
   is transferred to the consumer socket.

   Note: Since the UART RX will commit the data to UART TX only after the DMA buffer
   is filled, the remote UART receiver will only get the loopback data after receiving
   CY_FX_UART_DMA_BUF_SIZE bytes.
 */

#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3error.h>
#include <cyu3uart.h>

#define CY_FX_UARTLP_THREAD_STACK       (0x0400)  /* UART application thread stack size */
#define CY_FX_UARTLP_THREAD_PRIORITY    (8)       /* UART application thread priority */
#define CY_FX_UART_DMA_TX_SIZE	(0)               /* DMA transfer size */
#define CY_FX_UART_DMA_BUF_SIZE (32)              /* Buffer size */

CyU3PThread UartLpAppThread;	                  /* UART Example application thread structure */
CyU3PDmaChannel glUartLpChHandle;                 /* DMA Channel handle */

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

/* Callback funtion for the DMA event notification */
void
CyFxUartLpDmaCallback (
        CyU3PDmaChannel   *chHandle, /* Handle to the DMA channel. */
        CyU3PDmaCbType_t  type,      /* Callback type.             */
        CyU3PDmaCBInput_t *input)    /* Callback status.           */
{
    CyU3PReturnStatus_t status;

    if (type == CY_U3P_DMA_CB_PROD_EVENT)
    {
        /* This is a produce event notification to the CPU. This notification is 
         * received upon reception of every buffer. The buffer will not be sent
         * out unless it is explicitly committed. The call shall fail if there
         * is any application error. */
        status = CyU3PDmaChannelCommitBuffer (chHandle, input->buffer_p.count, 0);
        if (status != CY_U3P_SUCCESS)
        {
            CyFxAppErrorHandler (status);
        }
    }
}

/* This function initializes the UART module */
void
CyFxUartLpApplnInit (void)
{
    CyU3PUartConfig_t uartConfig;
    CyU3PDmaChannelConfig_t dmaConfig;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize the UART module */
    apiRetStatus = CyU3PUartInit ();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Configure the UART
       Baudrate = 115200, One stop bit, No parity, Hardware flow control enabled.
     */
    CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof(uartConfig));
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity = CY_U3P_UART_NO_PARITY;
    uartConfig.flowCtrl = CyTrue;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyTrue;
    uartConfig.isDma = CyTrue; /* DMA mode */

    /* Set the UART configuration */
    apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS )
    {
        /* Error handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Create a DMA Manual channel between UART producer socket
       and UART consumer socket */
    CyU3PMemSet ((uint8_t *)&dmaConfig, 0, sizeof(dmaConfig));
    dmaConfig.size = CY_FX_UART_DMA_BUF_SIZE;
    dmaConfig.count = 4;
    dmaConfig.prodSckId = CY_U3P_LPP_SOCKET_UART_PROD;
    dmaConfig.consSckId = CY_U3P_LPP_SOCKET_UART_CONS;
    dmaConfig.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dmaConfig.notification = CY_U3P_DMA_CB_PROD_EVENT;
    dmaConfig.cb = CyFxUartLpDmaCallback;
    dmaConfig.prodHeader = 0;
    dmaConfig.prodFooter = 0;
    dmaConfig.consHeader = 0;
    dmaConfig.prodAvailCount = 0;

    /* Create the channel */
    apiRetStatus = CyU3PDmaChannelCreate (&glUartLpChHandle,
            CY_U3P_DMA_TYPE_MANUAL, &dmaConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set UART Tx and Rx transfer Size to infinite */
    apiRetStatus = CyU3PUartTxSetBlockXfer(0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUartRxSetBlockXfer(0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set DMA Channel transfer size */
    apiRetStatus = CyU3PDmaChannelSetXfer (&glUartLpChHandle, CY_FX_UART_DMA_TX_SIZE);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

}

/* Entry function for the UartLpAppThread */
void
UartLpAppThread_Entry (
        uint32_t input)
{
    /* Initialize the UART Example Application */
    CyFxUartLpApplnInit();

    for (;;)
    {
        /* No operation in the thread */
        CyU3PThreadSleep (100);
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
    ptr = CyU3PMemAlloc (CY_FX_UARTLP_THREAD_STACK);

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (&UartLpAppThread,           /* UART Example App Thread structure */
                          "21:UART_loopback_DMA_mode",             /* Thread ID and Thread name */
                          UartLpAppThread_Entry,                   /* UART Example App Thread Entry function */
                          0,                                       /* No input parameter to thread */
                          ptr,                                     /* Pointer to the allocated thread stack */
                          CY_FX_UARTLP_THREAD_STACK,               /* UART Example App Thread stack size */
                          CY_FX_UARTLP_THREAD_PRIORITY,            /* UART Example App Thread priority */
                          CY_FX_UARTLP_THREAD_PRIORITY,            /* UART Example App Thread priority */
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
    status = CyU3PDeviceInit (0);
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

