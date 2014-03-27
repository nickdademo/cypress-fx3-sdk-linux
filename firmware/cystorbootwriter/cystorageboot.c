/*
 ## Cypress FX3 Storage Boot Writing Firmawre (cystorageboot.c)
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
 ## ***************************************************************************************************
 ##             
 ## WARNING:  DO NOT SHIP SOURCE
 ##               
 ## This code utilizes APIs internal to the FX3/FX3S SDK which are not meant to be publicly exposed.
 ## This file MUST NOT be shipped as source or with debug information. 
 ## Only ship IMG file created from Release build of this code.
 ## ***************************************************************************************************
*/


/**********************************************************************************************
 * This firmware is used to program the FX3s/SD3 boot image onto a storage medium (SD/MMC/eMMC) 
 * attached to the S0 port of the device.
 **********************************************************************************************
 * Firmware revision 0.01
 **********************************************************************************************
 */

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

#include "cystorageboot.h"

/* This example demonstrates the implementation of a USB Mass Storage Class (Bulk Only Transport) function
 * using the FX3S APIs.
 * The implementation enumerates CY_FX_SIB_PARTITIONS on each of the storage ports as separate mass storage
 * drives.
 */

CyBool_t glLunState[CY_FX_SIB_PARTITIONS];                   /* Whether the LUN has valid storage. */
uint8_t  glLunType[CY_FX_SIB_PARTITIONS];
uint8_t  glLunLoc[CY_FX_SIB_PARTITIONS];
CyBool_t glLunWriteable[CY_FX_SIB_PARTITIONS];               /* Whether the LUN is writeable. */
uint16_t glLunBlkSize[CY_FX_SIB_PARTITIONS];                 /* Sector size for each of the LUNs. */
uint32_t glLunNumBlks[CY_FX_SIB_PARTITIONS];                 /* Capacity of the LUN in sectors. */

uint8_t  glSensePtr[CY_FX_SIB_PARTITIONS] = {CY_FX_MSC_SENSE_DEVICE_RESET};
CyBool_t         glDevConfigured = CyFalse;              /* Whether the device has been configured. */
CyBool_t         glInPhaseError  = CyFalse;              /* Whether a phase error condition has been detected. */
CyFxMscFuncState glMscState      = CY_FX_MSC_STATE_INACTIVE;

CyU3PSibDevInfo_t   glDevInfo;        /* Structure to hold device info */
CyU3PDmaChannel     glChHandleMscOut;                   /* DMA channel for OUT endpoint. */
CyU3PDmaChannel     glChHandleMscIn;                    /* DMA channel for IN endpoint. */

static CyU3PThread mscAppThread;	                /* MSC application thread structure */
static CyU3PEvent  glMscAppEvent;                       /* MSC application Event group */

uint8_t *glMscCbwBuffer  = 0;                           /* Scratch buffer used for CBW. */
uint8_t *glMscCswBuffer  = 0;                           /* Scratch buffer used for CSW. */
uint8_t *glMscDataBuffer = 0;                           /* Scratch buffer used for query commands. */

uint8_t  glCmdDirection = 0;                            /* SCSI Command Direction */
uint8_t  glMscCmdStatus = 0;                            /* MSC command status. */
uint32_t glMscResidue   = 0;                            /* Residue length for CSW */
uint8_t glActivePartition = 0;                          /* Internal partition which is being enumerated to the Host App
                                                           Externally the LUN enumerated will always be 0 */     
CyBool_t glMscDriverBusy = CyFalse;
PartitionData glPartitionData;

/* Request Sense Table */
static uint8_t glReqSenseCode[13][3] __attribute__ ((aligned (32))) =
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
    {0x06, 0x29, 0x00}     /* senseDeviceReset            0xc  */
};

