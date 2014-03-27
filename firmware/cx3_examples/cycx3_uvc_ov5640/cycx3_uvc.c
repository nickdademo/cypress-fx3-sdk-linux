/*
## Cypress USB 3.0 Platform source file (cycx3_uvc.c)
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

/* This application example implemnets a USB UVC 1.1 compliant video camera on the CX3 using an
 * Omnivision OV5640 image sensor. The example provides uncompressed 16Bit YUV2 1920x1080 @30 fps 
 * and 1280x720 @60 fps streams over Super Speed USB, 640x480 @60fps and 320x240 @90fps over High-speed
 * USB and 320x240 @ 5fps over Full-speed USB connections.
 */
#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3i2c.h"
#include "cyu3uart.h"
#include "cyu3gpio.h"
#include "cyu3utils.h"
#include "cyu3pib.h"
#include "cyu3socket.h"
#include "sock_regs.h"
#include "cycx3_uvc.h"
#include "cyu3mipicsi.h"
#include "cyu3imagesensor.h"

/* Setup data field : Request */
#define CY_U3P_USB_REQUEST_MASK                 (0x0000FF00)
#define CY_U3P_USB_REQUEST_POS                  (8)


#define CX3_DMA_RESET_EVENT                     (1<<4)

/* MIPI Error event. Used to flag a MIPI-CSI2 bus error */
#define CX3_MIPI_ERROR_EVENT                    (1<<3)

/* Event generated on a USB Suspend Request*/
#define CX3_USB_SUSP_EVENT_FLAG                 (1<<5)


CyU3PThread uvcAppThread;               /* Primary application thread used for data transfer from the Mipi interface to USB*/
CyU3PThread uvcMipiErrorThread;         /* Thread used to poll the MIPI interface for Mipi bus errors */

CyU3PEvent glCx3Event;                  /* Application Event Group */

CyU3PEvent glMipiErrorEvent;            /* Application Event Group */

volatile int32_t glDMATxCount = 0;               /* Counter used to count the Dma Transfers */
volatile int32_t glDmaDone = 0;
volatile uint8_t glActiveSocket = 0;
CyBool_t glHitFV = CyFalse;             /* Flag used for state of FV signal. */
CyBool_t glMipiActive = CyFalse;        /* Flag set to true whin Mipi interface is active. Used for Suspend/Resume. */
CyBool_t glIsClearFeature = CyFalse;    /* Flag to signal when AppStop is called from the ClearFeature request. Need to Reset Toggle*/
volatile CyBool_t doLpmDisable = CyTrue;        /* Flag used to Enable/Disble low USB 3.0 LPM */

/* UVC Header */
uint8_t glUVCHeader[CX3_UVC_HEADER_LENGTH] =
{
    0x0C,                           /* Header Length */
    0x8C,                           /* Bit field header field */
    0x00,0x00,0x00,0x00,            /* Presentation time stamp field */
    0x00,0x00,0x00,0x00,0x00,0x00   /* Source clock reference field */
};

/* Video Probe Commit Control */
uint8_t glCommitCtrl[CX3_UVC_MAX_PROBE_SETTING_ALIGNED];
uint8_t glCurrentFrameIndex = 1;

CyU3PDmaMultiChannel glChHandleUVCStream;       /* DMA Channel Handle for UVC Stream  */
CyBool_t glIsApplnActive = CyFalse;             /* Whether the Mipi->USB application is active or not. */
CyBool_t glIsConfigured = CyFalse;              /* Whether Application is in configured state or not */
CyBool_t glIsStreamingStarted = CyFalse;        /* Whether streaming has started - Used for MAC OS support*/
#define RESET_TIMER_ENABLE 1

#ifdef RESET_TIMER_ENABLE
#define TIMER_PERIOD	(500)

static CyU3PTimer        UvcTimer;

    static void
UvcAppProgressTimer (
        uint32_t arg)
{
    /* This frame has taken too long to complete. 
     * Abort it, so that the next frame can be started. */
    CyU3PEventSet(&glCx3Event, CX3_DMA_RESET_EVENT,CYU3P_EVENT_OR);
}

#endif

/* Application critical error handler */
    void
CyCx3AppErrorHandler (
        CyU3PReturnStatus_t status        /* API return status */
        )
{
    /* Application failed with the error code status */

    /* Add custom debug or recovery actions here */

    /* Loop indefinitely */
    for (;;)
    {
        /* Thread sleep : 100 ms */
        CyU3PThreadSleep (100);
    }
}


/* UVC header addition function */
    static void
CyCx3UvcAddHeader (
        uint8_t *buffer_p,      /* Buffer pointer */
        uint8_t frameInd        /* EOF or normal frame indication */
        )
{
    /* Copy header to buffer */
    CyU3PMemCopy (buffer_p, (uint8_t *)glUVCHeader, CX3_UVC_HEADER_LENGTH);

    /* Check if last packet of the frame. */
    if (frameInd == CX3_UVC_HEADER_EOF)
    {
        /* Modify UVC header to toggle Frame ID */
        glUVCHeader[1] ^= CX3_UVC_HEADER_FRAME_ID;

        /* Indicate End of Frame in the buffer */
        buffer_p[1] |=  CX3_UVC_HEADER_EOF;
    }
}


/* This function starts the video streaming application. It is called
 * when there is a SET_INTERFACE event for alternate interface 1 
 * (in case of UVC over Isochronous Endpoint usage) or when a 
 * COMMIT_CONTROL(SET_CUR) request is received (when using BULK only UVC).
 */
    CyU3PReturnStatus_t 
