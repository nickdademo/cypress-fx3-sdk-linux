/*
 ## Cypress USB 3.0 Platform source file (cyfxmscdemo.c)
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

/* This file illustrates the Mass Storage Class Driver example.
   The example makes use of the the internal device memory to provide the storage space.
   A minimum of 32KB is required for the device to be formatted as FAT on the windows host */


#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3error.h>
#include <cyu3usb.h>
#include <cyu3usbconst.h>
#include <cyu3uart.h>
#include "cyfxmscdemo.h"

static CyU3PThread mscAppThread;	                    /* MSC application thread structure */
static CyU3PUSBSpeed_t glUsbSpeed; /* USB Bus speed */
static uint32_t glMscMaxSectors;
static uint16_t glMscSectorSize;
static CyBool_t setConfHandled; /* Flag to indicate Set Config event handled */

/* Initialize the CSW signature. Other fields are 0 */
static uint8_t glMscCswStatus[CY_FX_MSC_CSW_MAX_COUNT] __attribute__ ((aligned (32))) = {'U','S','B','S'};

static uint32_t glCswDataResidue;                           /* Residue length for CSW */
static CyU3PDmaChannel glChHandleMscOut, glChHandleMscIn;   /* Channel handles */
static CyU3PEvent  glMscAppEvent;                           /* MSC application DMA Event group */

/* Pointer for dynamic allocation of Storage device memory : 32K */
static uint8_t *glMscStorageDeviceMemory;

static uint8_t glMscOutBuffer[CY_FX_MSC_REPONSE_DATA_MAX_COUNT] __attribute__ ((aligned (32)));  /* Buffer for the MSC response data
                                                                     (Currently max is 18 bytes) */
static uint8_t glMscInBuffer[CY_FX_MSC_CBW_MAX_COUNT] __attribute__ ((aligned (32)));   /* Buffer to receive the CBW (Max of 31 bytes) */
static uint8_t glCmdDirection;                                    /* SCSI Command Direction */
static uint32_t glDataTxLength;                                   /* SCSI Data length */
uint8_t glInPhaseError = CyFalse;

/* Request Sense Table */
static uint8_t glReqSenseCode[13][3] __attribute__ ((aligned (32))) =
{
    /*SK,  ASC,  ASCQ*/
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

/* Current Request Sense Index */
static uint8_t glReqSenseIndex=CY_FX_MSC_SENSE_DEVICE_RESET;

/* Standard Inquiry Data */
static uint8_t CyFxMscScsiInquiryData[36] __attribute__ ((aligned (32))) = {0x00, /* PQ and PDT */
                                             0x80, /* RMB = 1 */
                                             0x00, /* Version */
                                             0x02, /* Response data format */
                                             0x1F, /* Addnl Length */
                                             0x00,
                                             0x00,
                                             0x00,
                                             'C',  /* Vendor Id */
                                             'y',
                                             'p',
                                             'r',
                                             'e',
                                             's',
                                             's',
                                             0x00,
                                             'F',
                                             'X',
                                             '3',
                                             0x00,
                                             'M',  /* Product Id */
                                             'S',
                                             'C',
                                             0x00,
                                             'D',
                                             'E',
                                             'M',
                                             'O',
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             '1'  /* Revision */
                                          };

void MscAppThread_Entry (uint32_t);     /* Forward declaration for the mscAppThread entry function */

CyBool_t
CyFxMscApplnUSBSetupCB (
        uint32_t setupdat0, /* SETUP Data 0 */
        uint32_t setupdat1  /* SETUP Data 1 */
    );

void
CyFxMscApplnUSBEventCB (
    CyU3PUsbEventType_t evtype, /* Event type */
    uint16_t            evdata  /* Event data */
    );


/* MSC application error handler */
void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus    /* API return status */
        );

CyFxMscCswReturnStatus_t
CyFxMscCheckScsiCmd(
    uint8_t *buffer, /* Pointer to the CBW buffer */
    uint16_t count   /* Length of the CBW */
    );

CyFxMscCswReturnStatus_t
CyFxMscSendCsw(
    CyFxMscCswReturnStatus_t cswReturnStatus    /* Command Status */
    );

CyU3PReturnStatus_t
CyFxMscSendUSBData(
    uint8_t *data,      /* Pointer to the Send data */
    uint32_t length     /* Length of data to be sent */
    );

CyU3PReturnStatus_t
CyFxMscReceiveUSBData (
    uint8_t *data   /* Pointer to the receive data */
    );

CyFxMscCswReturnStatus_t
CyFxMscParseScsiCmd(
    uint8_t *mscCbw     /* Pointer to the CBW array */
    );

CyFxMscCswReturnStatus_t
CyFxCheckCmdDirection(
    uint8_t scsiCmd,        /* SCSI Command */
    uint8_t cmdDirection    /* Direction */
    );

void
CyFxMscApplnDebugInit (void);

void
CyFxMscApplnInit (void);
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

    /* Initialize the caches. Enable both Instruction and Data Caches. */
    status = CyU3PDeviceCacheControl (CyTrue, CyTrue, CyTrue);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Configure the IO matrix for the device.
     * Pport is not GPIF 32bit; and no GPIO is currently required.
     */
    io_cfg.isDQ32Bit = CyFalse;
    io_cfg.s0Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.s1Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.gpioSimpleEn[0] = 0;
    io_cfg.gpioSimpleEn[1] = 0;
    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0;
    io_cfg.useUart = CyTrue;
    io_cfg.useI2C = CyFalse;
    io_cfg.useI2S = CyFalse;
    io_cfg.useSpi = CyFalse;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_DEFAULT;
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

    /* Allocate the Storage Device Memory */
    glMscStorageDeviceMemory = (uint8_t *)CyU3PDmaBufferAlloc(CY_FX_MSC_CARD_CAPACITY);
    if (glMscStorageDeviceMemory == NULL)
    {
        /* Error Handling */

        /* MSC Application cannot continue without Storage space */

        /* Loop indefinitely */
        while(1);
    }
}

/*
 * Entry function for the mscAppThread
 */