/* Standard Inquiry Data */
uint8_t CyFxMscScsiInquiryData[36] __attribute__ ((aligned (32))) = {
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

    'S',        /* Product Id */
    't',
    'o',
    'r',
    'a',
    'g',
    'e',
    'W',
    'r',
    'i',
    't',
    'e',
    'r',
    0x00,
    0x00,
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

/* Extern Declarations For unexposed SDK functions*/
extern CyU3PReturnStatus_t CyU3PSibWritePartitionDataWrapper (uint8_t  portId, uint32_t blkAddr, uint8_t *buffer);
extern CyU3PReturnStatus_t CyU3PSibReadOneDataSectorWrapper (uint8_t  portId, uint32_t address, uint8_t * buffer);

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
     * S0 port is enabled in 8 bit mode.
     * S1 port is enabled in 4 bit mode.
     * UART is enabled on remaining pins of the S1 port.
     */
    io_cfg.isDQ32Bit        = CyFalse;
    io_cfg.s0Mode           = CY_U3P_SPORT_8BIT;
    io_cfg.s1Mode           = CY_U3P_SPORT_INACTIVE;
    io_cfg.gpioSimpleEn[0]  = 0;
    io_cfg.gpioSimpleEn[1]  = 0x02102800;                       /* IOs 43, 45, 52 and 57 are chosen as GPIO. */
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

/* This function initializes the Debug Module for the MSC Application */
void
CyFxMscApplnDebugInit()
{

    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t apiRetStatus;

    /* Initialize the UART for printing debug messages */
    apiRetStatus = CyU3PUartInit ();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyFxAppErrorHandler(apiRetStatus);
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
    apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set the UART transfer */
    apiRetStatus = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Initialize the Debug application */
    apiRetStatus = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    CyU3PDebugPreamble (CyFalse);
}

/* Callback to handle the USB Setup Requests and Mass Storage Class requests */
CyBool_t
CyFxMscApplnUSBSetupCB (
        uint32_t setupdat0, /* SETUP Data 0 */
        uint32_t setupdat1  /* SETUP Data 1 */
    )
{
    uint8_t  maxLun = (CY_FX_LUN_COUNT - 1);
    CyBool_t isHandled = CyFalse;
    CyU3PReturnStatus_t status;

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
        /* Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
         * requests here. It should be allowed to pass if the device is in configured
         * state and failed otherwise. */
        if ((bTarget == CY_U3P_USB_TARGET_INTF) && ((bRequest == CY_U3P_USB_SC_SET_FEATURE)
                    || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) && (wValue == 0))
        {
            if (glDevConfigured)
                CyU3PUsbAckSetup ();
            else
                CyU3PUsbStall (0, CyTrue, CyFalse);

            isHandled = CyTrue;
        }

        /* Handle CLEAR_FEATURE (EP_HALT) requests for the mass storage EPs here. */
        if ((bTarget == CY_U3P_USB_TARGET_ENDPT) && (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE))
        {
            if ((wIndex == CY_FX_MSC_EP_BULK_IN) || (wIndex == CY_FX_MSC_EP_BULK_OUT))
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
                    isHandled = CyTrue;
                    status = CyU3PUsbSendEP0Data (0x01, &maxLun);
                    if (status != CY_U3P_SUCCESS)
                    {
                        CyU3PUsbStall (0, CyTrue, CyFalse);
                        CyU3PDebugPrint (4, "Send EP0 Data Failed, Error Code = %d\n", status);
                    }
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

                    CyU3PUsbSetEpNak (CY_FX_MSC_EP_BULK_OUT, CyTrue);
                    CyU3PUsbSetEpNak (CY_FX_MSC_EP_BULK_IN, CyTrue);
                    CyU3PBusyWait (120);

                    CyU3PDmaChannelReset(&glChHandleMscOut);
                    CyU3PDmaChannelReset(&glChHandleMscIn);

                    CyU3PUsbFlushEp (CY_FX_MSC_EP_BULK_OUT);
                    CyU3PUsbStall (CY_FX_MSC_EP_BULK_OUT, CyFalse, CyTrue);

                    CyU3PUsbFlushEp (CY_FX_MSC_EP_BULK_IN);
                    CyU3PUsbStall (CY_FX_MSC_EP_BULK_IN, CyFalse, CyTrue);

                    /* Request Sense Index */
                    glSensePtr[0] = CY_FX_MSC_SENSE_DEVICE_RESET;
                    glSensePtr[1] = CY_FX_MSC_SENSE_DEVICE_RESET;

                    CyU3PUsbSetEpNak (CY_FX_MSC_EP_BULK_OUT, CyFalse);
                    CyU3PUsbSetEpNak (CY_FX_MSC_EP_BULK_IN, CyFalse);

                    /* Inform the thread to start waiting for USB CBWs again. */
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SETCONF_EVENT_FLAG, CYU3P_EVENT_OR);
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

    status = CyU3PDmaChannelSetupSendBuffer (&glChHandleMscIn, &dmaBuf);
    if (status == CY_U3P_SUCCESS)
    {
           /* Wait for data to be sent */
	   status = CyU3PDmaChannelWaitForCompletion (&glChHandleMscIn,CYU3P_WAIT_FOREVER);
     }
    
    return status;
}


void SendPartitionData(uint8_t lun)
{
    uint32_t partDataSize, dataLength;
    CyU3PReturnStatus_t status;
    partDataSize = sizeof(PartitionData);
    CyU3PMemSet (glMscDataBuffer, 0, partDataSize);
    
    PopulateDeviceInformation((PRT_DATA)glMscDataBuffer);
    glMscState      = CY_FX_MSC_STATE_DATA;
    glSensePtr[lun] = CY_FX_MSC_SENSE_OK;
    
    if (glMscResidue >= partDataSize)
    {
        dataLength    = partDataSize;
        glMscResidue -= partDataSize;
    }
    else
    {
        dataLength   = glMscResidue;
        glMscResidue = 0;
    }
    status = CyFxMscApplnSendDataToHost (glMscDataBuffer, dataLength);
    if (status != CY_U3P_SUCCESS) 
    {
        glMscCmdStatus  = 1;
        glSensePtr[lun] = CY_FX_MSC_SENSE_CRC_ERROR;
        CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
    }
    
}


/* Parse the received CBW and handle the command. */
void
CyFxMscApplnParseCbw (
    void)
{
    CyU3PReturnStatus_t status;
    uint8_t cmd = glMscCbwBuffer[15];
    uint8_t lun = glMscCbwBuffer[13] & 0x0F;
    uint32_t startAddr, dataLength;
    uint16_t numBlks;
    
    /* Direction of data transfer. */
    glCmdDirection = glMscCbwBuffer[12] & 0x80;

    /* Expected transfer length. */
    glMscResidue   = *((uint32_t *)(glMscCbwBuffer + 8));
    glMscCmdStatus = 0;

    /* Invalid LUN. */
    if (lun >= CY_FX_LUN_COUNT)
    {
        glMscCmdStatus  = 1;
        CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
        return;
    }

    /* Execute the command */
    switch (cmd)
    {
        case CY_FX_MSC_SCSI_INQUIRY:
            {
                if ((glCmdDirection == 0) || (glMscResidue == 0))
                {
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    glMscCmdStatus  = 1;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                glMscState      = CY_FX_MSC_STATE_DATA;
                glSensePtr[lun] = CY_FX_MSC_SENSE_OK;
                if (glMscResidue >= 36)
                {
                    dataLength    = 36;
                    glMscResidue -= 36;
                }
                else
                {
                    dataLength   = glMscResidue;
                    glMscResidue = 0;
                }

                status = CyFxMscApplnSendDataToHost (CyFxMscScsiInquiryData, dataLength);
                if (status != CY_U3P_SUCCESS)
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            }
            break;

        case CY_FX_MSC_SCSI_READ_CAPACITY:
        case CY_FX_MSC_SCSI_READ_FORMAT_CAPACITY:
            {
                uint8_t i = 0;

                if ((glCmdDirection == 0) || (glMscResidue == 0))
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                if (!glLunState[lun])
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_NO_MEDIA;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                CyU3PMemSet (glMscDataBuffer, 0, 12);
                if (cmd == CY_FX_MSC_SCSI_READ_FORMAT_CAPACITY)
                {
                    glMscDataBuffer[3] = 0x08;
                    glMscDataBuffer[8] = 0x02;
                    i = 4;
                }

                glMscDataBuffer[i++] = (uint8_t)(glLunNumBlks[lun] >> 24);
                glMscDataBuffer[i++] = (uint8_t)(glLunNumBlks[lun] >> 16);
                glMscDataBuffer[i++] = (uint8_t)(glLunNumBlks[lun] >> 8);
                glMscDataBuffer[i++] = (uint8_t)(glLunNumBlks[lun]);

                i += 2;
                glMscDataBuffer[i++] = (uint8_t)(glLunBlkSize[lun] >> 8);
                glMscDataBuffer[i++] = (uint8_t)(glLunBlkSize[lun]);

                glMscState      = CY_FX_MSC_STATE_DATA;
                glSensePtr[lun] = CY_FX_MSC_SENSE_OK;

                if (glMscResidue >= i)
                {
                    dataLength    = i;
                    glMscResidue -= i;
                }
                else
                {
                    dataLength   = glMscResidue;
                    glMscResidue = 0;
                }

                status = CyFxMscApplnSendDataToHost (glMscDataBuffer, dataLength);
                if (status != CY_U3P_SUCCESS) 
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            }
            break;

        case CY_FX_MSC_SCSI_REQUEST_SENSE:
            {
                if ((glCmdDirection == 0) || (glMscResidue == 0))
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                CyU3PMemSet (glMscDataBuffer, 0, 18);
                glMscDataBuffer[0]  = 0x70;
                glMscDataBuffer[2]  = glReqSenseCode[glSensePtr[lun]][0];
                glMscDataBuffer[7]  = 0x0A;
                glMscDataBuffer[12] = glReqSenseCode[glSensePtr[lun]][1];
                glMscDataBuffer[13] = glReqSenseCode[glSensePtr[lun]][2];

                glSensePtr[lun] = CY_FX_MSC_SENSE_OK;
                glMscState    = CY_FX_MSC_STATE_DATA;

                if (glMscResidue >= 18)
                {
                    dataLength    = 18;
                    glMscResidue -= 18;
                }
                else
                {
                    dataLength   = glMscResidue;
                    glMscResidue = 0;
                }

                status = CyFxMscApplnSendDataToHost (glMscDataBuffer, dataLength);
                if (status != CY_U3P_SUCCESS)
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            }
            break;

            /* No real support for Verify, Start/stop unit and Prevent/Allow Medium Removal commands. Treat them as
               equivalent to TEST_UNIT_READY. */
        case CY_FX_MSC_SCSI_VERIFY_10:
        case CY_FX_MSC_SCSI_START_STOP_UNIT:
        case CY_FX_MSC_SCSI_PREVENT_ALLOW_MEDIUM:
        case CY_FX_MSC_SCSI_TEST_UNIT_READY:
            {
                if ((glCmdDirection != 0) || (glMscResidue != 0))
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                if (!glLunState[lun])
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_NO_MEDIA;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                glMscState      = CY_FX_MSC_STATE_STATUS;
                glSensePtr[lun] = CY_FX_MSC_SENSE_OK;
            }
            break;

        case CY_FX_MSC_SCSI_MODE_SENSE_6:
            {
                if ((glCmdDirection == 0) || (glMscResidue == 0))
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                if (!glLunState[lun])
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_NO_MEDIA;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                glMscDataBuffer[0] = 0x03;
                glMscDataBuffer[1] = 0x00;
                glMscDataBuffer[2] = (glLunWriteable[lun]) ? 0x00 : 0x80;
                glMscDataBuffer[3] = 0x00;

                glSensePtr[lun] = CY_FX_MSC_SENSE_OK;
                glMscState      = CY_FX_MSC_STATE_DATA;

                if (glMscResidue >= 4)
                {
                    dataLength    = 4;
                    glMscResidue -= 4;
                }
                else
                {
                    dataLength   = glMscResidue;
                    glMscResidue = 0;
                }

                status = CyFxMscApplnSendDataToHost (glMscDataBuffer, dataLength);
                if (status != CY_U3P_SUCCESS)
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            }
            break;

        case CY_FX_MSC_SCSI_READ_10:
            {
                startAddr = (((uint32_t)glMscCbwBuffer[17] << 24) | ((uint32_t)glMscCbwBuffer[18] << 16) |
                    ((uint32_t)glMscCbwBuffer[19] << 8) | ((uint32_t)glMscCbwBuffer[20]));
                numBlks   = (((uint16_t)glMscCbwBuffer[22] << 8) | ((uint16_t)glMscCbwBuffer[23]));

                if (!glLunState[lun])
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_NO_MEDIA;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                if ((glCmdDirection == 0) || (glMscResidue != (numBlks * glLunBlkSize[lun])))
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                glSensePtr[lun] = CY_FX_MSC_SENSE_OK;
                glMscState      = CY_FX_MSC_STATE_DATA;
	        	status = CyU3PDmaChannelSetXfer (&glChHandleMscIn, (numBlks * CY_FX_SIB_MAX_BLOCK_SIZE));
                if (status == CY_U3P_SUCCESS)
                {
                    status = CyU3PSibReadWriteRequest (CyTrue, 0, glActivePartition, numBlks, startAddr, 1);
                    if (status != CY_U3P_SUCCESS)
                    {
                        /* Abort the DMA Channel */
                        CyU3PDmaChannelReset (&glChHandleMscIn);
                    }
                }	

                if (status != CY_U3P_SUCCESS)
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            }
            break;

        case CY_FX_MSC_SCSI_WRITE_10:
            {
                startAddr = (((uint32_t)glMscCbwBuffer[17] << 24) | ((uint32_t)glMscCbwBuffer[18] << 16) |
                    ((uint32_t)glMscCbwBuffer[19] << 8) | ((uint32_t)glMscCbwBuffer[20]));
                numBlks   = (((uint16_t)glMscCbwBuffer[22] << 8) | ((uint16_t)glMscCbwBuffer[23]));

                if (!glLunState[lun])
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_NO_MEDIA;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                if ((glCmdDirection != 0) || (glMscResidue != (numBlks * glLunBlkSize[lun])))
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                glSensePtr[lun] = CY_FX_MSC_SENSE_OK;
		        status = CyU3PDmaChannelSetXfer (&glChHandleMscOut, (numBlks * CY_FX_SIB_MAX_BLOCK_SIZE));
                if (status == CY_U3P_SUCCESS)
                {
                    status = CyU3PSibReadWriteRequest (CyFalse, 0, glActivePartition, numBlks, startAddr, 0);
                    if (status != CY_U3P_SUCCESS)
                    {
                        /* Abort the DMA Channel */
                        CyU3PDmaChannelReset (&glChHandleMscOut);
                    }
                }	

                if (status != CY_U3P_SUCCESS)
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            }
            break;
       
        /* SCSI Vendor Commands to support Writing Boot Firmware Image to Storage.*/

        /* Partition the Storage Media (User area)*/
        
        case CY_FX3S_SCSI_SET_PARTITION_INFO:
            {
                uint8_t partitionNum=0;
                PRT_EL partEl;


                                
                partitionNum = glMscCbwBuffer[16];

                if ((glCmdDirection == 0) || (glMscResidue ==0 ) || (partitionNum > (CY_FX_SIB_PARTITIONS-1)))
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                if (!glLunState[lun])
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_NO_MEDIA;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }


                partEl = & glPartitionData.partitionElement[partitionNum];
                partEl-> type = glMscCbwBuffer[17];
                partEl-> location = glMscCbwBuffer[18];
                partEl-> partitionSize = ((uint32_t)glMscCbwBuffer[19] | ((uint32_t)glMscCbwBuffer[20] << 8) | ((uint32_t)glMscCbwBuffer[21] << 16) | ((uint32_t)glMscCbwBuffer[22] << 24));

                SendPartitionData(lun);
               
           }
            break;
        
        case CY_FX3S_SCSI_CREATE_PARTITIONS:
            {
                uint8_t numPartitions = 0;
                if ((glCmdDirection == 0) || (glMscResidue ==0 ))
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                if (!glLunState[lun])
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_NO_MEDIA;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }
                CyU3PDebugPrint(3,"\n\r Creating Partitions on Port 0 ...");
                CyU3PMemSet (glMscDataBuffer, 0 ,512);
                numPartitions = glMscCbwBuffer[16];
                status = CreatePartitions(numPartitions, &glPartitionData);
                CyU3PDebugPrint(3,"\n\rCreate partitons: Status = 0x%x", status);

                if(status == CY_U3P_SUCCESS)
                {
                    CyFxMscApplnQueryDevStatus();
                    SendPartitionData(lun);
                }
                else  
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
                
            }
            break;

       case CY_FX3S_SCSI_DELETE_PARTITIONS:
            {
                if ((glCmdDirection == 0) || (glMscResidue == 0))
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                if (!glLunState[lun])
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_NO_MEDIA;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }
                CyU3PDebugPrint(3,"\n\r Deleting Partitions...");
                
                status = CyU3PSibRemovePartitions(0);
                
                CyU3PDebugPrint(3,"\n\rDelete partitons: Status = 0x%x", status);
                if (status == CY_U3P_ERROR_NOT_PARTITIONED)
                {
                    CyU3PDebugPrint(3,"\n\rDelete partitons: Device not Partitioned");
                    status = CY_U3P_SUCCESS;
                }
                
                if ( (status == CY_U3P_SUCCESS) && (glDevInfo.cardType == CY_U3P_SIB_DEV_MMC))
                {
                   status = CyU3PSibSendSwitchCommand(0, 0x03B30000 , 0);
                   CyU3PDebugPrint (3,"\n\r Reset the EXT_CSD" );
                }
                if((status == CY_U3P_SUCCESS))
                {
                    CyFxMscApplnQueryDevStatus();
                    SendPartitionData(lun);
                }
                else  
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
                
            }
            break;


         case CY_FX3S_SCSI_FETCH_DEVICE_INFO:
            {
                if ((glCmdDirection == 0) || (glMscResidue == 0))
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                if (!glLunState[lun])
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_NO_MEDIA;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }
                CyU3PDebugPrint(3,"\n\r Fetching Device information...");

                //CyU3PSibCheckForPartition(0);
                //CyU3PThreadSleep(100);
                CyFxMscApplnQueryDevStatus();
                SendPartitionData(lun);
               }
            break;

        case CY_FX3S_SCSI_USE_PARTITION_FOR_BOOT:
            {
                
                uint8_t partNum;
                partNum = glMscCbwBuffer[16];

                

                if (partNum >= glDevInfo.numUnits)
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }
                
                glActivePartition = partNum;
                
                glSensePtr[lun] = CY_FX_MSC_SENSE_OK;
                glMscState      = CY_FX_MSC_STATE_STATUS;
              
            }
            break;

        case CY_FX3S_SCSI_GET_MMC_EXT_CSD:
            {
                if ((glCmdDirection == 0) || (glMscResidue == 0) || (glDevInfo.cardType != CY_U3P_SIB_DEV_MMC))
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                if (!glLunState[lun])
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_NO_MEDIA;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }


                CyU3PDebugPrint(3,"\n\r Fetching Extended CSD...");

                
                CyU3PMemSet (glMscDataBuffer, 0, 512);
                status = CyU3PSibGetMMCExtCsd(0, glMscDataBuffer);

                if( status != CY_U3P_SUCCESS)
                {                    
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_READ_ERROR;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }
 
                glMscState      = CY_FX_MSC_STATE_DATA;
                glSensePtr[lun] = CY_FX_MSC_SENSE_OK;

                if (glMscResidue >= 512)
                {
                    dataLength    = 512;
                    glMscResidue -= 512;
                }
                else
                {
                    dataLength   = glMscResidue;
                    glMscResidue = 0;
                }

                status = CyFxMscApplnSendDataToHost (glMscDataBuffer, dataLength);
                if (status != CY_U3P_SUCCESS) 
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_CRC_ERROR;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                }
            

            }
            break;

        case CY_FX3S_SCSI_ENABLE_PARTITION:
            {
                
                uint8_t bootConfig = 0;
                uint32_t checksum = 0;
                uint32_t switchParam = 0x03B30000;
                CyU3PDebugPrint(3,"\n\r Enable Partition for Boot.");
                if ((glCmdDirection == 0) || (glMscResidue == 0) || (glDevInfo.cardType == CY_U3P_SIB_DEV_SDIO))
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                if (!glLunState[lun])
                {
                    glMscCmdStatus  = 1;
                    glSensePtr[lun] = CY_FX_MSC_SENSE_NO_MEDIA;
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                    break;
                }

                switch (glLunLoc[glActivePartition+1])
                {
                    case CY_U3P_SIB_LOCATION_USER: 
                        {
                            bootConfig = 0x38;
                            /* TODO: Move the partition header code to the host app and use 
                             * CY_FX3S_SCSI_WRITE_USER_VBP_HEADER for this in the nnext rev*/
                            /* Write a fresh partition Header. Writes a fixed header for 1 image at LBA 0x00
                             * Sets next partition at 0x2800*/
                            CyU3PMemSet (glMscDataBuffer, 0, 512);
                            /* Signatur 'CYWB' */
                            glMscDataBuffer[0] = 0x43;
                            glMscDataBuffer[1] = 0x59;
                            glMscDataBuffer[2] = 0x57; 
                            glMscDataBuffer[3] = 0x42;
                        
                            /* Boot Config = User Partition*/
                            glMscDataBuffer[4] = 0x38;
                        
                            /* Bus Width */
                            glMscDataBuffer[5] = 0x01;

                            /* Active Image*/
                            glMscDataBuffer[7] = 0x00;
                        
                            /* Image 0 LBA */
                            glMscDataBuffer [8] = 0x40;
                            glMscDataBuffer [9] = 0x00;
                            glMscDataBuffer [10] = 0x00;
                            glMscDataBuffer [11] = 0x00;

                            /* Image 1 LBA -> Backup Image*/
                            glMscDataBuffer [12] = 0x40;
                            glMscDataBuffer [13] = 0x04;
                            glMscDataBuffer [14] = 0x00;
                            glMscDataBuffer [15] = 0x00;

 
                            /* User LBA -> Pointer to next partition 0x00002800*/

                            glMscDataBuffer [40] = 0x00;
                            glMscDataBuffer [41] = 0x28;
                            glMscDataBuffer [42] = 0x00;
                            glMscDataBuffer [43] = 0x00;
                        
                            /* partition type - Boot Partition*/
                            glMscDataBuffer [47] = 0xBB;

                            CyU3PComputeChecksum ((uint32_t*) glMscDataBuffer, 68, &checksum);
        
                            (*(uint32_t*)&glMscDataBuffer[68]) = checksum;

                            status = CyU3PSibWritePartitionDataWrapper(0, 0, glMscDataBuffer);

                            if ( status != CY_U3P_SUCCESS)
                            {
                                glMscCmdStatus  = 1;
                                glSensePtr[lun] = CY_FX_MSC_SENSE_WRITE_FAULT;
                                CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                                return;
                            }
                        }
                        break;
                    case CY_U3P_SIB_LOCATION_BOOT1:
                        {
                            bootConfig = 0x08;
                        }
                        break;
                    case CY_U3P_SIB_LOCATION_BOOT2:
                        {
                            bootConfig = 0x10;
                        }
                        break;
                    default:
                        {
                            glMscCmdStatus  = 1;
                            glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                            CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                        }
                        break;
                }
                if (glDevInfo.cardType == CY_U3P_SIB_DEV_SD)
                {
                    CyU3PDebugPrint(3, "\n\r Found SD Card while trying to write to Extended CSD. Returning Success...");
                    SendPartitionData(lun);
                    break;
                }

                status = CyU3PSibSendSwitchCommand(0, 0x03B30000 , 0);
                
                if (status == CY_U3P_SUCCESS)
                {
                    
                    switchParam |= ((uint32_t)bootConfig << 8);            
                    
                    status = CyU3PSibSendSwitchCommand(0, switchParam, 0);   

                    if (status == CY_U3P_SUCCESS)
                    {
                        CyU3PDebugPrint (3, "\n\r Enabling Boot partition Completed ");

                       SendPartitionData(lun) ;
                    }
                 }
                 
                 if(status != CY_U3P_SUCCESS)
                 {
                     glMscCmdStatus  = 1;
                     glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_PARAMETER;
                     CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
                 }
            }
            break;

        default:
            {
                /* Unsupported command. */
                glMscCmdStatus  = 1;
                glSensePtr[lun] = CY_FX_MSC_SENSE_INVALID_OP_CODE;
                CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
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
        case CY_U3P_USB_EVENT_DISCONNECT:
        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_CONNECT:
            {
                CyU3PDebugPrint (4, "USB event %d received\r\n", evtype);

                /* The DMA channels are created at the beginning using SuperSpeed settings.
                   They are reset when any disconnect, connect, suspend or reset events are received. */

                CyU3PDmaChannelAbort (&glChHandleMscIn);
                CyU3PDmaChannelAbort (&glChHandleMscOut);

                /* Request Sense Index */
                glSensePtr[0] = CY_FX_MSC_SENSE_DEVICE_RESET;
                glSensePtr[1] = CY_FX_MSC_SENSE_DEVICE_RESET;

                /* Clear Flag */
                glDevConfigured = CyFalse;
                glInPhaseError = CyFalse;
            }
            break;

        case CY_U3P_USB_EVENT_SETCONF:
            {
                if (evdata == 0)
                {
                    glDevConfigured = CyFalse;
                    break;
                }

                CyU3PDebugPrint (4, "USB Set Config completed\r\n");

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

                    CyU3PDmaChannelReset (&glChHandleMscIn);
                    CyU3PDmaChannelReset (&glChHandleMscOut);

                    /* Notify the thread to start waiting for a mass storage CBW. */
                    CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SETCONF_EVENT_FLAG, CYU3P_EVENT_OR);
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


CyU3PReturnStatus_t 
    CreatePartitions(uint8_t numPartitions, PRT_DATA partitions)
{
    CyU3PReturnStatus_t status;

    uint8_t i,j;
    uint32_t partsize[CY_FX_SIB_PARTITIONS-1] = {0};
    uint8_t  parttype[CY_FX_SIB_PARTITIONS-1] = {0};
    PRT_EL pe;

    CyU3PDebugPrint( 3, "\n\r Num Partitions Requested = %d", partitions->numPartitions);

    if (numPartitions> (CY_FX_SIB_PARTITIONS-1))
        return CY_U3P_ERROR_BAD_ARGUMENT;
    
    for (i = 0, j=0; i< numPartitions; i++)
    {
        pe = &partitions->partitionElement[i];
        if (pe->type !=0)
        {
            if( pe->location != CY_U3P_SIB_LOCATION_USER)
                continue;
            parttype[j] = pe->type;
            partsize[j] = pe->partitionSize;
            CyU3PDebugPrint (3, "\n Allocating User area Partition %d of Type 0x%x, of size %d blocks",j,parttype[j],partsize[j]);
            j++;
        }
        else 
        {
            break;
        }
    }
    if (j > 1)
    {
        status = CyU3PSibPartitionStorage(0, j, partsize, parttype);
    }
    else if(j == 1)
    {
        status = CY_U3P_SUCCESS; /* Only One partition requested. Returning success as the Delete partitions 
                                    which precedes creation would already have reduced numPartitions to 1. */
    }
    else
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }

    return status;
}

