/*
 ## Cypress USB 3.0 Platform source file (cyfxmain.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2013,
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

/* This file is an FX3 device firmware example */

#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3uart.h>
#include <cyu3error.h>
#include <cyu3usbconst.h>
#include <cyu3usb.h>
#include <cyu3utils.h>
#include <cyu3usbpp.h>
#include <cyu3usbhost.h>
#include <cyu3regs.h>
#include <cyu3usbotg.h>
#include <cyu3gpio.h>

CyU3PThread thread1, thread2;           /* Thread structures */
CyBool_t resetReq  = CyFalse;
CyBool_t warmReset = CyFalse;

#define THREAD1_STACK       (0x0400)    /* Thread1 stack size */
#define THREAD1_PRIORITY    (8)         /* Thread1 priority */
#define THREAD2_STACK       (0x0400)    /* Thread2 stack size */
#define THREAD2_PRIORITY    (8)         /* Thread2 priority */

/* device Descriptor */
uint8_t glUsbDeviceDesc[18] =
{
    0x12, 0x01, 0x00, 0x02,
    0x00, 0x00 ,0x00, 0x40,
    0xb4, 0x04, 0x20, 0x47,
    0x20, 0x00, 0x00, 0x00,
    0x00, 0x01
};

/* device Descriptor for Super Speed*/
uint8_t glUsbDeviceDescSS[18] =
{
    0x12, 0x01, 0x00, 0x03,
    0x00, 0x00 ,0x00, 0x09,
    0xb4, 0x04, 0x30, 0x47,
    0x20, 0x00, 0x00, 0x00,
    0x00, 0x01
};

uint8_t glUsbDeviceDescHS[18] =
{
    0x12, 0x01, 0x00, 0x02,
    0x00, 0x00 ,0x00, 0x40,
    0xb4, 0x04, 0x30, 0x47,
    0x20, 0x00, 0x00, 0x00,
    0x00, 0x01
};

uint8_t glUsbBOSDscr[] = 
{
    0x05,                           /* Descriptor Size */
    CY_U3P_BOS_DESCR,               /* Device Descriptor Type */
    0x16, 0x00,                     /* Length of this descriptor and all sub descriptors */
    0x02,                           /* Number of device capability descriptors */

    /* USB 2.0 Extension */
    0x07,                           /* Descriptor Size */
    CY_U3P_DEVICE_CAPB_DESCR,       /* Device Capability Type descriptor */
    CY_U3P_USB2_EXTN_CAPB_TYPE,     /* USB 2.0 Extension Capability Type */
    0x00, 0x00, 0x00, 0x00,         /* Supported device level features  */

    /* SuperSpeed Device Capability */
    0x0A,                           /* Descriptor Size */
    CY_U3P_DEVICE_CAPB_DESCR,       /* Device Capability Type descriptor */
    CY_U3P_SS_USB_CAPB_TYPE,        /* SuperSpeed Device Capability Type */
    0x00,                           /* Supported device level features */
    0x0E, 0x00,                     /* Speeds Supported by the device : SS, HS and FS */
    0x03,                           /* Functionality support */
    0x00,                           /* U1 Device Exit Latency */
    0x00, 0x00                      /* U2 Device Exit Latency */
};

/* device qualifier Descriptor */
uint8_t glUsbDeviceQualDesc[10] =
{
    0x0a,0x06,0x00,0x02,0x00,
    0x00,0x00,0x40,0x01,0x00
};

