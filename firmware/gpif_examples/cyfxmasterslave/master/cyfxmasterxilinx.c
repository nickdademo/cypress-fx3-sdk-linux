/*
 ## Cypress FX3 Application Example Source File (cyfxfpgaprog.c)
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

/* Summary
 *
 * This application shows how the Cypress FX3 device can be used as a master
 * device to program Xilinx Virtex-6 FPGAs using the Slave Serial mode.
 *
 * While the data interface on the FPGA side is serial, the data bus on the
 * FX3 side is configured as byte wide. Only the LS bit of this 8 bit data
 * bus contains valid data.
 *
 * The FPGA bitstream is expected to be transferred in burst mode over the
 * BULK-OUT endpoint that is enabled by the firmware application when it
 * enumerates. This data is then converted by the FX3 firmware into serial
 * format and sent out to the FPGA for configuration.
 */

#include "cyfxmasterxilinx.h"
#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3uart.h"
#include "cyu3gpif.h"
#include "cyu3pib.h"
#include "cyfxgpif2config.h"
#include "cyfxmasterxilinxusbdscr.h"

/***********************************************************************************************************************
 * Global variables.
 **********************************************************************************************************************/
static CyU3PThread     glAppThread;	            /* Application thread object. */
static CyU3PDmaChannel glUsbChannel;            /* DMA channel for getting data from the USB endpoint. */
static CyU3PDmaChannel glGpifChannel;           /* DMA channel for sending data out on the GPIF interface. */

static CyBool_t        glEndPointEnabled;       /* Endpoint enabled flag indicates whether data is to be transferred
                                                   from the EP to the GPIF interface. */

CyBool_t glIsApplnActive = CyFalse;             /* Whether the loopback application is active or not. */

/***********************************************************************************************************************
 * Static helper functions.
 **********************************************************************************************************************/

/* This function initializes the Debug Module for logging. */
static void
CyFxFpgaProgDebugInit (
        void)
{
    CyU3PUartConfig_t   uartConfig;
    CyU3PReturnStatus_t stat;

    /* Initialize the UART for printing debug messages */
    stat = CyU3PUartInit ();
    if (stat != CY_U3P_SUCCESS)
    {
        goto DebugInitError;
    }

    /* Set UART Configuration */
	CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof (uartConfig));
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;  /* 115200 baud. */
    uartConfig.stopBit  = CY_U3P_UART_ONE_STOP_BIT;     /* One stop bit. */
    uartConfig.parity   = CY_U3P_UART_NO_PARITY;        /* No parity bit. */
    uartConfig.txEnable = CyTrue;                       /* Enable UART transmitter. */
    uartConfig.rxEnable = CyFalse;                      /* Disable UART receiver. */
    uartConfig.flowCtrl = CyFalse;                      /* No flow control. */
    uartConfig.isDma    = CyTrue;                       /* DMA mode of operation is required when using UART for
                                                           logging. */

    stat = CyU3PUartSetConfig (&uartConfig, NULL);
    if (stat != CY_U3P_SUCCESS)
    {
        goto DebugInitError;
    }

    /* Enable UART in permanent transfer mode. */
    stat = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (stat != CY_U3P_SUCCESS)
    {
        goto DebugInitError;
    }

    /* Initialize the Debug application */
    stat = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (stat == CY_U3P_SUCCESS)
	{
		 CyU3PDebugPrint (2, " call sucees with status \r\n");
		return;
	}
DebugInitError:
    /* As debug module init has failed, we cannot do anything to report this error. */
    while (1);
}

/* Application Error Handler */
static void
CyFxFpgaProgErrorHandler (
        char                *apiname,
        CyU3PReturnStatus_t  stat)
{
    /* Application failed with the error code stat */
    CyU3PDebugPrint (2, "%s call failed with status %d\r\n", apiname, stat);

    /* As no well defined error handling methods are defined, just loop indefinitely. */
    for (;;)
    {
        CyU3PThreadSleep (100);
    }
}
/* Create the DMA channels for the application.
 * Two channels are created.
 *  1. One Manual-IN channel through which data is received from the USB endpoint.
 *  2. One Manual-OUT channel through which the data is sent to the GPIF Interface.
 */