void PopulateDeviceInformation(PRT_DATA pData)
{
    PRT_EL pe;
    uint8_t i; 
    pData->deviceType      = glDevInfo.cardType;
    pData->numPartitions   = glDevInfo.numUnits; 
    pData->numBlocks       = glDevInfo.numBlks;
    
    for (i= 0; i < pData->numPartitions; i++)
    {
        pe = &pData->partitionElement[i];
        
        pe->type        = glLunType[i+1];
        pe->location    = glLunLoc [i+1];
        pe->blockSize   = glLunBlkSize[i+1];
        pe->partitionSize = glLunNumBlks[i+1];
   }
}


void
CyFxMscApplnQueryDevStatus (
        void)
{
    CyU3PReturnStatus_t status;
    CyU3PSibLunInfo_t   unitInfo;
    uint8_t j;
    uint8_t bootSibPort = 0;

    /* Initialize LUN data structures with default values. */
    for (j = 0; j < CY_FX_SIB_PARTITIONS; j++)
    {
        if(j ==0)
        {
            /* Virtual Boot Partition*/
            glLunType[j]      = CY_U3P_SIB_LUN_BOOT;
            glLunLoc[j]       = CY_U3P_SIB_LOCATION_USER;
            glLunState[j]     = CyTrue;
            glLunWriteable[j] = CyTrue;
            glLunBlkSize[j]   = CY_FX_SIB_MAX_BLOCK_SIZE;
            glLunNumBlks[j]   = CY_FX_SIB_MAX_BOOT_BLOCKS;
        }
        else
        {
            glLunState[j]     = CyFalse;
            glLunWriteable[j] = CyTrue;
            glLunBlkSize[j]   = 0;
            glLunNumBlks[j]   = 0;
        }
    }
    
    status = CyU3PSibQueryDevice (bootSibPort, &glDevInfo);
    if (status == CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (8, "Found a device on port %d\r\n", bootSibPort);
        CyU3PDebugPrint (6, "\tType=%d, numBlks=%d, eraseSize=%d, clkRate=%d\r\n",
                glDevInfo.cardType, glDevInfo.numBlks, glDevInfo.eraseSize, glDevInfo.clkRate);
        CyU3PDebugPrint (6, "\tblkLen=%d removable=%d, writeable=%d, locked=%d\r\n",
                glDevInfo.blkLen, glDevInfo.removable, glDevInfo.writeable, glDevInfo.locked);
        CyU3PDebugPrint (6, "\tddrMode=%d, opVoltage=%d, busWidth=%d, numUnits=%d\r\n",
                glDevInfo.ddrMode, glDevInfo.opVoltage, glDevInfo.busWidth, glDevInfo.numUnits);

        /* Check for Existing Partitions and their types */
        for (j = 0; j < CY_FX_SIB_PARTITIONS-1; j++)
        {
            status = CyU3PSibQueryUnit (bootSibPort, j, &unitInfo);
            if (status == CY_U3P_SUCCESS)
            {
                CyU3PDebugPrint (6, "Dev %d, Unit %d: numBlocks=%d, Partition Type = %x, Location = %x\r\n", bootSibPort, j, unitInfo.numBlocks, unitInfo.type, unitInfo.location);
                glLunState[j+1]     = CyTrue;
                glLunType[j+1]      = unitInfo.type;
                glLunLoc[j+1]       = unitInfo.location;
                glLunBlkSize[j+1]   = unitInfo.blockSize;
                glLunNumBlks[j+1]   = unitInfo.numBlocks;
                glLunWriteable[j+1] = glDevInfo.writeable;
            }
            else
            {
                CyU3PDebugPrint (2, "Error: Failed to query partition %d on port %d\r\n", j, bootSibPort);
            }
        }

    }
    else
    {
            CyU3PDebugPrint (2, "StorageQueryDev (%d) failed with code %d\r\n", bootSibPort, status);
    }
    
}