uint8_t glUsbSSConfigDscr[] = 
{
    /* Configuration Descriptor Type */
    0x09,                           /* Descriptor Size */
    CY_U3P_USB_CONFIG_DESCR,        /* Configuration Descriptor Type */
    0x60,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x01,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* COnfiguration string index */
    0x00,                           /* Config characteristics - D6: Self power; D5: Remote Wakeup */
    0x32,                           /* Max power consumption of device (in 8mA unit) : 400mA */

    /* Interface Descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface Descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x06,                           /* Number of end points */
    0xFF,                           /* Interface class */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Endpoint Descriptor */
    0x07,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint Descriptor Type */
    0x01,                           /* Endpoint address and description */
    CY_U3P_USB_EP_BULK,             /* Bulk End point Type */
    0x00,0x04,                      /* Max packet size = 1024 bytes */
    0x01,                           /* Servicing interval for data transfers */ 

    /* Super Speed Endpoint Companion Descriptor for Producer EP */
    0x06,                           /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,       /* SS Endpoint Companion Descriptor Type */
    0x0F,                           /* Max no. of packets in a Burst : 16 */
    0x00,                           /* No streams. */
    0x00,0x00,                      /* Bytes per interval : Not applicable. */

    /* Endpoint Descriptor */
    0x07,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint Descriptor Type */
    0x81,                           /* Endpoint address and description */
    CY_U3P_USB_EP_BULK,             /* Bulk End point Type */
    0x00,0x04,                      /* Max packet size = 1024 bytes */
    0x01,                           /* Servicing interval for data transfers */

    /* Super Speed Endpoint Companion Descriptor for Consumer EP */
    0x06,                           /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,       /* SS Endpoint Companion Descriptor Type */
    0x0F,                           /* Max no. of packets in a Burst : 16 */
    0x00,                           /* No streams. */
    0x00,0x00,                      /* Bytes per interval : Not applicable. */

    /* Endpoint Descriptor */
    0x07,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint Descriptor Type */
    0x02,                           /* Endpoint address and description */
    CY_U3P_USB_EP_BULK,             /* Bulk End point Type */
    0x00,0x04,                      /* Max packet size = 1024 bytes */
    0x01,                           /* Servicing interval for data transfers */ 

    /* Super Speed Endpoint Companion Descriptor for Producer EP */
    0x06,                           /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,       /* SS Endpoint Companion Descriptor Type */
    0x00,                           /* Max no. of packets in a Burst : 1 */
    0x04,                           /* Sixteen streams. */
    0x00,0x00,                      /* Bytes per interval : Not applicable. */

    /* Endpoint Descriptor */
    0x07,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint Descriptor Type */
    0x82,                           /* Endpoint address and description */
    CY_U3P_USB_EP_BULK,             /* Bulk End point Type */
    0x00,0x04,                      /* Max packet size = 1024 bytes */
    0x01,                           /* Servicing interval for data transfers */

    /* Super Speed Endpoint Companion Descriptor for Consumer EP */
    0x06,                           /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,       /* SS Endpoint Companion Descriptor Type */
    0x00,                           /* Max no. of packets in a Burst : 1 */
    0x04,                           /* Sixteen streams. */
    0x00,0x00,                      /* Bytes per interval : Not applicable. */

    /* Endpoint Descriptor */
    0x07,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint Descriptor Type */
    0x03,                           /* Endpoint address and description */
    CY_U3P_USB_EP_BULK,             /* Bulk End point Type */
    0x00,0x04,                      /* Max packet size = 1024 bytes */
    0x01,                           /* Servicing interval for data transfers */ 

    /* Super Speed Endpoint Companion Descriptor for Producer EP */
    0x06,                           /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,       /* SS Endpoint Companion Descriptor Type */
    0x00,                           /* Max no. of packets in a Burst : 1 */
    0x03,                           /* Eight streams. */
    0x00,0x00,                      /* Bytes per interval : Not applicable. */

    /* Endpoint Descriptor */
    0x07,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint Descriptor Type */
    0x83,                           /* Endpoint address and description */
    CY_U3P_USB_EP_BULK,             /* Bulk End point Type */
    0x00,0x04,                      /* Max packet size = 1024 bytes */
    0x01,                           /* Servicing interval for data transfers */

    /* Super Speed Endpoint Companion Descriptor for Consumer EP */
    0x06,                           /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,       /* SS Endpoint Companion Descriptor Type */
    0x00,                           /* Max no. of packets in a Burst : 1 */
    0x03,                           /* Eight streams. */
    0x00,0x00                       /* Bytes per interval : Not applicable. */
};

