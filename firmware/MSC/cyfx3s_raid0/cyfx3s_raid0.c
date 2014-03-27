/*
## Cypress FX3S Example Application Source File (cyfx3s_raid0.c)
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

/* This file illustrates the Mass Storage Class Driver example.
   The example makes use of the the internal device memory to provide the storage space.
   A minimum of 32KB is required for the device to be formatted as FAT on the windows host */

#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3error.h>
#include <cyu3usb.h>
#include <cyu3usbconst.h>
#include <cyu3uart.h>
#include <cyu3sib.h>
#include <cyu3gpio.h>
#include <cyu3utils.h>

#include "cyfx3s_raid0.h"

/* This example demonstrates the implementation of a RAID-0 mass storage disk using the FX3S APIs. */

static CyBool_t glLunState;                     /* Whether the LUN has valid storage. */
static uint8_t  glLunUnit;                      /* The unit number corresponding to this LUN. */
static CyBool_t glLunWriteable;                 /* Whether the LUN is writeable. */
static uint16_t glLunBlkSize;                   /* Sector size for each of the LUNs. */
static uint32_t glLunNumBlks;                   /* Capacity of the LUN in sectors. */
static uint8_t  glSensePtr = CY_FX_MSC_SENSE_DEVICE_RESET;

static CyBool_t         glDevConfigured = CyFalse;              /* Whether the device has been configured. */
static CyBool_t         glInPhaseError  = CyFalse;              /* Whether a phase error condition has been detected. */
static CyFxMscFuncState glMscState      = CY_FX_MSC_STATE_INACTIVE;

static CyU3PDmaMultiChannel glChHandleMscOut;                   /* DMA channel for OUT endpoint. */
static CyU3PDmaMultiChannel glChHandleMscIn;                    /* DMA channel for IN endpoint. */

static CyU3PThread glRaidAppThread;	                        /* MSC application thread structure */
static CyU3PEvent  glRaidAppEvent;                              /* MSC application Event group */

uint8_t *glMscCbwBuffer  = 0;                                   /* Scratch buffer used for CBW. */
uint8_t *glMscCswBuffer  = 0;                                   /* Scratch buffer used for CSW. */
uint8_t *glMscDataBuffer = 0;                                   /* Scratch buffer used for query commands. */

static uint8_t  glCmdDirection = 0;                             /* SCSI Command Direction */
static uint8_t  glMscCmdStatus = 0;                             /* MSC command status. */
static uint32_t glMscResidue   = 0;                             /* Residue length for CSW */

static volatile uint8_t  glRaidOpPending = 0;                   /* Shows the S port(s) having pending requests. */
static volatile CyBool_t glMscDriverBusy = CyFalse;             /* Flag showing whether the driver is busy. */

/* SIB socket used for write to SD cards. This array is used to allow indexing by port. */
static const uint8_t glRaidWriteSocket[2] = {
    CY_FX_MSC_S0_WRITE_SOCKET,
    CY_FX_MSC_S1_WRITE_SOCKET
};

/* SIB socket used for read from SD cards. This array is used to allow indexing by port. */
static const uint8_t glRaidReadSocket[2] = {
    CY_FX_MSC_S0_READ_SOCKET,
    CY_FX_MSC_S1_READ_SOCKET
};

/* Request Sense Lookup Table */
static const uint8_t glReqSenseCode[][3] =
{
    /* SK,  ASC,  ASCQ */
    {0x00, 0x00, 0x00},    /* senseOk                     0    */
    {0x0b, 0x08, 0x03},    /* senseCRCError               1    */
    {0x05, 0x24, 0x00},    /* senseInvalidFieldInCDB      2    */
    {0x02, 0x3a, 0x00},    /* senseNoMedia                3    */
    {0x03, 0x03, 0x00},    /* senseWriteFault             4    */
    {0x03, 0x11, 0x00},    /* senseReadError              5    */
    {0x03, 0x12, 0x00},    /* senseAddrNotFound           6    */
    {0x05, 0x20, 0x00},    /* senseInvalidOpcode          7    */
    {0x05, 0x21, 0x00},    /* senseInvalidLBA             8    */
    {0x05, 0x26, 0x00},    /* senseInvalidParameter       9    */
    {0x05, 0x53, 0x02},    /* senseCantEject              0xa  */
    {0x06, 0x28, 0x00},    /* senseMediaChanged           0xb  */
    {0x06, 0x29, 0x00},    /* senseDeviceReset            0xc  */
    {0x07, 0x27, 0x00}     /* senseWriteProtected         0xd  */
};

/* Standard Inquiry Data */
static const uint8_t CyFxMscScsiInquiryData[36] __attribute__ ((aligned (32))) = {
    0x00,       /* PQ and PDT */
    0x80,       /* Removable device. */
    0x00,       /* Version */
    0x02,       /* Response data format */
    0x1F,       /* Addnl Length */
    0x00,
    0x00,
    0x00,

    'C',        /* Vendor Id */
    'y',
    'p',
    'r',
    'e',
    's',
    's',
    0x00,

    'F',        /* Product Id */
    'X',
    '3',
    'S',
    0x20,
    'R',
    'A',
    'I',
    'D',
    0x20,
    'D',
    'E',
    'M',
    'O',
    0x20,
    0x00,

    '0',        /* Revision */
    '0',
    '0',
    '1'
};

/* Data used by Cypress proprietary mass storage driver to identify the device. */
uint8_t A0VendBuf[8] __attribute__ ((aligned (32))) =
{
    0x02, 0x0E, 0, 0, 0, 0, 0, 0
};

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

    /* Initialize the caches. Enable instruction cache and keep data cache disabled.
     * The data cache is useful only when there is a large amount of CPU based memory
     * accesses. When used in simple cases, it can decrease performance due to large
     * number of cache flushes and cleans and also it adds to the complexity of the
     * code. */
    status = CyU3PDeviceCacheControl (CyTrue, CyFalse, CyFalse);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Configure the IO matrix for the device.
     * Enable both S ports in 8 bit mode.
     * No debug UART functionality is available.
     */
    io_cfg.isDQ32Bit        = CyFalse;
    io_cfg.s0Mode           = CY_U3P_SPORT_8BIT;
    io_cfg.s1Mode           = CY_U3P_SPORT_8BIT;
    io_cfg.gpioSimpleEn[0]  = 0;
    io_cfg.gpioSimpleEn[1]  = 0x02102800;                       /* IOs 43, 45, 52 and 57 are chosen as GPIO. */
    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0;
    io_cfg.useUart          = CyFalse;
    io_cfg.useI2C           = CyFalse;
    io_cfg.useI2S           = CyFalse;
    io_cfg.useSpi           = CyFalse;
    io_cfg.lppMode          = CY_U3P_IO_MATRIX_LPP_NONE;
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