CyCx3UvcApplnStart (void)
{
#ifdef CX3_DEBUG_ENABLED
    uint8_t SMState = 0;
#endif
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    glIsApplnActive = CyTrue;
    glDmaDone = 0;
    glDMATxCount = 0;
    glHitFV = CyFalse;
    doLpmDisable = CyTrue;
#ifdef RESET_TIMER_ENABLE
    CyU3PTimerStop (&UvcTimer);
#endif

    /* Place the EP in NAK mode before cleaning up the pipe. */
    CyU3PUsbSetEpNak (CX3_EP_BULK_VIDEO, CyTrue);
    CyU3PBusyWait (100);


    /* Reset USB EP and DMA */
    CyU3PUsbFlushEp(CX3_EP_BULK_VIDEO);
    status = CyU3PDmaMultiChannelReset (&glChHandleUVCStream);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4,"\n\rAplnStrt:ChannelReset Err = 0x%x", status);
        return status;
    }
    status = CyU3PDmaMultiChannelSetXfer (&glChHandleUVCStream, 0, 0);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAplnStrt:SetXfer Err = 0x%x", status);
        return status;
    }
    CyU3PUsbSetEpNak (CX3_EP_BULK_VIDEO, CyFalse);
    CyU3PBusyWait (200);



    /* Resume the Fixed Function GPIF State machine */
    CyU3PGpifSMControl(CyFalse);

    glActiveSocket = 0;
    CyU3PGpifSMSwitch(CX3_INVALID_GPIF_STATE, CX3_START_SCK0, 
            CX3_INVALID_GPIF_STATE, ALPHA_CX3_START_SCK0, CX3_GPIF_SWITCH_TIMEOUT);

    CyU3PThreadSleep(10);


#ifdef CX3_DEBUG_ENABLED
    CyU3PGpifGetSMState(&SMState);
    CyU3PDebugPrint (4, "\n\rAplnStrt:SMState = 0x%x",SMState);
#endif
    /* Wake Mipi interface and Image Sensor */
    CyU3PMipicsiWakeup();       
    CyCx3_ImageSensor_Wakeup();
    glMipiActive = CyTrue;

    CyCx3_ImageSensor_Trigger_Autofocus();
    return CY_U3P_SUCCESS;
}

/* This function stops the video streaming. It is called from the USB event
 * handler, when there is a reset / disconnect or SET_INTERFACE for alternate
 * interface 0 in case of ischronous implementation or when a Clear Feature (Halt) 
 * request is recieved (in case of bulk only implementation).
 */
    void
CyCx3UvcApplnStop(void)
{
#ifdef CX3_DEBUG_ENABLED
    uint8_t SMState = 0;
#endif
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Stop the image sensor and CX3 mipi interface */
    status = CyU3PMipicsiSleep();
    CyCx3_ImageSensor_Sleep();

    glMipiActive = CyFalse;
#ifdef RESET_TIMER_ENABLE
    CyU3PTimerStop (&UvcTimer);
#endif

#ifdef CX3_DEBUG_ENABLED
    CyU3PGpifGetSMState(&SMState);
    CyU3PDebugPrint (4, "\n\rAplnStop:SMState = 0x%x",SMState);
#endif

    /* Pause the GPIF interface*/
    CyU3PGpifSMControl(CyTrue);
    /* Update the flag so that the application thread is notified of this. */
    glIsApplnActive = CyFalse;
    
    CyU3PUsbSetEpNak (CX3_EP_BULK_VIDEO, CyTrue);
    CyU3PBusyWait (100);



    /* Abort and destroy the video streaming channel */
    /* Reset the channel: Set to DSCR chain starting point in PORD/CONS SCKT; set DSCR_SIZE field in DSCR memory*/
    status = CyU3PDmaMultiChannelReset(&glChHandleUVCStream);
    if (status != CY_U3P_SUCCESS)
    {
#ifdef CX3_DEBUG_ENABLED

        CyU3PDebugPrint (4,"\n\rAplnStop:ChannelReset Err = 0x%x",status);
#endif
    }
    CyU3PThreadSleep(25);

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp(CX3_EP_BULK_VIDEO);
    CyU3PUsbSetEpNak (CX3_EP_BULK_VIDEO, CyFalse);
    CyU3PBusyWait (200);
    /* Clear the stall condition and sequence numbers if ClearFeature. */
    if (glIsClearFeature)
    {
        CyU3PUsbStall (CX3_EP_BULK_VIDEO, CyFalse, CyTrue);
        glIsClearFeature = CyFalse;
    }
    glDMATxCount = 0;
    glDmaDone = 0;
    
    /* Enable USB 3.0 LPM */
    CyU3PUsbLPMEnable ();
}

/* GpifCB callback function is invoked when FV triggers GPIF interrupt */
    void
CyCx3GpifCB (
        CyU3PGpifEventType event,
        uint8_t currentState
        )
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    /* Handle interrupt from the State Machine */
    if (event == CYU3P_GPIF_EVT_SM_INTERRUPT)
    {
        /* Wrapup Socket 0*/
        if(currentState == CX3_PARTIAL_BUFFER_IN_SCK0)
        {
            status = CyU3PDmaMultiChannelSetWrapUp(&glChHandleUVCStream,0);
            if (status != CY_U3P_SUCCESS)
            {
                CyU3PDebugPrint (4, "\n\rGpifCB:WrapUp SCK0 Err = 0x%x",status);
                //CyCx3AppErrorHandler(status);
            }
        }
        /* Wrapup Socket 1 */
        else if(currentState == CX3_PARTIAL_BUFFER_IN_SCK1)
        {
            status = CyU3PDmaMultiChannelSetWrapUp(&glChHandleUVCStream,1);
            if (status != CY_U3P_SUCCESS)
            {
                CyU3PDebugPrint (4, "\n\rGpifCB:WrapUp SCK1 Err = 0x%x",status);
                //CyCx3AppErrorHandler(status);
            }
        }
    }
}


/* DMA callback function to handle the produce and consume events. */
    void