void
MscAppThread_Entry (
        uint32_t input)
{
    CyU3PDmaBuffer_t dmaMscInBuffer;
    CyFxMscCswReturnStatus_t cswReturnStatus=CY_FX_CBW_CMD_PASSED;
    CyU3PReturnStatus_t apiRetStatus;
    uint32_t txApiRetStatus,eventFlag;
    CyU3PDmaChannelConfig_t dmaConfig;;

    /* Clear the Storage memory */
    CyU3PMemSet (glMscStorageDeviceMemory, 0, (CY_FX_MSC_CARD_CAPACITY));

    /* Initialize the Debug Module */
    CyFxMscApplnDebugInit();

    /* Initialize the MSC Application */
    CyFxMscApplnInit();

    for (;;)
    {
        /* Wait for a Set Configuration Event */
        txApiRetStatus = CyU3PEventGet (&glMscAppEvent, CY_FX_MSC_SET_CONFIG_EVENT,CYU3P_EVENT_AND_CLEAR,
                                        &eventFlag, CYU3P_WAIT_FOREVER);

        if (txApiRetStatus == CY_U3P_SUCCESS)
        {

            /* Based on the Bus Speed configure the DMA buffer size */
            if (glUsbSpeed == CY_U3P_FULL_SPEED)
            {
                dmaConfig.size = 64;
                glMscSectorSize = 64;     /* Sector size */
                glMscMaxSectors = (CY_FX_MSC_CARD_CAPACITY/64);   /* Maximum no. of sectors on the storage device */
            }
            else if (glUsbSpeed == CY_U3P_HIGH_SPEED)
            {
                dmaConfig.size = 512;
                glMscSectorSize = 512;    /* Sector size */
                glMscMaxSectors = (CY_FX_MSC_CARD_CAPACITY/512);   /* Maximum no. of sectors on the storage device */
            }
            else if (glUsbSpeed == CY_U3P_SUPER_SPEED)
            {
                dmaConfig.size = 1024;
                glMscSectorSize = 1024;   /* Sector size */
                glMscMaxSectors = (CY_FX_MSC_CARD_CAPACITY/1024);   /* Maximum no. of sectors on the storage device */
            }
            else
            {
                /* Error Handling */
                CyU3PDebugPrint (4, "Error! USB Not connected\n");
                CyFxAppErrorHandler(CY_U3P_ERROR_INVALID_CONFIGURATION);
            }

            /* Create a DMA Manual IN channel between USB Producer socket
               and the CPU */
            /* DMA size is set above based on the USB Bus Speed */
            dmaConfig.count = CY_FX_MSC_DMA_BUF_COUNT;
            dmaConfig.prodSckId = (CyU3PDmaSocketId_t)(CY_U3P_UIB_SOCKET_PROD_0 | CY_FX_MSC_EP_BULK_OUT_SOCKET);
            dmaConfig.consSckId = CY_U3P_CPU_SOCKET_CONS;
            dmaConfig.dmaMode = CY_U3P_DMA_MODE_BYTE;
            dmaConfig.notification = 0;
            dmaConfig.cb = NULL;
            dmaConfig.prodHeader = 0;
            dmaConfig.prodFooter = 0;
            dmaConfig.consHeader = 0;
            dmaConfig.prodAvailCount = 0;

            /* Create the channel */
            apiRetStatus = CyU3PDmaChannelCreate (&glChHandleMscIn,
                                                  CY_U3P_DMA_TYPE_MANUAL_IN,
                                                  &dmaConfig);
            if (apiRetStatus != CY_U3P_SUCCESS)
            {
                /* Error handling */
                CyU3PDebugPrint (4, "DMA IN Channel Creation Failed, Error Code = %d\n",apiRetStatus);
                CyFxAppErrorHandler(apiRetStatus);
            }

            /* Create a DMA Manual OUT channel between CPU and USB consumer socket */
            dmaConfig.count = CY_FX_MSC_DMA_BUF_COUNT;
            dmaConfig.prodSckId = CY_U3P_CPU_SOCKET_PROD;
            dmaConfig.consSckId = (CyU3PDmaSocketId_t)(CY_U3P_UIB_SOCKET_CONS_0 | CY_FX_MSC_EP_BULK_IN_SOCKET);
            dmaConfig.dmaMode = CY_U3P_DMA_MODE_BYTE;
            dmaConfig.cb = NULL;
            dmaConfig.prodHeader = 0;
            dmaConfig.prodFooter = 0;
            dmaConfig.consHeader = 0;
            dmaConfig.prodAvailCount = 0;

            /* Create the channel */
            apiRetStatus = CyU3PDmaChannelCreate (&glChHandleMscOut,
                                                  CY_U3P_DMA_TYPE_MANUAL_OUT,
                                                  &dmaConfig);
            if (apiRetStatus != CY_U3P_SUCCESS)
            {
                /* Error handling */
                CyU3PDebugPrint (4, "DMA OUT Channel Creation Failed, Error Code = %d\n",apiRetStatus);
                CyFxAppErrorHandler(apiRetStatus);
            }

            /* Flush the Endpoint memory */
            CyU3PUsbFlushEp(CY_FX_MSC_EP_BULK_OUT);
            CyU3PUsbFlushEp(CY_FX_MSC_EP_BULK_IN);
        }

        /* Initialize the DMA IN buffer members */
        dmaMscInBuffer.buffer = glMscInBuffer;
        dmaMscInBuffer.status = 0;
        dmaMscInBuffer.count = 0;
        dmaMscInBuffer.size = dmaConfig.size;

        for (;;)
        {
            if (glInPhaseError == CyTrue)
            {
                continue;
            }

            /* Setup IN buffer */
            apiRetStatus = CyU3PDmaChannelSetupRecvBuffer (&glChHandleMscIn, &dmaMscInBuffer);
            if (apiRetStatus == CY_U3P_SUCCESS)
            {
                /* Wait for CBW received on the IN buffer */
                apiRetStatus = CyU3PDmaChannelWaitForRecvBuffer (&glChHandleMscIn,&dmaMscInBuffer, CYU3P_WAIT_FOREVER);
                if (apiRetStatus == CY_U3P_SUCCESS)
                {
                    /* Validate command */
                    cswReturnStatus = CyFxMscCheckScsiCmd(glMscInBuffer, dmaMscInBuffer.count);
                    if (cswReturnStatus == CY_FX_CBW_CMD_PASSED)
                    {
                        /* Save the tags in the CSW */
                        glMscCswStatus[4] = glMscInBuffer[4];
                        glMscCswStatus[5] = glMscInBuffer[5];
                        glMscCswStatus[6] = glMscInBuffer[6];
                        glMscCswStatus[7] = glMscInBuffer[7];

                        /* Parse the SCSI command and execute the command */
                        cswReturnStatus = CyFxMscParseScsiCmd(glMscInBuffer);
                    }

                    CyFxMscSendCsw (cswReturnStatus);
                }
            }

            /* Check for Reset conditions */
            /* Reset due to USB Reset Event or USB Cable Unplug */
            if ((apiRetStatus != CY_U3P_SUCCESS) || (cswReturnStatus == CY_FX_CBW_CMD_MSC_RESET))
            {
                /* Stall both the IN and OUT Endpoints. */
                CyU3PUsbStall (CY_FX_MSC_EP_BULK_OUT, 1, 0);
                CyU3PUsbStall (CY_FX_MSC_EP_BULK_IN, 1, 0);
                /* Reset the DMA channels */
                /* Reset the IN channel */
                CyU3PDmaChannelReset(&glChHandleMscIn);

                /* Reset the OUT Channel */
                CyU3PDmaChannelReset(&glChHandleMscOut);

                /* Request Sense Index */
                glReqSenseIndex = CY_FX_MSC_SENSE_DEVICE_RESET;
            }

            /* Check if USB Disconnected */
            if (glUsbSpeed == CY_U3P_NOT_CONNECTED)
            {
                /* Destroy the IN channel */
                CyU3PDmaChannelDestroy (&glChHandleMscIn);

                /* Destroy the OUT Channel */
                CyU3PDmaChannelDestroy (&glChHandleMscOut);

                /* Request Sense Index */
                glReqSenseIndex = CY_FX_MSC_SENSE_DEVICE_RESET;

                break;
            }
        }
    }
}

