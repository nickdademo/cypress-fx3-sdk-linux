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

/* This file illustrates the usb host mode application example. */

/*
   This example illustrates the use of the FX3 firmware APIs to implement
   USB host mode operation. The example supports simple HID mouse class
   and simple MSC class devices.

   USB HID MOUSE DRIVER:

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
   in cyfxusbhost.h file.

   USB MSC DRIVER:

   A simple single interface USB BOT MSC device will be successfully 
   enumerated and storage parameters will be queried. The example shall read
   the last sector of data, save it in local buffers and write a fixed pattern
   to location. It will  then read and verify the data written. Once this is
   done, the original data will be written back to the storage.
   
   NOTE: If MSC device is unplugged before all transactions have completed
   or if the example encounters an error, the data on the drive might be
   corrupted.
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
#include "cyfxusbhost.h"

CyU3PThread applnThread;                        /* Application thread structure */
CyBool_t glIsApplnActive = CyFalse;             /* Whether the application is active or not. */
CyBool_t glIsPeripheralPresent = CyFalse;       /* Whether a remote peripheral is present or not. */

uint8_t glSetupPkt[CY_FX_HOST_EP0_SETUP_SIZE] __attribute__ ((aligned (32)));   /* Setup packet buffer for host mode. */
uint8_t glEp0Buffer[CY_FX_HOST_EP0_BUFFER_SIZE] __attribute__ ((aligned (32))); /* Buffer to send / receive data for EP0. */

uint8_t glHostOwner = CY_FX_HOST_OWNER_NONE;    /* Current owner for the host controller. */

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

    CyU3PDebugPrint (4, "UsbOtg Example\r\n");
    if (CyTrue == CY_FX_HOST_VBUS_ENABLE_VALUE)
    {
        CyU3PDebugPrint (4, "Assuming that driving CTL4 high turns VBus output ON\n");
    }
    else
    {
        CyU3PDebugPrint (4, "Assuming that driving CTL4 low turns VBus output OFF\n");
    }
}

/* USB host stack event callback function. */
void
CyFxHostEventCb (CyU3PUsbHostEventType_t evType, uint32_t evData)
{
    /* This is connect / disconnect event. Log it so that the
     * application thread can handle it. */
    if (evType == CY_U3P_USB_HOST_EVENT_CONNECT)
    {
        CyU3PDebugPrint (4, "Host connect event received\r\n");
        glIsPeripheralPresent = CyTrue;
    }
    else
    {
        CyU3PDebugPrint (4, "Host disconnect event received\r\n");
        glIsPeripheralPresent = CyFalse;
    }
}

/* Helper function to send EP0 setup packet. */
CyU3PReturnStatus_t
CyFxSendSetupRqt (
        uint8_t type,
        uint8_t request,
        uint16_t value,
        uint16_t index,
        uint16_t length,
        uint8_t *buffer_p)
{
    CyU3PUsbHostEpStatus_t epStatus;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    glSetupPkt[0] = type;
    glSetupPkt[1] = request;
    glSetupPkt[2] = CY_U3P_GET_LSB(value);
    glSetupPkt[3] = CY_U3P_GET_MSB(value);
    glSetupPkt[4] = CY_U3P_GET_LSB(index);
    glSetupPkt[5] = CY_U3P_GET_MSB(index);
    glSetupPkt[6] = CY_U3P_GET_LSB(length);
    glSetupPkt[7] = CY_U3P_GET_MSB(length);

    status = CyU3PUsbHostSendSetupRqt (glSetupPkt, buffer_p);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }
    status = CyU3PUsbHostEpWaitForCompletion (0, &epStatus,
            CY_FX_HOST_EP0_WAIT_TIMEOUT);

    return status;
}