CyCx3UvcAppDmaCallback (
        CyU3PDmaMultiChannel   *chHandle,
        CyU3PDmaCbType_t  type,
        CyU3PDmaCBInput_t *input
        )
{
    CyU3PDmaBuffer_t DmaBuffer;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    if (type == CY_U3P_DMA_CB_PROD_EVENT)
    {
        /* This is a produce event notification to the CPU. This notification is
         * received upon reception of every buffer. The buffer will not be sent
         * out unless it is explicitly committed. The call shall fail if there
         * is a bus reset / usb disconnect or if there is any application error. */

        /* Disable USB 3.0 LPM while Buffer is being transmitted out*/
        if ((CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED) && (doLpmDisable))
        {
            CyU3PUsbLPMDisable ();
            CyU3PUsbSetLinkPowerState (CyU3PUsbLPM_U0);
            CyU3PBusyWait (200);

            doLpmDisable = CyFalse;
#ifdef RESET_TIMER_ENABLE
            CyU3PTimerStart (&UvcTimer);
#endif
        }

        status = CyU3PDmaMultiChannelGetBuffer(chHandle, &DmaBuffer, CYU3P_NO_WAIT);
        while (status == CY_U3P_SUCCESS)
        {
            /* Add Headers*/
            if(DmaBuffer.count < CX3_UVC_DATA_BUF_SIZE)
            {
                CyCx3UvcAddHeader ((DmaBuffer.buffer - CX3_UVC_PROD_HEADER), CX3_UVC_HEADER_EOF);
                glHitFV = CyTrue;
            }
            else
            {
                CyCx3UvcAddHeader ((DmaBuffer.buffer - CX3_UVC_PROD_HEADER), CX3_UVC_HEADER_FRAME);
            }
            /* Commit Buffer to USB*/

            status = CyU3PDmaMultiChannelCommitBuffer (chHandle, (DmaBuffer.count + 12), 0);
            if (status != CY_U3P_SUCCESS)
            {
                   CyU3PEventSet(&glCx3Event, CX3_DMA_RESET_EVENT,CYU3P_EVENT_OR);
                   break;                   
            }
            else
            {
                glDMATxCount++;
                glDmaDone++;
            }

            glActiveSocket ^= 1; /* Toggle the Active Socket */
            status = CyU3PDmaMultiChannelGetBuffer(chHandle, &DmaBuffer, CYU3P_NO_WAIT);
        }
    }
    else if(type == CY_U3P_DMA_CB_CONS_EVENT)
    {
        glDmaDone--;
        /* Check if Frame is completely transferred */
        glIsStreamingStarted = CyTrue;
        if((glHitFV == CyTrue) && (glDmaDone == 0))
        {
            glHitFV = CyFalse;

            glDMATxCount=0;
#ifdef RESET_TIMER_ENABLE
            CyU3PTimerStop (&UvcTimer);
#endif
            
            if (glActiveSocket)
                CyU3PGpifSMSwitch(CX3_INVALID_GPIF_STATE, CX3_START_SCK1, 
                        CX3_INVALID_GPIF_STATE, ALPHA_CX3_START_SCK1, CX3_GPIF_SWITCH_TIMEOUT);
            else
                CyU3PGpifSMSwitch(CX3_INVALID_GPIF_STATE, CX3_START_SCK0, 
                        CX3_INVALID_GPIF_STATE, ALPHA_CX3_START_SCK0, CX3_GPIF_SWITCH_TIMEOUT);
            
            CyU3PUsbLPMEnable ();
            doLpmDisable = CyTrue;
#ifdef RESET_TIMER_ENABLE
            CyU3PTimerModify (&UvcTimer, TIMER_PERIOD, 0);
#endif
        }
    }
}


/* This is the Callback function to handle the USB Events */
    static void
CyCx3UvcApplnUSBEventCB (
        CyU3PUsbEventType_t evtype,     /* Event type */
        uint16_t            evdata      /* Event data */
        )
{
    uint8_t interface = 0, altSetting = 0;

    switch (evtype)
    {
        case CY_U3P_USB_EVENT_SUSPEND:
            /* Suspend the device with Wake On Bus Activity set */
            glIsStreamingStarted = CyFalse;
            CyU3PEventSet (&glCx3Event, CX3_USB_SUSP_EVENT_FLAG, CYU3P_EVENT_OR);
            break;
        case CY_U3P_USB_EVENT_SETINTF:
            /* Start the video streamer application if the
             * interface requested was 1. If not, stop the
             * streamer. */
            interface = CY_U3P_GET_MSB(evdata);
            altSetting = CY_U3P_GET_LSB(evdata);
#if CX3_DEBUG_ENABLED
            CyU3PDebugPrint(4,"\n\rUsbCB: IF = %d, ALT = %d", interface, altSetting);
#endif
            glIsStreamingStarted = CyFalse;
            if ((altSetting == CX3_UVC_STREAM_INTERFACE) && (interface == 1))
            {
                /* Stop the application before re-starting. */
                if (glIsApplnActive)
                {
#if CX3_DEBUG_ENABLED
                    CyU3PDebugPrint (4, "\n\rUsbCB:Call AppStop");
#endif
                    CyCx3UvcApplnStop ();
                }
                CyCx3UvcApplnStart ();
                break;
            }

            /* Fall-through. */

        case CY_U3P_USB_EVENT_SETCONF:
        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_DISCONNECT:
        case CY_U3P_USB_EVENT_CONNECT:
            glIsStreamingStarted = CyFalse;
            if (evtype == CY_U3P_USB_EVENT_SETCONF)
                glIsConfigured = CyTrue;
            else
                glIsConfigured = CyFalse;

            /* Stop the video streamer application and enable LPM. */
            CyU3PUsbLPMEnable ();
            if (glIsApplnActive)
            {
#if CX3_DEBUG_ENABLED
                CyU3PDebugPrint (4, "\n\rUsbCB:Call AppStop");
#endif
                CyCx3UvcApplnStop ();
            }
            break;
        default:
            break;
    }
}