/* MSC application error handler */
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
        /* Thread Sleep : 100 ms */
        CyU3PThreadSleep (100);
    }
}


/* Callback to handle the USB Setup Requests and Mass Storage Class requests */
CyBool_t
CyFxMscApplnUSBSetupCB (
        uint32_t setupdat0, /* SETUP Data 0 */
        uint32_t setupdat1  /* SETUP Data 1 */
    )
{
    CyBool_t mscHandleReq = CyFalse;
    uint8_t maxLun = 0;
    CyU3PReturnStatus_t apiRetStatus;
    uint32_t txApiRetStatus;
    CyU3PEpConfig_t endPointConfig;

    uint8_t  bRequest, bReqType;
    uint8_t  bType, bTarget;
    uint16_t wValue, wIndex, wLength; /* Decode the fields from the setup request. */

    bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bType    = (bReqType & CY_U3P_USB_TYPE_MASK);
    bTarget  = (bReqType & CY_U3P_USB_TARGET_MASK);
    bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue   = ((setupdat0 & CY_U3P_USB_VALUE_MASK)   >> CY_U3P_USB_VALUE_POS);
    wIndex   = ((setupdat1 & CY_U3P_USB_INDEX_MASK)   >> CY_U3P_USB_INDEX_POS);
    wLength  = ((setupdat1 & CY_U3P_USB_LENGTH_MASK)  >> CY_U3P_USB_LENGTH_POS);

    /* Check for Set Configuration request */
    if (bType == CY_U3P_USB_STANDARD_RQT)
    {
        /* Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
         * requests here. It should be allowed to pass if the device is in configured
         * state and failed otherwise. */
        if ((bTarget == CY_U3P_USB_TARGET_INTF) && ((bRequest == CY_U3P_USB_SC_SET_FEATURE)
                    || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) && (wValue == 0))
        {
            if (setConfHandled)
                CyU3PUsbAckSetup ();
            else
                CyU3PUsbStall (0, CyTrue, CyFalse);

            mscHandleReq = CyTrue;
        }

        /* Check for Set Configuration request */
        if (bRequest == CY_U3P_USB_SC_SET_CONFIGURATION)
        {
            /* Check if Set Config event is handled */
            if (setConfHandled == CyFalse )
            {
                setConfHandled = CyTrue;

                /* Configure the endpoints based on the USB speed */

                /* Get the Bus speed */
                glUsbSpeed = CyU3PUsbGetSpeed();

                /* Based on the Bus Speed configure the endpoint packet size */
                if (glUsbSpeed == CY_U3P_FULL_SPEED)
                {
                    endPointConfig.pcktSize = 64;
                }
                else if (glUsbSpeed == CY_U3P_HIGH_SPEED)
                {
                    endPointConfig.pcktSize = 512;
                }
                else if (glUsbSpeed == CY_U3P_SUPER_SPEED)
                {
                    endPointConfig.pcktSize = 1024;
                }
                else
                {
                    /* Error Handling */
                    CyU3PDebugPrint (4, "Error! USB Not connected\n");
                }

                /* Set the Endpoint Configurations */

                /* Producer Endpoint configuration */
                endPointConfig.enable = 1;
                endPointConfig.epType = CY_U3P_USB_EP_BULK;
                endPointConfig.streams = 0;
                endPointConfig.burstLen = 1;

                /* Configure the Endpoint */
                apiRetStatus = CyU3PSetEpConfig(CY_FX_MSC_EP_BULK_OUT,&endPointConfig);
                if (apiRetStatus != CY_U3P_SUCCESS)
                {
                    /* Error Handling */
                    CyU3PDebugPrint (4, "USB Set Endpoint config failed, Error Code = %d\n",apiRetStatus);
                    CyFxAppErrorHandler(apiRetStatus);
                }

                /* Consumer Endpoint configuration */
                endPointConfig.enable = 1;
                endPointConfig.epType = CY_U3P_USB_EP_BULK;
                endPointConfig.streams = 0;
                endPointConfig.burstLen = 1;

                /* Configure the Endpoint */
                apiRetStatus = CyU3PSetEpConfig(CY_FX_MSC_EP_BULK_IN,&endPointConfig);
                if (apiRetStatus != CY_U3P_SUCCESS)
                {
                    /* Error Handling */
                    CyU3PDebugPrint (4, "USB Set Endpoint config failed, Error Code = %d\n",apiRetStatus);
                    CyFxAppErrorHandler(apiRetStatus);
                }

                /* Set Event to indicate Set Configuration */
                txApiRetStatus = CyU3PEventSet(&glMscAppEvent,CY_FX_MSC_SET_CONFIG_EVENT,CYU3P_EVENT_OR);
                if (txApiRetStatus != CY_U3P_SUCCESS)
                {
                    /* Error handling */
                    CyU3PDebugPrint (4, "Bulk Loop App Set Event Failed, Error Code = %d\n",txApiRetStatus);
                }
            }

        }
        else if (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)
        {
            if (glInPhaseError)
            {
                /* Acknowledge the Setup Request */
                CyU3PUsbAckSetup ();
                return CyTrue;
            }
            /* Check Clear Feature on IN EP */
            if (wIndex == CY_FX_MSC_EP_BULK_IN)
            {
                /* Clear stall */
                CyU3PUsbStall(CY_FX_MSC_EP_BULK_IN,CyFalse,CyTrue);

                mscHandleReq = CyTrue;

                /* Acknowledge the Setup Request */
                CyU3PUsbAckSetup ();
            }

            /* Check Clear Feature on OUT EP */
            if (wIndex == CY_FX_MSC_EP_BULK_OUT)
            {
                /* Clear stall */
                CyU3PUsbStall(CY_FX_MSC_EP_BULK_OUT,CyFalse,CyTrue);

                mscHandleReq = CyTrue;

                /* Acknowledge the Setup Request */
                CyU3PUsbAckSetup ();
            }
        }
        else
        {
            /* All other requests are ignored by application */
            /* No operation */
        }
    }
    /* Check for MSC Requests */
    else if (bType == CY_FX_MSC_USB_CLASS_REQ)
    {
        mscHandleReq = CyFalse;
        apiRetStatus = CY_U3P_SUCCESS;

        if ((bTarget == CY_U3P_USB_TARGET_INTF) &&
                (wIndex == CY_FX_USB_MSC_INTF) && (wValue == 0))
        {
            /* Get MAX LUN Request */
            if (bRequest == CY_FX_MSC_GET_MAX_LUN_REQ)
            {
                if (wLength == 1)
                {
                    mscHandleReq = CyTrue;
                    /* Send response */
                    apiRetStatus = CyU3PUsbSendEP0Data(0x01, &maxLun);
                    if (apiRetStatus != CY_U3P_SUCCESS)
                    {
                        /* Error handling */
                        CyU3PDebugPrint (4, "Send EP0 Data Failed, Error Code = %d\n",apiRetStatus);
                    }
                }
            }
            /* BOT Reset Request */
            else if (bRequest == CY_FX_MSC_BOT_RESET_REQ)
            {
                mscHandleReq = CyTrue;
                glInPhaseError = CyFalse;
                if (wLength == 0)
                {
                    CyU3PUsbAckSetup ();
                    CyU3PDmaChannelReset(&glChHandleMscOut);
                    CyU3PDmaChannelReset(&glChHandleMscIn);

                    CyU3PUsbFlushEp (CY_FX_MSC_EP_BULK_OUT);
                    CyU3PUsbStall (CY_FX_MSC_EP_BULK_OUT,  CyFalse, CyTrue);

                    CyU3PUsbFlushEp (CY_FX_MSC_EP_BULK_IN);
                    CyU3PUsbStall (CY_FX_MSC_EP_BULK_IN,  CyFalse, CyTrue);

                    /* Request Sense Index */
                    glReqSenseIndex = CY_FX_MSC_SENSE_DEVICE_RESET;
                }
                else
                {
                     CyU3PUsbStall (0x00, CyTrue, CyFalse);
                }
            }
            else
            {
                /* No operation */
            }
        }

        if ((mscHandleReq == CyFalse) || (apiRetStatus != CY_U3P_SUCCESS))
        {
            /* This is a failed handling. Stall the EP. */
            CyU3PUsbStall (0, CyTrue, CyFalse);
        }
    }
    else
    {
        /* No operation */
    }

    return mscHandleReq;
}