/* high speed descriptors */
uint8_t glUsbHSConfigDesc[32] =
{   
    0x09,0x02,0x20,0x0,
    0x01,0x01,0x00,0x80,
    0x32,0x09,0x04,0x00,
    0x00,0x02,0xFF,0xFF,
    0xFF,0x00,0x07,0x05,
    0x01,0x02,0x00,0x02,
    0x00,0x07,0x05,0x81,
    0x02,0x00,0x02,0x00
};

/* full speed descriptor */
uint8_t glUsbFSConfigDesc[32] =
{
    0x09,0x02,0x20,0x00,
    0x01,0x01,0x00,0x80,
    0x32,0x09,0x04,0x00,
    0x00,0x02,0xFF,0xFF,
    0xFF,0x00,0x07,0x05,
    0x01,0x02,0x40,0x00,
    0x00,0x07,0x05,0x81,
    0x02,0x40,0x00,0x00
};

/* String 0 descriptor. */
uint8_t glUsbString0Desc[4] =
{
    0x04, 0x03, 0x04, 0x09
};

CyU3PDmaChannel glLoopBackChannel[3];
uint8_t glFirmwareID[8] = {1,2,3,4,5,6,7,8};
uint8_t glDevStatus[2]  = {0,0};

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

    /* Initialize the caches. I-Cache is enabled, D-cache is disabled,
     * and DMA APIs are not handling caches. */
    status = CyU3PDeviceCacheControl (CyTrue, CyFalse, CyFalse);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Configure the IO matrix for the device.
     * Pport is not GPIF 32bit; and no GPIO is currently required.
     */
    io_cfg.isDQ32Bit = CyFalse;
    io_cfg.useUart   = CyTrue;
    io_cfg.useI2C    = CyFalse;
    io_cfg.useI2S    = CyFalse;
    io_cfg.useSpi    = CyFalse;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_UART_ONLY;
    io_cfg.gpioSimpleEn[0] = io_cfg.gpioSimpleEn[1] = 
        io_cfg.gpioComplexEn[0] = io_cfg.gpioComplexEn[1] = 0;
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