/* Callback for LPM requests. Always return true to allow host to transition device 
 * into required LPM state U1/U2/U3. When data trasmission is active LPM management 
 * is explicitly desabled to prevent data transmission errors. 
 */
static CyBool_t CyCX3ApplnLPMRqtCB (
        CyU3PUsbLinkPowerMode link_mode         /*USB 3.0 linkmode requested by Host */
        )
{
    return CyTrue;
}

/* Callback to handle the USB Setup Requests and UVC Class events */
    static CyBool_t
CyCx3UvcApplnUSBSetupCB (
        uint32_t setupdat0,     /* SETUP Data 0 */
        uint32_t setupdat1      /* SETUP Data 1 */
        )
{
    uint8_t  bRequest, bType,bRType, bTarget;
    uint16_t wValue, wIndex, wLength;
    uint16_t readCount = 0;
    uint8_t  ep0Buf[2];
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint8_t temp = 0;
    CyBool_t isHandled = CyFalse;

    /* Decode the fields from the setup request. */
    bRType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bType    = (bRType & CY_U3P_USB_TYPE_MASK);
    bTarget  = (bRType & CY_U3P_USB_TARGET_MASK);
    bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue   = ((setupdat0 & CY_U3P_USB_VALUE_MASK)   >> CY_U3P_USB_VALUE_POS);
    wIndex   = ((setupdat1 & CY_U3P_USB_INDEX_MASK)   >> CY_U3P_USB_INDEX_POS);
    wLength  = ((setupdat1 & CY_U3P_USB_LENGTH_MASK)  >> CY_U3P_USB_LENGTH_POS);

#if CX3_DEBUG_ENABLED
    CyU3PDebugPrint(4, "\n\rbRType = 0x%x, bRequest = 0x%x, wValue = 0x%x, wIndex = 0x%x, wLength= 0x%x",bRType, bRequest, wValue, wIndex, wLength);
#endif
    /* ClearFeature(Endpoint_Halt) received on the Streaming Endpoint. Stop Streaming */
    if((bTarget == CY_U3P_USB_TARGET_ENDPT) && (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE) 
            && (wIndex == CX3_EP_BULK_VIDEO) && (wValue == CY_U3P_USBX_FS_EP_HALT))
    {
        if ((glIsApplnActive) && (glIsStreamingStarted))
        {
            glIsClearFeature = CyTrue;
            CyCx3UvcApplnStop();
        }

        return CyFalse;
    }

    if( bRType == CY_U3P_USB_GS_DEVICE)
    {
        /* Make sure that we bring the link back to U0, so that the ERDY can be sent. */
        if (CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED)
            CyU3PUsbSetLinkPowerState (CyU3PUsbLPM_U0);
    }

    if ((bTarget == CY_U3P_USB_TARGET_INTF) && ((bRequest == CY_U3P_USB_SC_SET_FEATURE)
                || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) && (wValue == 0))
    {
#if CX3_DEBUG_ENABLED
        CyU3PDebugPrint (4, "\n\rStpCB:In SET_FTR %d::%d", glIsApplnActive, glIsConfigured);
#endif
        if (glIsConfigured)
        {
            CyU3PUsbAckSetup ();
        }
        else
        {
            CyU3PUsbStall (0, CyTrue, CyFalse);
        }
        return CyTrue;
    }

    if ((bRequest == CY_U3P_USB_SC_GET_STATUS) &&
            (bTarget == CY_U3P_USB_TARGET_INTF))
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
        return CyTrue;
    }

    /* Check for UVC Class Requests */
    if (bType == CY_U3P_USB_CLASS_RQT)
    {

        /* UVC Class Requests */
        /* Requests to the Video Streaming Interface (IF 1) */
        if((wIndex & 0x00FF) == CX3_UVC_STREAM_INTERFACE)
        {
            /* GET_CUR Request Handling Probe/Commit Controls*/
            if (bRequest == CX3_USB_UVC_GET_CUR_REQ)
            {
                isHandled = CyTrue;
                /* Host requests for probe data of 34 bytes (UVC 1.1) or 26 Bytes (UVC1.0). Send it over EP0. */
                if (CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED)
                {
                    if(glCurrentFrameIndex == 3)
                    {
                        CyU3PMemCopy(glProbeCtrl, (uint8_t *)gl5MpProbeCtrl, CX3_UVC_MAX_PROBE_SETTING);
                    }
                    /* Probe Control for 1280x720 stream*/
                    else if(glCurrentFrameIndex == 2)
                    {
                        CyU3PMemCopy(glProbeCtrl, (uint8_t *)gl720pProbeCtrl, CX3_UVC_MAX_PROBE_SETTING);
                    }
                    /* Probe Control for 1920x1080 stream*/
                    else
                    {
                        CyU3PMemCopy(glProbeCtrl, (uint8_t *)gl1080pProbeCtrl, CX3_UVC_MAX_PROBE_SETTING);
                    }
                }
                else if (CyU3PUsbGetSpeed () == CY_U3P_HIGH_SPEED)            
                {
                    /* Probe Control for 640x480 stream*/
                    CyU3PMemCopy(glProbeCtrl, (uint8_t *)glVga60ProbeCtrl, CX3_UVC_MAX_PROBE_SETTING);
                }
                else /* FULL-Speed*/
                {
                    /* Probe Control for 320x240 @5FPS stream*/
                    CyU3PMemCopy(glProbeCtrl, (uint8_t *)glVga60ProbeCtrl, CX3_UVC_MAX_PROBE_SETTING);
                }

                status = CyU3PUsbSendEP0Data(CX3_UVC_MAX_PROBE_SETTING, glProbeCtrl);
                if (status != CY_U3P_SUCCESS)
                {
                    CyU3PDebugPrint (4, "\n\rUSBStpCB:GET_CUR:SendEP0Data Err = 0x%x", status);
                }
            }
            /* SET_CUR request handling Probe/Commit controls */
            else if (bRequest == CX3_USB_UVC_SET_CUR_REQ)
            {
                isHandled = CyTrue;
                /* Get the UVC probe/commit control data from EP0 */
                status = CyU3PUsbGetEP0Data(CX3_UVC_MAX_PROBE_SETTING_ALIGNED,
                        glCommitCtrl, &readCount);
                if (status != CY_U3P_SUCCESS)
                {
                    CyU3PDebugPrint (4, "\n\rUSBStpCB:SET_CUR:GetEP0Data Err = 0x%x.", status);
                }
                /* Check the read count. Expecting a count of CX3_UVC_MAX_PROBE_SETTING bytes. */
                if (readCount > (uint16_t)CX3_UVC_MAX_PROBE_SETTING)
                {
                    CyU3PDebugPrint (4, "\n\rUSBStpCB:Invalid SET_CUR Rqt Len.");
                }
                else
                {
                    /* Set Probe Control */
                    if(wValue == CX3_UVC_VS_PROBE_CONTROL)
                    {
                        glCurrentFrameIndex = glCommitCtrl[3];
                    }
                    /* Set Commit Control and Start Streaming*/
                    else if(wValue == CX3_UVC_VS_COMMIT_CONTROL)
                    {
                        /* Super Speed USB Streams*/
                        if (CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED)

                        {
                            if(glCommitCtrl[3] == 0x01)
                            {
                                /* Write 1080pSettings */
                                status = CyU3PMipicsiSetIntfParams (&cfgUvc1080p30NoMclk, CyFalse);
                                if (status != CY_U3P_SUCCESS)
                                {
                                    CyU3PDebugPrint (4, "\n\rUSBStpCB:SetIntfParams SS1 Err = 0x%x", status);
                                }
                                CyCx3_ImageSensor_Set_1080p ();
                            }
                            else if(glCommitCtrl[3] == 0x02)
                            {
                                /* Write 720pSettings */

                                status = CyU3PMipicsiSetIntfParams (&cfgUvc720p60NoMclk, CyFalse);
                                if (status != CY_U3P_SUCCESS)
                                {
                                    CyU3PDebugPrint (4, "\n\rUSBStpCB:SetIntfParams SS2 Err = 0x%x", status);
                                }
                                CyCx3_ImageSensor_Set_720p();

                            }
                            else 
                            {
                                status = CyU3PMipicsiSetIntfParams (&cfgUvc5Mp15NoMclk, CyFalse);
                                if (status != CY_U3P_SUCCESS)
                                {
                                    CyU3PDebugPrint (4, "\n\rUSBStpCB:SetIntfParams SS2 Err = 0x%x", status);
                                }
                                CyCx3_ImageSensor_Set_5M();
                            }

                        }
                        /* High Speed USB Streams*/
                        else if (CyU3PUsbGetSpeed () == CY_U3P_HIGH_SPEED)            
                        {
#ifdef CX3_DEBUG_ENABLED
                            CyU3PDebugPrint(4, "\n\rUSBStpCB:Write HS-VGA Settings.");
#endif
                            /* Write VGA Settings */
                            status = CyU3PMipicsiSetIntfParams (&cfgUvcVgaNoMclk, CyFalse);
                            if (status != CY_U3P_SUCCESS)
                            {
                                CyU3PDebugPrint (4, "\n\rUSBStpCB:SetIntfParams HS Err = 0x%x", status);
                            }
                            CyCx3_ImageSensor_Set_Vga();


                        }
                        /* Full Speed USB Streams*/
                        else
                        {
#ifdef CX3_DEBUG_ENABLED
                            CyU3PDebugPrint(4, "\n\rUSBStpCB:Write FS-VGA Settings. Only for compliance.");
#endif
                            /* Write VGA Settings */
                            CyCx3_ImageSensor_Set_Vga();
                            status = CyU3PMipicsiSetIntfParams (&cfgUvcVgaNoMclk, CyFalse);
                            if (status != CY_U3P_SUCCESS)
                            {
                                CyU3PDebugPrint (4, "\n\rUSBStpCB:SetIntfParams FS Err = 0x%x", status);
                            }
                        }


                        if (glIsApplnActive)
                        {
#ifdef CX3_DEBUG_ENABLED
                            CyU3PDebugPrint (4, "\n\rUSBStpCB:Call AppSTOP1");
#endif
                            CyCx3UvcApplnStop();
                        }
                        CyCx3UvcApplnStart();
                    }
                }
            }
            else
            {
                /* Mark with error. */
                status = CY_U3P_ERROR_FAILURE;
            }
        }
        else if((wIndex & 0x00FF) == CX3_UVC_CONTROL_INTERFACE) /* Video Control Interface */
        {

            isHandled = CyTrue;
            /* Respond to VC_REQUEST_ERROR_CODE_CONTROL and stall every other request as this example does not support 
               any of the Video Control features */
            if((wValue == CX3_UVC_VC_REQUEST_ERROR_CODE_CONTROL) && (wIndex == 0x00))
            {
                temp = CX3_UVC_ERROR_INVALID_CONTROL;
                status = CyU3PUsbSendEP0Data(0x01, &temp);
                if (status != CY_U3P_SUCCESS)
                {
                    CyU3PDebugPrint (4, "\n\rUSBStpCB:VCI SendEP0Data = %d", status);
                }
            }
            else    
                CyU3PUsbStall(0,CyTrue, CyTrue);
        }
    }

    return isHandled;
}