/* This function validates the CBW received w.r.t signature and CBW length */
CyFxMscCswReturnStatus_t
CyFxMscCheckScsiCmd(
    uint8_t *buffer, /* Pointer to the CBW buffer */
    uint16_t count   /* Length of the CBW */
    )
{
    CyFxMscCswReturnStatus_t  retStatus = CY_FX_CBW_CMD_PASSED;

    /* Verify signature */
    if (buffer[0] != 'U' ||
        buffer[1] != 'S' ||
        buffer[2] != 'B' ||
        buffer[3] != 'C')
    {
        retStatus = CY_FX_CBW_CMD_PHASE_ERROR;
    }
    else
    {
        /* Verify count */
        if (count != CY_FX_MSC_CBW_MAX_COUNT)
        {
            retStatus = CY_FX_CBW_CMD_PHASE_ERROR;
        }
    }

    return retStatus;
}

/* This function frames the CSW and sends it to the host */
CyFxMscCswReturnStatus_t
CyFxMscSendCsw(
    CyFxMscCswReturnStatus_t cswReturnStatus    /* Command Status */
    )
{
    /* Update the residue length in the CSW */
    glMscCswStatus[11] = (uint8_t)((glCswDataResidue & 0xFF000000) >> 24);
    glMscCswStatus[10] = (uint8_t)((glCswDataResidue & 0x00FF0000) >> 16);
    glMscCswStatus[9] = (uint8_t)((glCswDataResidue & 0x0000FF00) >> 8);
    glMscCswStatus[8] = (uint8_t)(glCswDataResidue & 0x000000FF);

    /* Update the status in the CSW */
    glMscCswStatus[12] = (uint8_t)(cswReturnStatus & 0x3);

    /* Check for phase error */
    if (cswReturnStatus == CY_FX_CBW_CMD_PHASE_ERROR)
    {
        glInPhaseError = CyTrue;

        CyU3PUsbFlushEp(CY_FX_MSC_EP_BULK_IN);
        CyU3PUsbFlushEp(CY_FX_MSC_EP_BULK_OUT);
        /* Stall the IN endpoint */
        CyU3PUsbStall(CY_FX_MSC_EP_BULK_IN, CyTrue, CyFalse);

        /* Stall the OUT endpoint */
        CyU3PUsbStall(CY_FX_MSC_EP_BULK_OUT, CyTrue, CyFalse);

        CyU3PDmaChannelReset(&glChHandleMscIn);
        CyU3PDmaChannelReset(&glChHandleMscOut);
    }
    /* Check for Command failed */
    else if (cswReturnStatus == CY_FX_CBW_CMD_FAILED)
    {
        /* Only when data is expected or to be sent stall the EPs */
        if (glDataTxLength != 0)
        {
            /* Check direction */
            if (glCmdDirection == 0x00)
            {
                /* Stall the OUT endpoint */
                CyU3PUsbStall(CY_FX_MSC_EP_BULK_OUT, CyTrue, CyFalse);
            }
            else
            {
                CyU3PUsbFlushEp(CY_FX_MSC_EP_BULK_IN);
                /* Stall the IN endpoint */
                CyU3PUsbStall(CY_FX_MSC_EP_BULK_IN, CyTrue, CyFalse);
                CyU3PDmaChannelReset(&glChHandleMscIn);
            }
        }
    }
    else
    {
        /* No operation. Proceed to send the status */
    }

    /* Send the status to the host */
    if (CyFxMscSendUSBData(glMscCswStatus, CY_FX_MSC_CSW_MAX_COUNT) != CY_U3P_SUCCESS)
    {
        cswReturnStatus = CY_FX_CBW_CMD_MSC_RESET;
    }

    /* Return status */
    return cswReturnStatus;
}