/* This function initializes the mouse driver application. */
void
CyFxApplnStart ()
{
    uint16_t length;
    CyU3PReturnStatus_t status;
    CyU3PUsbHostEpConfig_t epCfg;

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
    status = CyFxSendSetupRqt (0x80, CY_U3P_USB_SC_GET_DESCRIPTOR,
            (CY_U3P_USB_DEVICE_DESCR << 8), 0, 8, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
    CyU3PDebugPrint (6, "Device descriptor received\r\n");

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
    status = CyFxSendSetupRqt (0x80, CY_U3P_USB_SC_GET_DESCRIPTOR,
            (CY_U3P_USB_DEVICE_DESCR << 8), 0, 18, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }

    /* Set the peripheral device address. */
    status = CyFxSendSetupRqt (0x00, CY_U3P_USB_SC_SET_ADDRESS,
            CY_FX_HOST_PERIPHERAL_ADDRESS, 0, 0, glEp0Buffer);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
    status = CyU3PUsbHostSetDeviceAddress (CY_FX_HOST_PERIPHERAL_ADDRESS);
    if (status != CY_U3P_SUCCESS)
    {
        goto enum_error;
    }
    CyU3PDebugPrint (6, "Device address set\r\n");

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
    CyU3PDebugPrint (6, "Config descriptor received\r\n");

    /* Identify if this is an HID mouse or MSC device that can be
     * supported. If the device cannot be supported, just disable
     * the port and wait for a new device to be attached. We support
     * only single interface with interface class = HID(0x03),
     * interface sub class = Boot (0x01) 
     * and interface protocol = Mouse (0x02).
     * or single interface with interface class = MSC(0x08),
     * interface sub class 0x06 and interface protocol BOT (0x50). */
    if ((glEp0Buffer[5] == 1) && (glEp0Buffer[14] == 0x03) &&
            (glEp0Buffer[15] == 0x01) && (glEp0Buffer[16] == 0x02) &&
            (glEp0Buffer[28] == CY_U3P_USB_ENDPNT_DESCR))
    {
        CyU3PDebugPrint (6, "Mouse device detected\r\n");
        status = CyFxMouseDriverInit ();
        if (status == CY_U3P_SUCCESS)
        {
            glIsApplnActive = CyTrue;
            glHostOwner = CY_FX_HOST_OWNER_MOUSE_DRIVER;
            return;
        }
    }
    else if ((glEp0Buffer[5] == 1) && (glEp0Buffer[14] == 0x08) &&
            (glEp0Buffer[15] == 0x06) && (glEp0Buffer[16] == 0x50))
    {
        CyU3PDebugPrint (6, "MSC device detected\r\n");
        status = CyFxMscDriverInit ();
        if (status == CY_U3P_SUCCESS)
        {
            glIsApplnActive = CyTrue;
            glHostOwner = CY_FX_HOST_OWNER_MSC_DRIVER;
            return;
        }
    }
    else
    {
        /* Do nothing here. Fall-through to disable the USB port.
         * We do not support this device. */
        CyU3PDebugPrint (6, "Unknown device type\r\n");
        status = CY_U3P_ERROR_NOT_SUPPORTED;
    }

enum_error:
    /* Remove EP0. and disable the port. */
    glHostOwner = CY_FX_HOST_OWNER_NONE;
    CyU3PUsbHostEpRemove (0);
    CyU3PUsbHostPortDisable ();
    CyU3PDebugPrint (4, "Application start failed with error: %d.\r\n", status);
}

/* This function disables the mouse driver application. */
void
CyFxApplnStop ()
{
    if (glHostOwner == CY_FX_HOST_OWNER_MOUSE_DRIVER)
    {
        CyFxMouseDriverDeInit ();
    }
    if (glHostOwner == CY_FX_HOST_OWNER_MSC_DRIVER)
    {
        CyFxMscDriverDeInit ();
    }

    /* Remove EP0. and disable the port. */
    CyU3PUsbHostEpRemove (0);
    CyU3PUsbHostPortDisable ();

    glHostOwner = CY_FX_HOST_OWNER_NONE;

    /* Clear state variables. */
    glIsApplnActive = CyFalse;
}

/* This function initializes the USB host stack. */
void
CyFxUsbHostStart ()
{
    CyU3PUsbHostConfig_t hostCfg;
    CyU3PReturnStatus_t status;

    hostCfg.ep0LowLevelControl = CyFalse;
    hostCfg.eventCb = CyFxHostEventCb;
    hostCfg.xferCb = NULL;
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

/* This function enables / disables driving VBUS. */
void
CyFxUsbVBusControl (
        CyBool_t isEnable)
{
    if (isEnable)
    {
        /* Drive VBUS only if no-one else is driving. */
        if (!CyU3POtgIsVBusValid ())
        {
            CyU3PGpioSimpleSetValue (21, CY_FX_HOST_VBUS_ENABLE_VALUE);
        }
    }
    else
    {
        CyU3PGpioSimpleSetValue (21, CY_FX_HOST_VBUS_DISABLE_VALUE);
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
            if (input == CY_U3P_OTG_TYPE_A_CABLE)
            {
                /* Enable the VBUS supply. */
                CyFxUsbVBusControl (CyTrue);
            }
            else
            {
                /* Make sure that the VBUS supply is disabled. */
                CyFxUsbVBusControl (CyFalse);

                /* Stop the previously started host stack. */
                if ((!CyU3POtgIsHostMode ()) && (CyU3PUsbHostIsStarted ()))
                {
                    CyFxUsbHostStop ();
                }
            }
            break;

        case CY_U3P_OTG_VBUS_VALID_CHANGE:
            if (input)
            {
                /* Start the host mode stack. */
                if (!CyU3PUsbHostIsStarted ())
                {
                    CyFxUsbHostStart ();
                }
            }
            else
            {
                /* If the OTG mode has changed, stop the previous stack. */
                if ((!CyU3POtgIsHostMode ()) && (CyU3PUsbHostIsStarted ()))
                {
                    /* Stop the previously started host stack. */
                    CyFxUsbHostStop ();
                }
            }
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
    simpleCfg.outValue = CY_FX_HOST_VBUS_DISABLE_VALUE;
    simpleCfg.driveLowEn = CyTrue;
    simpleCfg.driveHighEn = CyTrue;
    simpleCfg.inputEn = CyFalse;
    simpleCfg.intrMode = CY_U3P_GPIO_NO_INTR;
    status = CyU3PGpioSetSimpleConfig (21, &simpleCfg);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Wait until VBUS is stabilized. */
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
    CyBool_t isPresent = CyFalse;
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
                else
                {
                    CyU3PDebugPrint (2, "HostPortEnable failed with code %d\r\n", status);
                }
            }

            /* Update the state variable. */
            isPresent = glIsPeripheralPresent;
        }

        /* Since the test needs to be done from a thread,
         * this function is called at fixed interval. */
        if (glHostOwner == CY_FX_HOST_OWNER_MSC_DRIVER)
        {
            CyFxMscDriverDoWork ();
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