/* This function initialines the USB Module, creates event group,
   sets the enumeration descriptors, configures the Endpoints and
   configures the DMA module for the UVC Application */
    void
CyCx3UvcApplnInit (void)
{
    CyU3PEpConfig_t endPointConfig;
    CyU3PDmaMultiChannelConfig_t dmaCfg;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
#ifdef CX3_DEBUG_ENABLED
    CyU3PMipicsiCfg_t readCfg;
    CyU3PMipicsiErrorCounts_t errCnts;
#endif
    /* Initialize the I2C interface for Mipi Block Usage and Camera. */
    status = CyU3PMipicsiInitializeI2c (CY_U3P_MIPICSI_I2C_400KHZ);
    if( status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:I2CInit Err = 0x%x.",status);
        CyCx3AppErrorHandler(status);
    }

    /* Initialize GPIO module. */
    status = CyU3PMipicsiInitializeGPIO ();
    if( status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:GPIOInit Err = 0x%x",status);
        CyCx3AppErrorHandler(status);
    }

    /* Initialize the PIB block */
    status = CyU3PMipicsiInitializePIB ();
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:PIBInit Err = 0x%x",status);
        CyCx3AppErrorHandler(status);
    }

    /* Start the USB functionality */
    status = CyU3PUsbStart();
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:UsbStart Err = 0x%x",status);
        CyCx3AppErrorHandler(status);
    }
    /* The fast enumeration is the easiest way to setup a USB connection,
     * where all enumeration phase is handled by the library. Only the
     * class / vendor requests need to be handled by the application. */
    CyU3PUsbRegisterSetupCallback(CyCx3UvcApplnUSBSetupCB, CyTrue);

    /* Setup the callback to handle the USB events */
    CyU3PUsbRegisterEventCallback(CyCx3UvcApplnUSBEventCB);

    /* Register a callback to handle LPM requests from the USB 3.0 host. */
    CyU3PUsbRegisterLPMRequestCallback (CyCX3ApplnLPMRqtCB);

    /* Set the USB Enumeration descriptors */

    /* Super speed device descriptor. */
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, NULL, (uint8_t *)CyCx3USB30DeviceDscr);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:Set_SS_Device_Dscr Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    /* High speed device descriptor. */
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, NULL, (uint8_t *)CyCx3USB20DeviceDscr);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:Set_HS_Device_Dscr Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    /* BOS descriptor */
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, NULL, (uint8_t *)CyCx3USBBOSDscr);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:Set_BOS_Dscr Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    /* Device qualifier descriptor */
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, NULL, (uint8_t *)CyCx3USBDeviceQualDscr);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:Set_DEVQUAL_Dscr Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    /* Super speed configuration descriptor */
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, NULL, (uint8_t *)CyCx3USBSSConfigDscr);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:Set_SS_CFG_Dscr Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    /* High speed configuration descriptor */
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, NULL, (uint8_t *)CyCx3USBHSConfigDscr);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:Set_HS_CFG_Dscr Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    /* Full speed configuration descriptor */
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, NULL, (uint8_t *)CyCx3USBFSConfigDscr);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:Set_FS_CFG_Dscr Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    /* String descriptor 0 */
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyCx3USBStringLangIDDscr);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:Set_STRNG_Dscr0 Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    /* String descriptor 1 */
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyCx3USBManufactureDscr);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:Set_STRNG_Dscr1 Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    /* String descriptor 2 */
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyCx3USBProductDscr);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:Set_STRNG_Dscr2 Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }
    /* String descriptor 3 */
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 3, (uint8_t *)CyCx3USBConfigSSDscr);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:Set_STRNG_Dscr3 Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    /* String descriptor 4 */
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 4, (uint8_t *)CyCx3USBConfigHSDscr);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:Set_STRNG_Dscr4 Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }
    /* String descriptor 2 */
    status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 5, (uint8_t *)CyCx3USBConfigFSDscr);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:Set_STRNG_Dscr5 Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    CyU3PUsbVBattEnable (CyTrue);
    CyU3PUsbControlVBusDetect (CyFalse, CyTrue);

    /* Connect the USB pins and enable super speed operation */
    status = CyU3PConnectState(CyTrue, CyTrue);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:ConnectState Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    /* Since the status interrupt endpoint is not used in this application,
     * just enable the EP in the beginning. */
    /* Control status interrupt endpoint configuration */
    endPointConfig.enable = 1;
    endPointConfig.epType = CY_U3P_USB_EP_INTR;
    endPointConfig.pcktSize = 64;
    endPointConfig.isoPkts  = 1;
    endPointConfig.burstLen = 1;

    status = CyU3PSetEpConfig(CX3_EP_CONTROL_STATUS, &endPointConfig);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:CyU3PSetEpConfig CtrlEp Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    CyU3PUsbFlushEp(CX3_EP_CONTROL_STATUS);

    /* Setup the Bulk endpoint used for Video Streaming */
    endPointConfig.enable = CyTrue;
    endPointConfig.epType = CY_U3P_USB_EP_BULK;

    endPointConfig.isoPkts  = 0;
    endPointConfig.streams = 0;

    switch(CyU3PUsbGetSpeed())
    {
        case CY_U3P_HIGH_SPEED:
            endPointConfig.pcktSize = 0x200;
            endPointConfig.burstLen = 1;
            break;

        case CY_U3P_FULL_SPEED:
            endPointConfig.pcktSize = 0x40;
            endPointConfig.burstLen = 1;    
            break;

        case CY_U3P_SUPER_SPEED:
        default:
            endPointConfig.pcktSize = CX3_EP_BULK_VIDEO_PKT_SIZE;
            endPointConfig.burstLen = 16;
    }

    status = CyU3PSetEpConfig(CX3_EP_BULK_VIDEO, &endPointConfig);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:CyU3PSetEpConfig BulkEp Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }

    CyU3PUsbEPSetBurstMode (CX3_EP_BULK_VIDEO, CyTrue);

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp(CX3_EP_BULK_VIDEO);


    /* Create a DMA Manual OUT channel for streaming data */
    /* Video streaming Channel is not active till a stream request is received */
    dmaCfg.size                 = CX3_UVC_STREAM_BUF_SIZE;
    dmaCfg.count                = CX3_UVC_STREAM_BUF_COUNT;
    dmaCfg.validSckCount        = 2;

    dmaCfg.prodSckId[0]         = CX3_PRODUCER_PPORT_SOCKET_0;
    dmaCfg.prodSckId[1]         = CX3_PRODUCER_PPORT_SOCKET_1;

    dmaCfg.consSckId[0]         = CX3_EP_VIDEO_CONS_SOCKET;
    dmaCfg.dmaMode              = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.notification         = CY_U3P_DMA_CB_PROD_EVENT | CY_U3P_DMA_CB_CONS_EVENT;
    dmaCfg.cb                   = CyCx3UvcAppDmaCallback;
    dmaCfg.prodHeader           = CX3_UVC_PROD_HEADER;
    dmaCfg.prodFooter           = CX3_UVC_PROD_FOOTER;
    dmaCfg.consHeader           = 0;
    dmaCfg.prodAvailCount       = 0;

    status = CyU3PDmaMultiChannelCreate (&glChHandleUVCStream, 
            CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE , &dmaCfg);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:DmaMultiChannelCreate Err = 0x%x", status);
    }
    CyU3PThreadSleep(100);

    /* Reset the channel: Set to DSCR chain starting point in PORD/CONS SCKT; set
       DSCR_SIZE field in DSCR memory */
    status = CyU3PDmaMultiChannelReset(&glChHandleUVCStream);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4,"\n\rAppInit:MultiChannelReset Err = 0x%x", status);
    }

    /* Configure the Fixed Function GPIF on the CX3 to use a 16 bit bus, and 
     * a DMA Buffer of size CX3_UVC_DATA_BUF_SIZE 
     */
    status = CyU3PMipicsiGpifLoad(CY_U3P_MIPICSI_BUS_16, CX3_UVC_DATA_BUF_SIZE);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:MipicsiGpifLoad Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }
    CyU3PThreadSleep(50);

    CyU3PGpifRegisterCallback(CyCx3GpifCB);
    CyU3PThreadSleep(50);
    /* Start the state machine. */
    status = CyU3PGpifSMStart (CX3_START_SCK0, ALPHA_CX3_START_SCK0);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:GpifSMStart Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }
