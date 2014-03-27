/*
 ## Cypress USB 3.0 Platform source file (cyfxusbotg.c)
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

/* This file illustrates the usb OTG mode application example. */

/*
   This example illustrates the use of the FX3 firmware APIs to implement
   USB OTG mode operation. In host mode of operation, we expect to be
   connected to a USB mouse and in device mode operation we expected to be
   connected to a host PC which can do bulk loop operation.

   Host mode:

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

   Care should be taken so that an USB host is not connected to FX3 while
   host stack is active. This will mean that both sides will be driving
   the VBUS causing a hardware damage.

   Please refer to the FX3 DVK user manual for enabling VBUS control.
   The current example controls the VBUS supply via GPIO 21. The example assumes
   that a low on this line will turn on VBUS and allow remotely connected B-device
   to enumerate. A high / tri-state  on this line will turn off VBUS supply from
   the board. Also this requires that FX3 DVK is powered using external power supply.
   VBATT should also be enabled and connected. Depending upon the VBUS control,
   update the CY_FX_OTG_VBUS_ENABLE_VALUE and CY_FX_OTG_VBUS_DISABLE_VALUE definitions
   in cyfxusbotg.h file.

   Device mode:

   This example illustrates a loopback mechanism between two USB bulk endpoints.
   The example comprises of vendor class USB enumeration descriptors with two
   bulk endpoints. A bulk OUT endpoint acts as the producer of data from the host.
   A bulk IN endpint acts as the consumer of data to the host.

   The loopback is achieved with the help of a DMA AUTO channel. DMA AUTO channel
   is created between the producer USB bulk endpoint and the consumer USB bulk
   endpoint. Data is transferred from the host into the producer endpoint which
   is then directly transferred to the consumer endpoint by the DMA engine.
   CPU is not involved in the data transfer.

   The DMA buffer size is defined based on the USB speed. 64 for full speed,
   512 for high speed and 1024 for super speed. CY_FX_BULKLP_DMA_BUF_COUNT in the
   header file defines the number of DMA buffers.
*/

#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3usbhost.h"
#include "cyu3usbotg.h"
#include "cyu3uart.h"
#include "cyu3gpio.h"
#include "cyu3utils.h"
#include "cyfxusbotg.h"

CyU3PThread applnThread;                        /* Application thread structure */

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
    }
}

/* This function initializes the debug module. The debug prints
 * are routed to the UART and can be seen using a UART console
 * running at 115200 baud rate. */
void
CyFxApplnDebugInit (void)
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

    CyU3PDebugPreamble (CyFalse);

    CyU3PDebugPrint (2, "UsbOtg Example\r\n");
    if (CyTrue == CY_FX_OTG_VBUS_ENABLE_VALUE)
    {
        CyU3PDebugPrint (2, "Assuming that driving CTL4 high turns VBus output ON\n");
    }
    else
    {
        CyU3PDebugPrint (2, "Assuming that driving CTL4 low turns VBus output OFF\n");
    }
}

void
CyFxUsbVBusControl (
        CyBool_t isEnable)
{
    /* This function enables / disables driving VBUS. */
    if (isEnable)
    {
        /* Drive VBUS only if no-one else is driving. */
        if (!CyU3POtgIsVBusValid ())
        {
            CyU3PGpioSimpleSetValue (21, CY_FX_OTG_VBUS_ENABLE_VALUE);
        }
    }
    else
    {
        CyU3PGpioSimpleSetValue (21, CY_FX_OTG_VBUS_DISABLE_VALUE);
    }
}