void
CyFxMscApplnSibCB (
        uint8_t             portId,
        CyU3PSibEventType   evt,
        CyU3PReturnStatus_t status)
{
    CyU3PDmaSocketConfig_t sockConf;

    if (evt == CY_U3P_SIB_EVENT_XFER_CPLT)
    {
        if (status != CY_U3P_SUCCESS)
        {
            glMscCmdStatus     = 1;
            glSensePtr[portId] = CY_FX_MSC_SENSE_CRC_ERROR;

            /* Transfer has failed. Reset the DMA channel. */
            if (glCmdDirection)
            {
                CyU3PDmaSocketGetConfig ((uint16_t)(CY_U3P_UIB_SOCKET_CONS_0 | CY_FX_MSC_EP_BULK_IN_SOCKET),
                        &sockConf);
                glMscResidue -= sockConf.xferCount;
                CyU3PDmaChannelReset (&glChHandleMscIn);
            }
            else
            {
                CyU3PDmaSocketGetConfig ((uint16_t)(CY_U3P_UIB_SOCKET_PROD_0 + CY_FX_MSC_EP_BULK_OUT_SOCKET),
                        &sockConf);
                glMscResidue -= sockConf.xferCount;
                CyU3PDmaChannelReset (&glChHandleMscOut);
            }
        }
        else
        {
            glMscCmdStatus = 0;
            glMscResidue   = 0;
            glSensePtr[portId] = CY_FX_MSC_SENSE_OK;
        }

        CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_SIBCB_EVENT_FLAG, CYU3P_EVENT_OR);
    }

    if ((evt == CY_U3P_SIB_EVENT_DATA_ERROR) || (evt == CY_U3P_SIB_EVENT_ABORT) )
    {
         /* Transfer has failed. Reset the DMA channel. */
        if (glCmdDirection)
        {
            CyU3PDmaChannelReset ((CyU3PDmaChannel *) &glChHandleMscOut);
        }
        else
        {
            CyU3PDmaChannelReset ((CyU3PDmaChannel *) &glChHandleMscIn);
        }
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
        CyU3PDebugPrint (4, "GPIO Init failed, code=%d\r\n", status);
        CyFxAppErrorHandler (status);
    }

    intfParams.resetGpio       = 0xFF;                          /* No GPIO control on SD/MMC power. */
    intfParams.rstActHigh      = CyTrue;                        /* Don't care as no GPIO is selected. */
    intfParams.cardDetType     = CY_U3P_SIB_DETECT_NONE;        /* No Card detect. */
    intfParams.writeProtEnable = CyTrue;                        /* Write protect handling enabled. */
    intfParams.lowVoltage      = CyFalse;                       /* Low voltage operation disabled. */
    intfParams.voltageSwGpio   = 45;                            /* Use GPIO_45 for voltage switch on S0 port. */
    intfParams.lvGpioState     = CyFalse;                       /* Driving GPIO low selects 1.8 V on SxVDDQ. */
    intfParams.useDdr          = CyFalse;                       /* DDR clocking enabled. */
    intfParams.maxFreq         = CY_U3P_SIB_FREQ_104MHZ;        /* No S port clock limitation. */
    intfParams.cardInitDelay   = 0;                             /* No delay required between SD card insertion
                                                                   before initialization. */

    status = CyU3PSibSetIntfParams (0, &intfParams);

    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "Set SIB interface parameters failed, code=%d\r\n", status);
        CyFxAppErrorHandler (status);
    }

    status = CyU3PSibStart ();
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "SIB start failed, code=%d\r\n", status);
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
    epConf.burstLen = CY_FX_MSC_EP_BURST_SIZE;
    epConf.isoPkts  = 0;

    status = CyU3PSetEpConfig (CY_FX_MSC_EP_BULK_OUT, &epConf);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set EP config failed, code=%d\r\n", status);
        CyFxAppErrorHandler (status);
    } 

    status = CyU3PSetEpConfig (CY_FX_MSC_EP_BULK_IN, &epConf);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set EP config failed, code=%d\r\n", status);
        CyFxAppErrorHandler (status);
    } 
}

