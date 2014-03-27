/*
## Cypress USB 3.0 Platform header file (cyu3usbpp.h)
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
#ifndef _INCLUDED_CYU3USBPP_H_
#define _INCLUDED_CYU3USBPP_H_

#include <cyu3dma.h>
#include <cyu3types.h>
#include <cyu3os.h>
#include <cyu3usbconst.h>
#include <cyu3usb.h>
#include <cyu3externcstart.h>

/*
 * Summary 
 * Top level header for USB functionality in the USB 3.0 platform.
 */

/**************************************************************************
 ********************************* Macros *********************************
 **************************************************************************/

/* De-activate the pull-up on the D+ pin to disconnect from USB 2.0 host. */
#define CyU3PDisconUsbPins()    { UIB->dev_pwr_cs |= CY_U3P_UIB_DISCON; }

/* Activate the pull-up on the D+ pin to connect to USB 2.0 host. */
#define CyU3PConnectUsbPins()   { UIB->dev_pwr_cs &= ~CY_U3P_UIB_DISCON; }

#define CyU3PUsbCanConnect()            \
    ((glUibDeviceInfo.vbusDetectMode == 0) || ((GCTL->iopower & glUibDeviceInfo.vbusDetectMode) != 0))

/**************************************************************************
****************************** Data Types ********************************
**************************************************************************/

/* Feature Commands Recipient */

#define CY_U3P_UIB_RECIPIENT_MASK               (0x03)
#define CY_U3P_UIB_FT_DEVICE                    (0x00)
#define CY_U3P_UIB_FT_INTERFACE                 (0x01)
#define CY_U3P_UIB_FT_ENDPOINT                  (0x02)
#define CY_U3P_UIB_RQT_TYPE_MASK	        (0x60)

#define CY_U3P_UIB_SC_MASS_STORAGE_RESET        (0xFF)
#define CY_U3P_UIB_SC_GET_MAX_LUN               (0xFE)
#define CY_U3P_USB_DSCR_DEVICE_LEN              (18)
#define CY_U3P_USB_DSCR_DEVQUAL_LEN             (10)

#define CY_U3P_USBENUM_PPORT			(0x01)
#define CY_U3P_USBENUM_WB			(0x00)

#define CY_U3P_INTF_DESC_PTR			(0x09)	/* Offset to the interface desc in the config desc. */
#define CY_U3P_USB_CONFDESC_LENGTH_OFFSET       (0x02)
#define CY_U3P_USB_CONFDESC_NUMINTF_OFFSET      (0x02)
#define CY_U3P_USB_RQT_TYPE_POS                 (0x05)
#define CY_U3P_USB_DSCR_CONFIG_LEN              (0x09)

#define CY_U3P_EP0_XFER_SIZE                    (512)
#define CY_U3P_EP0_SHIFT_COUNT                  (9)

#define CY_U3P_UIB_PROT_LMP_PORT_CAP_TIMER_VALUE        (0x00000908)    /* 18.5 us. */
#define CY_U3P_UIB_PROT_LMP_PORT_CFG_TIMER_VALUE        (0x00000908)    /* 18.5 us. */

#define CY_U3P_USB_DEVSTAT_SELFPOWER            (1)
#define CY_U3P_USB_DEVSTAT_REMOTEWAKE           (2)
#define CY_U3P_USB_DEVSTAT_U1ENABLE             (4)
#define CY_U3P_USB_DEVSTAT_U2ENABLE             (8)