#ifdef CX3_DEBUG_ENABLED
    CyU3PDebugPrint (4, "\n\rAppInit:GpifSMStart passed");
#endif    
    CyU3PThreadSleep(50);
    /* Pause the GPIF*/
    CyU3PGpifSMControl(CyTrue);

    /* Initialize the MIPI block */
    status =  CyU3PMipicsiInit();
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:MipicsiInit Err = 0x%x", status);
        CyCx3AppErrorHandler(status);
    }
    status = CyU3PMipicsiSetIntfParams(&cfgUvcVgaNoMclk, CyFalse);

    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:MipicsiSetIntfParams Err = 0x%x",status);
        CyCx3AppErrorHandler(status);
    }

#ifdef CX3_DEBUG_ENABLED
    status = CyU3PMipicsiQueryIntfParams (&readCfg);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rAppInit:MipicsiQueryIntfParams Err = 0x%x",status);
        CyCx3AppErrorHandler(status);
    }

    status = CyU3PMipicsiGetErrors (CyFalse, &errCnts);
#endif
    /* Setup Image Sensor */

    CyCx3_ImageSensor_Init();

    CyCx3_ImageSensor_Sleep();
#ifdef RESET_TIMER_ENABLE
    CyU3PTimerCreate (&UvcTimer, UvcAppProgressTimer, 0x00, TIMER_PERIOD, 0, CYU3P_NO_ACTIVATE);