void
CyFxMscApplnDmaCb (
        CyU3PDmaChannel   *handle,
        CyU3PDmaCbType_t   evtype,
        CyU3PDmaCBInput_t *input)
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
            CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_CBW_EVENT_FLAG, CYU3P_EVENT_OR);
            break;

        case CY_U3P_DMA_CB_SEND_CPLT:
            CyU3PEventSet (&glMscAppEvent, CY_FX_MSC_DATASENT_EVENT_FLAG, CYU3P_EVENT_OR);
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
    CyU3PDmaChannelConfig_t dmaConfig;
    CyU3PReturnStatus_t status;

    glMscCbwBuffer  = (uint8_t *)CyU3PDmaBufferAlloc (1024);
    glMscCswBuffer  = (uint8_t *)CyU3PDmaBufferAlloc (1024);
    glMscDataBuffer = (uint8_t *)CyU3PDmaBufferAlloc (1024);
    if ((glMscCbwBuffer == 0) || (glMscCswBuffer == 0) || (glMscDataBuffer == 0))
    {
        CyU3PDebugPrint (4, "Failed to allocate scratch buffer\r\n");
        CyFxAppErrorHandler (CY_U3P_ERROR_MEMORY_ERROR);
    }

    /* Both DMA channels are created with SuperSpeed parameters. The CyU3PSetEpPacketSize () API is used
       to reconfigure the endpoints to work with DMA channels with large buffers. */
    dmaConfig.size           = 1024 * CY_FX_MSC_EP_BURST_SIZE;
    dmaConfig.count          = CY_FX_MSC_DMA_BUF_COUNT;
    dmaConfig.prodSckId      = (CyU3PDmaSocketId_t)(CY_U3P_UIB_SOCKET_PROD_0 | CY_FX_MSC_EP_BULK_OUT_SOCKET);
    dmaConfig.consSckId      = CY_U3P_SIB_SOCKET_0;
    dmaConfig.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    dmaConfig.notification   = CY_U3P_DMA_CB_RECV_CPLT;
    dmaConfig.cb             = CyFxMscApplnDmaCb;
    dmaConfig.prodHeader     = 0;
    dmaConfig.prodFooter     = 0;
    dmaConfig.consHeader     = 0;
    dmaConfig.prodAvailCount = 0;

    status = CyU3PDmaChannelCreate (&glChHandleMscOut, CY_U3P_DMA_TYPE_AUTO, &dmaConfig);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "DMA channel create failed, code=%d\r\n", status);
        CyFxAppErrorHandler (status);
    }

    dmaConfig.prodSckId     = CY_U3P_SIB_SOCKET_1;
    dmaConfig.consSckId     = (CyU3PDmaSocketId_t)(CY_U3P_UIB_SOCKET_CONS_0 | CY_FX_MSC_EP_BULK_IN_SOCKET);
    dmaConfig.notification  = CY_U3P_DMA_CB_SEND_CPLT;
    dmaConfig.cb            = CyFxMscApplnDmaCb;

    status = CyU3PDmaChannelCreate (&glChHandleMscIn, CY_U3P_DMA_TYPE_AUTO, &dmaConfig);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "DMA channel create failed, code=%d\r\n", status);
        CyFxAppErrorHandler (status);
    }
}