CyBool_t
MyUsbSetupCb (uint32_t setupdat0, uint32_t setupdat1)
{
    uint8_t cmdType = 0, UsbRqtClass = 0;
    uint16_t streamId;
    uint8_t epNum;
    uint8_t descType;

    CyBool_t isHandled = CyFalse;
    cmdType = (setupdat0 & 0xFF00) >> 8;
    UsbRqtClass = ((setupdat0 & 0xFF) & 0x60);

    CyU3PDebugPrint (6, "USB setup request: %x, %x\r\n", setupdat0, setupdat1);

    if(UsbRqtClass == 0x40)
    {
        isHandled = CyTrue;
        switch(cmdType)
        {
            case 0xA0:
                CyU3PUsbSendEP0Data (8, (uint8_t *)glFirmwareID);
                break;
            case 0xA1:
                CyU3PUsbGetEP0Data (8, (uint8_t *)glFirmwareID,NULL);
                break;

            case 0xA2:
                /* Request to do a device cold reboot. */
                warmReset = CyFalse;
                resetReq  = CyTrue;
                CyU3PUsbAckSetup ();
                break;
            case 0xA3:
                /* Request to do a device warm reboot. */
                warmReset = CyTrue;
                resetReq  = CyTrue;
                CyU3PUsbAckSetup ();
                break;

            case 0xF0:
                /* Change the active stream on both IN and OUT endpoints. */
                streamId = (uint16_t)(setupdat0 >> 16);
                epNum    = (setupdat1 & 0xFF);
                CyU3PUsbChangeMapping (epNum, epNum, CyTrue, streamId, epNum);
                CyU3PUsbChangeMapping ((epNum | 0x80), epNum, CyTrue, streamId, (epNum | 0x80));
                CyU3PUsbAckSetup ();
                break;

            case 0xF1:
                /* Stall the selected endpoint. */
                CyU3PUsbStall (((setupdat0 >> 16) & 0xFF), CyTrue, CyFalse);
                CyU3PUsbAckSetup ();
                break;

            default:
                isHandled = CyFalse;
                break;
        }
    }
    else if (UsbRqtClass == 0x00)
    {
        isHandled = CyTrue;
        switch (cmdType)
        {
            case CY_U3P_USB_SC_CLEAR_FEATURE:
                {
                    if (((setupdat0 & CY_U3P_UIB_SETUP_REQUEST_TYPE_MASK) == CY_U3P_UIB_FT_ENDPOINT) &&
                            (((setupdat0 & CY_U3P_UIB_SETUP_VALUE_MASK) >> 16) == 0))
                    {
                        epNum = (setupdat1 & CY_U3P_UIB_SETUP_INDEX_MASK);
                        if (CyU3PUsbStall (epNum, CyFalse, CyTrue) == CY_U3P_SUCCESS)
                        {
                            switch (epNum & 0x7F)
                            {
                                case 0x01:
                                    CyU3PDmaChannelReset (&glLoopBackChannel[0]);
                                    CyU3PDmaChannelSetXfer (&glLoopBackChannel[0], 0);
                                    break;
                                case 0x02:
                                    CyU3PDmaChannelReset (&glLoopBackChannel[1]);
                                    CyU3PDmaChannelSetXfer (&glLoopBackChannel[1], 0);
                                    break;
                                case 0x03:
                                    CyU3PDmaChannelReset (&glLoopBackChannel[2]);
                                    CyU3PDmaChannelSetXfer (&glLoopBackChannel[2], 0);
                                    break;
                                default:
                                    break;
                            }

                            isHandled = CyTrue;
                            CyU3PUsbAckSetup ();
                        }
                    }
                }
                break;

            case CY_U3P_USB_SC_GET_DESCRIPTOR:
                {
                    descType = (uint8_t)((setupdat0 & CY_U3P_UIB_SETUP_VALUE_MASK) >> 24);
                    switch (descType)
                    {
                        case CY_U3P_USB_DEVICE_DESCR:
                            {
                                if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
                                {
                                    CyU3PUsbSendEP0Data (18, glUsbDeviceDescSS);
                                }
                                else
                                {
                                    CyU3PUsbSendEP0Data (18, glUsbDeviceDescHS);
                                }
                            }
                            break;

                        case CY_U3P_USB_CONFIG_DESCR:
                            {
                                if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
                                {
                                    CyU3PUsbSendEP0Data (glUsbSSConfigDscr[2], glUsbSSConfigDscr);
                                }
                                else
                                {
                                    CyU3PUsbSendEP0Data (glUsbHSConfigDesc[2], glUsbHSConfigDesc);
                                }
                            }
                            break;

                        case CY_U3P_USB_DEVQUAL_DESCR:
                            {
                                if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
                                {
                                    isHandled = CyFalse;
                                }
                                else
                                {
                                    CyU3PUsbSendEP0Data (glUsbDeviceQualDesc[0], glUsbDeviceQualDesc);
                                }
                            }
                            break;

                        case CY_U3P_BOS_DESCR:
                            {
                                if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
                                {
                                    CyU3PUsbSendEP0Data (glUsbBOSDscr[2], glUsbBOSDscr);
                                }
                                else
                                {
                                    isHandled = CyFalse;
                                }
                            }
                            break;

                        case CY_U3P_USB_STRING_DESCR:
                            {
                                if ((setupdat0 & 0x00FF0000) == 0)
                                {
                                    CyU3PUsbSendEP0Data (4, glUsbString0Desc);
                                }
                                else
                                {
                                    isHandled = CyFalse;
                                }
                            }
                            break;
                    }
                }
                break;

            case CY_U3P_USB_SC_SET_CONFIGURATION:
                {
                    CyU3PUsbAckSetup ();
                }
                break;

            case CY_U3P_USB_SC_GET_STATUS:
                {
                    CyU3PUsbSendEP0Data (2, glDevStatus);
                }
                break;

            default:
                isHandled = CyFalse;
                break;
        }
    }

    if (!isHandled)
    {
        CyU3PUsbStall (0, CyTrue, CyFalse);
    }
    return CyTrue;
}