/* MSC application error handler */
void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus)
{

    /* Application failed with the error code apiRetStatus */

    /* Add custom debug or recovery actions here */

    /* Loop Indefinitely */
    for (;;)
    {
        /* Thread Sleep : 100 ms */
        CyU3PThreadSleep (100);
    }
}

static void
CyFxMscApplnResetDatapath (
        void)
{
    CyU3PUsbSetEpNak (CY_FX_MSC_EP_BULK_OUT, CyTrue);
    CyU3PUsbSetEpNak (CY_FX_MSC_EP_BULK_IN, CyTrue);
    CyU3PBusyWait (120);

    CyU3PDmaMultiChannelReset (&glChHandleMscOut);
    CyU3PDmaMultiChannelReset (&glChHandleMscIn);

    CyU3PUsbFlushEp (CY_FX_MSC_EP_BULK_OUT);
    CyU3PUsbStall (CY_FX_MSC_EP_BULK_OUT, CyFalse, CyTrue);

    CyU3PUsbFlushEp (CY_FX_MSC_EP_BULK_IN);
    CyU3PUsbStall (CY_FX_MSC_EP_BULK_IN, CyFalse, CyTrue);

    /* Request Sense Index */
    glSensePtr = CY_FX_MSC_SENSE_DEVICE_RESET;

    CyU3PUsbSetEpNak (CY_FX_MSC_EP_BULK_OUT, CyFalse);
    CyU3PUsbSetEpNak (CY_FX_MSC_EP_BULK_IN, CyFalse);

    glInPhaseError  = CyFalse;
    glMscDriverBusy = CyFalse;
}

/* Callback to handle the USB Setup Requests and Mass Storage Class requests */
CyBool_t
CyFxMscApplnUSBSetupCB (
        uint32_t setupdat0, /* SETUP Data 0 */
        uint32_t setupdat1  /* SETUP Data 1 */
    )
{
    CyBool_t isHandled = CyFalse;
    uint8_t  ep0Buf[2];

    uint8_t  bRequest, bReqType;
    uint8_t  bType, bTarget;
    uint16_t wValue, wIndex, wLength;

    /* Decode the fields from the setup request. */
    bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bType    = (bReqType & CY_U3P_USB_TYPE_MASK);
    bTarget  = (bReqType & CY_U3P_USB_TARGET_MASK);
    bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue   = ((setupdat0 & CY_U3P_USB_VALUE_MASK)   >> CY_U3P_USB_VALUE_POS);
    wIndex   = ((setupdat1 & CY_U3P_USB_INDEX_MASK)   >> CY_U3P_USB_INDEX_POS);
    wLength  = ((setupdat1 & CY_U3P_USB_LENGTH_MASK)  >> CY_U3P_USB_LENGTH_POS);

    /* Some setup requests have to be handled in a non-standard way for this device. */
    if (bType == CY_U3P_USB_STANDARD_RQT)
    {
        switch (bRequest)
        {
            case CY_U3P_USB_SC_CLEAR_FEATURE:
                /* Handle EP RESET requests based on interface state. */
                if ((bTarget == CY_U3P_USB_TARGET_ENDPT) && (
                            (wIndex == CY_FX_MSC_EP_BULK_IN) || (wIndex == CY_FX_MSC_EP_BULK_OUT)))
                {
                    /* In case of phase error, the stall condition can only be cleared by bus reset or
                       mass storage reset. */
                    if (!glInPhaseError)
                        CyU3PUsbStall (wIndex, CyFalse, CyTrue);
                    else
                    {
                        /* Even though we cannot clear the STALL condition, the sequence number should be cleared. */
                        if (CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED)
                            CyU3PUsbSetEpSeqNum (wIndex, 0);
                    }

                    CyU3PUsbAckSetup ();
                    isHandled = CyTrue;
                }
                /* No break */
            case CY_U3P_USB_SC_SET_FEATURE:
                /* Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
                 * requests here. It should be allowed to pass if the device is in configured
                 * state and failed otherwise. */
                if ((bTarget == CY_U3P_USB_TARGET_INTF) && (wValue == 0))
                {
                    if (glDevConfigured)
                        CyU3PUsbAckSetup ();
                    else
                        CyU3PUsbStall (0, CyTrue, CyFalse);

                    isHandled = CyTrue;
                }
                break;

                /* We need to handle all Interface specific requests here. */
            case CY_U3P_USB_SC_GET_STATUS:
                if (bTarget == CY_U3P_USB_TARGET_INTF)
                {
                    /* We support only interface 0. */
                    if (wIndex == 0)
                    {
                        ep0Buf[0] = 0;
                        ep0Buf[1] = 0;
                        CyU3PUsbSendEP0Data (0x02, ep0Buf);
                    }
                    else
                        CyU3PUsbStall (0, CyTrue, CyFalse);

                    isHandled = CyTrue;
                }
                break;

            case CY_U3P_USB_SC_SET_INTERFACE:
                isHandled = CyTrue;
                if ((wValue == 0) && (wIndex == 0))
                    CyU3PUsbAckSetup ();
                else
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                break;

            case CY_U3P_USB_SC_GET_INTERFACE:
                isHandled = CyTrue;
                if (wIndex == 0)
                {
                    ep0Buf[0] = 0;
                    CyU3PUsbSendEP0Data (0x01, ep0Buf);
                }
                else
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                break;

            default:
                break;
        }
    }

    /* MSC class specific request handling. */
    if (bType == CY_FX_MSC_USB_CLASS_REQ)
    {
        if ((bTarget == CY_U3P_USB_TARGET_INTF) &&
                (wIndex == CY_FX_USB_MSC_INTF) && (wValue == 0))
        {
            /* Get MAX LUN Request */
            if (bRequest == CY_FX_MSC_GET_MAX_LUN_REQ)
            {
                if (wLength == 1)
                {
                    /* Only one LUN is supported. */
                    isHandled = CyTrue;
                    ep0Buf[0] = 0;
                    CyU3PUsbSendEP0Data (0x01, ep0Buf);
                }
            }

            /* BOT Reset Request */
            if (bRequest == CY_FX_MSC_BOT_RESET_REQ)
            {
                isHandled      = CyTrue;
                glInPhaseError = CyFalse;

                if (wLength == 0)
                {
                    CyU3PUsbAckSetup ();
                    CyFxMscApplnResetDatapath ();

                    /* Inform the thread to start waiting for USB CBWs again. */
                    CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SETCONF_EVENT_FLAG, CYU3P_EVENT_OR);
                }
                else
                {
                    CyU3PUsbStall (0x00, CyTrue, CyFalse);
                }
            }
        }
    }

    /* This request is not needed when using the standard Mass Storage class driver on any OS.
       It is used by the Cypress proprietary Mass Storage Class Driver for device identification.
       */
    if (bType == CY_FX_MSC_USB_VENDOR_REQ)
    {
        if ((bReqType & 0x80) && (bRequest == 0xA0))
        {
            if (wLength < 8)
                CyU3PUsbSendEP0Data ((uint8_t)wLength, A0VendBuf);
            else
                CyU3PUsbSendEP0Data (8, A0VendBuf);
            isHandled = CyTrue;
        }
    }

    return isHandled;
}