/* This is wrapper funtion to send the USB data to the host from the give data area */
CyU3PReturnStatus_t
CyFxMscSendUSBData(
    uint8_t *data,      /* Pointer to the Send data */
    uint32_t length     /* Length of data to be sent */
    )
{
    CyU3PDmaBuffer_t dmaMscOutBuffer;
    CyU3PReturnStatus_t apiRetStatus;

    /* Prepare the DMA Buffer */
    dmaMscOutBuffer.buffer = data;
    dmaMscOutBuffer.status = 0;
    if (glUsbSpeed == CY_U3P_FULL_SPEED)
    {
        dmaMscOutBuffer.size = 64;
    }
    else if (glUsbSpeed == CY_U3P_HIGH_SPEED)
    {
        dmaMscOutBuffer.size = 512;
    }
    else if (glUsbSpeed == CY_U3P_SUPER_SPEED)
    {
        dmaMscOutBuffer.size = 1024;
    }
    else
    {
        CyU3PDebugPrint (4, "USB Not connected\n");
        return CY_U3P_ERROR_FAILURE;
    }

    dmaMscOutBuffer.count = length;

    /* Setup OUT buffer to send the data */
    /* OUT buffer setup when there is no abort */
    apiRetStatus = CyU3PDmaChannelSetupSendBuffer (&glChHandleMscOut, &dmaMscOutBuffer);
    if (apiRetStatus == CY_U3P_SUCCESS)
    {
        /* Wait for data to be sent */
        apiRetStatus = CyU3PDmaChannelWaitForCompletion (&glChHandleMscOut,CYU3P_WAIT_FOREVER);
    }

    /* Return Status */
    return apiRetStatus;
}

/* This is wrapper funtion to receive the USB data from the host and store into the give data area */
CyU3PReturnStatus_t
CyFxMscReceiveUSBData (
    uint8_t *data   /* Pointer to the receive data */
    )
{
    CyU3PDmaBuffer_t dmaMscInBuffer;
    CyU3PReturnStatus_t apiRetStatus;

    /* Prepare the DMA buffer */
    dmaMscInBuffer.buffer = data;
    dmaMscInBuffer.status = 0;
    if (glUsbSpeed == CY_U3P_FULL_SPEED)
    {
        dmaMscInBuffer.size = 64;
    }
    else if (glUsbSpeed == CY_U3P_HIGH_SPEED)
    {
        dmaMscInBuffer.size = 512;
    }
    else if (glUsbSpeed == CY_U3P_SUPER_SPEED)
    {
        dmaMscInBuffer.size = 1024;
    }
    else
    {
        CyU3PDebugPrint (4, "USB Not connected\n");
        return CY_U3P_ERROR_FAILURE;
    }

    /* Setup IN buffer to receive the data */
    apiRetStatus  = CyU3PDmaChannelSetupRecvBuffer (&glChHandleMscIn, &dmaMscInBuffer);
    if (apiRetStatus == CY_U3P_SUCCESS)
    {
        /* Wait for data to be received */
        apiRetStatus  = CyU3PDmaChannelWaitForRecvBuffer (&glChHandleMscIn,&dmaMscInBuffer, CYU3P_WAIT_FOREVER);
    }
    /* Return Status */
    return apiRetStatus;
}