static void
CyFxMscAppInitVars (
		void)
{
    uint8_t i;

    for (i = 0; i < (CY_FX_LUN_COUNT); i++)
    {
        glLunState[i]     = CyFalse;
        glLunWriteable[i] = CyTrue;
        glLunBlkSize[i]   = 0;
        glLunNumBlks[i]   = 0;
        glSensePtr[i]     = CY_FX_MSC_SENSE_DEVICE_RESET;
    }
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
    txApiRetStatus = CyU3PEventCreate (&glMscAppEvent);
    if (txApiRetStatus != 0)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "MSC Appln Create Event Failed, Error Code = %d\n", txApiRetStatus);
    }

    /* Start the USB functionality */
    apiRetStatus = CyU3PUsbStart ();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Function Failed to Start, Error Code = %d\n",apiRetStatus);
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
        CyU3PDebugPrint (4, "USB Set Device Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB30DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Device Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, NULL, (uint8_t *)CyFxUSBDeviceQualDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Device Qualifier Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBHSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set HS Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBFSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set FS Configuration Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBSSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set SS Configuration Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, NULL, (uint8_t *)CyFxUSBBOSDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set BOS Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set String Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set String Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set String Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 3, (uint8_t *)CyFxUSBSerialNumberDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set String Descriptor failed, Error Code = %d\n",apiRetStatus);
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
        CyU3PDebugPrint (4, "USB Connect failed, Error Code = %d\n",apiRetStatus);
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
    status = CyU3PDmaChannelSetupSendBuffer (&glChHandleMscIn, &dmaBuf);
    if (status == CY_U3P_SUCCESS)
        glMscState = CY_FX_MSC_STATE_CSW;
    else
        CyU3PDmaChannelReset (&glChHandleMscIn);
}