/* OTG peripheral change event handler. */
static void
CyFxOtgPeripheralChangeHandler (
        CyU3POtgPeripheralType_t otgType)
{
    /* Make sure that the VBUS is disabled. */
    CyFxUsbVBusControl (CyFalse);

    /* If the OTG mode has changed, stop the previous stack. */
    if (((!CyU3POtgIsDeviceMode ()) && (CyU3PUsbIsStarted ())) ||
            (CyU3POtgGetPeripheralType () == CY_U3P_OTG_TYPE_ACA_B_CHG))
    {
        /* Stop the previously started device stack. */
        CyFxUsbStop ();
    }
    if ((!CyU3POtgIsHostMode ()) && (CyU3PUsbHostIsStarted ()))
    {
        /* Stop the previously started host stack. */
        CyFxUsbHostStop ();
    }

    switch (otgType)
    {
        case CY_U3P_OTG_TYPE_A_CABLE:
            /* Enable VBUS in this case. */
            CyFxUsbVBusControl (CyTrue);
            break;

        case CY_U3P_OTG_TYPE_ACA_A_CHG:
            /* Initialize the USB host mode of operation. */
            if (!CyU3PUsbHostIsStarted ())
            {
                CyFxUsbHostStart ();
            }
            break;

        case CY_U3P_OTG_TYPE_B_CABLE:
        case CY_U3P_OTG_TYPE_ACA_B_CHG:
            /* Initiate SRP with a repeat interval
             * of 1s if VBUS is not valid. */
            if (!CyU3POtgIsVBusValid ())
            {
                CyU3POtgSrpStart (1000);
            }
            else
            {
                /* The VBUS is already valid. Start the device stack. */
                if (!CyU3PUsbIsStarted ())
                {
                    CyFxUsbStart ();
                }
            }
            break;

        case CY_U3P_OTG_TYPE_ACA_C_CHG:
            /* Start the device mode of operation if not
             * already started. */
            if (!CyU3PUsbIsStarted ())
            {
                CyFxUsbStart ();
            }
            break;

        default:
            /* Do nothing. */
            break;
    }
}

/* OTG VBUS change event handler. */
static void
CyFxOtgVbusChangeHandler (CyBool_t vbusValid)
{
    if (vbusValid)
    {
        /* Stop the previously running stack and
         * start the required stack. */
        if ((CyU3POtgIsDeviceMode ()) && (!CyU3PUsbIsStarted ()))
        {
            if (CyU3PUsbHostIsStarted ())
            {
                /* Stop the previously started host stack. */
                CyFxUsbHostStop ();
            }

            /* Start the device mode stack only if this is not
             * a CY_U3P_OTG_TYPE_ACA_B_CHG type of charger. */
            if ((CyU3POtgGetPeripheralType () != CY_U3P_OTG_TYPE_ACA_B_CHG)
                    && (!CyU3PUsbIsStarted ()))
            {
                CyFxUsbStart ();
            }
        }
        if ((CyU3POtgIsHostMode ()) && (!CyU3PUsbHostIsStarted ()))
        {
            if (CyU3PUsbIsStarted ())
            {
                /* Stop the previously started device stack. */
                CyFxUsbStop ();
            }

            /* Start the host mode stack if a remote device has
             * been detected. */
            if (!CyU3PUsbHostIsStarted ())
            {
                CyFxUsbHostStart ();
            }
        }
    }
    else
    {
        /* If the OTG mode has changed, stop the previous stack. */
        if ((!CyU3POtgIsDeviceMode ()) && (CyU3PUsbIsStarted ()))
        {
            /* Stop the previously started device stack. */
            CyFxUsbStop ();
        }
        if ((!CyU3POtgIsHostMode ()) && (CyU3PUsbHostIsStarted ()))
        {
            /* Stop the previously started host stack. */
            CyFxUsbHostStop ();
        }
    }
}

/* OTG event handler. */
void
CyFxOtgEventCb (
        CyU3POtgEvent_t event,
        uint32_t input)
{
    CyU3PDebugPrint (4, "OTG Event: %d, Input: %d.\r\n", event, input);
    switch (event)
    {
        case CY_U3P_OTG_PERIPHERAL_CHANGE:
            CyFxOtgPeripheralChangeHandler ((CyU3POtgPeripheralType_t)input);
            break;

        case CY_U3P_OTG_SRP_DETECT:
            /* Turn on the VBUS. We will not get this interrupt unless
             * we are connected to an OTG B-type device. */
            break;

        case CY_U3P_OTG_VBUS_VALID_CHANGE:
            CyFxOtgVbusChangeHandler ((CyBool_t)input);
            break;

        default:
            /* do nothing */
            break;
    }
}