static void
CyFxFpgaProgCreateDmaChannels (
        uint16_t size)
{
    CyU3PDmaChannelConfig_t chConf = {0};
    CyU3PReturnStatus_t stat;
    uint16_t usbPktSize, gpifPktSize;

	/*  Packet size is set according to the Usbspeed */
    usbPktSize = size;
    gpifPktSize = size;    
    
    
    CyU3PDebugPrint (4, "Creating DMA channel for USB OUT endpoint\r\n");
    chConf.size           = usbPktSize;
    chConf.count          = CY_FX_APP_DMA_BUF_COUNT;
    chConf.prodSckId      = CY_FX_EP_PRODUCER_USB_SOCKET;
    chConf.consSckId      = CY_U3P_CPU_SOCKET_CONS;
    chConf.prodAvailCount = 0;
    chConf.prodHeader     = 0;
    chConf.prodFooter     = 0;
    chConf.consHeader     = 0;
    chConf.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    chConf.notification   = 0;
    chConf.cb             = 0;
    stat = CyU3PDmaChannelCreate (&glUsbChannel, CY_U3P_DMA_TYPE_MANUAL_IN, &chConf);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PDmaChannelCreate", stat);
    }

    CyU3PDebugPrint (4, "Creating DMA channel for GPIF egress socket\r\n");
    chConf.size           = gpifPktSize;
	chConf.prodSckId      = CY_U3P_CPU_SOCKET_PROD;
    chConf.consSckId      = CY_FX_EP_CONSUMER_PPORT_SOCKET;
    stat = CyU3PDmaChannelCreate (&glGpifChannel, CY_U3P_DMA_TYPE_MANUAL_OUT, &chConf);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PDmaChannelCreate", stat);
    }

    /* Set DMA Channel transfer size to infinity */
    stat = CyU3PDmaChannelSetXfer (&glUsbChannel, 0);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PDmaChannelSetXfer", stat);
    }

    /* Set DMA Channel transfer size to infinity */
    stat = CyU3PDmaChannelSetXfer (&glGpifChannel, 0);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PDmaChannelSetXfer", stat);
    }
/* Update the flag so that the application thread is notified of this. */
	glIsApplnActive = CyTrue;
}

/* This function starts the bulk loop application. This is called
 * when a SET_CONF event is received from the USB host. The endpoints
 * are configured and the DMA pipe is setup in this function. */

void
CyFxBulkLpApplnStart (
        void)
{
    uint16_t size = 0;
    CyU3PEpConfig_t epConf = {0};
	CyU3PReturnStatus_t stat = CY_U3P_SUCCESS;
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
          
            break;
    }

    /* Configure the USB endpoint as required. */
    epConf.enable   = CyTrue;
    epConf.epType   = CY_U3P_USB_EP_BULK;           /* BULK endpoint. */
    epConf.streams  = 0;
    epConf.pcktSize = 1024;                         /* Set max packet size for SS operation. The FX3 SDK takes
                                                       care of this for other speeds. */
    epConf.burstLen = 0;                            /* No burst support. */
    epConf.isoPkts  = 0;                            /* ISO MULT field. Don't care. */
    
	stat = CyU3PSetEpConfig (CY_FX_EP_PRODUCER, &epConf);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PSetEpConfig", stat);
    }

	CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
	
	/* Create the DMA channel required for data transfers. */
	CyFxFpgaProgCreateDmaChannels (size);
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

    /* Destroy the channels */
    CyU3PDmaChannelDestroy (&glUsbChannel);
    CyU3PDmaChannelDestroy (&glGpifChannel);

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
   

    /* Disable endpoints. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyFalse;

    /* Producer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
    }
    /* Update the flag so that the application thread is notified that endpoints are disabled. */
	glEndPointEnabled = CyFalse;
}