void
MyUsbEventCb (CyU3PUsbEventType_t evtype,
        uint16_t evdata)
{
}

void
MyDmaCb (CyU3PDmaChannel *ch, CyU3PDmaCbType_t type, CyU3PDmaCBInput_t *input)
{
    CyU3PDebugPrint (6, "DMA produce event - count: %x,status: %x\r\n",
            input->buffer_p.count, input->buffer_p.status);
}

void
CyFxDebugStart (void)
{
    CyU3PUartConfig_t uartConfig;

    /* Initialize the UART */
    CyU3PUartInit ();
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyFalse;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma = CyTrue;

    CyU3PUartSetConfig (&uartConfig, NULL);

    /* Set the dma for an inifinity transfer */
    CyU3PUartTxSetBlockXfer (0xFFFFFFFF);

    CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
}

void
CyFxUsbStart (void)
{
    CyU3PEpConfig_t epinfo;
    CyU3PDmaChannelConfig_t dmaConfig;
    CyU3PReturnStatus_t ret;
    uint8_t i;

    CyU3PUsbRegisterSetupCallback(MyUsbSetupCb, CyTrue);
    CyU3PUsbRegisterEventCallback(MyUsbEventCb);

    /* start usb stack */
    CyU3PUsbStart ();

    /* set Usb descriptors */
    CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, 0, glUsbDeviceDescSS);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, 0, glUsbDeviceDescHS);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, 0, glUsbDeviceQualDesc);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, 0, glUsbSSConfigDscr);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, 0, glUsbHSConfigDesc);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, 0, glUsbFSConfigDesc);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, glUsbString0Desc);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, 0, glUsbBOSDscr);

    /* Configure the EPs. */
    epinfo.enable = 1;
    epinfo.epType = CY_U3P_USB_EP_BULK;
    epinfo.pcktSize = 1024;
    epinfo.isoPkts = 0;
    epinfo.streams = 0;
    epinfo.burstLen = 0;

    /* Configure EP 1-IN */
    CyU3PSetEpConfig (0x81, &epinfo); 
    /* Configure EP 1-OUT */
    CyU3PSetEpConfig (0x01, &epinfo);

    epinfo.streams = 8;
    epinfo.burstLen = 1;

    /* Configure EP 2-IN */
    CyU3PSetEpConfig (0x82, &epinfo); 
    /* Configure EP 2-OUT */
    CyU3PSetEpConfig (0x02, &epinfo);

    /* Configure EP 3-IN */
    CyU3PSetEpConfig (0x83, &epinfo); 
    /* Configure EP 3-OUT */
    CyU3PSetEpConfig (0x03, &epinfo);

    /* Create the loopback channel. */
    dmaConfig.count = 4;
    dmaConfig.prodAvailCount = 0;
    dmaConfig.size = 1024;
    dmaConfig.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dmaConfig.prodHeader = 0;
    dmaConfig.prodFooter = 0;
    dmaConfig.consHeader = 0;
    dmaConfig.cb = 0;
    dmaConfig.notification = CY_U3P_DMA_CB_PROD_EVENT;

    for (i = 0; i < 3; i++)
    {
        dmaConfig.size = (i == 0) ? 4096 : 1024;
        dmaConfig.prodSckId = (CyU3PDmaSocketId_t) ((uint16_t)CY_U3P_UIB_SOCKET_PROD_1 + i);
        dmaConfig.consSckId = (CyU3PDmaSocketId_t) ((uint16_t)CY_U3P_UIB_SOCKET_CONS_1 + i);
        ret = CyU3PDmaChannelCreate (&glLoopBackChannel[i], CY_U3P_DMA_TYPE_AUTO, &dmaConfig);

        if (ret == CY_U3P_SUCCESS)
        {
            /* Set the channel for infinite data transfer. */
            ret = CyU3PDmaChannelSetXfer (&glLoopBackChannel[i], 0);
        }

        if (ret != CY_U3P_SUCCESS)
            break;
    }
    while (ret != CY_U3P_SUCCESS);

    /* Connect usb pins */
    CyU3PConnectState (CyTrue, CyTrue);
}