/* This function initializes the USB module. */
CyU3PReturnStatus_t
CyFxApplnInit (void)
{
    CyU3POtgConfig_t otgCfg;
    CyU3PGpioClock_t clkCfg;
    CyU3PGpioSimpleConfig_t simpleCfg;
    CyU3PReturnStatus_t status;

    /* Initialize GPIO module. */
    clkCfg.fastClkDiv = 2;
    clkCfg.slowClkDiv = 0;
    clkCfg.halfDiv = CyFalse;
    clkCfg.simpleDiv = CY_U3P_GPIO_SIMPLE_DIV_BY_2;
    clkCfg.clkSrc = CY_U3P_SYS_CLK;
    status = CyU3PGpioInit (&clkCfg, NULL);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Override GPIO 21 for VBUS control. */
    status = CyU3PDeviceGpioOverride (21, CyTrue);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Configure GPIO 21 as output for VBUS control. */
    simpleCfg.outValue = CY_FX_OTG_VBUS_DISABLE_VALUE;
    simpleCfg.driveLowEn = CyTrue;
    simpleCfg.driveHighEn = CyTrue;
    simpleCfg.inputEn = CyFalse;
    simpleCfg.intrMode = CY_U3P_GPIO_NO_INTR;
    status = CyU3PGpioSetSimpleConfig (21, &simpleCfg);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Wait until the VBUS is stabilized. */
    CyU3PThreadSleep (100);

    /* Initialize the OTG module. */
    otgCfg.otgMode = CY_U3P_OTG_MODE_OTG;
    otgCfg.chargerMode = CY_U3P_OTG_CHARGER_DETECT_ACA_MODE;
    otgCfg.cb = CyFxOtgEventCb;
    status = CyU3POtgStart (&otgCfg);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Since VBATT or VBUS is required for OTG operation enable it. */
    status = CyU3PUsbVBattEnable (CyTrue);

    return status;
}

/* Entry function for the AppThread. */
void
ApplnThread_Entry (
        uint32_t input)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Initialize the debug logger. */
    CyFxApplnDebugInit ();

    /* Initialize the example application. */
    status = CyFxApplnInit();
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (2, "Application initialization failed. Aborting.\r\n");
        CyFxAppErrorHandler (status);
    }

    for (;;)
    {
        CyU3PThreadSleep (100);
        if (CyU3PUsbHostIsStarted ())
        {
            CyFxUsbHostDoWork ();
        }
    }
}

/* Application define function which creates the threads. */
void
CyFxApplicationDefine (
        void)
{
    void *ptr = NULL;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_APPLN_THREAD_STACK);

    /* Create the thread for the application */
    status = CyU3PThreadCreate (&applnThread,               /* App Thread structure */
                          "21:USB_DEBUG",                   /* Thread ID and Thread name */
                          ApplnThread_Entry,                /* App Thread Entry function */
                          0,                                /* No input parameter to thread */
                          ptr,                              /* Pointer to the allocated thread stack */
                          CY_FX_APPLN_THREAD_STACK,         /* App Thread stack size */
                          CY_FX_APPLN_THREAD_PRIORITY,      /* App Thread priority */
                          CY_FX_APPLN_THREAD_PRIORITY,      /* App Thread pre-emption threshold */
                          CYU3P_NO_TIME_SLICE,              /* No time slice for the application thread */
                          CYU3P_AUTO_START                  /* Start the Thread immediately */
                          );

    /* Check the return code */
    if (status != 0)
    {
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
    /* GPIO 21 is enabled for VBUS control. But since this IO is part of p-port
     * it has to be overridden. Here no GPIO is enabled. */
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