typedef struct
{
    uint8_t             *dscrToAdd;                     /* Descriptor to add in case of MSC */
    uint16_t            size;                           /* Size of the data to transfer */
    uint8_t             usbDeviceStat;                  /* Variable stores the device status. */
    uint8_t             enumMethod;                     /* Indicates type of enumeration pport/fast */
    uint8_t             usbState;                       /* Current state of USB connection. */
    uint8_t             usbSpeed;                       /* Current USB connection speed. */
    uint8_t             psocketIn;                      /* PPORT socket no. for EP0 IN */ 
    uint8_t             psocketOut;                     /* PPORT socket no. for EP0 OUT */
    uint8_t             usbActiveConfig;                /* Variable stores the active configuration of USB */
    uint8_t             usbAltSetting;                  /* Variable stores the alternative setting value */
    CyBool_t            changeConfigDscr;               /* Variable to indicate if part of the descr is to sent */ 
    CyBool_t            ackPending;                     /* Variable indicating whether a setup packet is pending */
    CyBool_t            inReset;                        /* Flag indicating that a reset event has come */
    CyBool_t            sendEp0Zlp;                     /* Flag indicating that an EP0-ZLP has to be sent. */
    CyBool_t            vbat_Power;                     /* Flag that indicates if vbat power can be used or not*/
    CyBool_t            enableSS;                       /* Flag that indicates if SS is enabled or not */
    CyBool_t            enableHostMode;                 /* Flag that indicates if host mode is enabled or not */
    CyBool_t            isConnected;                    /* Indicates that the connect event has been sent already. */
    CyBool_t            isHostMode;                     /* If the USB stack is performing as a USB host. */
    CyBool_t            isHnpRequested;                 /* Whether OTG HNP is being requested by the device. */
    CyBool_t            isOtgMode;                      /* Whether OTG mode is enabled. */
    CyBool_t            sofEventEnable;                 /* Whether SOF/ITP events are enabled. */
    CyBool_t            sendStatusEvent;                /* Whether EP0 status event has to be sent. */
    CyBool_t            isLpmDisabled;                  /* Whether LPM is disabled. */

    CyBool_t            forceLpmAccept;                 /* Whether we are forced to accept U1/U2 requests. */
    CyBool_t            enableUsb3;                     /* Whether USB 3.0 connection should be enabled. */
    CyBool_t            ssCmdSeen;                      /* Whether 3.0 command has been seen by device. */
    CyBool_t            ssHostResume;                   /* Whether the device is restarting from U3 state. */
    uint32_t            tDisabledCount;                 /* Represents the number of failed 3.0 link trainings. */
    uint32_t            hpTimeoutCnt;                   /* Number of HP timeouts that have happened. */
    CyBool_t            exitCompliance;                 /* Flag to request exit from compliance loop. */
    CyBool_t            ssCompliance;                   /* Flag indicating USB 3.0 link is in compliance state. */
    CyBool_t            enablePrefetch;                 /* Flag to enable/disable EP pre-fetch. */
    CyBool_t            ctrlAckDone;                    /* Whether the user is completing a control request. */
    CyBool_t            newCtrlRqt;                     /* Whether a new control request has been received. */
    uint32_t            vbusDetectMode;                 /* IOPOWER Mask used to detect VBus presence. */
    CyBool_t            usb2Disable;                    /* Whether USB 2.0 support is disabled. */
} CyU3PUsbDevStat;


typedef struct
{
    uint8_t             *usbDevDesc_p;			/*pointer to device desc of device*/
    uint8_t		*usbSSDevDesc_p;		/*pointer to SS device desc of device*/
    uint8_t             *usbDevQualDesc_p;		/*pointer to device qualifier desc of device*/
    uint8_t             *usbConfigDesc_p;		/*pointer to config desc of device*/
    uint8_t             *usbOtherSpeedConfigDesc_p;	/*pointer to other speed configuration desc of device*/
    uint8_t             *usbHSConfigDesc_p;             /*pointer to HIGH SPEED speed configuration desc of device*/
    uint8_t             *usbFSConfigDesc_p;             /*pointer to FULL SPEED speed configuration desc of device*/
    uint8_t             *usbSSConfigDesc_p;             /*pointer to SUPER SPEED speed configuration desc of device*/
    uint8_t             *usbStringDesc_p[CY_U3P_MAX_STRING_DESC_INDEX];/*ARRAY OF POINTERS TO STRING DESCRIPTORS*/
    uint8_t             *usbIntfDescHS_p;               /*pointer to interface descriptor*/
    uint8_t             *usbIntfDescFS_p;               /*pointer to interface descriptor*/
    uint8_t             *usbIntfDescSS_p;               /* Pointer to the SS Interface Descriptor */
    uint8_t             *usbSSBOSDesc_p;                /*pointer to Super speed BOS descriptor*/
    uint8_t             *usbOtgDesc_p;                  /*pointer to USB 2.0 OTG descriptor*/
} CyU3PUsbDescrPtrs;

typedef struct
{
    uint8_t             usbUserDeviceDesc[64];          /* User provided Device descriptor*/
    uint8_t     	usbSSUserDeviceDesc[64];        /* User provided Super Speed Device descriptor*/
    uint8_t             usbSSUserBOSDesc[128];          /* User provided Super Speed BOS descriptor*/
    uint8_t             usbUserDescFS[256];             /* User provided Full speed descriptor*/
    uint8_t             usbUserDescHS[256];             /* User provided High speed descriptor*/
    uint8_t             usbUserDescSS[256];             /* User provided SUPER speed descriptor*/
    uint8_t             usbUserDeviceQualDesc[32];      /* User provided Device qualifier descriptor*/
    uint8_t             usbUserOtgDesc[32];             /* User provided OTG descriptor*/
    uint8_t             usbStringDescBuf[512];          /* Buffer to hold all user provided string descriptors. */
    uint16_t            usbStringIndex;                 /* Pointer to free location inside string buffer. */
} CyU3PUsbUserDesc;

typedef struct 
{
    uint16_t size;                                      /* Size of the endpoint*/
    CyBool_t valid;                                     /* Whether this endpoint has been enabled. */
    CyBool_t mapped;                                    /* Whether this EP has been mapped to a socket. */
} CyU3PUsbEpInfo;