/* Function to initiate sending of data to the USB host. */
CyU3PReturnStatus_t
CyFxMscApplnSendDataToHost (
    uint8_t  *data,
    uint32_t length)
{
    CyU3PDmaBuffer_t    dmaBuf;
    CyU3PReturnStatus_t status;

    /* Prepare the DMA Buffer */
    dmaBuf.buffer = data;
    dmaBuf.status = 0;
    dmaBuf.size   = (length + 15) & 0xFFF0;      /* Round up to a multiple of 16.  */
    dmaBuf.count  = length;

    status = CyU3PDmaMultiChannelSetupSendBuffer (&glChHandleMscIn, &dmaBuf, 0);
    return status;
}

/* Common function to check all CBW parameters against expected values. */
CyBool_t
CyFxMscApplnCheckCbwParams (
        CyBool_t  lunState,             /* Whether the LUN should be active or don't care (for INQUIRY/REQ_SENSE) */
        CyBool_t  isReadExpected,       /* Whether the op-code corresponds to a read (IN) data transfer. */
        CyBool_t  isDataCmd,            /* Whether this is a read or write command. Exact data length match is
                                           expected in data commands. */
        uint32_t  expLength,            /* Default data transfer length for this opcode. */
        uint32_t *actLength_p           /* Return parameter to pass the actual data transfer length. */
        )
{
    /* Verify that:
        1. Direction of data transfer matches the expected value.
        2. That the transfer length matches the expected value for read/write commands.
        3. That a non-zero transfer length is provided for commands that want to return data.
        4. That zero transfer length is provided for commands that have no data.
     */
    if ((isReadExpected != (glCmdDirection == 0x80)) || ((isDataCmd) && (expLength != glMscResidue)) ||
            ((expLength != 0) && (glMscResidue == 0)) || ((expLength == 0) && (glMscResidue != 0)))
    {
        glMscCmdStatus = 1;
        glSensePtr     = CY_FX_MSC_SENSE_INVALID_PARAMETER;
        CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
        return CyFalse;
    }

    /* Verify that the LUN is ready for commands that report the LUN state or do data transfers. */
    if (lunState)
    {
        if (!glLunState)
        {
            glMscCmdStatus = 1;
            glSensePtr     = CY_FX_MSC_SENSE_NO_MEDIA;
            CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
            return CyFalse;
        }
        else
        {
            glSensePtr = CY_FX_MSC_SENSE_OK;
        }
    }

    /* Calculate the size of data to be transferred in the case of non-data commands. */
    if (!isDataCmd)
    {
        if (expLength > glMscResidue)
        {
            *actLength_p = glMscResidue;
            glMscResidue = 0;
        }
        else
        {
            *actLength_p  = expLength;
            glMscResidue -= expLength;
        }
    }

    return CyTrue;
}