/* This function parses the CBW for the SCSI commands and services the command */
CyFxMscCswReturnStatus_t
CyFxMscParseScsiCmd(
    uint8_t *mscCbw     /* Pointer to the CBW array */
    )
{
    CyFxMscCswReturnStatus_t retParseStatus = CY_FX_CBW_CMD_PASSED;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    uint32_t dataTxLength;
    uint8_t scsiCmd;
    uint32_t allocLength;
    uint32_t mscLba;
    uint16_t mscSector;
    uint8_t idx, numBytes;

    uint32_t temp = 0;

    glCswDataResidue = 0;
    /* Determine datalength */
    dataTxLength = ( ((uint32_t)mscCbw[11] << 24) |
                     ((uint32_t)mscCbw[10] << 16) |
                     ((uint32_t)mscCbw[9] << 8) |
                     ((uint32_t)mscCbw[8])
                   );

    /* Save the data length */
    /* This will be used in case of failures to Stall EPs */
    glDataTxLength = dataTxLength;

    /* Retrieve the SCSI command */
    scsiCmd = mscCbw[15];

    /* Verify if the direction  bit is valid for the command */
    /* Ignore the direction when Tx length is 0 */
    if (dataTxLength != 0)
    {
        /* Determine direction */
        glCmdDirection = ((mscCbw[12] & 0x80) >> 7);
        glCswDataResidue = glDataTxLength;

        /* Check for phase error */
        retParseStatus = CyFxCheckCmdDirection(scsiCmd,glCmdDirection);
    }

    /* Execute commands when there is no phase error */
    if (retParseStatus == CY_FX_CBW_CMD_PASSED)
    {
        /* Execute the command */
        switch (scsiCmd)
        {
            case CY_FX_MSC_SCSI_INQUIRY:
            {
                /* Get the allocation length */
                allocLength = ((uint16_t)(mscCbw[15 + 3] >> 8) |  (uint16_t)(mscCbw[15 + 4]));

                /* If data length requested is zero then no operation */
                if ((dataTxLength != 0) &&  (allocLength != 0))
                {
                    /* Update the residue length */
                    if (dataTxLength < allocLength )
                    {
                        glCswDataResidue = (uint32_t)(allocLength - dataTxLength);
                    }
                    else
                    {
                        glCswDataResidue = (uint32_t)(dataTxLength - allocLength);
                    }

                    /* Construct the Inquiry response */
                    /* Return the standard Inquiry data */
                    apiRetStatus = CyFxMscSendUSBData(CyFxMscScsiInquiryData, allocLength);
                }

                /* Set sense index to OK */
                glReqSenseIndex = CY_FX_MSC_SENSE_OK;

                break;
            }
            case CY_FX_MSC_SCSI_READ_FORMAT_CAPACITY:
            case  CY_FX_MSC_SCSI_READ_CAPACITY:
            {
                 /* Check transfer length */
                 if (dataTxLength != 0)
                 {
                     idx = 0;
                     numBytes = 8;

                     /* Clear the Out memory */
                    CyU3PMemSet (glMscOutBuffer, 0, 12);

                    /* Check for Read Format Capacity */
                    if (scsiCmd == CY_FX_MSC_SCSI_READ_FORMAT_CAPACITY)
                    {
                        glMscOutBuffer[0+idx] = 0x00;
                        glMscOutBuffer[1+idx] = 0x00;
                        glMscOutBuffer[2+idx] = 0x00;
                        glMscOutBuffer[3+idx] = 0x08;
                        idx = 4;
                        numBytes = 12;
                    }

                    /* Report to the host the capacity of the device */
                    /* Report the LBA */
                    glMscOutBuffer[0+idx] = (uint8_t)((glMscMaxSectors & 0xFF000000) >> 24);
                    glMscOutBuffer[1+idx] = (uint8_t)((glMscMaxSectors & 0x00FF0000) >> 16);
                    glMscOutBuffer[2+idx] = (uint8_t)((glMscMaxSectors & 0x0000FF00) >> 8);
                    glMscOutBuffer[3+idx] = (uint8_t)(glMscMaxSectors & 0x000000FF);

                    /* Check for Read Format Capacity */
                    /* Report bytes per sector */
                    if (scsiCmd == CY_FX_MSC_SCSI_READ_FORMAT_CAPACITY)
                    {
                        /* Indicate format flag */
                        glMscOutBuffer[4+idx] = 0x02;
                    }
                    else
                    {
                        glMscOutBuffer[4+idx] = 0x00;
                    }
                    glMscOutBuffer[5+idx] = 0x00;
                    glMscOutBuffer[6+idx] = (uint8_t)((glMscSectorSize & 0xFF00) >> 8); /* Sector Size */
                    glMscOutBuffer[7+idx] = (uint8_t)(glMscSectorSize & 0x00FF);

                    /* Update the residue length */
                    if (dataTxLength < numBytes )
                    {
                        glCswDataResidue = (uint32_t)(numBytes - dataTxLength);
                    }
                    else
                    {
                        glCswDataResidue = (uint32_t)(dataTxLength - numBytes);
                    }

                    /* Send data to USB */
                    apiRetStatus = CyFxMscSendUSBData(glMscOutBuffer, numBytes);

                    /* Set sense index to OK */
                    glReqSenseIndex = CY_FX_MSC_SENSE_OK;
                }
                else
                {
                    /* Command Failed */
                    retParseStatus = CY_FX_CBW_CMD_FAILED;

                    /* Set the Residue */
                    glCswDataResidue = dataTxLength;

                    /* Set sense index to INVALID FIELD IN CBW */
                    glReqSenseIndex = CY_FX_MSC_SENSE_INVALID_FIELD_IN_CBW;
                }
                break;
            }

            case CY_FX_MSC_SCSI_REQUEST_SENSE:
            {
                /* Check transfer length */
                if (dataTxLength != 0)
                {
                    /* Clear the response array */
                    CyU3PMemSet (glMscOutBuffer, 0, 18);

                    /* Report Sense codes */
                    glMscOutBuffer[0] = 0x70; /* Current errors */
                    glMscOutBuffer[1] = 0x00;
                    glMscOutBuffer[2] = glReqSenseCode[glReqSenseIndex][0]; /* SK */
                    glMscOutBuffer[7] = 0x0A; /* Length of following data */
                    glMscOutBuffer[12] = glReqSenseCode[glReqSenseIndex][1]; /* ASC */
                    glMscOutBuffer[13] = glReqSenseCode[glReqSenseIndex][2]; /* ASCQ */

                    /* Set sense index to OK */
                    glReqSenseIndex = CY_FX_MSC_SENSE_OK;

                    temp = dataTxLength >= 18 ? 18 : dataTxLength;
                    /* Send data to USB */
                    apiRetStatus = CyFxMscSendUSBData (glMscOutBuffer, temp);
                    glCswDataResidue = (uint32_t)(dataTxLength - temp);

                    if (glCswDataResidue > 18)
                    {
                        apiRetStatus = CY_FX_CBW_CMD_FAILED;
                    }
                }
                else
                {
                    /* Command Failed */
                    retParseStatus = CY_FX_CBW_CMD_FAILED;

                    /* Set the Residue */
                    glCswDataResidue = dataTxLength;

                    /* Set sense index to INVALID FIELD IN CBW */
                    glReqSenseIndex = CY_FX_MSC_SENSE_INVALID_FIELD_IN_CBW;
                }
                break;
            }

            case CY_FX_MSC_SCSI_FORMAT_UNIT:
            case CY_FX_MSC_SCSI_START_STOP_UNIT:
            case CY_FX_MSC_SCSI_TEST_UNIT_READY:
            {
                /* Check transfer length */
                if (dataTxLength != 0)
                {
                    /* Command Failed */
                    retParseStatus = CY_FX_CBW_CMD_FAILED;

                    /* Set the Residue */
                    glCswDataResidue = dataTxLength;

                    /* Set sense index to INVALID FIELD IN CBW */
                    glReqSenseIndex = CY_FX_MSC_SENSE_INVALID_FIELD_IN_CBW;
                }
                else
                {
                    /* Set sense index to OK */
                    glReqSenseIndex = CY_FX_MSC_SENSE_OK;

                    /* Clear Residue */
                    glCswDataResidue = 0;
                }
                break;
            }

            case CY_FX_MSC_SCSI_MODE_SENSE_6:
            {
                /* Check transfer length */
                if (dataTxLength != 0)
                {
                    /* Report Sense codes */
                    glMscOutBuffer[0] = 0x03;
                    glMscOutBuffer[1] = 0x00;
                    glMscOutBuffer[2] = 0x00;
                    glMscOutBuffer[3] = 0x00;

                    /* Set sense index to OK */
                    glReqSenseIndex = CY_FX_MSC_SENSE_OK;

                    /* Update the residue length */
                    if (dataTxLength < 4 )
                    {
                        glCswDataResidue = (uint32_t)(4 - dataTxLength);
                    }
                    else
                    {
                        glCswDataResidue = (uint32_t)(dataTxLength - 4);
                    }

                    /* Send data to USB */
                    apiRetStatus = CyFxMscSendUSBData(glMscOutBuffer, 4);
                }
                else
                {
                    /* Command Failed */
                    retParseStatus = CY_FX_CBW_CMD_FAILED;

                    /* Set the Residue */
                    glCswDataResidue = 0;

                    /* Set sense index to INVALID FIELD IN CBW */
                    glReqSenseIndex = CY_FX_MSC_SENSE_INVALID_FIELD_IN_CBW;
                }

                break;
            }

            case CY_FX_MSC_SCSI_PREVENT_ALLOW_MEDIUM:
            {
                /* Check transfer length */
                /* Allow Medium removal only */
                if (dataTxLength != 0)
                {
                    /* Command Failed */
                    retParseStatus = CY_FX_CBW_CMD_FAILED;

                    /* Set the Residue */
                    glCswDataResidue = dataTxLength;

                    /* Set sense index to INVALID FIELD IN CBW */
                    glReqSenseIndex = CY_FX_MSC_SENSE_INVALID_FIELD_IN_CBW;
                }
                else
                {
                    /* Set sense index to OK */
                    glReqSenseIndex = CY_FX_MSC_SENSE_OK;

                    /* Clear residue */
                    glCswDataResidue = 0;
                }

                break;
            }

            case CY_FX_MSC_SCSI_READ_10:
            {
                /* Get the LBA */
                mscLba = (((uint32_t)mscCbw[15+2] << 24) |
                            ((uint32_t)mscCbw[15+3] << 16) |
                            ((uint32_t)mscCbw[15+4] << 8) |
                            ((uint32_t)mscCbw[15+5]));

                /* Get sector count */
                mscSector = (((uint32_t)mscCbw[15+7] << 8) |
                                 (uint32_t)mscCbw[15+8]);

                /* Check transfer length */
                if ((dataTxLength == 0) && (mscSector == 0))
                {
                    /* Set sense index to OK */
                    glReqSenseIndex = CY_FX_MSC_SENSE_OK;
                    /* Clear the Residue */
                    glCswDataResidue = 0;
                    break;
                }
                /* Check LBA and Sectors requested */
                if ((mscLba > glMscMaxSectors) || ((mscLba + mscSector) > (glMscMaxSectors+1)) ||
                        (((mscSector * glMscSectorSize)) != dataTxLength))
                {
                    /* Command Failed */
                    retParseStatus = CY_FX_CBW_CMD_FAILED;

                    /* Set the Residue */
                    glCswDataResidue = dataTxLength;

                    /* Set sense index to INVALID FIELD IN CBW */
                    glReqSenseIndex = CY_FX_MSC_SENSE_INVALID_FIELD_IN_CBW;

                    break;
                }

                /* Send the data blocks to USB one bye one */
                while ((mscSector > 0) && dataTxLength)
                {
                    /* Send data to USB */
                    apiRetStatus = CyFxMscSendUSBData(&glMscStorageDeviceMemory[((mscLba++)*glMscSectorSize)],
                                                      glMscSectorSize);
                    if (apiRetStatus != CY_U3P_SUCCESS)
                    {
                        /* Stop Sending further data */
                        break;
                    }
                    mscSector--;
                    dataTxLength -= glMscSectorSize;
                }

                /* Set the Residue */
                glCswDataResidue = dataTxLength;

                /* Set sense index to OK */
                glReqSenseIndex = CY_FX_MSC_SENSE_OK;

                break;
            }

            case CY_FX_MSC_SCSI_WRITE_10:
            {
                /* Get the LBA */
                mscLba = (((uint32_t)mscCbw[15+2] << 24) |
                            ((uint32_t)mscCbw[15+3] << 16) |
                            ((uint32_t)mscCbw[15+4] << 8) |
                            ((uint32_t)mscCbw[15+5]));

                /* Get sector count */
                mscSector =   (((uint32_t)mscCbw[15+7] << 8) |
                                 (uint32_t)mscCbw[15+8]);

                /* Check transfer length */
                if ((dataTxLength == 0) && (mscSector == 0))
                {
                    /* Set sense index to OK */
                    glReqSenseIndex = CY_FX_MSC_SENSE_OK;
                    /* Clear the Residue */
                    glCswDataResidue = 0;
                    break;
                }
                /* Check LBA */
                if ((mscLba > glMscMaxSectors) || ((mscLba + mscSector) > (glMscMaxSectors+1)) ||
                       (((mscSector * glMscSectorSize)) != dataTxLength))
                {
                    /* Command Failed */
                    retParseStatus = CY_FX_CBW_CMD_FAILED;

                    /* Set the Residue */
                    glCswDataResidue = dataTxLength;

                    /* Set sense index to INVALID FIELD IN CBW */
                    glReqSenseIndex = CY_FX_MSC_SENSE_INVALID_FIELD_IN_CBW;
                    break;
                }

                /* Send the data blocks to USB one bye one */
                while ((mscSector > 0) && dataTxLength)
                {
                    /* Receive data from USB */
                    apiRetStatus = CyFxMscReceiveUSBData(&glMscStorageDeviceMemory[((mscLba++)*glMscSectorSize)]);
                    if (apiRetStatus != CY_U3P_SUCCESS)
                    {
                        /* Stop receving further data */
                        break;
                    }
                    mscSector--;
                    dataTxLength -= glMscSectorSize;
                }
                
                /* Set the Residue */
                glCswDataResidue = dataTxLength;

                /* Set sense index to OK */
                glReqSenseIndex = CY_FX_MSC_SENSE_OK;

                break;

            }
            case CY_FX_MSC_SCSI_VERIFY_10:
            {

                /* This is spoofed command */

                /* Clear the residue */
                glCswDataResidue = 0;

                /* Set sense index to OK */
                glReqSenseIndex = CY_FX_MSC_SENSE_OK;

                break;
            }

            default :
            {
                /* Command Failed */
                retParseStatus = CY_FX_CBW_CMD_FAILED;

                /* Set the Residue */
                glCswDataResidue = dataTxLength;

                /* Set sense index to INVALID OP CODE */
                glReqSenseIndex = CY_FX_MSC_SENSE_INVALID_OP_CODE;

                break;
            }
        } /* End of switch (scsiCmd)... */
    } /* End of if (retParseStatus == CY_FX_CBW_CMD_PASSED)... */

    /* Check for API failure */
    /* API failures lead to MSC Reset */
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Update the return status */
        retParseStatus = CY_FX_CBW_CMD_FAILED;
    }

    return retParseStatus;
}