#endif
}

/* This function initializes the debug module for the UVC application */
    void
CyCx3UvcApplnDebugInit (void)
{
    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Initialize the UART for printing debug messages */
    status = CyU3PUartInit();
    if (status != CY_U3P_SUCCESS)
    {
        //CyCx3AppErrorHandler(status);
        CyU3PDebugPrint (4, "\n\rCyCx3UvcApplnDebugInit:CyU3PUartInit failed Error = 0x%x",status);
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
    status = CyU3PUartSetConfig (&uartConfig, NULL);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rCyCx3UvcApplnDebugInit:CyU3PUartSetConfig failed Error = 0x%x",status);
        //CyCx3AppErrorHandler(status);
    }

    /* Set the UART transfer */
    status = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rCyCx3UvcApplnDebugInit:CyU3PUartTxSetBlockXfer failed Error = 0x%x",status);
        //CyCx3AppErrorHandler(status);
    }

    /* Initialize the debug application */
    status = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\n\rCyCx3UvcApplnDebugInit:CyU3PDebugInit failed Error = 0x%x",status);
        //CyCx3AppErrorHandler(status);
    }
    CyU3PDebugPreamble (CyFalse);

}

/* Entry function for the UVC application thread. */
    void
CyCx3UvcAppThread_Entry (
        uint32_t input)
{
    uint16_t wakeReason;
    uint32_t eventFlag;
    CyU3PReturnStatus_t status;

    /* Initialize the Debug Module */
    CyCx3UvcApplnDebugInit();

    /* Initialize the UVC Application */
    CyCx3UvcApplnInit();
    for (;;)
    {
        CyU3PEventGet (&glCx3Event,CX3_USB_SUSP_EVENT_FLAG|CX3_DMA_RESET_EVENT, CYU3P_EVENT_OR_CLEAR, &eventFlag, CYU3P_WAIT_FOREVER);
        /* Handle FrameDone Event*/

        if( eventFlag & CX3_DMA_RESET_EVENT)
        {
            if (glIsApplnActive)
            {
                CyCx3UvcApplnStop();
            }
            CyCx3UvcApplnStart();
#ifdef RESET_TIMER_ENABLE
            CyU3PTimerModify (&UvcTimer, TIMER_PERIOD, 0);
#endif
        }
        /* Handle Suspend Event*/
        if(eventFlag & CX3_USB_SUSP_EVENT_FLAG)
        {
            /* Place CX3 in Low Power Suspend mode, with USB bus activity as the wakeup source. */
            CyU3PMipicsiSleep();
            CyCx3_ImageSensor_Sleep();


            status = CyU3PSysEnterSuspendMode (CY_U3P_SYS_USB_BUS_ACTVTY_WAKEUP_SRC, 0, &wakeReason);
#ifdef CX3_DEBUG_ENABLED
            CyU3PDebugPrint (4, "\n\rEnterSuspendMode Status =  0x%x, Wakeup reason = 0x%x", status, wakeReason);
#endif
            if(glMipiActive)
            {
                
                CyU3PMipicsiWakeup();
                CyCx3_ImageSensor_Wakeup();
            }
            continue;
        }
    } /* End of for(;;) */
}