/* Parse the received CBW and handle the command. */
void
CyFxMscApplnParseCbw (
    void)
{
    CyU3PReturnStatus_t status;
    uint8_t cmd = glMscCbwBuffer[15];
    uint8_t lun = glMscCbwBuffer[13] & 0x0F;
    uint32_t startAddr, dataLength, sibAddr;
    uint16_t numBlks, sibBlks;
    uint8_t  port;

    /* Direction of data transfer. */
    glCmdDirection = glMscCbwBuffer[12] & 0x80;

    /* Expected transfer length. */
    glMscResidue   = *((uint32_t *)(glMscCbwBuffer + 8));
    glMscCmdStatus = 0;

    /* Invalid LUN. */
    if (lun >= 1)
    {
        glMscCmdStatus  = 1;
        CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
        return;
    }

    /* Execute the command */
    switch (cmd)
    {
        case CY_FX_MSC_SCSI_INQUIRY:
            {
                if (!CyFxMscApplnCheckCbwParams (CyFalse, CyTrue, CyFalse, 36, &dataLength))
                    break;

                glMscState = CY_FX_MSC_STATE_DATA;
                status = CyFxMscApplnSendDataToHost ((uint8_t *)CyFxMscScsiInquiryData, dataLength);
                if (status != CY_U3P_SUCCESS)
                {
                    glMscCmdStatus = 1;
                    glSensePtr     = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            }
            break;

        case CY_FX_MSC_SCSI_READ_CAPACITY:
        case CY_FX_MSC_SCSI_READ_FORMAT_CAPACITY:
            {
                uint8_t i = 0;

                if (!CyFxMscApplnCheckCbwParams (CyTrue, CyTrue, CyFalse,
                            ((cmd == CY_FX_MSC_SCSI_READ_CAPACITY) ? 8 : 12), &dataLength))
                    break;

                CyU3PMemSet (glMscDataBuffer, 0, 12);
                if (cmd == CY_FX_MSC_SCSI_READ_FORMAT_CAPACITY)
                {
                    glMscDataBuffer[3] = 0x08;
                    glMscDataBuffer[8] = 0x02;
                    i = 4;
                }

                glMscDataBuffer[i++] = (uint8_t)(glLunNumBlks >> 24);
                glMscDataBuffer[i++] = (uint8_t)(glLunNumBlks >> 16);
                glMscDataBuffer[i++] = (uint8_t)(glLunNumBlks >> 8);
                glMscDataBuffer[i++] = (uint8_t)(glLunNumBlks);

                i += 2;
                glMscDataBuffer[i++] = (uint8_t)(glLunBlkSize >> 8);
                glMscDataBuffer[i++] = (uint8_t)(glLunBlkSize);

                glMscState = CY_FX_MSC_STATE_DATA;
                status = CyFxMscApplnSendDataToHost (glMscDataBuffer, dataLength);
                if (status != CY_U3P_SUCCESS)
                {
                    glMscCmdStatus = 1;
                    glSensePtr     = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            }
            break;

        case CY_FX_MSC_SCSI_REQUEST_SENSE:
            {
                if (!CyFxMscApplnCheckCbwParams (CyFalse, CyTrue, CyFalse, 18, &dataLength))
                    break;

                CyU3PMemSet (glMscDataBuffer, 0, 18);
                glMscDataBuffer[0]  = 0x70;
                glMscDataBuffer[2]  = glReqSenseCode[glSensePtr][0];
                glMscDataBuffer[7]  = 0x0A;
                glMscDataBuffer[12] = glReqSenseCode[glSensePtr][1];
                glMscDataBuffer[13] = glReqSenseCode[glSensePtr][2];

                glMscState = CY_FX_MSC_STATE_DATA;

                status = CyFxMscApplnSendDataToHost (glMscDataBuffer, dataLength);
                if (status != CY_U3P_SUCCESS)
                {
                    glMscCmdStatus = 1;
                    glSensePtr     = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            }
            break;

            /* No real support for Verify, Start/stop unit and Prevent/Allow Medium Removal commands. */
        case CY_FX_MSC_SCSI_VERIFY_10:
        case CY_FX_MSC_SCSI_START_STOP_UNIT:
        case CY_FX_MSC_SCSI_PREVENT_ALLOW_MEDIUM:
        case CY_FX_MSC_SCSI_TEST_UNIT_READY:
            {
                if (!CyFxMscApplnCheckCbwParams (CyTrue, CyFalse, CyFalse, 0, &dataLength))
                    break;

                glMscState = CY_FX_MSC_STATE_STATUS;
            }
            break;

        case CY_FX_MSC_SCSI_MODE_SENSE_6:
            {
                if (!CyFxMscApplnCheckCbwParams (CyTrue, CyTrue, CyFalse, 4, &dataLength))
                    break;

                glMscDataBuffer[0] = 0x03;
                glMscDataBuffer[1] = 0x00;
                glMscDataBuffer[2] = (glLunWriteable) ? 0x00 : 0x80;
                glMscDataBuffer[3] = 0x00;

                glMscState = CY_FX_MSC_STATE_DATA;
                status = CyFxMscApplnSendDataToHost (glMscDataBuffer, dataLength);
                if (status != CY_U3P_SUCCESS)
                {
                    glMscCmdStatus = 1;
                    glSensePtr     = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            }
            break;

        case CY_FX_MSC_SCSI_READ_10:
            {
                startAddr = CY_U3P_MAKEDWORD (glMscCbwBuffer[17], glMscCbwBuffer[18], glMscCbwBuffer[19],
                        glMscCbwBuffer[20]);
                numBlks   = CY_U3P_MAKEWORD (glMscCbwBuffer[22], glMscCbwBuffer[23]);

                if (!CyFxMscApplnCheckCbwParams (CyTrue, CyTrue, CyTrue, (numBlks * glLunBlkSize), &dataLength))
                    break;

                /* Zero block read. Go straight to status phase. */
                if (numBlks == 0)
                {
                    glMscCmdStatus = 0;
                    glSensePtr     = CY_FX_MSC_SENSE_OK;
                    glMscState     = CY_FX_MSC_STATE_STATUS;
                    break;
                }

                /* Figure out the port on which the transfer is to be started based on address. */
                glMscState = CY_FX_MSC_STATE_DATA;
                status = CyU3PDmaMultiChannelSetXfer (&glChHandleMscIn, (numBlks * CY_FX_RAID_BLOCK_SIZE),
                        (startAddr & 0x01));

                if (status == CY_U3P_SUCCESS)
                {
                    if (numBlks == 1)
                    {
                        /* Only one SD card is to be touched in case of a single block read. */
                        port    = startAddr & 0x01;             /* LSB of address only specifies port. */
                        sibAddr = (startAddr >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                        sibBlks = CY_FX_RAID_SECTOR_MULTIPLIER;

                        glRaidOpPending = (1 << port);

                        status = CyU3PSibReadWriteRequest (CY_FX_SIB_READ, port,
                                glLunUnit, sibBlks, sibAddr, glRaidReadSocket[port]);
                    }
                    else
                    {
                        /* Read covers both storage devices in this case. */
                        glRaidOpPending = 3;

                        port = 0;
                        if (startAddr & 0x01)
                        {
                            sibAddr = ((startAddr >> 1) + 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                            sibBlks = (numBlks >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                        }
                        else
                        {
                            sibAddr = (startAddr >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                            sibBlks = ((numBlks + 1) >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                        }

                        status = CyU3PSibReadWriteRequest (CY_FX_SIB_READ, 0, glLunUnit, sibBlks, sibAddr,
                                glRaidReadSocket[0]);
                        if (status != CY_U3P_SUCCESS)
                            goto SkipS1Read;

                        port    = 1;
                        sibAddr = (startAddr >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                        if (startAddr & 0x01)
                        {
                            sibBlks = ((numBlks + 1) >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                        }
                        else
                        {
                            sibBlks = (numBlks >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                        }

                        status = CyU3PSibReadWriteRequest (CY_FX_SIB_READ, 1, glLunUnit, sibBlks, sibAddr,
                                glRaidReadSocket[1]);
                        if (status != CY_U3P_SUCCESS)
                            CyU3PSibAbortRequest (0);
                    }

SkipS1Read:
                    if (status != CY_U3P_SUCCESS)
                    {
                        /* Abort the DMA Channel */
                        glRaidOpPending = 0;
                        CyU3PDmaMultiChannelReset (&glChHandleMscIn);
                    }
                }

                if (status != CY_U3P_SUCCESS)
                {
                    glMscCmdStatus = 1;
                    glSensePtr     = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            }
            break;

        case CY_FX_MSC_SCSI_WRITE_10:
            {
                startAddr = CY_U3P_MAKEDWORD (glMscCbwBuffer[17], glMscCbwBuffer[18], glMscCbwBuffer[19],
                        glMscCbwBuffer[20]);
                numBlks   = CY_U3P_MAKEWORD (glMscCbwBuffer[22], glMscCbwBuffer[23]);

                if (!CyFxMscApplnCheckCbwParams (CyTrue, CyFalse, CyTrue, (numBlks * glLunBlkSize), &dataLength))
                    break;

                if (!glLunWriteable)
                {
                    glMscCmdStatus = 1;
                    glSensePtr     = CY_FX_MSC_SENSE_WRITE_PROTECT;
                    CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                /* Zero block write. Go straight to status phase. */
                if (numBlks == 0)
                {
                    glMscCmdStatus = 0;
                    glSensePtr     = CY_FX_MSC_SENSE_OK;
                    glMscState     = CY_FX_MSC_STATE_STATUS;
                    break;
                }

                /* Figure out the port on which the transfer is to be started based on address. */
                glMscState = CY_FX_MSC_STATE_DATA;
                status = CyU3PDmaMultiChannelSetXfer (&glChHandleMscOut, (numBlks * CY_FX_RAID_BLOCK_SIZE),
                        (startAddr & 0x01));

                if (status == CY_U3P_SUCCESS)
                {
                    if (numBlks == 1)
                    {
                        /* Only one SD card is to be touched in case of a single block write. */
                        port    = startAddr & 0x01;             /* LSB of address only specifies port. */
                        sibAddr = (startAddr >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                        sibBlks = CY_FX_RAID_SECTOR_MULTIPLIER;

                        glRaidOpPending = (1 << port);

                        status = CyU3PSibReadWriteRequest (CY_FX_SIB_WRITE, port,
                                glLunUnit, sibBlks, sibAddr, glRaidWriteSocket[port]);
                    }
                    else
                    {
                        /* Write covers both storage devices in this case. */
                        glRaidOpPending = 3;

                        port = 0;
                        if (startAddr & 0x01)
                        {
                            sibAddr = ((startAddr >> 1) + 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                            sibBlks = (numBlks >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                        }
                        else
                        {
                            sibAddr = (startAddr >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                            sibBlks = ((numBlks + 1) >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                        }

                        status = CyU3PSibReadWriteRequest (CY_FX_SIB_WRITE, 0, glLunUnit, sibBlks, sibAddr,
                                glRaidWriteSocket[0]);
                        if (status != CY_U3P_SUCCESS)
                            goto SkipS1Write;

                        port    = 1;
                        sibAddr = (startAddr >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                        if (startAddr & 0x01)
                        {
                            sibBlks = ((numBlks + 1) >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                        }
                        else
                        {
                            sibBlks = (numBlks >> 1) * CY_FX_RAID_SECTOR_MULTIPLIER;
                        }

                        status = CyU3PSibReadWriteRequest (CY_FX_SIB_WRITE, 1, glLunUnit, sibBlks, sibAddr,
                                glRaidWriteSocket[1]);
                        if (status != CY_U3P_SUCCESS)
                            CyU3PSibAbortRequest (0);
                    }

SkipS1Write:
                    if (status != CY_U3P_SUCCESS)
                    {
                        /* Abort the DMA Channel */
                        glRaidOpPending = 0;
                        CyU3PDmaMultiChannelReset (&glChHandleMscOut);
                    }
                }

                if (status != CY_U3P_SUCCESS)
                {
                    glMscCmdStatus = 1;
                    glSensePtr     = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            }
            break;

        default:
            {
                /* Unsupported command. */
                glMscCmdStatus = 1;
                glSensePtr     = CY_FX_MSC_SENSE_INVALID_OP_CODE;
                CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
            }
            break;
    }
}

/* This is the Callback function to handle the USB Events */
void
CyFxMscApplnUSBEventCB (
    CyU3PUsbEventType_t evtype, /* Event type */
    uint16_t            evdata  /* Event data */
    )
{
    uint16_t epPktSize = 64;    /* Full speed setting by default. */

    switch (evtype)
    {
        case CY_U3P_USB_EVENT_SUSPEND:
            CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_USB_SUSP_EVENT_FLAG, CYU3P_EVENT_OR);
            break;

        case CY_U3P_USB_EVENT_DISCONNECT:
        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_CONNECT:
            {
                CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_RESET_EVENT_FLAG, CYU3P_EVENT_OR);

                CyFxMscApplnResetDatapath ();
                glDevConfigured = CyFalse;
            }
            break;

        case CY_U3P_USB_EVENT_SETCONF:
            {
                if (evdata == 0)
                {
                    glDevConfigured = CyFalse;
                    break;
                }

                /* Check if Set Config event is handled */
                if (glDevConfigured == CyFalse )
                {
                    glDevConfigured = CyTrue;

                    /* The max. packet size depends on connection speed. */
                    switch (CyU3PUsbGetSpeed ())
                    {
                        case CY_U3P_FULL_SPEED:
                            epPktSize = 64;
                            break;
                        case CY_U3P_HIGH_SPEED:
                            epPktSize = 512;
                            break;
                        case CY_U3P_SUPER_SPEED:
                        default:
                            epPktSize = 1024;
                            break;
                    }

                    /* Update the packet size for the endpoints based on connection speed. */
                    CyU3PSetEpPacketSize (CY_FX_MSC_EP_BULK_OUT, epPktSize);
                    CyU3PSetEpPacketSize (CY_FX_MSC_EP_BULK_IN, epPktSize);

                    /* Clear stall on the endpoints and reset the DMA channels. */
                    CyU3PUsbStall (CY_FX_MSC_EP_BULK_OUT, CyFalse, CyTrue);
                    CyU3PUsbStall (CY_FX_MSC_EP_BULK_IN, CyFalse, CyTrue);

                    CyU3PDmaMultiChannelReset (&glChHandleMscIn);
                    CyU3PDmaMultiChannelReset (&glChHandleMscOut);

                    /* Notify the thread to start waiting for a mass storage CBW. */
                    CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SETCONF_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            }
            break;

        default:
            break;
    }
}

/* Callback function to handle LPM requests from the USB 3.0 host. This function is invoked by the API
   whenever a state change from U0 -> U1 or U0 -> U2 happens. If we return CyTrue from this function, the
   FX3S device is retained in the low power state. If we return CyFalse, the FX3S device immediately tries
   to trigger an exit back to U0.
 */
CyBool_t
CyFxMscApplnLPMRqtCB (
        CyU3PUsbLinkPowerMode link_mode)
{
    /* If we are in the middle of processing a mass storage command, keep reverting to U0 state. */
    return !glMscDriverBusy;
}

void
CyFxMscApplnQueryDevStatus (
        void)
{
    CyU3PSibDevInfo_t devInfo;
    CyU3PSibLunInfo_t unitInfo;
    CyU3PReturnStatus_t status;

    uint32_t unitCapacity = 0;
    uint8_t  unitToUse = 100;
    uint8_t  j;

    /* Default init for all LUN state variables. */
    glLunState     = CyFalse;
    glLunUnit      = 100;         /* Invalid value. */
    glLunWriteable = CyTrue;
    glLunBlkSize   = 0;
    glLunNumBlks   = 0;

    status = CyU3PSibQueryDevice (0, &devInfo);
    if ((status != CY_U3P_SUCCESS) ||
            ((devInfo.cardType != CY_U3P_SIB_DEV_MMC) && (devInfo.cardType != CY_U3P_SIB_DEV_SD)))
    {
        /* No usable device on S0 port. We cannot enable the LUN. */
        return;
    }

    /* If either of the devices is write protected, treat the whole LUN as read-only. */
    if (!devInfo.writeable)
        glLunWriteable = CyFalse;

    /* Find the first data LUN index. */
    for (j = 0; j < devInfo.numUnits; j++)
    {
        status = CyU3PSibQueryUnit (0, j, &unitInfo);
        if ((status == CY_U3P_SUCCESS) && (unitInfo.location == CY_U3P_SIB_LOCATION_USER))
        {
            unitToUse    = j;
            unitCapacity = unitInfo.numBlocks;
        }
    }

    if (unitCapacity != 0)
    {
        /* Verify that the second device exists and has a valid unit at the selected index. */
        status = CyU3PSibQueryDevice (1, &devInfo);
        if ((status != CY_U3P_SUCCESS) ||
                ((devInfo.cardType != CY_U3P_SIB_DEV_MMC) && (devInfo.cardType != CY_U3P_SIB_DEV_SD)))
        {
            /* No usable device on S1 port. We cannot enable the LUN. */
            return;
        }

        /* If either of the devices is write protected, treat the whole LUN as read-only. */
        if (!devInfo.writeable)
            glLunWriteable = CyFalse;

        status = CyU3PSibQueryUnit (1, unitToUse, &unitInfo);
        if ((status == CY_U3P_SUCCESS) && (unitInfo.location == CY_U3P_SIB_LOCATION_USER))
        {
            /* Valid partition found on both devices. Store values into global variables. */
            unitCapacity = CY_U3P_MIN (unitCapacity, unitInfo.numBlocks);

            glLunState   = CyTrue;
            glLunUnit    = unitToUse;
            glLunBlkSize = CY_FX_RAID_BLOCK_SIZE;
            glLunNumBlks = (unitCapacity * 2 / CY_FX_RAID_SECTOR_MULTIPLIER) - 1;
        }
    }

    /* Set write commit size for both devices to 8 MB. */
    CyU3PSibSetWriteCommitSize (0, 16384);
    CyU3PSibSetWriteCommitSize (1, 16384);
}

void
CyFxMscApplnSibCB (
        uint8_t             portId,
        CyU3PSibEventType   evt,
        CyU3PReturnStatus_t status)
{
    CyU3PDmaSocketConfig_t sockConf;

    if ((evt == CY_U3P_SIB_EVENT_DATA_ERROR) || (evt == CY_U3P_SIB_EVENT_ABORT))
    {
        /* Treat DATA_ERROR and ABORT events as equivalent to XFER_CPLT event with an error status. */
        status = CY_U3P_ERROR_XFER_CANCELLED;
        evt    = CY_U3P_SIB_EVENT_XFER_CPLT;
    }

    if (evt == CY_U3P_SIB_EVENT_XFER_CPLT)
    {
        glRaidOpPending &= ~(1 << portId);
        if (status != CY_U3P_SUCCESS)
        {
            /* Abort the request to other device, when an error is seen on one device. */
            if (glRaidOpPending)
            {
                CyU3PSibAbortRequest (!portId);
                glRaidOpPending = 0;
            }

            glMscCmdStatus = 1;
            glSensePtr     = CY_FX_MSC_SENSE_CRC_ERROR;

            /* Transfer has failed. Reset the DMA channel. */
            if (glCmdDirection)
            {
                CyU3PDmaSocketGetConfig ((uint16_t)(CY_U3P_UIB_SOCKET_CONS_0 | CY_FX_MSC_EP_BULK_IN_SOCKET),
                        &sockConf);
                glMscResidue -= sockConf.xferCount;
                CyU3PDmaMultiChannelReset (&glChHandleMscIn);
            }
            else
            {
                CyU3PDmaSocketGetConfig ((uint16_t)(CY_U3P_UIB_SOCKET_PROD_0 + CY_FX_MSC_EP_BULK_OUT_SOCKET),
                        &sockConf);
                glMscResidue -= sockConf.xferCount;
                CyU3PDmaMultiChannelReset (&glChHandleMscOut);
            }
        }
        else
        {
            if (glRaidOpPending == 0)
            {
                glMscCmdStatus = 0;
                glMscResidue   = 0;
                glSensePtr     = CY_FX_MSC_SENSE_OK;
            }
        }

        if (glRaidOpPending == 0)
            CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
    }

    /* If a remove event is seen, check whether the LUN can still be active. */
    if ((evt == CY_U3P_SIB_EVENT_INSERT) || (evt == CY_U3P_SIB_EVENT_REMOVE))
    {
        CyFxMscApplnQueryDevStatus ();
    }
}

void
CyFxMscApplnSibInit (
        void)
{
    CyU3PGpioClock_t     gpioClock;
    CyU3PReturnStatus_t  status;
    CyU3PSibIntfParams_t intfParams;

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
        CyFxAppErrorHandler (status);
    }

    intfParams.resetGpio       = 0xFF;                          /* No GPIO control on SD/MMC power. */
    intfParams.rstActHigh      = CyTrue;                        /* Don't care as no GPIO is selected. */
    intfParams.cardDetType     = CY_U3P_SIB_DETECT_NONE;        /* Hotplug is not supported. */
    intfParams.writeProtEnable = CyTrue;                        /* Write protect handling enabled. */
    intfParams.lowVoltage      = CyTrue;                        /* Low voltage operation enabled. */
    intfParams.voltageSwGpio   = 45;                            /* Use GPIO_45 for voltage switch on S0 port. */
    intfParams.lvGpioState     = CyFalse;                       /* Driving GPIO low selects 1.8 V on SxVDDQ. */
    intfParams.useDdr          = CyTrue;                        /* DDR clocking enabled. */
    intfParams.maxFreq         = CY_U3P_SIB_FREQ_104MHZ;        /* No S port clock limitation. */
    intfParams.cardInitDelay   = 0;                             /* No delay required between SD card insertion
                                                                   before initialization. */

    status = CyU3PSibSetIntfParams (0, &intfParams);
    if (status == CY_U3P_SUCCESS)
    {
        intfParams.voltageSwGpio = 57;                          /* Use GPIO_57 for voltage switch on S1 port. */
        status = CyU3PSibSetIntfParams (1, &intfParams);
    }

    if (status != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (status);
    }

    status = CyU3PSibStart ();
    if (status != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (status);
    }

    /* Register a callback for SIB events. */
    CyU3PSibRegisterCbk (CyFxMscApplnSibCB);

    /* Query the current device status. */
    CyFxMscApplnQueryDevStatus ();
}

/* Configure the endpoints required for the mass storage device operation. */
void
CyFxMscApplnConfigEndpoints (
        void)
{
    CyU3PEpConfig_t epConf = {0};
    CyU3PReturnStatus_t status;

    epConf.enable   = CyTrue;
    epConf.epType   = CY_U3P_USB_EP_BULK;
    epConf.streams  = 0;
    epConf.pcktSize = 1024;                     /* Configuring for super speed by default. */
    epConf.burstLen = CY_FX_RAID_EP_BURST_SIZE;
    epConf.isoPkts  = 0;

    status = CyU3PSetEpConfig (CY_FX_MSC_EP_BULK_OUT, &epConf);
    if (status != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (status);
    }

    status = CyU3PSetEpConfig (CY_FX_MSC_EP_BULK_IN, &epConf);
    if (status != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (status);
    }
}

void
CyFxMscApplnDmaCb (
        CyU3PDmaMultiChannel *handle,
        CyU3PDmaCbType_t      evtype,
        CyU3PDmaCBInput_t    *input)
{
    CyU3PDmaBuffer_t dmaBuf = input->buffer_p;

    switch (evtype)
    {
        case CY_U3P_DMA_CB_RECV_CPLT:
            if ((dmaBuf.count != 31) || (dmaBuf.buffer[0] != 'U') || (dmaBuf.buffer[1] != 'S') ||
                    (dmaBuf.buffer[2] != 'B') || (dmaBuf.buffer[3] != 'C'))
                glInPhaseError = CyTrue;
            else
                glInPhaseError = CyFalse;
            CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_CBW_EVENT_FLAG, CYU3P_EVENT_OR);
            break;

        case CY_U3P_DMA_CB_SEND_CPLT:
            CyU3PEventSet (&glRaidAppEvent, CY_FX_MSC_DATASENT_EVENT_FLAG, CYU3P_EVENT_OR);
            break;

        default:
            break;
    }
}

/* Create the DMA channels required for the mass storage device operation. */
void
CyFxMscApplnDmaInit (
        void)
{
    CyU3PDmaMultiChannelConfig_t dmaConfig;
    CyU3PReturnStatus_t status;

    glMscCbwBuffer  = (uint8_t *)CyU3PDmaBufferAlloc (1024);
    glMscCswBuffer  = (uint8_t *)CyU3PDmaBufferAlloc (1024);
    glMscDataBuffer = (uint8_t *)CyU3PDmaBufferAlloc (1024);
    if ((glMscCbwBuffer == 0) || (glMscCswBuffer == 0) || (glMscDataBuffer == 0))
    {
        CyFxAppErrorHandler (CY_U3P_ERROR_MEMORY_ERROR);
    }

    /* Both DMA channels are created with SuperSpeed parameters. The CyU3PSetEpPacketSize () API is used
       to reconfigure the endpoints to work with DMA channels with large buffers. */
    CyU3PMemSet ((uint8_t *)&dmaConfig, 0, sizeof (dmaConfig));
    dmaConfig.size           = CY_FX_RAID_BLOCK_SIZE;
    dmaConfig.count          = CY_FX_RAID_DMA_BUF_COUNT;
    dmaConfig.validSckCount  = 2;
    dmaConfig.prodSckId[0]   = (CyU3PDmaSocketId_t)(CY_U3P_UIB_SOCKET_PROD_0 | CY_FX_MSC_EP_BULK_OUT_SOCKET);
    dmaConfig.consSckId[0]   = (CyU3PDmaSocketId_t)(CY_U3P_SIB_SOCKET_0 | CY_FX_MSC_S0_WRITE_SOCKET);
    dmaConfig.consSckId[1]   = (CyU3PDmaSocketId_t)(CY_U3P_SIB_SOCKET_0 | CY_FX_MSC_S1_WRITE_SOCKET);
    dmaConfig.prodAvailCount = 0;
    dmaConfig.prodHeader     = 0;
    dmaConfig.prodFooter     = 0;
    dmaConfig.consHeader     = 0;
    dmaConfig.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    dmaConfig.notification   = CY_U3P_DMA_CB_RECV_CPLT;
    dmaConfig.cb             = CyFxMscApplnDmaCb;

    status = CyU3PDmaMultiChannelCreate (&glChHandleMscOut, CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY, &dmaConfig);
    if (status != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (status);
    }

    dmaConfig.prodSckId[0]  = (CyU3PDmaSocketId_t)(CY_U3P_SIB_SOCKET_0 | CY_FX_MSC_S0_READ_SOCKET);
    dmaConfig.prodSckId[1]  = (CyU3PDmaSocketId_t)(CY_U3P_SIB_SOCKET_0 | CY_FX_MSC_S1_READ_SOCKET);
    dmaConfig.consSckId[0]  = (CyU3PDmaSocketId_t)(CY_U3P_UIB_SOCKET_CONS_0 | CY_FX_MSC_EP_BULK_IN_SOCKET);
    dmaConfig.consSckId[1]  = (CyU3PDmaSocketId_t)0;
    dmaConfig.notification  = CY_U3P_DMA_CB_SEND_CPLT;
    dmaConfig.cb            = CyFxMscApplnDmaCb;

    status = CyU3PDmaMultiChannelCreate (&glChHandleMscIn, CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE, &dmaConfig);
    if (status != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (status);
    }
}

static void
CyFxMscAppInitVars (
		void)
{
    glLunState     = CyFalse;
    glLunUnit      = 100;
    glLunWriteable = CyTrue;
    glLunBlkSize   = 0;
    glLunNumBlks   = 0;
    glSensePtr     = CY_FX_MSC_SENSE_DEVICE_RESET;
}

void
CyFxMscApplnInit (
        void)
{
    CyU3PReturnStatus_t apiRetStatus;
    uint32_t txApiRetStatus;

    CyFxMscAppInitVars ();

    /* Initialize SIB and get device information. */
    CyFxMscApplnSibInit ();

    /* Create FX MSC events */
    txApiRetStatus = CyU3PEventCreate (&glRaidAppEvent);
    if (txApiRetStatus != 0)
    {
        /* Error Handling */
        CyFxAppErrorHandler(txApiRetStatus);
    }

    /* Start the USB functionality */
    apiRetStatus = CyU3PUsbStart ();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Setup the Callback to Handle the USB Setup Requests */
    CyU3PUsbRegisterSetupCallback (CyFxMscApplnUSBSetupCB, CyTrue);

    /* Setup the Callback to Handle the USB Events */
    CyU3PUsbRegisterEventCallback (CyFxMscApplnUSBEventCB);

    /* Register a callback to handle LPM requests from the USB 3.0 host. */
    CyU3PUsbRegisterLPMRequestCallback (CyFxMscApplnLPMRqtCB);

    /* Register the USB descriptors. */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_HS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB20DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB30DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, NULL, (uint8_t *)CyFxUSBDeviceQualDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBHSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBFSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBSSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, NULL, (uint8_t *)CyFxUSBBOSDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 3, (uint8_t *)CyFxUSBSerialNumberDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Configure the USB endpoints as required. We configure for SuperSpeed and update at runtime as required. */
    CyFxMscApplnConfigEndpoints ();

    /* Create the required DMA channels. */
    CyFxMscApplnDmaInit ();

    /* Enable USB connection to host. SuperSpeed is supported. */
    apiRetStatus = CyU3PConnectState (CyTrue, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }
}

void
CyFxMscApplnSendCsw (
        void)
{
    CyU3PDmaBuffer_t dmaBuf;
    CyU3PReturnStatus_t status;

    /* Compute the CSW information. */
    glMscCswBuffer[0] = 'U';
    glMscCswBuffer[1] = 'S';
    glMscCswBuffer[2] = 'B';
    glMscCswBuffer[3] = 'S';
    CyU3PMemCopy (glMscCswBuffer + 4, glMscCbwBuffer + 4, 4);       /* Copy the tag. */
    *((uint32_t *)(glMscCswBuffer + 8)) = glMscResidue;
    glMscCswBuffer[12] = glMscCmdStatus;

    dmaBuf.buffer = glMscCswBuffer;
    dmaBuf.status = 0;
    dmaBuf.size   = 32;
    dmaBuf.count  = 13;
    status = CyU3PDmaMultiChannelSetupSendBuffer (&glChHandleMscIn, &dmaBuf, 0);
    if (status == CY_U3P_SUCCESS)
        glMscState = CY_FX_MSC_STATE_CSW;
    else
        CyU3PDmaMultiChannelReset (&glChHandleMscIn);
}

/*
 * Entry function for the glRaidAppThread
 */
void
MscAppThread_Entry (
        uint32_t input)
{
    CyU3PReturnStatus_t status;
    CyU3PDmaBuffer_t    dmaBuf;

    uint32_t evMask = CY_FX_MSC_SETCONF_EVENT_FLAG | CY_FX_MSC_RESET_EVENT_FLAG | CY_FX_MSC_CBW_EVENT_FLAG |
        CY_FX_MSC_DATASENT_EVENT_FLAG | CY_FX_MSC_SIBCB_EVENT_FLAG | CY_FX_MSC_USB_SUSP_EVENT_FLAG;
    uint32_t evStat;
    uint32_t cardStat;
    uint16_t wakeReason;

    /* Initialize the MSC Application */
    CyFxMscApplnInit ();

    for (;;)
    {
        status = CyU3PEventGet (&glRaidAppEvent, evMask, CYU3P_EVENT_OR_CLEAR, &evStat, CYU3P_WAIT_FOREVER);
        if (status == CY_U3P_SUCCESS)
        {
            if (evStat & CY_FX_MSC_RESET_EVENT_FLAG)
            {
                if (glMscState == CY_FX_MSC_STATE_DATA)
                {
                    /* Make sure any ongoing storage requests are aborted, and then verify that the cards can be
                       accessed. */
                    CyU3PSibAbortRequest (0);
                    status = CyU3PSibGetCardStatus (0, &cardStat);
                    CyU3PSibAbortRequest (1);
                    status = CyU3PSibGetCardStatus (1, &cardStat);
                }

                glMscState = CY_FX_MSC_STATE_INACTIVE;
            }

            if (evStat & CY_FX_MSC_SETCONF_EVENT_FLAG)
                glMscState = CY_FX_MSC_STATE_CBW;

            if (evStat & CY_FX_MSC_USB_SUSP_EVENT_FLAG)
            {
                /* Place FX3S in Low Power Suspend mode, with USB bus activity as the wakeup source. */
                status = CyU3PSysEnterSuspendMode (CY_U3P_SYS_USB_BUS_ACTVTY_WAKEUP_SRC, 0, &wakeReason);
            }

            if (evStat & CY_FX_MSC_CBW_EVENT_FLAG)
            {
                if (glInPhaseError)
                {
                    /* Phase error: Send an error CSW and then stall both endpoints. */
                    glMscCmdStatus = 2;
                    CyU3PUsbStall (CY_FX_MSC_EP_BULK_IN, CyTrue, CyFalse);
                    CyU3PUsbStall (CY_FX_MSC_EP_BULK_OUT, CyTrue, CyFalse);
                }
                else
                {
                    glMscDriverBusy = CyTrue;
                    CyU3PUsbLPMDisable ();

                    if (CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED)
                    {
                        /* Make sure the link is brought back to U0 state at this stage. */
                        CyU3PUsbSetLinkPowerState (CyU3PUsbLPM_U0);
                    }

                    /* Parse CBW here. */
                    glMscState      = CY_FX_MSC_STATE_DATA;
                    CyFxMscApplnParseCbw ();
                }
            }

            if (evStat & CY_FX_MSC_DATASENT_EVENT_FLAG)
            {
                /* If CSW has been sent, go to CBW state. */
                if (glMscState == CY_FX_MSC_STATE_CSW)
                    glMscState = CY_FX_MSC_STATE_CBW;

                /* If data has been sent, go to CSW state. */
                if (glMscState == CY_FX_MSC_STATE_DATA)
                {
                    /* If we sent less data than the host wanted, stall the endpoint afterwards. */
                    if ((glMscResidue != 0) && (glMscCmdStatus == 0))
                        CyU3PUsbStall (CY_FX_MSC_EP_BULK_IN, CyTrue, CyFalse);

                    glMscState = CY_FX_MSC_STATE_STATUS;
                }
            }

            if (evStat & CY_FX_MSC_SIBCB_EVENT_FLAG)
            {
                /* Stall the endpoint if any data transfer is expected. */
                if ((glMscCmdStatus) && (glMscResidue))
                {
                    /* The DMA channel has already been reset. */
                    if (glCmdDirection)
                        CyU3PUsbStall (CY_FX_MSC_EP_BULK_IN, CyTrue, CyFalse);
                    else
                        CyU3PUsbStall (CY_FX_MSC_EP_BULK_OUT, CyTrue, CyFalse);
                }
                else
                {
                    /* Wait for all of the data to be transferred to the host in the case of a read. */
                    if (glCmdDirection)
                        CyU3PDmaMultiChannelWaitForCompletion (&glChHandleMscIn, CYU3P_WAIT_FOREVER);
                }

                /* Now move to the STATUS state to try and send the CSW. */
                glMscState = CY_FX_MSC_STATE_STATUS;
            }

            if (glMscState == CY_FX_MSC_STATE_CBW)
            {
                /* Re-enable Link power management now that the command has been handled. */
                glMscDriverBusy = CyFalse;
                CyU3PUsbLPMEnable ();

                dmaBuf.buffer = glMscCbwBuffer;
                dmaBuf.status = 0;
                dmaBuf.count  = 0;
                dmaBuf.size   = 64;     /* CBW is expected to be smaller than 64 bytes. */
                status = CyU3PDmaMultiChannelSetupRecvBuffer (&glChHandleMscOut, &dmaBuf, 0);
                if (status == CY_U3P_SUCCESS)
                {
                    glMscState = CY_FX_MSC_STATE_WAITING;

                    /* Make sure that we bring the link back to U0, so that the ERDY can be sent. */
                    if (CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED)
                        CyU3PUsbSetLinkPowerState (CyU3PUsbLPM_U0);
                }
                else
                    CyU3PDmaMultiChannelReset (&glChHandleMscOut);
            }

            if (glMscState == CY_FX_MSC_STATE_STATUS)
            {
                CyFxMscApplnSendCsw ();
            }
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
    uint32_t retThrdCreate = CY_U3P_ERROR_MEMORY_ERROR;
    void *ptr;

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_MSC_THREAD_STACK);

    if (ptr != 0)
    {
        /* Create the thread for the application */
        retThrdCreate = CyU3PThreadCreate (&glRaidAppThread,       /* MSC App Thread structure */
                "25:MSC Application",                           /* Thread ID and Thread name */
                MscAppThread_Entry,                             /* MSC App Thread Entry function */
                0,                                              /* No input parameter to thread */
                ptr,                                            /* Pointer to the allocated thread stack */
                CY_FX_MSC_THREAD_STACK,                         /* MSC App Thread stack size */
                CY_FX_MSC_THREAD_PRIORITY,                      /* MSC App Thread priority */
                CY_FX_MSC_THREAD_PRIORITY,                      /* MSC App Thread priority */
                CYU3P_NO_TIME_SLICE,                            /* No time slice for the application thread */
                CYU3P_AUTO_START                                /* Start the Thread immediately */
                );
    }

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

/*[]*/