/* This function checks the direction bit and verifies against given command */
CyFxMscCswReturnStatus_t
CyFxCheckCmdDirection(
    uint8_t scsiCmd,        /* SCSI Command */
    uint8_t cmdDirection    /* Direction */
    )
{
    CyFxMscCswReturnStatus_t retStatus = CY_FX_CBW_CMD_PASSED;

    /* Check for IN or OUT command and verify direction */
    /* Phase error checked only for supported commands */
    if (cmdDirection == 0x00)
    {
        if ((scsiCmd == CY_FX_MSC_SCSI_INQUIRY) ||
            (scsiCmd == CY_FX_MSC_SCSI_READ_CAPACITY) ||
            (scsiCmd == CY_FX_MSC_SCSI_REQUEST_SENSE) ||
            (scsiCmd == CY_FX_MSC_SCSI_FORMAT_UNIT) ||
            (scsiCmd == CY_FX_MSC_SCSI_START_STOP_UNIT) ||
            (scsiCmd == CY_FX_MSC_SCSI_MODE_SENSE_6) ||
            (scsiCmd == CY_FX_MSC_SCSI_PREVENT_ALLOW_MEDIUM) ||
            (scsiCmd == CY_FX_MSC_SCSI_READ_10) ||
            (scsiCmd == CY_FX_MSC_SCSI_READ_FORMAT_CAPACITY) ||
            (scsiCmd == CY_FX_MSC_SCSI_VERIFY_10) ||
            (scsiCmd == CY_FX_MSC_SCSI_TEST_UNIT_READY))
        {
            retStatus = CY_FX_CBW_CMD_FAILED;
        }
    }
    else
    {
        if (scsiCmd == CY_FX_MSC_SCSI_WRITE_10)
        {
            retStatus = CY_FX_CBW_CMD_FAILED;
        }
    }

    return retStatus;
}