void
CyFxUsbStop (void)
{
    uint8_t i;
    CyU3PEpConfig_t epinfo;
    CyU3PReturnStatus_t ret;

    /* Destroy the DMA channels. */
    for (i = 0; i < 3; i++)
    {
        CyU3PDmaChannelDestroy (&glLoopBackChannel[i]);
    }

    /* Disable the EPs. */
    CyU3PMemSet ((uint8_t *)&epinfo, 0, sizeof (epinfo));
    /* Configure EP 1-IN */
    CyU3PSetEpConfig (0x81, &epinfo); 
    /* Configure EP 1-OUT */
    CyU3PSetEpConfig (0x01, &epinfo);

    /* Configure EP 2-IN */
    CyU3PSetEpConfig (0x82, &epinfo); 
    /* Configure EP 2-OUT */
    CyU3PSetEpConfig (0x02, &epinfo);

    /* Configure EP 3-IN */
    CyU3PSetEpConfig (0x83, &epinfo); 
    /* Configure EP 3-OUT */
    CyU3PSetEpConfig (0x03, &epinfo);

    /* Disconnect the USB lines. */
    CyU3PConnectState (CyFalse, CyFalse);

    /* Stop the device mode stack. */
    ret = CyU3PUsbStop ();
    if (ret != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PUsbStop failed with: %d.\r\n", ret);
    }
}

void
CyFxUsbHostStart ()
{
    /* TODO: Write host mode stack initialization. */
}

void
CyFxUsbHostStop ()
{
    /* TODO: Write host mode stack de-initialization. */
}

static void
CyFxOtgPeripheralChangeHandler (
        CyU3POtgPeripheralType_t otgType)
{
    /* Make sure that the VBUS is disabled. */
    CyU3PGpioSimpleSetValue (21, CyFalse);

    /* Stop any previously running stack. */
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

    switch (otgType)
    {
        case CY_U3P_OTG_TYPE_A_CABLE:
            /* Enable VBUS in this case. */
            CyU3PGpioSimpleSetValue (21, CyTrue);

            /* Deliberate fall-through in this case. */
        case CY_U3P_OTG_TYPE_ACA_A_CHG:
            /* Initialize the USB host mode of operation. */
            if (!CyU3PUsbHostIsStarted ())
            {
                CyFxUsbHostStart ();
            }
            break;

        case CY_U3P_OTG_TYPE_B_CABLE:
        case CY_U3P_OTG_TYPE_ACA_B_CHG:
            /* Initiate SRP with a repeat interval of 1s. */
            CyU3POtgSrpStart (1000);
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

            /* Start the device mode stack. */
            CyFxUsbStart ();
        }
        if ((CyU3POtgIsHostMode ()) && (!CyU3PUsbHostIsStarted ()))
        {
            if (CyU3PUsbIsStarted ())
            {
                /* Stop the previously started device stack. */
                CyFxUsbStop ();
            }

            /* Start the host mode stack if a remote device has
             * been detected. In this case, the stack is already
             * started with the device detection. */
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
            /* Turn on the VBUS. */
            break;

        case CY_U3P_OTG_VBUS_VALID_CHANGE:
            CyFxOtgVbusChangeHandler ((CyBool_t)input);
            break;

        default:
            /* do nothing */
            break;
    }
}