extern CyU3PUsbDescrPtrs  glUibDescrPtrs;                    
extern CyU3PUsbDevStat    glUibDeviceInfo;
extern CyU3PUsbUserDesc   glUibUserDesc;
extern CyU3PUsbEpInfo     glPcktSizeIn[16];             /* Array to store the packet size for EPs */
extern CyU3PUsbEpInfo     glPcktSizeOut[16];            /* Array to store the packet size for EPs */
extern CyU3PDmaChannel    glUibChHandle;                /* In channel handle for ep0 */
extern CyU3PDmaChannel    glUibChHandleOut;	        /* Out channel handle for ep0 */
extern CyU3PDmaChannel   *glUibInChHandle[16];          /* Handlers to the P-U IN channels */
extern CyU3PDmaChannel   *glUibOutChHandle[16];         /* Handlers to the P-U OUT channels */
extern volatile uint32_t  glUsbLinkErrorCount;          /* Number of link errors encountered in one session. */
extern uint32_t           glUsbUserLinkErrorCount;      /* Number of link errors reported to the user. */
extern uint8_t            glUibSelBuffer[];             /* Buffer to hold SEL numbers from the host. */
extern uint32_t           glUsb3TxTrimVal;              /* Settings for LNK_PHY_TX_TRIM register. */
extern uint32_t           glUsbMpllDefault;             /* Default value for lnk_phy_mpll_status register. */
extern CyU3PUSBEventCb_t  glUsbEvtCb;                   /* Callback for usb events. */
extern CyU3PUSBSetupCb_t  glUsbSetupCb;                 /* Callback for usb events. */
extern CyU3PUsbLPMReqCb_t glLpmRqtCb;                   /* Callback for USB LPM requests. */
extern CyBool_t           glUsbForceLPMAccept;          /* Force LPM request acceptance. */
extern volatile CyBool_t  glPollingRxEqSeen;            /* Whether Polling.RxEq has been seen since link on. */
extern CyU3PMutex         glUibLock;                    /* Lock for UIB registers and data structures. */
extern CyBool_t           glUibThreadStarted;           /* Flag that indicates whether UIB thread is running. */

/* Descriptor Table Data structure to handle the No ReEnumeration feature.
   The descriptors that are set as part of the booter will be stored in the following
   data structure to be used by the final application.
 */
typedef struct CyU3PUsbDescPtrs_t
{
    uint8_t   descType;
    uint8_t   descIndex;
    uint8_t   resvd0;
    uint8_t   resvd1;
    uint32_t *descPtr;
} CyU3PUsbDescPtrs_t;

/* Data structure and definitions used to handle device state when switching from/to boot firmware. */
typedef struct CyU3PUsbDescTable_t
{
    uint32_t           signature;
    uint32_t           usbSpeed;
    uint32_t           numDesc;
    uint32_t           length;
    CyU3PUsbDescPtrs_t descPtrs[25];
    uint32_t           bootSignature;
    uint32_t           revision;
    uint32_t           ssConnect;
    uint32_t           switchToBooter;
    uint32_t           leaveGpioOn;
} CyU3PUsbDescTable_t;

#define CYU3P_DEVSTATE_LOCATION  (0x40002000) /* Base address of booter descriptor table */
#define CY_USBDEV_SIGNATURE      (0x42575943) /* Signature for device state structure. */
#define CY_USB_BOOTER_SIGNATURE  (0x42335846) /* Booter Signature */
#define CY_USB_BOOTER_REV_1_1_1  (0x00010101) /* Booter revision 1.1.1 */
#define CY_USB_BOOTER_REV_1_2_0  (0x00010200) /* Booter revision 1.2.0 */
#define CY_USB_BOOTER_REV_1_2_1  (0x00010201) /* Booter revision 1.2.1 */
#define CY_USB_BOOTER_REV_1_3_0  (0x00010300) /* Booter revision 1.3.0 */
#define CY_USB_BOOTER_REV_1_3_1  (0x00010301) /* Booter revision 1.3.1 */

extern CyBool_t glSdk_UsbIsOn;
extern CyBool_t glSdk_GpioIsOn;

/**************************************************************************
*************************** Function prototypes **************************
**************************************************************************/

extern void
CyU3PUibIntHandler (
                    void);

extern void
CyU3PUsbDescInit (
        void);

extern void
CyU3PUsbPhyEnable (
        CyBool_t is_ss);

extern void
CyU3PUsbPhyDisable (
        CyBool_t is_ss);

/* Summary
   Function to add data to the USB event log buffer.

   See Also
   * CyU3PUsbInitEventLog

   Return Values
   None
 */
void
CyU3PUsbAddToEventLog (
        uint8_t data                            /* Data value to be logged. */
        );

extern void
CyU3PUsbRestartUib (
        void);

extern void
CyU3PUibSocketInit (
        void);

/* USB Host Events handlers */
extern void
CyU3PUsbHostIntHandler (
        void);

extern void
CyU3PUsbHostEpIntHandler (
        void);
extern void
CyU3PUsbHostEhciIntHandler (
        void);

extern void
CyU3PUsbHostOhciIntHandler (
        void);

extern void
CyU3PUsbOtgIntHandler (
        void);

extern void
CyU3PUsbChgdetIntHandler (
        void);

extern void
CyU3PUsbOtgVbusChangeHandler (
        void);

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYU3USBPP_H_ */

/*[]*/