/* This is the Callback function to handle the USB Events */
void
CyFxMscApplnUSBEventCB (
    CyU3PUsbEventType_t evtype, /* Event type */
    uint16_t            evdata  /* Event data */
    )
{
    /* Check for Reset / Suspend / Disconnect / Connect Events */
    if ((evtype == CY_U3P_USB_EVENT_RESET) || (evtype == CY_U3P_USB_EVENT_SUSPEND) ||
        (evtype == CY_U3P_USB_EVENT_DISCONNECT) || (evtype == CY_U3P_USB_EVENT_CONNECT))
    {
        /* Abort the IN Channel */
        CyU3PDmaChannelAbort(&glChHandleMscIn);

        /* Abort the OUT Channel */
        CyU3PDmaChannelAbort(&glChHandleMscOut);

        /* Request Sense Index */
        glReqSenseIndex = CY_FX_MSC_SENSE_DEVICE_RESET;

        /* Init the USB Speed */
        glUsbSpeed = CY_U3P_NOT_CONNECTED;

        /* Clear Flag */
        setConfHandled = CyFalse;
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
CyFxMscApplnLPMRqtCB (
        CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}

/* This function initializes the Debug Module for the MSC Application */
void
CyFxMscApplnDebugInit()
{

    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t apiRetStatus;

    /* Initialize the UART for printing debug messages */
    apiRetStatus = CyU3PUartInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
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
}

/* This function initializes the USB Module, sets the enumeration descriptors,
   configures the Endpoints and configures the DMA module for the
   MSC Application */
void
CyFxMscApplnInit (void)
{
    CyU3PReturnStatus_t apiRetStatus;
    uint32_t txApiRetStatus;

    /* Create FX MSC events */
    txApiRetStatus = CyU3PEventCreate(&glMscAppEvent);
    if (txApiRetStatus != 0)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "MSC Appln Create Event Failed, Error Code = %d\n",txApiRetStatus);
    }

    /* Start the USB functionality */
    apiRetStatus = CyU3PUsbStart();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Function Failed to Start, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Setup the Callback to Handle the USB Setup Requests */
    /* Set Fast enumeration to False */
    CyU3PUsbRegisterSetupCallback(CyFxMscApplnUSBSetupCB, CyFalse);

    /* Setup the Callback to Handle the USB Events */
    CyU3PUsbRegisterEventCallback(CyFxMscApplnUSBEventCB);

    /* Register a callback to handle LPM requests from the USB 3.0 host. */
    CyU3PUsbRegisterLPMRequestCallback(CyFxMscApplnLPMRqtCB);
    /* Set the USB Enumeration descriptors */

    /* Device Descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB20DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Set Device Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Device Descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB30DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Set Device Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Device Qualifier Descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, NULL, (uint8_t *)CyFxUSBDeviceQualDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Set Device Qualifier Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Other Speed Descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBHSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Set Other Speed Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Configuration Descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBFSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Set Configuration Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Configuration Descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBSSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Set Configuration Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* BOS Descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, NULL, (uint8_t *)CyFxUSBBOSDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Set Configuration Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String Descriptor 0 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Set String Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String Descriptor 1 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Set String Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String Descriptor 2 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Set String Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String Descriptor 3 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 3, (uint8_t *)CyFxUSBSerialNumberDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Set String Descriptor failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Connect the USB Pins */
    /* Enable Super Speed operation */
    apiRetStatus = CyU3PConnectState(1,CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Connect failed, Error Code = %d\n",apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }
}