#ifdef CX3_ERROR_THREAD_ENABLE
    void
CyCx3UvcMipiErrorThread (
        uint32_t input)
{
    uint32_t eventFlag;
    CyU3PMipicsiErrorCounts_t errCnts;
#ifdef CX3_DEBUG_ENABLED
    CyU3PDebugPrint (4,"\n\rMipiErrorThread Init.");
#endif            
    for(;;)
    {
        /* Read Errors every 5 Seconds */
        CyU3PEventGet (&glMipiErrorEvent,CX3_MIPI_ERROR_EVENT, 
                CYU3P_EVENT_OR_CLEAR, &eventFlag, 5000);
        if(glIsApplnActive == CyTrue)
            CyU3PMipicsiGetErrors( CyTrue, &errCnts);
    }
}
#endif


/* Application define function which creates the threads. */
    void
CyFxApplicationDefine (
        void)
{
    void *ptr = NULL;
    uint32_t retThrdCreate = CY_U3P_SUCCESS;
#ifdef CX3_ERROR_THREAD_ENABLE
    void *ptr2 = NULL;
    uint32_t retThrdCreate2 = CY_U3P_SUCCESS;
#endif
    /* Allocate the memory for the thread and create the thread */
    ptr = CyU3PMemAlloc (UVC_APP_THREAD_STACK);
    retThrdCreate = CyU3PThreadCreate (&uvcAppThread,   /* UVC Thread structure */
            "30:UVC_app_thread",         /* Thread Id and name */
            CyCx3UvcAppThread_Entry,          /* UVC Application Thread Entry function */
            0,                           /* No input parameter to thread */
            ptr,                         /* Pointer to the allocated thread stack */
            UVC_APP_THREAD_STACK,        /* UVC Application Thread stack size */
            UVC_APP_THREAD_PRIORITY,     /* UVC Application Thread priority */
            UVC_APP_THREAD_PRIORITY,     /* Pre-emption threshold */
            CYU3P_NO_TIME_SLICE,         /* No time slice for the application thread */
            CYU3P_AUTO_START             /* Start the Thread immediately */
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

    /* Create GPIO application event group */
    retThrdCreate = CyU3PEventCreate(&glCx3Event);
    if (retThrdCreate != 0)
    {
        /* Event group creation failed with the error code retThrdCreate */

        /* Add custom recovery or debug actions here */

        /* Application cannot continue */
        /* Loop indefinitely */
        while(1);
    }
#ifdef CX3_ERROR_THREAD_ENABLE
    /* Allocate the memory for the thread and create the thread */
    ptr2 = CyU3PMemAlloc (UVC_MIPI_ERROR_THREAD_STACK);
    retThrdCreate2 = CyU3PThreadCreate (&uvcMipiErrorThread,   /* UVC Thread structure */
            "30:UVC_Mipi_Error_thread",         /* Thread Id and name */
            CyCx3UvcMipiErrorThread,          /* UVC Application Thread Entry function */
            0,                           /* No input parameter to thread */
            ptr2,                         /* Pointer to the allocated thread stack */
            UVC_MIPI_ERROR_THREAD_STACK,        /* UVC Application Thread stack size */
            UVC_MIPI_ERROR_THREAD_PRIORITY,     /* UVC Application Thread priority */
            UVC_MIPI_ERROR_THREAD_PRIORITY,     /* Pre-emption threshold */
            CYU3P_NO_TIME_SLICE,         /* No time slice for the application thread */
            CYU3P_AUTO_START             /* Start the Thread immediately */
            );

    /* Check the return code */
    if (retThrdCreate2 != 0)
    {
        /* Thread Creation failed with the error code retThrdCreate */
        /* Add custom recovery or debug actions here */
        /* Application cannot continue */
        /* Loop indefinitely */
        while(1);
    }

    retThrdCreate2 = CyU3PEventCreate(&glMipiErrorEvent);
    if (retThrdCreate2 != 0)
    {
        /* Event group creation failed with the error code retThrdCreate */

        /* Add custom recovery or debug actions here */

        /* Application cannot continue */
        /* Loop indefinitely */
        while(1);
    }
#endif
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

    /* Configure the IO matrix for the device.*/
    io_cfg.isDQ32Bit = CyFalse;

    io_cfg.useUart   = CyTrue;
    io_cfg.useI2C    = CyTrue;
    io_cfg.useI2S    = CyFalse;
    io_cfg.useSpi    = CyFalse;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_DEFAULT;
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