/*
 * Entry function for the mscAppThread
 */
void
MscAppThread_Entry (
        uint32_t input)
{
    CyU3PReturnStatus_t status;

    uint32_t evMask = CY_FX_MSC_SETCONF_EVENT_FLAG | CY_FX_MSC_RESET_EVENT_FLAG | CY_FX_MSC_CBW_EVENT_FLAG |
        CY_FX_MSC_DATASENT_EVENT_FLAG | CY_FX_MSC_SIBCB_EVENT_FLAG;
    uint32_t evStat;

    CyU3PDmaBuffer_t       dmaBuf;

    /* Initialize the Debug Module */
    CyFxMscApplnDebugInit ();

    /* Initialize the MSC Application */
    CyFxMscApplnInit ();

    for (;;)
    {
        status = CyU3PEventGet (&glMscAppEvent, evMask, CYU3P_EVENT_OR_CLEAR, &evStat, CYU3P_WAIT_FOREVER);
        if (status == CY_U3P_SUCCESS)
        {
            if (evStat & CY_FX_MSC_RESET_EVENT_FLAG)
                glMscState = CY_FX_MSC_STATE_INACTIVE;

            if (evStat & CY_FX_MSC_SETCONF_EVENT_FLAG)
                glMscState = CY_FX_MSC_STATE_CBW;

            if (evStat & CY_FX_MSC_CBW_EVENT_FLAG)
            {
                if (glInPhaseError)
                {
                    /* Phase error: Send an error CSW and then stall both endpoints. */
                    glMscCmdStatus = 2;
                    glMscResidue   = 0;
                    CyFxMscApplnSendCsw ();

                    CyU3PUsbStall (CY_FX_MSC_EP_BULK_IN, CyTrue, CyFalse);
                    CyU3PUsbStall (CY_FX_MSC_EP_BULK_OUT, CyTrue, CyFalse);
                }
                else
                {
                    glMscDriverBusy = CyTrue;
                    if (CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED)
                    {
                        /* Keep U1/U2 entry disabled whenever we are processing a command. */
                        CyU3PUsbLPMDisable ();
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
                {
                    glMscState      = CY_FX_MSC_STATE_CBW;
                }

                /* If data has been sent, go to CSW state. */
                if (glMscState == CY_FX_MSC_STATE_DATA)
                    glMscState = CY_FX_MSC_STATE_STATUS;
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
                        CyU3PDmaChannelWaitForCompletion (&glChHandleMscIn, CYU3P_WAIT_FOREVER);
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
                status = CyU3PDmaChannelSetupRecvBuffer (&glChHandleMscOut, &dmaBuf);
                if (status == CY_U3P_SUCCESS)
                {
                    glMscState = CY_FX_MSC_STATE_WAITING;

                    /* Make sure that we bring the link back to U0, so that the ERDY can be sent. */
                    if (CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED)
                        CyU3PUsbSetLinkPowerState (CyU3PUsbLPM_U0);
                }
                else
                    CyU3PDmaChannelReset (&glChHandleMscOut);
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
    void *ptr;
    uint32_t retThrdCreate;

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_MSC_THREAD_STACK);

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (&mscAppThread,   /* MSC App Thread structure */
                          "25:MSC Application",         /* Thread ID and Thread name */
                          MscAppThread_Entry,           /* MSC App Thread Entry function */
                          0,                            /* No input parameter to thread */
                          ptr,                          /* Pointer to the allocated thread stack */
                          CY_FX_MSC_THREAD_STACK,       /* MSC App Thread stack size */
                          CY_FX_MSC_THREAD_PRIORITY,    /* MSC App Thread priority */
                          CY_FX_MSC_THREAD_PRIORITY,    /* MSC App Thread priority */
                          CYU3P_NO_TIME_SLICE,          /* No time slice for the application thread */
                          CYU3P_AUTO_START              /* Start the Thread immediately */
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

/*[]*/