/* Callback to handle the USB Setup Requests */
static CyBool_t
CyFxFpgaProgUsbSetupCbk (
        uint32_t setupdat0,
        uint32_t setupdat1)
{
    /* No setup requests to be handled at the application level. Return
     * CyFalse to indicate that we have not handled the request. */
    CyU3PDebugPrint (4, "Received USB setup request: %x %x\r\n", setupdat0, setupdat1);
    return CyFalse;
}

/* Callback to handle USB related events. */
static void
CyFxFpgaProgUsbEventCbk (
    CyU3PUsbEventType_t evtype,
    uint16_t            evdata)
{
    switch (evtype)
    {
    case CY_U3P_USB_EVENT_CONNECT:
        CyU3PDebugPrint (4, "USB connect event received, speed=%d\r\n", evdata);
        break;

    case CY_U3P_USB_EVENT_DISCONNECT:
        CyU3PDebugPrint (4, "USB disconnect event received\r\n");
        if (glIsApplnActive)
         {        
          /* Stop the application. */
                CyFxBulkLpApplnStop ();
         }
        break;

    case CY_U3P_USB_EVENT_RESET:
         CyU3PDebugPrint (4, "USB reset event received\r\n");
         if (glIsApplnActive)
          {
            /* Stop the application. */
                CyFxBulkLpApplnStop ();
          }
		break;

    case CY_U3P_USB_EVENT_SETCONF:
          /* Stop the application before re-starting. */
          if (glIsApplnActive)
          {
            /* Stop the application. */
             CyFxBulkLpApplnStop ();
          }
          /* Start the application. */
          CyFxBulkLpApplnStart ();
          glEndPointEnabled = CyTrue;
		  break;

    default:
        CyU3PDebugPrint (4, "Unhandled USB event type %d\r\n", evtype);
        break;
    }
}

/* Perform USB module initialization for the FPGA programmer application. */
static void
CyFxFpgaProgUsbInit (
        void)
{
    
    CyU3PReturnStatus_t stat = CY_U3P_SUCCESS;

    /* Start the USB functionality. */
    CyU3PDebugPrint (4, "Calling CyU3PUsbStart\r\n");
    stat = CyU3PUsbStart ();
    if (stat != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PUsbStart failed to Start, Error code = %d\n", stat);
		CyFxFpgaProgErrorHandler ("CyU3PUsbStart", stat);
    }

    /* Register USB related callback functions. */
    CyU3PUsbRegisterSetupCallback (CyFxFpgaProgUsbSetupCbk, CyTrue);
    CyU3PUsbRegisterEventCallback (CyFxFpgaProgUsbEventCbk);

    /* Register the USB descriptors with the FX3 library. */

    /* USB 2.0 Device descriptor. */
    stat = CyU3PUsbSetDesc (CY_U3P_USB_SET_HS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB20DeviceDscr);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PUsbSetDesc", stat);
    }

    /* USB 3.0 device descriptor. */
    stat = CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB30DeviceDscr);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PUsbSetDesc", stat);
    }

    /* Device Qualifier Descriptor */
    stat = CyU3PUsbSetDesc (CY_U3P_USB_SET_DEVQUAL_DESCR, NULL, (uint8_t *)CyFxUSBDeviceQualDscr);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PUsbSetDesc", stat);
    }

    /* HS Configuration descriptor. */
    stat = CyU3PUsbSetDesc (CY_U3P_USB_SET_HS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBHSConfigDscr);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PUsbSetDesc", stat);
    }

    /* FS Configuration descriptor. */
    stat = CyU3PUsbSetDesc (CY_U3P_USB_SET_FS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBFSConfigDscr);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PUsbSetDesc", stat);
    }

    /* SS Configuration Descriptor */
    stat = CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBSSConfigDscr);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PUsbSetDesc", stat);
    }

    /* BOS Descriptor */
    stat = CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_BOS_DESCR, NULL, (uint8_t *)CyFxUSBBOSDscr);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PUsbSetDesc", stat);
    }

    /* String Descriptor 0 */
    stat = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PUsbSetDesc", stat);
    }

    /* String Descriptor 1 */
    stat = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PUsbSetDesc", stat);
    }

    /* String Descriptor 2 */
    stat = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PUsbSetDesc", stat);
    }

    CyU3PDebugPrint (4, "Finished registering USB descriptors\r\n");

    /* Connect to the USB host in SS mode if possible. */
    stat = CyU3PConnectState (CyTrue, CyFalse);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PConnectState", stat);
    }
}