CyU3PReturnStatus_t
CyFxOtgStart ()
{
    CyU3POtgConfig_t cfg;
    CyU3PGpioClock_t clkCfg;
    CyU3PGpioSimpleConfig_t simpleCfg;
    CyU3PReturnStatus_t status;

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

    /* Configure GPIO 21 as output. */
    simpleCfg.outValue = CyFalse;
    simpleCfg.driveLowEn = CyTrue;
    simpleCfg.driveHighEn = CyTrue;
    simpleCfg.inputEn = CyFalse;
    simpleCfg.intrMode = CY_U3P_GPIO_NO_INTR;
    status = CyU3PGpioSetSimpleConfig (21, &simpleCfg);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    cfg.otgMode = CY_U3P_OTG_MODE_OTG;
    cfg.chargerMode = CY_U3P_OTG_CHARGER_DETECT_ACA_MODE;
    cfg.cb = CyFxOtgEventCb;
    status = CyU3POtgStart (&cfg);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Since VBATT is required for OTG operation enable it. */
    status = CyU3PUsbVBattEnable (CyTrue);

    return status;
}

void
CyFxStart (void)
{
    /* Initialize the debug interface. */
    CyFxDebugStart ();

#ifdef USE_DEVICE_ONLY_MODE
    /* Initialize device only stack. */
    CyFxUsbStart ();
#else
    /* Initialize the OTG module. */
    CyFxOtgStart ();
#endif
}

void
CyFxStop (void)
{
    CyU3PDebugDeInit ();

    if (CyU3PUsbIsStarted ())
    {
        CyFxUsbStop ();
    }

    if (CyU3PUsbHostIsStarted ())
    {
        CyFxUsbHostStop ();
    }

    CyU3PGpioSimpleSetValue (21, CyFalse);

    if (CyU3POtgIsStarted ())
    {
        CyU3POtgStop ();
    }
}

/*
 * Entry function for the thread1 
 */
void
Thread1_Entry (
        uint32_t input)
{
    /* Initialize the application. */
    CyFxStart ();

    for (;;)
    {
        CyU3PThreadSleep (100);
    }
}

/*
 * Entry function for the thread2 
 */
void
Thread2_Entry (
        uint32_t input)
{
    uint8_t count = 0;

    for (;;)
    {
        CyU3PDebugPrint (8, "Heartbeat: %x\r\n", count++);
        CyU3PThreadSleep (5000);
        if (resetReq)
        {
            resetReq = CyFalse;
#ifdef USE_DEVICE_ONLY_MODE
            CyU3PConnectState (0, 0);
            CyU3PUsbStop ();
#else
            CyFxStop ();
#endif
            CyU3PThreadSleep (1000);
            CyU3PDeviceReset (warmReset);
        }
    }
}

/* 
 * Application define function which creates the application threads.
 */
void
CyFxApplicationDefine (
        void)
{
    void *ptr;

    /* Allocate the memory for the threads and create threads */
    ptr = CyU3PMemAlloc (THREAD1_STACK);
    CyU3PThreadCreate (&thread1, "21_Thread1", Thread1_Entry, 0,
            ptr, THREAD1_STACK, THREAD1_PRIORITY, THREAD1_PRIORITY, \
            CYU3P_NO_TIME_SLICE, CYU3P_AUTO_START);

    ptr = CyU3PMemAlloc (THREAD2_STACK);
    CyU3PThreadCreate (&thread2, "22_Thread2", Thread2_Entry, 0,
            ptr, THREAD2_STACK, THREAD2_PRIORITY, THREAD2_PRIORITY, \
            CYU3P_NO_TIME_SLICE, CYU3P_DONT_START);
}

/* [] */