/* Perform GPIF module initialization for the FPGA programmer application. */
static void
CyFxFpgaProgGpifInit (
        void)
{
    CyU3PPibClock_t pibClock;
    CyU3PReturnStatus_t stat;

    CyU3PDebugPrint (4, "Initializing PIB block\r\n");

    pibClock.clkDiv      = 2;
    pibClock.clkSrc      = CY_U3P_SYS_CLK;
    pibClock.isHalfDiv   = CyFalse;
    pibClock.isDllEnable = CyTrue;
    stat = CyU3PPibInit (CyTrue, &pibClock);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PPibInit", stat);
    }


    CyU3PDebugPrint (4, "Loading GPIF configuration for master\r\n");
    stat = CyU3PGpifLoad ((CyU3PGpifConfig_t *)&Xilinx_Progmaster_CyFxGpifConfig);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PGpifLoad", stat);
    }

    CyU3PDebugPrint (4, "Finished GPIF configuration\r\n");
}


/***********************************************************************************************************************
 * Application logic.
 **********************************************************************************************************************/

/* Main function for the APP - Perform FX3 API library initialization. */
int
main (
        void)
{
    CyU3PReturnStatus_t   status;
    CyU3PIoMatrixConfig_t io_cfg;

    /* Initialize the device */
    status = CyU3PDeviceInit (NULL);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }
	
    status = CyU3PDeviceCacheControl (CyTrue, CyFalse, CyFalse);
    
	/* Configure the IO matrix for the device.*/
    io_cfg.isDQ32Bit        = CyFalse;                          /* GPIF data bus is only 8 bits wide. */
    io_cfg.useUart          = CyTrue;                           /* UART is enabled for logging. */
    io_cfg.useI2C           = CyFalse;                          /* I2C is not used. */
    io_cfg.useI2S           = CyFalse;                          /* I2S is not used. */
    io_cfg.useSpi           = CyFalse;                          /* SPI is not used. */
    io_cfg.lppMode          = CY_U3P_IO_MATRIX_LPP_UART_ONLY;     /* Default LPP IO configuration. */
    io_cfg.gpioSimpleEn[0]  = 0;                                /* No GPIOs are in use. */
    io_cfg.gpioSimpleEn[1]  = 0;                                /* No GPIOs are in use. */
    io_cfg.gpioComplexEn[0] = 0;                                /* No GPIOs are in use. */
    io_cfg.gpioComplexEn[1] = 0;                                /* No GPIOs are in use. */

    status = CyU3PDeviceConfigureIOMatrix (&io_cfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }
    
    /* This is a non returning call for initializing the RTOS kernel */
    CyU3PKernelEntry ();

    /* Dummy return to make the compiler happy */
    return 0;

handle_fatal_error:
    /* Cannot recover from this error. */
    while (1);
}

/* Entry function for the master thread. */
void
CyFxFpgaProgThreadEntry (
        uint32_t input)
{
    CyU3PDmaBuffer_t srcInfo = {0};
    CyU3PDmaBuffer_t dstInfo = {0};
    CyU3PReturnStatus_t stat;

	
    glEndPointEnabled = CyFalse;        /* Keep endpoint disabled at startup. */
    

	/* Initialize the Debug Module. */
	CyFxFpgaProgDebugInit ();
    
    /* Start-up initialization for GPIF and USB blocks. */
	CyFxFpgaProgGpifInit ();
    CyFxFpgaProgUsbInit ();

	
	/* Wait for the Endpoint Enable flag to get set. This happens on receiving a USB
     * SET_CONFIGURATION request. */
    while (!glEndPointEnabled)
        CyU3PThreadSleep (1000);
     
	/* Start the GPIF state machine off. */
    stat = CyU3PGpifSMStart (XILINX_PROGMASTER_START, XILINX_PROGMASTER_ALPHA_START);
    if (stat != CY_U3P_SUCCESS)
    {
        CyFxFpgaProgErrorHandler ("CyU3PGpifSMStart", stat);
    }
  
     for (;;)
     {
        if (glIsApplnActive)
        {
            /* Wait for receiving a buffer from the producer socket (OUT endpoint). The call
             * will fail if there was an error or if the USB connection was reset / disconnected.
             * In case of error invoke the error handler and in case of reset / disconnection,
             * glIsApplnActive will be CyFalse; continue to beginning of the loop. */


	    stat = CyU3PDmaChannelGetBuffer (&glUsbChannel, &srcInfo, 1000);
        if ((stat != CY_U3P_SUCCESS) && (stat != CY_U3P_ERROR_TIMEOUT))
        {
		    CyFxFpgaProgErrorHandler ("CyU3PDmaChannelGetBuffer", stat);
        }
		/* CyU3PDebugPrint (2, "USB Get Buffer"); */
        if (stat == CY_U3P_ERROR_TIMEOUT)
            continue;
		
        stat = CyU3PDmaChannelGetBuffer (&glGpifChannel, &dstInfo, CYU3P_WAIT_FOREVER);
        if (stat != CY_U3P_SUCCESS)
        { 
		    CyFxFpgaProgErrorHandler ("CyU3PDmaChannelGetBuffer", stat);
        }
         /* Copy the data from the producer channel to the consumer channel. 
          * The srcInfo.count holds the amount of valid data received. */
		CyU3PMemCopy (dstInfo.buffer, srcInfo.buffer, srcInfo.count);

		/* Commit the data on the outgoing data buffer. */
        stat = CyU3PDmaChannelCommitBuffer (&glGpifChannel, srcInfo.count, 0);
		
        if (stat != CY_U3P_SUCCESS)
        {
            CyFxFpgaProgErrorHandler ("CyU3PDmaChannelCommitBuffer", stat);
        }
		stat = CyU3PDmaChannelDiscardBuffer (&glUsbChannel);
        /* Discard the data in the ingress data buffer. */
        if (stat != CY_U3P_SUCCESS)
        {
            CyFxFpgaProgErrorHandler ("CyU3PDmaChannelDiscardBuffer", stat);
        }
	}
	else
		{
			CyU3PThreadSleep (100);
		}
  }
}

/*
 * Application define function which creates the threads. This is called from
 * the tx_application _define function.
 */
void
CyFxApplicationDefine (
        void)
{
    void *threadStack = NULL;
    uint32_t stat = 0;

    /* Initialize the Debug Module */
    

    /* Allocate the memory for the threads */
    threadStack = CyU3PMemAlloc (CY_FX_APP_THREAD_STACK_SIZE);
    if (threadStack == 0)
    {
        CyFxFpgaProgErrorHandler ("CyU3PMemAlloc", CY_U3P_ERROR_MEMORY_ERROR);
    }

    /* Create and start the thread for the application. */
    stat = CyU3PThreadCreate (&glAppThread, "31:Fpga Programmer",
            CyFxFpgaProgThreadEntry, 0, threadStack,
            CY_FX_APP_THREAD_STACK_SIZE, CY_FX_APP_THREAD_PRIORITY,
            CY_FX_APP_THREAD_PRIORITY, CYU3P_NO_TIME_SLICE, CYU3P_AUTO_START);
    if (stat != 0)
    {
        CyFxFpgaProgErrorHandler ("CyU3PThreadCreate", stat);
    }
}

/*[]*/

