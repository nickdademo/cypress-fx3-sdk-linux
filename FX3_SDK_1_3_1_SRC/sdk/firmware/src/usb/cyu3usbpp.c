/*
## Cypress USB 3.0 Platform source file (cyu3usbpp.c)
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

#include <cyu3usbpp.h>
#include <cyu3error.h>
#include <cyu3protocol.h>
#include <cyu3dma.h>
#include <cyu3vic.h>
#include <cyu3utils.h>
#include <cyu3regs.h>
#include <cyu3socket.h>
#include <cyu3system.h>
#include <cyu3usbhost.h>
#include <cyu3usbotg.h>
#include <cyu3pib.h>

#include <cyfx3_api.h>

#define CY_U3P_UIB_STACK_SIZE           (0x400)         /* 1K stack for PIB Thread */
#define CY_U3P_UIB_THREAD_PRIORITY      (4)             /* UIB thread priority = 4 */
#define CY_U3P_UIB_QUEUE_SIZE           (128)           /* Queue Size in bytes */
#define CY_U3P_UIB_MSG_SIZE             (4)             /* Message size in byte */
#define CY_U3P_UIB_EVENT_MASK           (0xFFFFFFFF)
#define CY_U3P_UIB_EVT_EP_INT           (1 << 5)
#define CY_U3P_UIB_EVT_DEV_CTRL_INT     (1 << 6)
#define CY_U3P_UIB_EVT_LNK_INT          (1 << 7)
#define CY_U3P_UIB_EVT_PROT_INT         (1 << 8)
#define CY_U3P_UIB_EVT_PROT_EP_INT      (1 << 9)
#define CY_U3P_UIB_EVT_VBUS_CHANGE      (1 << 10)
#define CY_U3P_UIB_EVT_USB3_COMPLIANCE  (1 << 11)
#define CY_U3P_UIB_EVT_USB3_WARM_RESET  (1 << 12)
#define CY_U3P_UIB_EVT_USB3_U3_SUSPEND  (1 << 13)
#define CY_U3P_UIB_EVT_USB3_U0_RESUME   (1 << 14)
#define CY_U3P_UIB_EVT_UX_REENABLE      (1 << 15)
#define CY_U3P_UIB_EVT_TRY_UX_EXIT      (1 << 24)
#define CY_U3P_UIB_EVT_LNK_ERR_LIMIT    (1 << 25)
#define CY_U3P_UIB_EVT_EPM_UNDERRUN     (1 << 26)

/* USB Host Events. */
#define CY_U3P_UIB_EVT_HOST_INT         (1 << 16)
#define CY_U3P_UIB_EVT_HOST_EP_INT      (1 << 17)
#define CY_U3P_UIB_EVT_EHCI_INT         (1 << 18)
#define CY_U3P_UIB_EVT_OHCI_INT         (1 << 19)

/* USB OTG events. */
#define CY_U3P_UIB_EVT_OTG_INT          (1 << 20)
#define CY_U3P_UIB_EVT_CHGDET_INT       (1 << 21)

/* Max. number of Link errors permitted within one second before the USB link is cycled by the firmware. */
#define CY_U3P_USBLNK_ERRCNT_LIMIT      0x40

/* Global variable and static variable */

CyU3PThread         glUibThread;                            /* UIB thread */
CyU3PQueue          glUibQueue;                             /* UIB Queue */
CyU3PEvent          glUibEvent;                             /* UIB Event Handle */
CyU3PMutex          glUibLock;                              /* UIB lock. */
CyBool_t            glUibThreadStarted = CyFalse;           /* Flag that indicates whether UIB thread was started. */
CyU3PUSBEventCb_t   glUsbEvtCb;                             /* Callback for usb events. */
CyU3PUSBSetupCb_t   glUsbSetupCb;                           /* Callback for usb events. */
CyU3PUsbLPMReqCb_t  glLpmRqtCb;                             /* Callback for USB LPM requests. */
CyU3PDmaChannel    *glUibInChHandle[16];                    /* Handlers to the P-U IN channels */
CyU3PDmaChannel    *glUibOutChHandle[16];                   /* Handlers to the P-U OUT channels */
CyU3PUsbDevStat     glUibDeviceInfo = {0};                  /* Structure with device information */
CyU3PUsbDescrPtrs   glUibDescrPtrs;                         /* Pointers to descriptors used for enumeration */

uint8_t glUibSelBuffer[32] __attribute__ ((aligned (32)));  /* Buffer to hold SEL numbers from the host.
                                                               Avoid placing any other data in the same cache line. */

/* Volatile variables used for handling switches from USB 3.0 to 2.0 and back. */
volatile uint32_t glInSSReset = 0;
volatile uint32_t glInCheckUsbDisconnect = 0;
volatile CyBool_t glUsbUxWake = CyFalse;
volatile CyBool_t glRxValidMod = CyFalse;
volatile CyBool_t glPollingRxEqSeen = CyFalse;
CyU3PTimer        glUibStatusTimer;                         /* Timer to ensure status stage completion. */
CyU3PTimer        glUibLnkErrClrTimer;                      /* Timer used to clear USB 3.0 link error counts. */
CyBool_t          glUibStatusSendErdy;                      /* Flag indicating whether to send ERDY for EP0 status. */
CyBool_t          glUibEp0StatusPending;                    /* Pending status phase for USB-3 setup rqt. */
CyU3PUsbEpEvtCb_t glUsbEpCb = NULL;                         /* Endpoint event callback function. */
uint32_t          glUsbEpEvtMask = 0;                       /* Endpoint events that have been enabled. */
uint32_t          glUsbEvtEnabledEps = 0;                   /* Endpoints that have been enabled for events. */
volatile uint32_t glUsbLinkErrorCount = 0;                  /* Number of link errors seen so far. */
uint32_t          glUsbUserLinkErrorCount = 0;              /* Number of link errors reported to the user. */
uint32_t          glUsb3TxTrimVal = 0x0B569011;             /* Settings for the LNK_PHY_TX_TRIM register.
                                                               TX amplitude = 900mV, De-emp = 17 #only for FX3 */

extern void
CyU3PSetUsbCoreClock (
        uint8_t pclk,
        uint8_t epmclk);

/* Structure containing callback handlers for various Host/OTG related events. */
struct CyU3PUsbOtgHostEvtHandlers {
    void (*HostIntHandler) (void);
    void (*HostEpIntHandler) (void);
    void (*EhciIntHandler) (void);
    void (*OhciIntHandler) (void);
    void (*OtgIntHandler) (void);
    void (*ChgDetIntHandler) (void);
};
struct CyU3PUsbOtgHostEvtHandlers glHostOtgHandlers = {NULL, NULL, NULL, NULL, NULL, NULL};

void
CyU3PUsbSetHostIntHandler (
        void (*func_p) (void))
{
    glHostOtgHandlers.HostIntHandler = func_p;
}

void
CyU3PUsbSetHostEPIntHandler (
        void (*func_p) (void))
{
    glHostOtgHandlers.HostEpIntHandler = func_p;
}

void
CyU3PUsbSetEHCIIntHandler (
        void (*func_p) (void))
{
    glHostOtgHandlers.EhciIntHandler = func_p;
}

void
CyU3PUsbSetOHCIIntHandler (
        void (*func_p) (void))
{
    glHostOtgHandlers.OhciIntHandler = func_p;
}

void
CyU3PUsbSetOTGIntHandler (
        void (*func_p) (void))
{
    glHostOtgHandlers.OtgIntHandler = func_p;
}

void
CyU3PUsbSetChgDetIntHandler (
        void (*func_p) (void))
{
    glHostOtgHandlers.ChgDetIntHandler = func_p;
}

/* Functions to log UIB state data into a user provided circular buffer. */
uint8_t          *glUibEventLogBuf  = NULL;
uint32_t          glUibEventLogIdx  = 0;
uint32_t          glUibEventLogSize = 0;

void
CyU3PUsbInitEventLog (
        uint8_t *buf_p,
        uint32_t bufSize)
{
    glUibEventLogBuf  = buf_p;
    glUibEventLogSize = bufSize;
    glUibEventLogIdx  = 0;

    CyU3PMemSet (glUibEventLogBuf, 0x00, glUibEventLogSize);
}

uint16_t
CyU3PUsbGetEventLogIndex (
        void)
{
    return glUibEventLogIdx;
}

void
CyU3PUsbAddToEventLog (
        uint8_t data)
{
    if (glUibEventLogBuf != NULL)
    {
        glUibEventLogBuf[glUibEventLogIdx++] = data;
        if (glUibEventLogIdx >= glUibEventLogSize)
            glUibEventLogIdx = 0;
    }
}

CyU3PReturnStatus_t
CyU3PUsbGetErrorCounts (
        uint16_t *phy_err_cnt,
        uint16_t *lnk_error_cnt)
{
    uint32_t cnt;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        return CY_U3P_ERROR_NOT_STARTED;
    if (glUibDeviceInfo.usbSpeed != CY_U3P_SUPER_SPEED)
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    if ((phy_err_cnt == 0) || (lnk_error_cnt == 0))
        return CY_U3P_ERROR_BAD_ARGUMENT;

    cnt = USB3LNK->lnk_error_count;

    /* Keep a running count of the errors while the USB link is up. */
    *phy_err_cnt   = (uint16_t)(cnt >> 16) - (uint16_t)(glUsbUserLinkErrorCount >> 16);
    *lnk_error_cnt = (uint16_t)cnt - (uint16_t)glUsbUserLinkErrorCount;
    glUsbUserLinkErrorCount = cnt;

    return CY_U3P_SUCCESS;
}

/* Array of USB standard max. packet size setting for each connection speed
   and endpoint type.
 */
const uint32_t glUsbMaxPacketSize[3][3] = {
    /* {Iso, Bulk, Intr} */
    {1023,   64,   64}, /* Full speed */
    {1024,  512, 1024}, /* High speed */
    {1024, 1024, 1024}  /* Super speed */
};

void
CyU3PUibSocketInit (
        void)
{
    uint8_t i;

    /* Initalize all egress sockets. */
    for (i = 0; i < CY_U3P_DMA_UIB_NUM_SCK; i++)
    {
        UIB->sck[i].status = CY_U3P_UIB_SCK_STATUS_DEFAULT;
        UIB->sck[i].intr = ~CY_U3P_UIB_SCK_INTR_DEFAULT;
        UIB->sck[i].intr_mask = CY_U3P_UIB_SCK_INTR_MASK_DEFAULT;
    }

    /* Initialize all ingress sockets. */
    for (i = 0; i < CY_U3P_DMA_UIBIN_NUM_SCK; i++)
    {
        UIBIN->sck[i].status = CY_U3P_UIBIN_SCK_STATUS_DEFAULT;
        UIBIN->sck[i].intr = ~CY_U3P_UIBIN_SCK_INTR_DEFAULT;
        UIBIN->sck[i].intr_mask = CY_U3P_UIBIN_SCK_INTR_MASK_DEFAULT;
    }
}

static void
CyU3PUsbFallBackToUsb2 (
		void)
{
    uint32_t mask;

    /* If Vbus is not available, do not do anything here. */
    if (!(CyU3PUsbCanConnect ()))
        return;

    CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USB_FALLBACK);

    mask = CyU3PVicDisableAllInterrupts ();
    CyFx3UsbWritePhyReg (0x1005, 0x0000);
    CyU3PVicEnableInterrupts (mask);

    /* Force the link state machine into SS.Disabled. */
    USB3LNK->lnk_ltssm_state = (CY_U3P_UIB_LNK_STATE_SSDISABLED << CY_U3P_UIB_LTSSM_OVERRIDE_VALUE_POS) |
        CY_U3P_UIB_LTSSM_OVERRIDE_EN;

    /* Keep track of the number of times the 3.0 link training has failed. */
    glUibDeviceInfo.tDisabledCount++;

    glUibStatusSendErdy = CyFalse;
    CyU3PTimerStop (&glUibStatusTimer);

    /* Change EPM config to full speed */
    CyU3PBusyWait (2);
    CyU3PSetUsbCoreClock (2, 2);
    CyU3PBusyWait (2);

    /* Switch the EPM to USB 2.0 mode, turn off USB 3.0 PHY and remove Rx Termination. */
    UIB->otg_ctrl &= ~CY_U3P_UIB_SSDEV_ENABLE;
    CyU3PBusyWait (2);
    UIB->otg_ctrl &= ~CY_U3P_UIB_SSEPM_ENABLE;

    UIB->intr_mask &= ~(CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT | CY_U3P_UIB_LNK_INT |
            CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);

    USB3LNK->lnk_phy_conf        &= 0x1FFFFFFF;
    USB3LNK->lnk_phy_mpll_status  = glUsbMpllDefault;

    /* Power cycle the PHY blocks. */
    GCTLAON->control &= ~CY_U3P_GCTL_USB_POWER_EN;
    CyU3PBusyWait (5);
    GCTLAON->control |= CY_U3P_GCTL_USB_POWER_EN;
    CyU3PBusyWait (10);

    /* Clear USB 2.0 interrupts. */
    UIB->dev_ctl_intr = 0xFFFFFFFF;
    UIB->dev_ep_intr  = 0xFFFFFFFF;
    UIB->otg_intr     = 0xFFFFFFFF;

    /* Reset the EP-0 DMA channels. */
    CyU3PDmaChannelReset (&glUibChHandle);
    CyU3PDmaChannelReset (&glUibChHandleOut);

    /* Clear and disable USB 3.0 interrupts. */
    USB3LNK->lnk_intr_mask   = 0x00000000;
    USB3LNK->lnk_intr        = 0xFFFFFFFF;
    USB3PROT->prot_intr_mask = 0x00000000;
    USB3PROT->prot_intr      = 0xFFFFFFFF;

    UIB->intr_mask |= (CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT |
	CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);

    /* Disable EP0-IN and EP0-OUT (USB-2). */
    UIB->dev_epi_cs[0] &= ~CY_U3P_UIB_EPI_VALID;
    UIB->dev_epo_cs[0] &= ~CY_U3P_UIB_EPO_VALID;

    glUibDeviceInfo.usbSpeed      = CY_U3P_FULL_SPEED;
    glUibDeviceInfo.isLpmDisabled = CyFalse;
    UIB->ehci_portsc = CY_U3P_UIB_WKOC_E;

    /* Enable USB 2.0 PHY. */
    CyU3PBusyWait (2);
    UIB->otg_ctrl |= CY_U3P_UIB_DEV_ENABLE;

    CyU3PBusyWait (100);
    CyFx3Usb2PhySetup ();
    UIB->phy_clk_and_test = (CY_U3P_UIB_DATABUS16_8 | CY_U3P_UIB_VLOAD | CY_U3P_UIB_SUSPEND_N |
            CY_U3P_UIB_EN_SWITCH);
    CyU3PBusyWait (80);

    CyU3PSetUsbCoreClock (2, 0);

    /* For USB 2.0 connections, enable pull-up on D+ pin. */
    CyU3PConnectUsbPins ();
}

static void
CyU3PUsbReEnableUsb3 (
        void)
{
    uint32_t mask;

    CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USBSS_ENABLE);

    UIB->intr_mask &= ~(CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT |
            CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);

    /* Make sure that all relevant USB 3.0 interrupts are enabled. */
    USB3LNK->lnk_intr = 0xFFFFFFFF;
    USB3LNK->lnk_intr_mask = CY_U3P_UIB_LGO_U3 |
        CY_U3P_UIB_LTSSM_CONNECT | CY_U3P_UIB_LTSSM_DISCONNECT | CY_U3P_UIB_LTSSM_RESET | CY_U3P_UIB_LTSSM_STATE_CHG;
    USB3PROT->prot_intr = 0xFFFFFFFF;
    USB3PROT->prot_intr_mask = (CY_U3P_UIB_STATUS_STAGE | CY_U3P_UIB_SUTOK_EN | CY_U3P_UIB_EP0_STALLED_EN |
            CY_U3P_UIB_TIMEOUT_PORT_CAP_EN | CY_U3P_UIB_TIMEOUT_PORT_CFG_EN | CY_U3P_UIB_LMP_RCV_EN |
            CY_U3P_UIB_LMP_PORT_CAP_EN | CY_U3P_UIB_LMP_PORT_CFG_EN);
    if (glUibDeviceInfo.sofEventEnable)
        USB3PROT->prot_intr_mask |= CY_U3P_UIB_ITP_EN;

    /* Allow automatic transition into U1 and U2 power states on host request. */
    glUibDeviceInfo.isLpmDisabled = CyFalse;
    if (glUsbForceLPMAccept)
        USB3LNK->lnk_device_power_control = CY_U3P_UIB_YES_U1 | CY_U3P_UIB_YES_U2;
    else
        USB3LNK->lnk_device_power_control = CY_U3P_UIB_AUTO_U1 | CY_U3P_UIB_AUTO_U2;

    CyFx3Usb3LnkSetup ();

    /* Set port config and capability timers to their initial values. */
    USB3PROT->prot_lmp_port_capability_timer    = CY_U3P_UIB_PROT_LMP_PORT_CAP_TIMER_VALUE;
    USB3PROT->prot_lmp_port_configuration_timer = CY_U3P_UIB_PROT_LMP_PORT_CFG_TIMER_VALUE;

    /* Turn on AUTO response to LGO_U3 command from host. */
    USB3LNK->lnk_compliance_pattern_8 |= CY_U3P_UIB_LFPS;
    USB3LNK->lnk_phy_conf = 0x20000001;     /* Disable terminations. */

    CyU3PSetUsbCoreClock (1, 0);
    CyU3PBusyWait (10);

    /* Disable all interrupts until the PHY init sequence is complete. */
    mask = CyU3PVicDisableAllInterrupts ();
    glPollingRxEqSeen = CyFalse;

    /* Force LTSSM into SS.Disabled state for 100us after the PHY is turned on. */
    USB3LNK->lnk_ltssm_state = (CY_U3P_UIB_LNK_STATE_SSDISABLED << CY_U3P_UIB_LTSSM_OVERRIDE_VALUE_POS) |
        CY_U3P_UIB_LTSSM_OVERRIDE_EN;
    UIB->otg_ctrl |= CY_U3P_UIB_SSDEV_ENABLE;
    CyU3PBusyWait (100);

    USB3LNK->lnk_conf = (USB3LNK->lnk_conf & ~CY_U3P_UIB_EPM_FIRST_DELAY_MASK) |
        (12 << CY_U3P_UIB_EPM_FIRST_DELAY_POS) | CY_U3P_UIB_LDN_DETECTION;
    USB3LNK->lnk_phy_mpll_status   = 0x00310018 | CY_U3P_UIB_SSC_EN;

    /* Hardware work-arounds: Control LFPS behavior. */
    CyFx3UsbWritePhyReg (0x1010, 0x0080);
    CyFx3UsbWritePhyReg (0x1006, 0x0180);
    CyFx3UsbWritePhyReg (0x1024, 0x0080);

    UIB->intr_mask |= (CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT |
            CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);
    CyU3PVicEnableInterrupts (mask);

    USB3LNK->lnk_ltssm_state &= ~CY_U3P_UIB_LTSSM_OVERRIDE_EN;
    USB3LNK->lnk_phy_conf = 0xE0000001;
    CyU3PBusyWait (100);
}

/* Summary:
 * This function registers a callback for USB events
 */
void
CyU3PUsbRegisterEventCallback (CyU3PUSBEventCb_t callback)
{
    glUsbEvtCb = callback;

    /* If USB start has already been called, and VBus is present; notify the user. */
    if ((glUibDeviceInfo.usbState != CY_U3P_USB_INACTIVE) && ((GCTL->iopower & glUibDeviceInfo.vbusDetectMode) != 0))
        callback (CY_U3P_USB_EVENT_VBUS_VALID, 0x00);
}

/* Summary:
 * This function registers callback for class,vendor and other requests to be handled by Fx3/pport
 */
void
CyU3PUsbRegisterSetupCallback (CyU3PUSBSetupCb_t callback, CyBool_t fastEnum)
{
    glUsbSetupCb = callback;

    if (fastEnum == CyTrue)
    {
        glUibDeviceInfo.enumMethod = CY_U3P_USBENUM_WB;
    }
    else
    {
        glUibDeviceInfo.enumMethod = CY_U3P_USBENUM_PPORT;

        /* Work-around for compatibility reasons. UsbStart may not have been called so far. */
        if ((glUibDeviceInfo.usbState > CY_U3P_USB_INACTIVE) &&
                (glUibDeviceInfo.usbState < CY_U3P_USB_CONFIGURED))
        {
            glUibDeviceInfo.usbState = CY_U3P_USB_CONFIGURED;
        }
    }
}

void
CyU3PUsbRegisterLPMRequestCallback (
        CyU3PUsbLPMReqCb_t cb)
{
    glLpmRqtCb = cb;
}

/* Summary:
 * This function sets up descriptors to their default values
 */
void
CyU3PUsbDescInit ()
{
    CyU3PMemSet ((uint8_t *)(&glUibDescrPtrs), 0, sizeof (glUibDescrPtrs));
}

/* Summary:
 * This function sends the stall status of the endpoint to the host
 */
CyU3PReturnStatus_t
CyU3PUsbGetEpStatus (uint8_t ep)
{
    CyU3PReturnStatus_t ret;
    uint32_t descTransBuff;
    uint8_t  epNum = ep & 0x0F;

    if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
    {
        if (ep & 0x80)
        {
            descTransBuff = (USB3PROT->prot_epi_cs1[epNum] & CY_U3P_UIB_SSEPI_STALL);
        }
        else
        {
            descTransBuff = (USB3PROT->prot_epo_cs1[epNum] & CY_U3P_UIB_SSEPO_STALL);
        }
    }
    else
    {
        if (ep & 0x80)
        {
            descTransBuff = (UIB->dev_epi_cs[epNum] & CY_U3P_UIB_EPI_STALL);
        }
        else
        {
            descTransBuff = (UIB->dev_epo_cs[epNum] & CY_U3P_UIB_EPO_STALL);
        }
    }

    if (descTransBuff != 0)
    {
        descTransBuff = 1;
    }

    ret = CyU3PUsbSendEP0Data (2 , (uint8_t *)&descTransBuff);
    return ret;
}

/* Configure the EP packet sizes and clear stall on the endpoint. */
void
CyU3PUsbEpPrepare (
        CyU3PUSBSpeed_t speed)
{
    uint8_t  epnum, eptype;
    uint32_t pktSize;
    uint32_t specSize;

    /* Configure all valid EPs to have the appropriate max. packet size, and clear any stall condition. */
    for (epnum = 1; epnum < 16; epnum++)
    {
        /* Set packet size to full speed */
        if (UIB->dev_epi_cs[epnum] & CY_U3P_UIB_EPI_VALID)
        {
            pktSize = glPcktSizeIn[epnum].size;
            eptype  = (uint8_t)((UIB->dev_epi_cs[epnum] & CY_U3P_UIB_EPI_TYPE_MASK) >> CY_U3P_UIB_EPI_TYPE_POS);

            specSize = glUsbMaxPacketSize[speed - 1][eptype - 1];
            if ((pktSize > specSize) || (pktSize == 0))
            {
                pktSize = specSize;
            }

            if (speed == CY_U3P_SUPER_SPEED)
            {
                USB3PROT->prot_epi_cs1[epnum] |= CY_U3P_UIB_SSEPI_VALID;
            }
            else
            {
                UIB->dev_epi_cs[epnum] = ((UIB->dev_epi_cs[epnum] & ~CY_U3P_UIB_EPI_PAYLOAD_MASK) |
                        (pktSize & CY_U3P_UIB_EPI_PAYLOAD_MASK));
            }

            UIB->eepm_endpoint[epnum] = pktSize;

            /* Clear the stall and toggles on this endpoint. */
            CyU3PUsbStall (0x80 | epnum, CyFalse, CyTrue);
        }

        if (UIB->dev_epo_cs[epnum] & CY_U3P_UIB_EPO_VALID)
        {
            pktSize = glPcktSizeOut[epnum].size;
            eptype  = (uint8_t)((UIB->dev_epo_cs[epnum] & CY_U3P_UIB_EPI_TYPE_MASK) >> CY_U3P_UIB_EPI_TYPE_POS);

            specSize = glUsbMaxPacketSize[speed - 1][eptype - 1];
            if ((pktSize > specSize) || (pktSize == 0))
            {
                pktSize = specSize;
            }

            if (speed == CY_U3P_SUPER_SPEED)
            {
                USB3PROT->prot_epo_cs1[epnum] |= CY_U3P_UIB_SSEPO_VALID;
            }
            else
            {
                UIB->dev_epo_cs[epnum] = ((UIB->dev_epo_cs[epnum] & ~CY_U3P_UIB_EPO_PAYLOAD_MASK) |
                        (pktSize & CY_U3P_UIB_EPO_PAYLOAD_MASK));
            }

            UIB->iepm_endpoint[epnum] = (UIB->iepm_endpoint[epnum] & CY_U3P_UIB_EOT_EOP) | pktSize;

            /* Clear the stall and toggles on this endpoint. */
            CyU3PUsbStall (epnum, CyFalse, CyTrue);
        }
    }

    CyFx3UsbDmaPrefetchEnable ((speed == CY_U3P_SUPER_SPEED) && (glUibDeviceInfo.enablePrefetch));
}

/* Summary:
 * Interrupt handler for handling the the HS grant interrupt
 */
static void
CyU3PHsGrantIntrHandler ()
{
    if (UIB->dev_pwr_cs & CY_U3P_UIB_HSM)
    {
        glUibDescrPtrs.usbConfigDesc_p           = glUibDescrPtrs.usbHSConfigDesc_p;
        glUibDescrPtrs.usbOtherSpeedConfigDesc_p = glUibDescrPtrs.usbFSConfigDesc_p;

        if (glUsbEvtCb != NULL)
        {
            glUsbEvtCb (CY_U3P_USB_EVENT_SPEED, 1);
        }

        glUibDeviceInfo.usbSpeed = CY_U3P_HIGH_SPEED;

        /* Configuring the EPs to the required packet sizes. */
        CyU3PUsbEpPrepare (CY_U3P_HIGH_SPEED);
    }
}

/* Summary:
 * Interrupt handler for USB 2.0 Bus Reset Condition.
 */
static void
CyU3PUsbResetHandler ()
{
    /* Fall back to full speed on receiving a bus reset. */
    glUibDeviceInfo.usbSpeed = CY_U3P_FULL_SPEED;
    glUibDescrPtrs.usbConfigDesc_p              = glUibDescrPtrs.usbFSConfigDesc_p;
    glUibDescrPtrs.usbOtherSpeedConfigDesc_p    = glUibDescrPtrs.usbHSConfigDesc_p;

    if (glUsbEvtCb != NULL)
    {
        if (!glUibDeviceInfo.isConnected)
        {
            glUsbEvtCb (CY_U3P_USB_EVENT_CONNECT, 0);
            glUibDeviceInfo.isConnected = CyTrue;
        }

        glUsbEvtCb (CY_U3P_USB_EVENT_RESET, 0);
    }

    /* Reset the EPM mux. */
    UIB->iepm_cs |= (CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
    CyU3PBusyWait (1);
    UIB->iepm_cs &= ~(CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
    CyU3PBusyWait (1);
    UIB->eepm_endpoint[0] = 0x40;
    UIB->iepm_endpoint[0] = 0x40;

    /* Enable EP0-IN and EP0-OUT. */
    UIB->dev_epi_cs[0] |= CY_U3P_UIB_EPI_VALID;
    UIB->dev_epo_cs[0] |= CY_U3P_UIB_EPO_VALID;

    /* Configure EPs for FULL speed operation. */
    CyU3PUsbEpPrepare (CY_U3P_FULL_SPEED);

    /* On receiving a 2.0 bus reset, we need to attempt 3.0 link training iff the number of 3.0 training attempts
       is less than 3.
     */
    if (glUibDeviceInfo.enableUsb3)
    {
        if (glUibDeviceInfo.tDisabledCount >= 3)
            glUibDeviceInfo.enableUsb3 = CyFalse;

        /* Do this only if the PHY is not already on. */
        if ((UIB->otg_ctrl & CY_U3P_UIB_SSDEV_ENABLE) == 0)
            CyU3PUsbReEnableUsb3 ();
    }
}

/* Summary:
 * Interrupt handler for handling the the suspend interrupt
 */
static void
CyU3PUsbSuspendHandler ()
{
    if (glUibDeviceInfo.usbState >= CY_U3P_USB_CONNECTED)
    {
        if (glUsbEvtCb != NULL)
        {
            glUsbEvtCb (CY_U3P_USB_EVENT_SUSPEND, 0);
        }

        glUibDeviceInfo.usbState = CY_U3P_USB_CONNECTED;
    }
}

static void
CyU3PUsbResumeHandler ()
{
    if (glUibDeviceInfo.usbState >= CY_U3P_USB_CONNECTED)
    {
        if (glUsbEvtCb != NULL)
        {
            glUsbEvtCb (CY_U3P_USB_EVENT_RESUME, 0);
        }

        glUibDeviceInfo.usbState = CY_U3P_USB_ESTABLISHED;
    }
}

/* Handler for USB Get-Status request. */
static CyBool_t
CyU3PUsbHandleGetStatus (
        uint8_t bTarget,
        uint16_t wIndex)
{
    CyBool_t isHandled = CyFalse;
    CyU3PReturnStatus_t status;
    uint8_t statBuf[2] = {0, 0};

    switch (bTarget)
    {
    case CY_U3P_USB_TARGET_DEVICE:
        {
            if (wIndex == CY_U3P_USB_OTG_STATUS_SELECTOR)
            {
                if (CyU3POtgGetMode () == CY_U3P_OTG_MODE_OTG)
                {
                    statBuf[0] = (glUibDeviceInfo.isHnpRequested) ? (1) : (0);
                }
                else
                {
                    /* This request needs to be stalled. */
                    return CyFalse;
                }
            }
            else
            {
                /* Send device / interface status or remote wakeup status
                 * and selfpowered status */
                statBuf[0] = glUibDeviceInfo.usbDeviceStat;
                if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
                {
                    statBuf[0] &= ~CY_U3P_USB_DEVSTAT_REMOTEWAKE; /* Make sure remote wakeup bit is turned off. */
                }
            }
        }
        /* Fall-through. */
    case CY_U3P_USB_TARGET_INTF:
        {
            /* No support for function suspend/wake as of now. */
            status = CyU3PUsbSendEP0Data (2, statBuf);
            if (status == CY_U3P_SUCCESS)
            {
                isHandled = CyTrue;
            }
        }
        break;

    case CY_U3P_USB_TARGET_ENDPT:
        if (CyU3PUsbGetEpStatus ((uint8_t)wIndex) == CY_U3P_SUCCESS)
        {
            isHandled = CyTrue;
        }
        break;

    default: /* Invalid Command */
        break;
    }

    return isHandled;
}

static void
CyU3PUsbHandleClearFeature (
        uint8_t feature)
{
    CyBool_t isHandled = CyFalse;

    switch (feature)
    {
    case CY_U3P_USB2_FS_REMOTE_WAKE:
        glUibDeviceInfo.usbDeviceStat &= ~CY_U3P_USB_DEVSTAT_REMOTEWAKE;  /* Clear the remote wakeup bit. */
        /* Fall-through here. */
    case CY_U3P_USB2_FS_TEST_MODE:
        if (glUibDeviceInfo.usbSpeed != CY_U3P_SUPER_SPEED)
        {
            isHandled = CyTrue;
        }
        break;

    case CY_U3P_USB3_FS_U1_ENABLE:
        if ((glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED) &&
                (glUibDeviceInfo.usbState == CY_U3P_USB_ESTABLISHED))
        {
            glUibDeviceInfo.usbDeviceStat &= ~CY_U3P_USB_DEVSTAT_U1ENABLE;
            isHandled = CyTrue;
        }
        break;

    case CY_U3P_USB3_FS_U2_ENABLE:
        if ((glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED) &&
                (glUibDeviceInfo.usbState == CY_U3P_USB_ESTABLISHED))
        {
            glUibDeviceInfo.usbDeviceStat &= ~CY_U3P_USB_DEVSTAT_U2ENABLE;
            isHandled = CyTrue;
        }
        break;

    default:
        break;
    }

    if (isHandled)
    {
        /* Mark handling as completed. */
        CyU3PUsbAckSetup ();
    }
    else
    {
        /* Unknown request. Stall EP0. */
        CyU3PUsbStall (0x00, CyTrue, CyFalse);
    }
}

static void
CyU3PUsbHandleSetFeature (
        uint8_t feature)
{
    CyBool_t isHandled = CyFalse;

    switch (feature)
    {
        case CY_U3P_USB2_FS_REMOTE_WAKE:
            glUibDeviceInfo.usbDeviceStat |= CY_U3P_USB_DEVSTAT_REMOTEWAKE;
            /* Fall-through here. */
        case CY_U3P_USB2_FS_TEST_MODE:
            if (CyU3PUsbGetSpeed() != CY_U3P_SUPER_SPEED)
            {
                isHandled = CyTrue;
            }
            break;

        case CY_U3P_USB3_FS_U1_ENABLE:
            if ((CyU3PUsbGetSpeed() == CY_U3P_SUPER_SPEED) &&
                    (glUibDeviceInfo.usbState == CY_U3P_USB_ESTABLISHED))
            {
                glUibDeviceInfo.usbDeviceStat |= CY_U3P_USB_DEVSTAT_U1ENABLE;
                isHandled = CyTrue;
            }
            break;

        case CY_U3P_USB3_FS_U2_ENABLE:
            if ((CyU3PUsbGetSpeed() == CY_U3P_SUPER_SPEED) &&
                    (glUibDeviceInfo.usbState == CY_U3P_USB_ESTABLISHED))
            {
                glUibDeviceInfo.usbDeviceStat |= CY_U3P_USB_DEVSTAT_U2ENABLE;
                isHandled = CyTrue;
            }
            break;

        default:
            break;
    }

    if (isHandled)
    {
        /* Mark handling as completed. */
        CyU3PUsbAckSetup ();
    }
    else
    {
        /* Unknown request. Stall EP0. */
        CyU3PUsbStall (0x00, CyTrue, CyFalse);
    }
}

/* Set pointers to descriptors and setup data transfer for each.*/
static CyBool_t
CyU3PUibSendDescr(uint32_t setupdat0, uint32_t setupdat1)
{
    uint16_t count = 0;
    uint32_t status = 0;
    uint8_t *buf = NULL;
    CyBool_t isHandled = CyFalse;

    uint16_t wLength = ((setupdat1 & CY_U3P_UIB_SETUP_LENGTH_MASK) >> CY_U3P_UIB_SETUP_LENGTH_POS);
    uint16_t max = (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED) ? 512 : 64;

    switch (((setupdat0 & CY_U3P_UIB_SETUP_VALUE_MASK) >> 24))
    {
    case CY_U3P_USB_DEVICE_DESCR:
        {
            if(glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
            {
                if(glUibDescrPtrs.usbSSDevDesc_p != NULL)
                {
                    isHandled = CyTrue;
                    count = CY_U3P_USB_DSCR_DEVICE_LEN;
                    buf = (uint8_t *)(glUibDescrPtrs.usbSSDevDesc_p);
                }
            }
            else
            {
                if(glUibDescrPtrs.usbDevDesc_p != NULL)
                {
                    isHandled = CyTrue;
                    count = CY_U3P_USB_DSCR_DEVICE_LEN;
                    buf = (uint8_t *)(glUibDescrPtrs.usbDevDesc_p);
                }
            }
        }
        break;

    case CY_U3P_USB_CONFIG_DESCR:
        {
            if (glUibDeviceInfo.usbSpeed == CY_U3P_HIGH_SPEED)
            {
                if (glUibDescrPtrs.usbHSConfigDesc_p != NULL)
                {
                    isHandled = CyTrue;
                    buf = (uint8_t *)(glUibDescrPtrs.usbHSConfigDesc_p);
                    count = CY_U3P_MIN (((setupdat1 & CY_U3P_UIB_SETUP_LENGTH_MASK) >> 16),
                            CY_U3P_MAKEWORD (glUibDescrPtrs.usbHSConfigDesc_p[3], glUibDescrPtrs.usbHSConfigDesc_p[2]));
                }
            }
            else if (glUibDeviceInfo.usbSpeed == CY_U3P_FULL_SPEED)
            {
                if (glUibDescrPtrs.usbFSConfigDesc_p != NULL)
                {
                    isHandled = CyTrue;
                    buf = (uint8_t *)(glUibDescrPtrs.usbFSConfigDesc_p);
                    count = CY_U3P_MIN (((setupdat1 & CY_U3P_UIB_SETUP_LENGTH_MASK) >> 16),
                            CY_U3P_MAKEWORD (glUibDescrPtrs.usbFSConfigDesc_p[3], glUibDescrPtrs.usbFSConfigDesc_p[2]));
                }
            }
            else
            {
                if (glUibDescrPtrs.usbSSConfigDesc_p != NULL)
                {
                    isHandled = CyTrue;
                    buf = (uint8_t *)(glUibDescrPtrs.usbSSConfigDesc_p);
                    count = CY_U3P_MIN (((setupdat1 & CY_U3P_UIB_SETUP_LENGTH_MASK) >> 16),
                            CY_U3P_MAKEWORD (glUibDescrPtrs.usbSSConfigDesc_p[3], glUibDescrPtrs.usbSSConfigDesc_p[2]));
                }
            }

            /* Make sure the descriptor type is set as config descriptor. */
            if (buf != NULL)
                buf[1] = CY_U3P_USB_CONFIG_DESCR;
        }
        break;

    case CY_U3P_USB_STRING_DESCR:
        {
            uint8_t strIndex;

            strIndex = ((setupdat0 & 0x00FF0000) >> 16);
            if ((strIndex > CY_U3P_MAX_STRING_DESC_INDEX) || ((glUibDescrPtrs.usbStringDesc_p[strIndex]) == 0))
            {
                /* Send the request to the callback only if this was not forwarded earlier. */
                if ((glUibDeviceInfo.enumMethod != CY_U3P_USBENUM_PPORT) && (glUsbSetupCb != NULL))
                {
                    isHandled = glUsbSetupCb (setupdat0, setupdat1);
                    if (isHandled)
                        glUibDeviceInfo.sendStatusEvent = CyTrue;
                }
            }
            else
            {
                count = CY_U3P_MIN (((setupdat1 & CY_U3P_UIB_SETUP_LENGTH_MASK) >> 16),
                        *(glUibDescrPtrs.usbStringDesc_p[strIndex]));
                if(glUibDescrPtrs.usbStringDesc_p[strIndex] != NULL)
                {
                    isHandled = CyTrue;
                    buf = glUibDescrPtrs.usbStringDesc_p[strIndex];
                }
            }
        }
        break;

    case CY_U3P_USB_DEVQUAL_DESCR:
        {
            /* Send DEVQUAL descriptor at high speed only. */
            if ((glUibDescrPtrs.usbDevQualDesc_p != NULL) && (glUibDeviceInfo.usbSpeed != CY_U3P_SUPER_SPEED))
            {
                isHandled = CyTrue;
                count = CY_U3P_USB_DSCR_DEVQUAL_LEN;
                buf = glUibDescrPtrs.usbDevQualDesc_p;
            }
        }
        break;

    case CY_U3P_USB_OTHERSPEED_DESCR:
        {
            if (glUibDeviceInfo.usbSpeed == CY_U3P_HIGH_SPEED)
            {
                if (glUibDescrPtrs.usbFSConfigDesc_p != NULL)
                {
                    isHandled = CyTrue;
                    buf = glUibDescrPtrs.usbFSConfigDesc_p;
                    count = CY_U3P_MIN (((setupdat1 & CY_U3P_UIB_SETUP_LENGTH_MASK) >> 16),
                            CY_U3P_MAKEWORD (glUibDescrPtrs.usbFSConfigDesc_p[3], glUibDescrPtrs.usbFSConfigDesc_p[2]));
                }
            }
            else if (glUibDeviceInfo.usbSpeed == CY_U3P_FULL_SPEED)
            {
                if (glUibDescrPtrs.usbHSConfigDesc_p != NULL)
                {
                    isHandled = CyTrue;
                    buf = glUibDescrPtrs.usbHSConfigDesc_p;
                    count = CY_U3P_MIN (((setupdat1 & CY_U3P_UIB_SETUP_LENGTH_MASK) >> 16),
                            CY_U3P_MAKEWORD (glUibDescrPtrs.usbHSConfigDesc_p[3], glUibDescrPtrs.usbHSConfigDesc_p[2]));
                }
            }

            /* Make sure the descriptor type is set as other speed config descriptor. */
            if (buf != NULL)
                buf[1] = CY_U3P_USB_OTHERSPEED_DESCR;
        }
        break;

    case CY_U3P_BOS_DESCR:
        {
            /* Send BOS descriptor at all USB speeds. */
            if ((glUibDescrPtrs.usbSSBOSDesc_p != NULL) && ((glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED) ||
                        ((glUibDescrPtrs.usbDevDesc_p != 0) && (glUibDescrPtrs.usbDevDesc_p[2] == 0x10))))
            {
                isHandled = CyTrue;
                buf = glUibDescrPtrs.usbSSBOSDesc_p;
                count = CY_U3P_MIN (((setupdat1 & CY_U3P_UIB_SETUP_LENGTH_MASK) >> 16),
                        CY_U3P_MAKEWORD (glUibDescrPtrs.usbSSBOSDesc_p[3], glUibDescrPtrs.usbSSBOSDesc_p[2]));
            }
        }
        break;

    case CY_U3P_USB_OTG_DESCR:
        {
            /* Send the descriptor only if it is registered.
             * Otherwise stall the request. */
            if (glUibDescrPtrs.usbOtgDesc_p != NULL)
            {
                isHandled = CyTrue;
                buf = glUibDescrPtrs.usbOtgDesc_p;
                count = CY_U3P_MIN (((setupdat1 & CY_U3P_UIB_SETUP_LENGTH_MASK) >> 16),
                        glUibDescrPtrs.usbOtgDesc_p[0]);
            }
        }
        break;

    default:
        /* Unknown descriptor type. Forward to setup callback. */
        {
            if (glUsbSetupCb)
                isHandled = glUsbSetupCb (setupdat0, setupdat1);
        }
        break;
    }

    if ((isHandled == CyTrue) && (buf != NULL))
    {
        status = CyU3PUsbSendEP0Data (count, buf);

        /* If we send less data than the host asked for, see if we need to send a ZLP. */
        if ((status == CY_U3P_SUCCESS) && (count < wLength) && ((count & (max - 1)) == 0))
            status = CyU3PUsbSendEP0Data (0, buf);

        if (status != CY_U3P_SUCCESS)
        {
            /* There was some error. We should try stalling EP0. */
            CyU3PUsbStall(0, CyTrue, CyFalse);
        }
    }

    return isHandled;
}

/* Parses the USB setup command received. */
static void
CyU3PUsbSetupCommand (
        void)
{
    uint32_t setupdat0;
    uint32_t setupdat1;
    uint32_t status = 0;
    CyBool_t isHandled = CyFalse;

    uint8_t  bRequest, bReqType;
    uint8_t  bType, bTarget;
    uint16_t wValue, wIndex, wLength;

    /* For super speed handling. */
    if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
    {
        setupdat0 = USB3PROT->prot_setupdat0;
        setupdat1 = USB3PROT->prot_setupdat1;

        glUibDeviceInfo.ssCmdSeen  = CyTrue;
        glUibStatusSendErdy        = CyFalse;
        CyU3PTimerStop (&glUibStatusTimer);

        /* Clear the status stage interrupt. We later check for this. */
        USB3PROT->prot_intr = CY_U3P_UIB_STATUS_STAGE;

        /* If the LTSSM is currently in U1/U2, set an event to trigger a wakeup. */
        status = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
        if ((status == CY_U3P_UIB_LNK_STATE_U1) || (status == CY_U3P_UIB_LNK_STATE_U2))
            CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_TRY_UX_EXIT, CYU3P_EVENT_OR);
        status = 0;
    }
    else
    {
        /* If USB-SS is enabled, set a flag indicating that the 3.0 PHY should
         * be turned on at the next bus reset. */
        if ((glUibDeviceInfo.enableSS) && (glUibDeviceInfo.tDisabledCount < 3))
        {
            glUibDeviceInfo.enableUsb3 = CyTrue;
        }

        setupdat0 = UIB->dev_setupdat0;
        setupdat1 = UIB->dev_setupdat1;
    }

    status = CyU3PDmaChannelWaitForCompletion (&glUibChHandleOut, 100);
    if ((status != CY_U3P_SUCCESS) && (status != CY_U3P_ERROR_NOT_STARTED))
    {
        /* The endpoint needs to be NAKed before the channel is reset. */
        CyU3PUsbSetEpNak (0x00, CyTrue);
        CyU3PBusyWait (100);
        CyU3PDmaChannelReset (&glUibChHandleOut);
        CyU3PUsbSetEpNak (0x00, CyFalse);
    }

    status = CyU3PDmaChannelWaitForCompletion (&glUibChHandle, 100);
    if ((status != CY_U3P_SUCCESS) && (status != CY_U3P_ERROR_NOT_STARTED))
    {
        /* The endpoint needs to be NAKed before the channel is reset. */
        CyU3PUsbSetEpNak (0x80, CyTrue);
        CyU3PBusyWait (100);
        CyU3PDmaChannelReset (&glUibChHandle);
        CyU3PUsbFlushEp (0x80);
        CyU3PUsbSetEpNak (0x80, CyFalse);
    }
    status = 0;

    /* Decode the fields from the setup request. */
    bReqType = ((setupdat0 & CY_U3P_UIB_SETUP_REQUEST_TYPE_MASK) >> CY_U3P_UIB_SETUP_REQUEST_TYPE_POS);
    bType    = (bReqType & CY_U3P_USB_TYPE_MASK);
    bTarget  = (bReqType & CY_U3P_USB_TARGET_MASK);
    bRequest = ((setupdat0 & CY_U3P_UIB_SETUP_REQUEST_MASK) >> CY_U3P_UIB_SETUP_REQUEST_POS);
    wValue   = ((setupdat0 & CY_U3P_UIB_SETUP_VALUE_MASK) >> CY_U3P_UIB_SETUP_VALUE_POS);
    wIndex   = ((setupdat1 & CY_U3P_UIB_SETUP_INDEX_MASK) >> CY_U3P_UIB_SETUP_INDEX_POS);
    wLength  = ((setupdat1 & CY_U3P_UIB_SETUP_LENGTH_MASK) >> CY_U3P_UIB_SETUP_LENGTH_POS);

    if((setupdat0 & CY_U3P_UIB_SETUP_REQUEST_TYPE_MASK) & 0x80)
    {
        UIB->dev_epi_xfer_cnt[0] = wLength;
    }
    else
    {
        UIB->dev_epo_xfer_cnt[0] = wLength;
    }

    /* Default setting: Don't send status event notifications. */
    glUibDeviceInfo.sendStatusEvent = CyFalse;

    /* Clear the inReset flag. */
    UIB->dev_ctl_intr_mask &= ~CY_U3P_UIB_URESET;
    glUibDeviceInfo.inReset    = 0;
    glUibDeviceInfo.newCtrlRqt = CyFalse;
    UIB->dev_ctl_intr_mask |= CY_U3P_UIB_URESET;

    /* Forward handling to the callback function iff it is not fast enumeration,
       or if it is not a standard request.
     */
    if ((glUibDeviceInfo.enumMethod == CY_U3P_USBENUM_PPORT) || (bType != CY_U3P_USB_STANDARD_RQT))
    {
        if ((bType == CY_U3P_USB_STANDARD_RQT) && (bRequest == CY_U3P_USB_SC_SET_CONFIGURATION))
        {
            if (wValue == 1)
            {
                /* Make sure that all EPs are cleared from stall condition and sequence numbers are cleared
                   at this stage. */
                CyU3PUsbEpPrepare ((CyU3PUSBSpeed_t)glUibDeviceInfo.usbSpeed);
                glUibDeviceInfo.usbState         = CY_U3P_USB_ESTABLISHED;
                glUibDeviceInfo.usbActiveConfig  = (uint8_t)wValue;
                glUibDeviceInfo.usbDeviceStat   &= CY_U3P_USB_DEVSTAT_SELFPOWER;
            }
            else
            {
                glUibDeviceInfo.usbState = CY_U3P_USB_CONNECTED;
            }
        }

        if (glUsbSetupCb != NULL)
        {
            isHandled = glUsbSetupCb (setupdat0, setupdat1);
        }

        if (isHandled == CyTrue)
        {
            /* If the request is already handled, then return from this function. */
            glUibDeviceInfo.sendStatusEvent = CyTrue;
            return;
        }
    }

    /* Handle the standard requests iff: fast enumeration or if the callback
     * failed to handle the request. */
    if (bType == CY_U3P_USB_STANDARD_RQT)
    {
        /* Identify and handle setup request. */
        switch (bRequest)
        {
        case CY_U3P_USB_SC_GET_STATUS:
            {
                /* Let the setup callback handle GET_STATUS requests addressed to the interface. */
                if (bTarget == CY_U3P_USB_TARGET_INTF)
                {
                    if (glUsbSetupCb)
                    {
                        isHandled = glUsbSetupCb (setupdat0, setupdat1);
                        if (isHandled)
                            break;
                    }

                    /* If the callback did not handle the request, fall back to the default handler. */
                }

                isHandled = CyU3PUsbHandleGetStatus (bTarget, wIndex);
            }
            break;

        case CY_U3P_USB_SC_CLEAR_FEATURE:
            {
                /* Handle device level feature requests implicitly. */
                if (bTarget == CY_U3P_USB_TARGET_DEVICE)
                {
                    CyU3PUsbHandleClearFeature ((uint8_t)wValue);
                    return;
                }

                isHandled = CyFalse;

                /* Clear feature request is forwarded to the application regardless of the enumeration mode.
                   If the application returns false, then the request is handled here. */
                if ((glUibDeviceInfo.enumMethod == CY_U3P_USBENUM_WB) && (glUsbSetupCb != NULL))
                {
                    isHandled = glUsbSetupCb (setupdat0, setupdat1);
                    if (isHandled)
                        glUibDeviceInfo.sendStatusEvent = CyTrue;
                }

                /* If the application has not handled this request and this is a clear feature (EP HALT),
                   handle it here. */
                if ((!isHandled) && (bTarget == CY_U3P_USB_TARGET_ENDPT) && (wValue == CY_U3P_USBX_FS_EP_HALT))
                {
                    if (CyU3PUsbStall (wIndex, CyFalse, CyTrue) == CY_U3P_SUCCESS)
                    {
                        /* Reset the endpoint as well */
                        CyU3PUsbResetEp (wIndex);
                        CyU3PUsbAckSetup ();
                        isHandled = CyTrue;
                        break;
                    }
                }
            }
            break;

        case CY_U3P_USB_SC_SET_FEATURE:
            {
                /* Handle all Device specific SET Feature commands automatically. */
                if (bTarget == CY_U3P_USB_TARGET_DEVICE)
                {
                    /* All device level SET_FEATURE requests except for OTG requests are handled in firmware. */
                    if ((wValue != CY_U3P_USB2_OTG_B_HNP_ENABLE) && (wValue != CY_U3P_USB2_OTG_A_HNP_SUPPORT))
                    {
                        CyU3PUsbHandleSetFeature ((uint8_t)wValue);
                        return;
                    }
                    else
                    {
                        /* Send the request to the application. */
                        if ((glUibDeviceInfo.enumMethod == CY_U3P_USBENUM_WB) && (glUsbSetupCb != NULL))
                        {
                            isHandled = glUsbSetupCb (setupdat0, setupdat1);
                            if (isHandled)
                            {
                                glUibDeviceInfo.sendStatusEvent = CyTrue;
                                return;
                            }
                        }
                    }
                    break;
                }

                isHandled = CyFalse;

                /* Set feature requests are forwarded to the application regardless of the enumeration mode.
                   If the application returns false, then the request is handled here. */
                if ((glUibDeviceInfo.enumMethod == CY_U3P_USBENUM_WB) && (glUsbSetupCb != NULL))
                {
                    isHandled = glUsbSetupCb (setupdat0, setupdat1);
                    if (isHandled)
                        glUibDeviceInfo.sendStatusEvent = CyTrue;
                }

                /* If the application has not handled this request and this is a set feature (EP HALT),
                   handle it here. */
                if ((!isHandled) && (bTarget == CY_U3P_USB_TARGET_ENDPT) && (wValue == CY_U3P_USBX_FS_EP_HALT))
                {
                    if ((CyU3PUsbStall (wIndex, CyTrue, CyFalse)) == CY_U3P_SUCCESS)
                    {
                        CyU3PUsbAckSetup ();
                        isHandled = CyTrue;
                    }
                }
            }
            break;

        case CY_U3P_USB_SC_GET_DESCRIPTOR:
            {
                isHandled = CyU3PUibSendDescr (setupdat0, setupdat1);
            }
            break;

        case CY_U3P_USB_SC_GET_CONFIGURATION:
            {
                isHandled = CyTrue;
                status    = CyU3PUsbSendEP0Data (1, (uint8_t *)&glUibDeviceInfo.usbActiveConfig);
            }
            break;

        case CY_U3P_USB_SC_SET_CONFIGURATION:
            {
                {
                    isHandled = CyTrue;

                    switch (wValue)
                    {
                    case 1:
                        /* Make sure that all EPs are cleared from stall condition and sequence numbers are cleared
                           at this stage. */
                        CyU3PUsbEpPrepare ((CyU3PUSBSpeed_t)glUibDeviceInfo.usbSpeed);

                    case 0:
                        glUibDeviceInfo.usbState        = (wValue == 0) ? CY_U3P_USB_CONNECTED : CY_U3P_USB_ESTABLISHED;
                        glUibDeviceInfo.usbActiveConfig = (uint8_t)wValue;
                        glUibDeviceInfo.usbDeviceStat  &= CY_U3P_USB_DEVSTAT_SELFPOWER;

                        if (glUsbEvtCb != NULL)
                        {
                            glUsbEvtCb (CY_U3P_USB_EVENT_SETCONF, wValue);
                        }

                        CyU3PUsbAckSetup ();
                        break;

                    default:
                        status = CY_U3P_ERROR_BAD_ARGUMENT;
                        break;
                    }
                }
            }
            break;

        case CY_U3P_USB_SC_GET_INTERFACE:
            {
                if (glUsbSetupCb)
                {
                    isHandled = glUsbSetupCb (setupdat0, setupdat1);
                    if (isHandled)
                        break;
                }

                isHandled = CyTrue;
                status    = CyU3PUsbSendEP0Data (1, (uint8_t *)&glUibDeviceInfo.usbAltSetting);
            }
            break;

        case CY_U3P_USB_SC_SET_INTERFACE:
            {
                /* First send the command to the Setup callback. Accept the request if not handled by the callback. */
                if (glUsbSetupCb)
                {
                    isHandled = glUsbSetupCb (setupdat0, setupdat1);
                    if (isHandled)
                        break;
                }

                glUibDeviceInfo.usbAltSetting = wValue;

                if (glUsbEvtCb != NULL)
                {
                    glUsbEvtCb (CY_U3P_USB_EVENT_SETINTF, CY_U3P_MAKEWORD ((uint8_t)wIndex, (uint8_t)wValue));
                }

                isHandled = CyTrue;
                CyU3PUsbAckSetup ();
            }
            break;

        case CY_U3P_USB_SC_SYNC_FRAME:
            {
                isHandled = CyTrue;
                CyU3PUsbAckSetup ();
            }
            break;

        case CY_U3P_USB_SC_SET_SEL:
            {
                if ((CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED) && (wValue == 0) && (wIndex == 0) && (wLength == 6))
                {
                    isHandled = CyTrue;
                    status = CyU3PUsbGetEP0Data (32, glUibSelBuffer, 0);
                    if ((glUsbEvtCb != NULL) && (status == CY_U3P_SUCCESS))
                    {
                        glUsbEvtCb (CY_U3P_USB_EVENT_SET_SEL, 0x00);
                    }
                }
                else
                {
                    isHandled = CyFalse;
                }
            }
            break;

        case CY_U3P_USB_SC_SET_ISOC_DELAY:
            {
                if ((CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED) && (wIndex == 0) && (wLength == 0))
                {
                    isHandled = CyTrue;
                    CyU3PUsbAckSetup ();
                }
            }
            break;

        default:
            break;
        }
    }

    /* If there has been an error, stall EP0 to fail the transaction. */
    if ((isHandled != CyTrue) || (status != CY_U3P_SUCCESS))
    {
        /* This is an unhandled setup command. Stall the EP. */
        CyU3PUsbStall (0, CyTrue, CyFalse);
    }
}

static void
CyU3PUsbSSDisConnecthandler (
        void);

void CyU3PUsbSSConnecthandler (void)
{
    uint32_t state;

    /* If USB 2.0 PHY is enabled, switch it off and take out the USB 2.0 pullup. */
    if (UIB->otg_ctrl & CY_U3P_UIB_DEV_ENABLE)
    {
        state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
        while ((UIB->otg_ctrl & CY_U3P_UIB_SSDEV_ENABLE) && (state == CY_U3P_UIB_LNK_STATE_POLLING_LFPS))
        {
            CyU3PThreadRelinquish ();
            state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
        }

        if (state == CY_U3P_UIB_LNK_STATE_COMP)
        {
            if (!glUibDeviceInfo.ssCmdSeen)
            {
                CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USBSS_DISCONNECT);
                CyU3PUsbSSDisConnecthandler ();
            }

            glUibDeviceInfo.usbSpeed = CY_U3P_SUPER_SPEED;
            return;
        }

        if ((UIB->otg_ctrl & CY_U3P_UIB_SSDEV_ENABLE) == 0)
        {
            return;
        }

        CyU3PDisconUsbPins ();
        UIB->otg_ctrl &= ~CY_U3P_UIB_DEV_ENABLE;            /* Disable USB 2.0 PHY. */
        UIB->dev_ctl_intr = UIB->dev_ctl_intr;
    }

    /* Switch EPM clock to USB 3.0 mode. */
    GCTL->uib_core_clk = CY_U3P_GCTL_UIBCLK_CLK_EN | (1 << CY_U3P_GCTL_UIBCLK_PCLK_SRC_POS) |
        (1 << CY_U3P_GCTL_UIBCLK_EPMCLK_SRC_POS);

    USB3LNK->lnk_phy_conf = 0xE0000001;

    glUibDeviceInfo.usbSpeed = CY_U3P_SUPER_SPEED;      /* Remember connection speed. */
    UIB->otg_ctrl |= CY_U3P_UIB_SSEPM_ENABLE;           /* Switch EPMs for USB-SS. */

    /* Reset the EPM mux. */
    UIB->iepm_cs |= (CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
    CyU3PBusyWait (1);
    UIB->iepm_cs &= ~(CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
    CyU3PBusyWait (1);

    CyU3PUsbFlushEp (0x00);
    CyU3PUsbFlushEp (0x80);

    /* Enable USB 3.0 control eps. */
    USB3PROT->prot_epi_cs1[0] |= CY_U3P_UIB_SSEPI_VALID;
    UIB->eepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */
    USB3PROT->prot_epo_cs1[0] |= CY_U3P_UIB_SSEPO_VALID;
    UIB->iepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */

    CyU3PUsbResetEp (0x00);
    CyU3PUsbFlushEp (0x00);
    CyU3PUsbResetEp (0x80);
    CyU3PUsbFlushEp (0x80);

    UIB->eepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */
    UIB->iepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */

    /* Propagate the event to the application. */
    if (glUsbEvtCb != NULL)
    {
        glUsbEvtCb (CY_U3P_USB_EVENT_CONNECT, 0x01);
    }

    /* Configure the EPs for super-speed operation. */
    CyU3PUsbEpPrepare (CY_U3P_SUPER_SPEED);
}

static void
HandleVbusOffEvent (
        void)
{
    if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
    {
        CyU3PUsbPhyDisable (CyTrue);
    }
    else
    {
        if (UIB->otg_ctrl & CY_U3P_UIB_SSDEV_ENABLE)
        {
            /* 3.0 PHY is still on, we need to turn that off as well. */
            UIB->otg_ctrl &= ~(CY_U3P_UIB_SSDEV_ENABLE | CY_U3P_UIB_SSEPM_ENABLE);
            CyU3PBusyWait (2);

            /* Need to disable interrupts while updating the MPLL. */
            UIB->intr_mask &= ~(CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT | CY_U3P_UIB_LNK_INT |
                    CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);
            CyU3PBusyWait (1);
            USB3LNK->lnk_phy_conf        &= 0x1FFFFFFF;
            USB3LNK->lnk_phy_mpll_status  = glUsbMpllDefault;
            CyU3PBusyWait (1);
            UIB->intr_mask |= (CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT | CY_U3P_UIB_LNK_INT |
                    CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);
        }

        CyU3PUsbPhyDisable (CyFalse);
    }

    glUsbEvtCb (CY_U3P_USB_EVENT_DISCONNECT, NULL);
    glUibDeviceInfo.isConnected    = CyFalse;
    glUibDeviceInfo.usbState       = CY_U3P_USB_VBUS_WAIT;
    glUibDeviceInfo.usbSpeed       = CY_U3P_NOT_CONNECTED;
    glUibDeviceInfo.ssHostResume   = CyFalse;
    glUibDeviceInfo.tDisabledCount = 0;
    glUibDeviceInfo.forceLpmAccept = CyFalse;
}

void CyU3PUibVbusChangeHandler (
        void)
{
    glUibDeviceInfo.exitCompliance = CyFalse;

    if ((GCTL->iopower & glUibDeviceInfo.vbusDetectMode) != 0)
    {
        CyU3PUsbAddToEventLog (CYU3P_USB_LOG_VBUS_ON);
        if (glUsbEvtCb != NULL)
            glUsbEvtCb (CY_U3P_USB_EVENT_VBUS_VALID, 0);
    }
    else
    {
        CyU3PUsbAddToEventLog (CYU3P_USB_LOG_VBUS_OFF);
        if (glUsbEvtCb != NULL)
            glUsbEvtCb (CY_U3P_USB_EVENT_VBUS_REMOVED, 0);
    }

    /* If VBus is seen to be on and we still have USB in a connected state, we have to assume that
       VBus was removed and then re-applied.
     */
    if ((GCTL->iopower & glUibDeviceInfo.vbusDetectMode) != 0)
    {
        if (glUibDeviceInfo.usbState >= CY_U3P_USB_CONNECTED)
            HandleVbusOffEvent ();

        if (glUibDeviceInfo.usbState == CY_U3P_USB_VBUS_WAIT)
        {
            glUibDeviceInfo.enableUsb3 = glUibDeviceInfo.enableSS;

            if (glUibDeviceInfo.enableSS)
            {
                /* Clear USB 3.0 link errors when there is a VBus state change. */
                USB3LNK->lnk_error_count = 0;
                glUsbLinkErrorCount      = 0;
                glUsbUserLinkErrorCount  = 0;

                glUibDeviceInfo.ssHostResume   = CyFalse;
                glUibDeviceInfo.ssCmdSeen      = CyFalse;
                glUibDeviceInfo.tDisabledCount = 0;
            }

            CyU3PUsbPhyEnable (glUibDeviceInfo.enableSS);
            glUibDeviceInfo.usbState = CY_U3P_USB_CONNECTED;
        }
    }
    else
    {
        if (glUibDeviceInfo.usbState >= CY_U3P_USB_CONNECTED)
            HandleVbusOffEvent ();
    }

    /* Pass on the information to OTG module only if it is enabled. */
    if (CyU3POtgIsStarted ())
    {
        CyU3PUsbOtgVbusChangeHandler ();
    }

    /* Re-enable the interrupt. */
    GCTL->iopwr_intr_mask = glUibDeviceInfo.vbusDetectMode;
}

static void
CyU3PUsbSSDisConnecthandler (
        void)
{
    /* If we still have VBUS, try to connect in USB 2.0 mode. */
    if (CyU3PUsbCanConnect ())
    {
        glUibDeviceInfo.tDisabledCount++;

        if (UIB->otg_ctrl & CY_U3P_UIB_DEV_ENABLE)
        {
            /* If the 2.0 PHY is already on, simply turn off the USB 3.0 PHY. */
            UIB->otg_ctrl &= ~(CY_U3P_UIB_SSDEV_ENABLE | CY_U3P_UIB_SSEPM_ENABLE);
            CyU3PBusyWait (2);

            /* Need to disable interrupts while updating the MPLL. */
            UIB->intr_mask &= ~(CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT | CY_U3P_UIB_LNK_INT |
                    CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);
            CyU3PBusyWait (1);
            USB3LNK->lnk_phy_conf        &= 0x1FFFFFFF;
            USB3LNK->lnk_phy_mpll_status  = glUsbMpllDefault;
            CyU3PBusyWait (1);
            UIB->intr_mask |= (CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT |
                    CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);

            CyU3PSetUsbCoreClock (2, 0);
        }
        else
        {
            glUibDeviceInfo.usbSpeed = CY_U3P_FULL_SPEED;
            CyU3PUsbPhyDisable (CyTrue);

            if (glUibDeviceInfo.usb2Disable)
            {
                if (glPollingRxEqSeen)
                {
                    glPollingRxEqSeen = CyFalse;
                    CyU3PUsbPhyEnable (CyTrue);
                }
                else
                {
                    glUibDeviceInfo.usbState       = CY_U3P_USB_CONFIGURED;
                    glUibDeviceInfo.usbSpeed       = CY_U3P_NOT_CONNECTED;
                    glUibDeviceInfo.isConnected    = CyFalse;
                    if (glUsbEvtCb != NULL)
                        glUsbEvtCb (CY_U3P_USB_EVENT_USB3_LNKFAIL, 0);
                }
            }
            else
            {
                CyU3PUsbPhyEnable (CyFalse);
            }
        }
    }
    else
    {
        /* If no VBUS, disconnect and turn off PHYs. */
        CyU3PUibVbusChangeHandler ();
    }
}

void CyU3PUibDevCtrlEvtHandler (void)
{
    uint32_t state;

    state = UIB->dev_ctl_intr;

    if (state & CY_U3P_UIB_URESUME)
    {
        UIB->dev_ctl_intr = CY_U3P_UIB_URESUME;
        CyU3PUsbResumeHandler ();
        UIB->dev_ctl_intr_mask |= CY_U3P_UIB_URESUME;
    }
    if (state & CY_U3P_UIB_SUSP)
    {
        UIB->dev_ctl_intr = CY_U3P_UIB_SUSP;
        CyU3PUsbSuspendHandler ();
        UIB->dev_ctl_intr_mask |=  CY_U3P_UIB_SUSP;
    }
    if (state & CY_U3P_UIB_URESET)
    {
        UIB->dev_ctl_intr = CY_U3P_UIB_URESET;
        CyU3PUsbResetHandler ();
        UIB->dev_ctl_intr_mask |= CY_U3P_UIB_URESET;
    }
    if (state & CY_U3P_UIB_HSGRANT)
    {
        UIB->dev_ctl_intr = CY_U3P_UIB_HSGRANT;
        CyU3PHsGrantIntrHandler ();
        UIB->dev_ctl_intr_mask |= CY_U3P_UIB_HSGRANT;
    }
    if (state & CY_U3P_UIB_SUDAV)
    {
        /* Make sure that LPM-L1 requests are not accepted from now until the status handshake. */
        UIB->ehci_portsc &= ~CY_U3P_UIB_WKOC_E;

        /* Clear any stall condition before proceeding. */
        UIB->dev_epi_cs[0] &= ~CY_U3P_UIB_EPI_STALL;
        UIB->dev_epo_cs[0] &= ~CY_U3P_UIB_EPO_STALL;

        UIB->dev_ctl_intr = CY_U3P_UIB_SUDAV;
        glUibDeviceInfo.ackPending = CyTrue;
        CyU3PUsbSetupCommand ();
        UIB->dev_ctl_intr_mask |= CY_U3P_UIB_SUDAV;
    }
    if (state & CY_U3P_UIB_ERRLIMIT)
    {
        UIB->dev_ctl_intr = CY_U3P_UIB_ERRLIMIT;
        UIB->dev_ctl_intr_mask |= CY_U3P_UIB_ERRLIMIT;
    }
    if (state & CY_U3P_UIB_STATUS_STAGE)
    {
        /* We can re-enable LPM-L1 handling if not disabled by the application. */
        if (!glUibDeviceInfo.isLpmDisabled)
            UIB->ehci_portsc = CY_U3P_UIB_WKOC_E;

        UIB->dev_ctl_intr = CY_U3P_UIB_STATUS_STAGE;
        if (glUibDeviceInfo.sendStatusEvent)
        {
            if (glUsbEvtCb != NULL)
                glUsbEvtCb (CY_U3P_USB_EVENT_EP0_STAT_CPLT, 0);
            glUibDeviceInfo.sendStatusEvent = CyFalse;
        }
        UIB->dev_ctl_intr_mask |= CY_U3P_UIB_STATUS_STAGE;
    }
}

void CyU3PUibEPEvtHandler (void)
{
    uint32_t state;
    uint8_t  i;
    uint32_t epCs;

    state  = UIB->dev_ep_intr & UIB->dev_ep_intr_mask;
    state &= glUsbEvtEnabledEps;

    /* For each endpoint, check if any endpoint events have been enabled and call the callback function if so. */
    for (i = 1; i < 16; i++)
    {
        if ((state & (1 << i)) != 0)
        {
            epCs = UIB->dev_epi_cs[i];

            if (glUsbEpCb)
            {
                if ((glUsbEpEvtMask & CYU3P_USBEP_NAK_EVT) && (epCs & CY_U3P_UIB_EPI_BNAK))
                {
                    glUsbEpCb (CYU3P_USBEP_NAK_EVT, (CyU3PUSBSpeed_t)glUibDeviceInfo.usbSpeed, 0x80 | i);
                }

                if ((glUsbEpEvtMask & CYU3P_USBEP_ZLP_EVT) && (epCs & CY_U3P_UIB_EPI_ZERO))
                {
                    glUsbEpCb (CYU3P_USBEP_ZLP_EVT, (CyU3PUSBSpeed_t)glUibDeviceInfo.usbSpeed, 0x80 | i);
                }

                if ((glUsbEpEvtMask & CYU3P_USBEP_SLP_EVT) && (epCs & CY_U3P_UIB_EPI_SHORT))
                {
                    glUsbEpCb (CYU3P_USBEP_SLP_EVT, (CyU3PUSBSpeed_t)glUibDeviceInfo.usbSpeed, 0x80 | i);
                }

                if ((glUsbEpEvtMask & CYU3P_USBEP_ISOERR_EVT) && (epCs & CY_U3P_UIB_EPI_ISOERR))
                {
                    glUsbEpCb (CYU3P_USBEP_ISOERR_EVT, (CyU3PUSBSpeed_t)glUibDeviceInfo.usbSpeed, 0x80 | i);
                }
            }

            /* Clear any EP interrupts at this point. */
            UIB->dev_epi_cs[i] = epCs;
        }

        if ((state & (1 << (i + 16))) != 0)
        {
            epCs = UIB->dev_epo_cs[i];

            if (glUsbEpCb)
            {
                if ((glUsbEpEvtMask & CYU3P_USBEP_NAK_EVT) && (epCs & CY_U3P_UIB_EPO_BNAK))
                {
                    glUsbEpCb (CYU3P_USBEP_NAK_EVT, (CyU3PUSBSpeed_t)glUibDeviceInfo.usbSpeed, i);
                }

                if ((glUsbEpEvtMask & CYU3P_USBEP_ZLP_EVT) && (epCs & CY_U3P_UIB_EPO_ZERO))
                {
                    glUsbEpCb (CYU3P_USBEP_ZLP_EVT, (CyU3PUSBSpeed_t)glUibDeviceInfo.usbSpeed, i);
                }

                if ((glUsbEpEvtMask & CYU3P_USBEP_SLP_EVT) && (epCs & CY_U3P_UIB_EPO_SHORT))
                {
                    glUsbEpCb (CYU3P_USBEP_SLP_EVT, (CyU3PUSBSpeed_t)glUibDeviceInfo.usbSpeed, i);
                }

                if ((glUsbEpEvtMask & CYU3P_USBEP_ISOERR_EVT) && (epCs & CY_U3P_UIB_EPO_ISOERR))
                {
                    glUsbEpCb (CYU3P_USBEP_ISOERR_EVT, (CyU3PUSBSpeed_t)glUibDeviceInfo.usbSpeed, i);
                }
            }

            /* Clear any EP interrupts at this point. */
            UIB->dev_epo_cs[i] = epCs;
        }
    }

    /* Re-enable the interrupts. */
    UIB->intr_mask |= CY_U3P_UIB_DEV_EP_INT;
}

void CyU3PUibProtIntrEvtHandler(void)
{
    uint32_t state;
    state = (USB3PROT->prot_intr & USB3PROT->prot_intr_mask);

    /* Setup packet handling for USB 3.0 */
    if (state & CY_U3P_UIB_SUTOK_EV)
    {
       USB3PROT->prot_intr = CY_U3P_UIB_SUTOK_EV;
       glUibDeviceInfo.ackPending = CyTrue;
       CyU3PUsbSetupCommand ();
    }
    if (state & CY_U3P_UIB_EP0_STALLED_EV)
    {
        USB3PROT->prot_epo_cs1[0] &= ~CY_U3P_UIB_SSEPO_STALL;
        USB3PROT->prot_epi_cs1[0] &= ~CY_U3P_UIB_SSEPI_STALL;
        USB3PROT->prot_intr = CY_U3P_UIB_EP0_STALLED_EV;
    }
    if (state & CY_U3P_UIB_STATUS_STAGE)
    {
        if (glUibDeviceInfo.sendStatusEvent)
        {
            if (glUsbEvtCb != NULL)
                glUsbEvtCb (CY_U3P_USB_EVENT_EP0_STAT_CPLT, 0);
            glUibDeviceInfo.sendStatusEvent = CyFalse;
        }
        USB3PROT->prot_intr = CY_U3P_UIB_STATUS_STAGE;
    }

    UIB->intr_mask |= CY_U3P_UIB_PROT_INT;
}

void CyU3PUibProtEpIntrEvtHandler (
        void)
{
    int i;
    uint32_t x, epCs;

    x = USB3PROT->prot_ep_intr & USB3PROT->prot_ep_intr_mask;
    for (i = 1; i < 16; i++)
    {
        /* IN-EP i */
        if ((x & (1 << i)) != 0)
        {
            epCs = USB3PROT->prot_epi_cs1[i];

            /* Out-of-sequence error handler: Set NRDY, Send NRDY TP, Reset EP, Send ERDY TP, Clear NRDY. */
            if (epCs & CY_U3P_UIB_SSEPI_OOSERR)
            {
                USB3PROT->prot_epi_cs1[i] = ((epCs | CY_U3P_UIB_SSEPI_NRDY) & ~(0x0007FF00));

                CyU3PUsbSendNrdy ((i | 0x80), (uint16_t)USB3PROT->prot_epi_mapped_stream[i]);
                CyU3PUsbResetEp (i | 0x80);
                CyU3PBusyWait (10);

                CyU3PUsbSendErdy ((i | 0x80), (uint16_t)USB3PROT->prot_epi_mapped_stream[i]);
                USB3PROT->prot_epi_cs1[i] = epCs | CY_U3P_UIB_SSEPI_OOSERR;

                if ((glUsbEpEvtMask & CYU3P_USBEP_SS_SEQERR_EVT) && (glUsbEvtEnabledEps & (1 << i)))
                    glUsbEpCb (CYU3P_USBEP_SS_SEQERR_EVT, CY_U3P_SUPER_SPEED, 0x80 | i);
            }

            /* DBTERM handler: Just clear the interrupt. */
            if (epCs & CY_U3P_UIB_SSEPI_DBTERM)
            {
                USB3PROT->prot_epi_cs1[i] = (epCs & ~(0x0007FF00)) | CY_U3P_UIB_SSEPI_DBTERM;
            }

            if ((glUsbEvtEnabledEps & (1 << i)) && (glUsbEpCb != 0))
            {
                if ((glUsbEpEvtMask & CYU3P_USBEP_NAK_EVT) && (epCs & CY_U3P_UIB_SSEPI_FLOWCONTROL))
                    glUsbEpCb (CYU3P_USBEP_NAK_EVT, CY_U3P_SUPER_SPEED, 0x80 | i);
                if ((glUsbEpEvtMask & CYU3P_USBEP_ZLP_EVT) && (epCs & CY_U3P_UIB_SSEPI_ZERO))
                    glUsbEpCb (CYU3P_USBEP_ZLP_EVT, CY_U3P_SUPER_SPEED, 0x80 | i);
                if ((glUsbEpEvtMask & CYU3P_USBEP_SLP_EVT) && (epCs & CY_U3P_UIB_SSEPI_SHORT))
                    glUsbEpCb (CYU3P_USBEP_SLP_EVT, CY_U3P_SUPER_SPEED, 0x80 | i);
                if ((glUsbEpEvtMask & CYU3P_USBEP_SS_RETRY_EVT) && (epCs & CY_U3P_UIB_SSEPI_RETRY))
                    glUsbEpCb (CYU3P_USBEP_SS_RETRY_EVT, CY_U3P_SUPER_SPEED, 0x80 | i);
                if ((glUsbEpEvtMask & CYU3P_USBEP_SS_STREAMERR_EVT) && (epCs & CY_U3P_UIB_SSEPI_STREAM_ERROR))
                    glUsbEpCb (CYU3P_USBEP_SS_STREAMERR_EVT, CY_U3P_SUPER_SPEED, 0x80 | i);
            }

            /* Clear all EP interrupts. */
            USB3PROT->prot_epi_cs1[i] = epCs;
        }

        /* OUT-EP i */
        if ((x & (1 << (16 + i))) != 0)
        {
            epCs = USB3PROT->prot_epo_cs1[i];

            if ((glUsbEvtEnabledEps & (1 << (16 + i))) && (glUsbEpCb != 0))
            {
                if ((glUsbEpEvtMask & CYU3P_USBEP_NAK_EVT) && (epCs & CY_U3P_UIB_SSEPO_FLOWCONTROL))
                    glUsbEpCb (CYU3P_USBEP_NAK_EVT, CY_U3P_SUPER_SPEED, i);
                if ((glUsbEpEvtMask & CYU3P_USBEP_ZLP_EVT) && (epCs & CY_U3P_UIB_SSEPO_ZERO))
                    glUsbEpCb (CYU3P_USBEP_ZLP_EVT, CY_U3P_SUPER_SPEED, i);
                if ((glUsbEpEvtMask & CYU3P_USBEP_SLP_EVT) && (epCs & CY_U3P_UIB_SSEPO_SHORT))
                    glUsbEpCb (CYU3P_USBEP_SLP_EVT, CY_U3P_SUPER_SPEED, i);
                if ((glUsbEpEvtMask & CYU3P_USBEP_SS_RETRY_EVT) && (epCs & CY_U3P_UIB_SSEPO_RETRY))
                    glUsbEpCb (CYU3P_USBEP_SS_RETRY_EVT, CY_U3P_SUPER_SPEED, i);
                if ((glUsbEpEvtMask & CYU3P_USBEP_SS_STREAMERR_EVT) && (epCs & CY_U3P_UIB_SSEPO_STREAM_ERROR))
                    glUsbEpCb (CYU3P_USBEP_SS_STREAMERR_EVT, CY_U3P_SUPER_SPEED, i);
                if ((glUsbEpEvtMask & CYU3P_USBEP_SS_SEQERR_EVT) && (epCs & CY_U3P_UIB_SSEPO_OOSERR))
                    glUsbEpCb (CYU3P_USBEP_SS_SEQERR_EVT, CY_U3P_SUPER_SPEED, i);
            }

            USB3PROT->prot_epo_cs1[i] = epCs;
        }
    }

    UIB->intr_mask |= CY_U3P_UIB_PROT_EP_INT;
}

void CyU3PUsbSSReset(void)
{
    uint8_t ep = 0;

    /* Ignore a USB 3.0 reset that happens before we get LTSSM_CONNECT. */
    if (glUibDeviceInfo.usbSpeed != CY_U3P_SUPER_SPEED)
        return;

    /* Send the reset event to the application. */
    if (glUsbEvtCb != NULL)
    {
        glUsbEvtCb (CY_U3P_USB_EVENT_RESET, 1);
    }

    /* Reset the EPM mux. */
    UIB->iepm_cs |= (CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
    CyU3PBusyWait (1);
    UIB->iepm_cs &= ~(CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
    CyU3PBusyWait (1);

    CyU3PUsbFlushEp (0x00);
    CyU3PUsbFlushEp (0x80);

    /* Enable USB 3.0 control eps. */
    USB3PROT->prot_epi_cs1[0] |= CY_U3P_UIB_SSEPI_VALID;
    UIB->eepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */
    USB3PROT->prot_epo_cs1[0] |= CY_U3P_UIB_SSEPO_VALID;
    UIB->iepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */

    CyU3PUsbResetEp (0x00);
    CyU3PUsbFlushEp (0x00);
    CyU3PUsbResetEp (0x80);
    CyU3PUsbFlushEp (0x80);

    UIB->eepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */
    UIB->iepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */

    for (ep = 1; ep < 16; ep++)
    {
        /* Reset, flush and clear stall condition on all valid endpoints. */
        if (glPcktSizeIn[ep].valid == CyTrue)
        {
            CyU3PUsbFlushEp (ep | 0x80);
            CyU3PUsbStall (ep | 0x80, CyFalse, CyTrue);
        }
        if (glPcktSizeOut[ep].valid == CyTrue)
        {
            CyU3PUsbFlushEp (ep);
            CyU3PUsbStall (ep, CyFalse, CyTrue);
        }
    }

    glUibDeviceInfo.usbState = CY_U3P_USB_CONNECTED;
}

const uint32_t glUsb3CompliancePatterns[9] = {
    CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_0_DEFAULT,
    CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_1_DEFAULT,
    CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_2_DEFAULT,
    CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_3_DEFAULT,
    CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_4_DEFAULT,
    CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_5_DEFAULT,
    CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_6_DEFAULT,
    CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_7_DEFAULT,
    CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_8_DEFAULT
};

static void
CyU3PUsbSendCompliancePatterns (
        void)
{
    uint8_t next_cp = 1;
    uint32_t state;

    /* Return from this loop if LTSSM is not currently in Compliance state. */
    state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
    if (state != CY_U3P_UIB_LNK_STATE_COMP)
        return;

    glUibDeviceInfo.usbSpeed     = CY_U3P_SUPER_SPEED;
    glUibDeviceInfo.ssCompliance = CyTrue;

    /* Force LTSSM to stay in compliance state. */
    glUibDeviceInfo.exitCompliance = CyFalse;
    USB3LNK->lnk_ltssm_state = (CY_U3P_UIB_LNK_STATE_COMP << CY_U3P_UIB_LTSSM_OVERRIDE_VALUE_POS) |
        CY_U3P_UIB_LTSSM_OVERRIDE_EN;

    if (glUsbEvtCb != NULL)
        glUsbEvtCb (CY_U3P_USB_EVENT_SS_COMP_ENTRY, 0);

    /* While the device is in compliance state, keep updating the compliance patterns as required. */
    do
    {
        if (USB3LNK->lnk_lfps_observe & CY_U3P_UIB_PING_DET)
        {
            /* Update the compliance pattern and sleep for 1 second before clearing the PING detected bit. */
            USB3LNK->lnk_compliance_pattern_0 = glUsb3CompliancePatterns[next_cp];

            CyU3PThreadSleep (1);
            USB3LNK->lnk_lfps_observe &= ~CY_U3P_UIB_PING_DET;

            /* Special case handling for CP 4 (LFPS signalling). */
            if (next_cp == 4)
            {
                while (((USB3LNK->lnk_lfps_observe & (CY_U3P_UIB_RESET_DET | CY_U3P_UIB_PING_DET)) == 0) &&
                        (CyU3PUsbCanConnect ()) && (glUibDeviceInfo.exitCompliance == CyFalse))
                {
                    USB3LNK->lnk_phy_conf |= CY_U3P_UIB_TXDETECTRX_LB_OVR;
                    CyU3PBusyWait (10);
                    USB3LNK->lnk_phy_conf &= ~CY_U3P_UIB_TXDETECTRX_LB_OVR;
                    CyU3PBusyWait (30);
                }
            }

            next_cp = (next_cp + 1) % 9;
        }

        if ((USB3LNK->lnk_lfps_observe & CY_U3P_UIB_RESET_DET) || (!CyU3PUsbCanConnect ()) ||
                (glUibDeviceInfo.exitCompliance == CyTrue))
        {
            USB3LNK->lnk_lfps_observe &= ~CY_U3P_UIB_RESET_DET;
            USB3LNK->lnk_compliance_pattern_0 = CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_0_DEFAULT;
            USB3LNK->lnk_ltssm_state = 0;
            break;
        }

        CyU3PThreadSleep (1);
    }
    while (1);

    if (glUsbEvtCb != NULL)
        glUsbEvtCb (CY_U3P_USB_EVENT_SS_COMP_EXIT, 0);
    glUibDeviceInfo.ssCompliance = CyFalse;
}

static void
CyU3PUsbCheckUsb3Disconnect (
        void)
{
    uint16_t trials = 400;
    uint8_t state, interval = 2;

    if (glOsTimerInterval > 1)
    {
        trials /= glOsTimerInterval;
        trials++;
        interval = 1;
    }

    state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
    if ((state >= CY_U3P_UIB_LNK_STATE_U0) && (state <= CY_U3P_UIB_LNK_STATE_COMP))
    {
        glInSSReset = 0;
    }

    if (glInSSReset == 0)
        return;

    while (trials--)
    {
        CyU3PThreadSleep (interval);
        state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
        if ((glInSSReset == 0) || ((state >= CY_U3P_UIB_LNK_STATE_U0) && (state <= CY_U3P_UIB_LNK_STATE_COMP)))
        {
            glInSSReset = 0;
            break;
        }
    }

    if (glInSSReset != 0)
    {
        /* Fallback to USB 2.0, if the 2.0 PHY is not already on. */
        if ((UIB->otg_ctrl & CY_U3P_UIB_DEV_ENABLE) != 0)
        {
            UIB->otg_ctrl &= ~(CY_U3P_UIB_SSDEV_ENABLE | CY_U3P_UIB_SSEPM_ENABLE);
            CyU3PBusyWait (2);

            /* Need to disable interrupts while updating the MPLL. */
            UIB->intr_mask &= ~(CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT | CY_U3P_UIB_LNK_INT |
                    CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);
            CyU3PBusyWait (1);
            USB3LNK->lnk_phy_conf        &= 0x1FFFFFFF;
            USB3LNK->lnk_phy_mpll_status  = glUsbMpllDefault;
            CyU3PBusyWait (1);
            UIB->intr_mask |= (CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT |
                    CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);
            CyU3PSetUsbCoreClock (2, 0);
        }
        else
        {
            if (glPollingRxEqSeen)
            {
                glPollingRxEqSeen = CyFalse;
                CyU3PUsbPhyDisable (CyTrue);
                CyU3PUsbPhyEnable (CyTrue);
                return;
            }

            if (glUibDeviceInfo.usb2Disable)
            {
                CyU3PUsbPhyDisable (CyTrue);

                glUibDeviceInfo.usbState    = CY_U3P_USB_CONFIGURED;
                glUibDeviceInfo.usbSpeed    = CY_U3P_NOT_CONNECTED;
                glUibDeviceInfo.isConnected = CyFalse;
                if (glUsbEvtCb != NULL)
                    glUsbEvtCb (CY_U3P_USB_EVENT_USB3_LNKFAIL, 0);
            }
            else
            {
                CyU3PUsbFallBackToUsb2 ();
            }
        }
    }
}

void CyU3PUibLnkIntrEvtHandler(void)
{
    uint32_t state = USB3LNK->lnk_intr;

    if (state & CY_U3P_UIB_LTSSM_CONNECT)
    {
        USB3LNK->lnk_phy_tx_trim = glUsb3TxTrimVal;

        CyU3PUsbSSConnecthandler ();
        USB3LNK->lnk_intr       = CY_U3P_UIB_LTSSM_CONNECT;
        USB3LNK->lnk_intr_mask |= CY_U3P_UIB_LTSSM_CONNECT;
    }
    if (state & CY_U3P_UIB_LTSSM_DISCONNECT)
    {
        CyU3PUsbSSDisConnecthandler ();
        USB3LNK->lnk_intr       = CY_U3P_UIB_LTSSM_DISCONNECT;
        USB3LNK->lnk_intr_mask |= CY_U3P_UIB_LTSSM_DISCONNECT;
    }
    if (state & CY_U3P_UIB_LTSSM_RESET)
    {
        CyU3PUsbSSReset();
        USB3LNK->lnk_intr       = CY_U3P_UIB_LTSSM_RESET;
        USB3LNK->lnk_intr_mask |= CY_U3P_UIB_LTSSM_RESET;
    }
}

void
CyU3PUsbHandleEvents (
        uint32_t flag)
{
    uint32_t mask;

    if (flag & CY_U3P_UIB_EVT_DEV_CTRL_INT)
    {
        CyU3PUibDevCtrlEvtHandler ();
    }
    if (flag & CY_U3P_UIB_EVT_EP_INT)
    {
        CyU3PUibEPEvtHandler ();
    }
    if (flag & CY_U3P_UIB_EVT_LNK_INT)
    {
        CyU3PUibLnkIntrEvtHandler ();
    }
    if(flag & CY_U3P_UIB_EVT_PROT_INT)
    {
        CyU3PUibProtIntrEvtHandler ();
    }
    if(flag & CY_U3P_UIB_EVT_PROT_EP_INT)
    {
        CyU3PUibProtEpIntrEvtHandler ();
    }
    if (flag & CY_U3P_UIB_EVT_VBUS_CHANGE)
    {
        if (glUibDeviceInfo.vbusDetectMode == CY_U3P_VBAT)
            CyU3PThreadSleep (1000);
        CyU3PUibVbusChangeHandler ();
    }
    if (flag & CY_U3P_UIB_EVT_USB3_COMPLIANCE)
    {
        CyU3PUsbSendCompliancePatterns ();
    }
    if (flag & CY_U3P_UIB_EVT_USB3_WARM_RESET)
    {
        if (UIB->otg_ctrl & CY_U3P_UIB_SSEPM_ENABLE)
        {
            glInSSReset = 1;
            glInCheckUsbDisconnect = 1;
            CyU3PUsbCheckUsb3Disconnect ();
            glInCheckUsbDisconnect = 0;
        }
    }
    if (flag & CY_U3P_UIB_EVT_EPM_UNDERRUN)
    {
        if (glUsbEvtCb != NULL)
        {
            glUsbEvtCb (CY_U3P_USB_EVENT_EP_UNDERRUN,
                    (0x80 | ((UIB->eepm_cs & CY_U3P_UIB_URUN_EP_NUM_MASK) >> CY_U3P_UIB_URUN_EP_NUM_POS)));
        }
        UIB->intr = CY_U3P_UIB_EPM_URUN;
        UIB->intr_mask |= CY_U3P_UIB_EPM_URUN;
    }
    if (flag & CY_U3P_UIB_EVT_USB3_U0_RESUME)
    {
        if (glUsbEvtCb != NULL)
        {
            glUsbEvtCb (CY_U3P_USB_EVENT_RESUME, 0);
        }
    }
    if (flag & CY_U3P_UIB_EVT_USB3_U3_SUSPEND)
    {
        if (glUsbEvtCb != NULL)
        {
            glUsbEvtCb (CY_U3P_USB_EVENT_SUSPEND, 0);
        }
    }
    if (flag & CY_U3P_UIB_EVT_UX_REENABLE)
    {
        if (glUsbUxWake)
        {
            /* Make sure the NRDY_ALL bit is cleared. */
            mask = CyU3PVicDisableAllInterrupts ();
            CyU3PBusyWait (1);
            USB3PROT->prot_cs = (USB3PROT->prot_cs & ~(CY_U3P_UIB_SS_NRDY_ALL | CY_U3P_UIB_SS_SETUP_CLR_BUSY));
            CyU3PVicEnableInterrupts (mask);

            CyU3PThreadSleep (1);
            glUsbUxWake = CyFalse;
            if (((USB3PROT->prot_intr & CY_U3P_UIB_SUTOK_EV) == 0) && (!glUibDeviceInfo.isLpmDisabled) &&
                    (!glUibDeviceInfo.ctrlAckDone))
            {
                USB3LNK->lnk_device_power_control = CY_U3P_UIB_AUTO_U1 | CY_U3P_UIB_AUTO_U2;
            }
        }
    }

    if (flag & CY_U3P_UIB_EVT_TRY_UX_EXIT)
    {
        uint32_t state;

        /* Keep trying to return to U0 state as long as there is a pending control request. */
        state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
        if (((state == CY_U3P_UIB_LNK_STATE_U1) || (state == CY_U3P_UIB_LNK_STATE_U2)) && (glUibEp0StatusPending))
        {
            CyU3PBusyWait (5);
            state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
            if ((state == CY_U3P_UIB_LNK_STATE_U1) || (state == CY_U3P_UIB_LNK_STATE_U2))
                CyU3PUsbSetLinkPowerState (CyU3PUsbLPM_U0);
            CyU3PThreadSleep (1);

            /* If the control operation is still pending, keep queueing this event. */
            if (glUibEp0StatusPending)
                CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_TRY_UX_EXIT, CYU3P_EVENT_OR);
        }
    }

    if (flag & CY_U3P_UIB_EVT_LNK_ERR_LIMIT)
    {
        /* There have been too many link errors. Data transfers are likely to be broken. Disconnect and reconnect
           to the host to clean up the link state. */
        CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USBSS_LNKERR);
        CyU3PUsbPhyDisable (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED);
        CyU3PThreadSleep (1);
        if (CyU3PUsbCanConnect ())
            CyU3PUsbPhyEnable (glUibDeviceInfo.enableSS);
    }

    if (flag & CY_U3P_UIB_EVT_HOST_INT)
    {
        if (glHostOtgHandlers.HostIntHandler)
            glHostOtgHandlers.HostIntHandler ();
    }
    if (flag & CY_U3P_UIB_EVT_HOST_EP_INT)
    {
        if (glHostOtgHandlers.HostEpIntHandler)
            glHostOtgHandlers.HostEpIntHandler ();
    }
    if (flag & CY_U3P_UIB_EVT_EHCI_INT)
    {
        if (glHostOtgHandlers.EhciIntHandler)
            glHostOtgHandlers.EhciIntHandler ();
    }
    if (flag & CY_U3P_UIB_EVT_OHCI_INT)
    {
        if (glHostOtgHandlers.OhciIntHandler)
            glHostOtgHandlers.OhciIntHandler ();
    }
    if (flag & CY_U3P_UIB_EVT_OTG_INT)
    {
        if (glHostOtgHandlers.OtgIntHandler)
            glHostOtgHandlers.OtgIntHandler ();
    }
    if (flag & CY_U3P_UIB_EVT_CHGDET_INT)
    {
        if (glHostOtgHandlers.ChgDetIntHandler)
            glHostOtgHandlers.ChgDetIntHandler ();
    }
}

static void
CyU3PUibStatusTimerCb (
		uint32_t arg)
{
    if ((glUibStatusSendErdy) && (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED) && (glUibEp0StatusPending))
    {
        /* Repeat sending ERDY until the status handshake is completed. */
        CyU3PUsbSendErdy (0x00, 0x0000);

        CyU3PTimerModify (&glUibStatusTimer, 100, 0);
        CyU3PTimerStart (&glUibStatusTimer);
    }
}

static void
CyU3PUibLnkErrClrTimerCb (
		uint32_t arg)
{
    /* Clear the 3.0 link error counts. */
    if (CyU3PUsbGetSpeed() == CY_U3P_SUPER_SPEED)
    {
        /* Register the error count for comparison in the STATE CHANGE interrupt handler. */
        glUsbLinkErrorCount = USB3LNK->lnk_error_count;
    }
}

/*
 * Uib Thread: This processes different requests from other fw modules.
 */
void
CyU3PUibThreadEntry (
                     uint32_t threadInput)
{
    /* Local variable */
    uint32_t flag, status;
    uint32_t uibMask = 0;

    /* Setup the event mask */
    uibMask = (CY_U3P_EVENT_QUEUE | CY_U3P_UIB_EVT_DEV_CTRL_INT | CY_U3P_UIB_EVT_EP_INT |
            CY_U3P_UIB_EVT_LNK_INT | CY_U3P_UIB_EVT_PROT_INT | CY_U3P_UIB_EVT_PROT_EP_INT |
            CY_U3P_UIB_EVT_VBUS_CHANGE | CY_U3P_UIB_EVT_USB3_COMPLIANCE | CY_U3P_UIB_EVT_USB3_WARM_RESET |
            CY_U3P_UIB_EVT_USB3_U3_SUSPEND | CY_U3P_UIB_EVT_UX_REENABLE | CY_U3P_UIB_EVT_TRY_UX_EXIT |
            CY_U3P_UIB_EVT_LNK_ERR_LIMIT | CY_U3P_UIB_EVT_HOST_INT | CY_U3P_UIB_EVT_HOST_EP_INT |
            CY_U3P_UIB_EVT_EHCI_INT | CY_U3P_UIB_EVT_OHCI_INT | CY_U3P_UIB_EVT_OTG_INT |
            CY_U3P_UIB_EVT_CHGDET_INT | CY_U3P_UIB_EVT_EPM_UNDERRUN);

    glUibThreadStarted = CyTrue;
        CyU3PMemSet (glUibSelBuffer, 0, 32);

    /* Run the following code repeatedly */
    while (1)
    {
        /* Initialize the variables */
        /* Wait for some event to happen */
        status = CyU3PEventGet (&glUibEvent, uibMask, CYU3P_EVENT_OR_CLEAR, &flag, CYU3P_WAIT_FOREVER);
        if (status != CY_U3P_SUCCESS)
        {
            continue;
        }

        /* Invoke the USB event handler. */
        CyU3PUsbHandleEvents (flag);
        CyU3PThreadRelinquish ();
    }
}

CyU3PReturnStatus_t
CyU3PUibMsgSend (
                 uint32_t *msg,
                 uint32_t waitOption,
                 CyBool_t priority)
{
    uint32_t status;

    if (priority == CyTrue)
    {
        status = CyU3PQueuePrioritySend (&glUibQueue, msg, waitOption);
    }
    else
    {
        status = CyU3PQueueSend (&glUibQueue, msg, waitOption);
    }

    /* Set the Queue event */
    if (status == CY_U3P_SUCCESS)
    {
        status = CyU3PEventSet (&glUibEvent, CY_U3P_EVENT_QUEUE, CYU3P_EVENT_OR);
    }

    return status;
}

static void
CyU3PUibInitVars (
        void)
{
    CyU3PMemSet ((uint8_t *)&glUibDeviceInfo, 0, sizeof (glUibDeviceInfo));
    CyU3PMemSet ((uint8_t *)&glUibDescrPtrs,  0, sizeof (glUibDescrPtrs));
    glUibThreadStarted     = CyFalse;
    glInSSReset            = 0;
    glInCheckUsbDisconnect = 0;
    glUibEp0StatusPending  = CyFalse;
    glUsbEvtCb             = 0;
    glUsbSetupCb           = 0;
    glLpmRqtCb             = 0;
    glUsbEpCb              = 0;
    glUsbEpEvtMask         = 0;
    glUsbEvtEnabledEps     = 0;
    glUsbForceLPMAccept    = CyFalse;
}

/*
 * This function contains the instructions required to be called from CyU3PApplicationDefine
 * function for the UIB module to be working. e.g memory allocation for Uib thread etc.
 */
void
CyU3PUibApplicationDefine (
        void)
{
    uint32_t status;
    uint8_t *pointer;

    CyU3PUibInitVars ();

    /* Define the pport thread */
    pointer = CyU3PMemAlloc (CY_U3P_UIB_STACK_SIZE);
    if (pointer != NULL)
    {
        status = CyU3PThreadCreate (&glUibThread, "04_UIB_THREAD", CyU3PUibThreadEntry, 0, pointer,
        CY_U3P_UIB_STACK_SIZE, CY_U3P_UIB_THREAD_PRIORITY, CY_U3P_UIB_THREAD_PRIORITY,
                CYU3P_NO_TIME_SLICE, CYU3P_DONT_START);
        if (status == 0)
        {
    pointer = CyU3PMemAlloc (CY_U3P_UIB_QUEUE_SIZE);
            if (pointer != NULL)
            {
                status = CyU3PQueueCreate (&glUibQueue, CY_U3P_UIB_MSG_SIZE, pointer, CY_U3P_UIB_QUEUE_SIZE);
            }
            else
                status = CY_U3P_ERROR_MEMORY_ERROR;

            if (status == 0)
                status = CyU3PEventCreate (&glUibEvent);
            if (status == 0)
                status = CyU3PMutexCreate (&glUibLock, CYU3P_NO_INHERIT);
            if (status == 0)
                status = CyU3PTimerCreate (&glUibStatusTimer, CyU3PUibStatusTimerCb, 0x00, 100, 0, CYU3P_NO_ACTIVATE);
            if (status == 0)
                status = CyU3PTimerCreate (&glUibLnkErrClrTimer, CyU3PUibLnkErrClrTimerCb, 0, 1000, 1000,
                        CYU3P_AUTO_ACTIVATE);

            /* Start the thread only if all required resources are created successfully. */
            if (status == 0)
                CyU3PThreadResume (&glUibThread);
        }
    }
}

extern void
CyU3PUibIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));
extern void
CyU3PGctlPowerIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));

/*
 * UIB Interrupt handler
 */
void
CyU3PUibIntHandler (
                    void)
{
    uint32_t state, devState;
    uint32_t lnkState, tmp;

    state = (UIB->intr & UIB->intr_mask);

    if (state & CY_U3P_UIB_LNK_INT)
    {
        lnkState = USB3LNK->lnk_intr & USB3LNK->lnk_intr_mask;

        if (lnkState & CY_U3P_UIB_LGO_U3)
        {
            /* Unconditionally accept entry into U3 state. */
            USB3LNK->lnk_device_power_control |= CY_U3P_UIB_TX_LAU;

            USB3LNK->lnk_intr = CY_U3P_UIB_LGO_U3;
            CyU3PUsbAddToEventLog (CYU3P_USB_LOG_LGO_U3);
        }

        if (lnkState & CY_U3P_UIB_LTSSM_STATE_CHG)
        {
            /* Clear the interrupt first to improve chances of not missing state change interrupts. */
            USB3LNK->lnk_intr = CY_U3P_UIB_LTSSM_STATE_CHG;

            /* If HP timeouts have happened twice, increase the timeout period to 20 us. */
            if (USB3LNK->lnk_error_status & CY_U3P_UIB_HP_TIMEOUT_EV)
            {
                glUibDeviceInfo.hpTimeoutCnt++;
                if (glUibDeviceInfo.hpTimeoutCnt >= 2)
                    CyFx3Usb3LnkRelaxHpTimeout ();

                USB3LNK->lnk_error_status = CY_U3P_UIB_HP_TIMEOUT_EV;
                CyU3PUsbAddToEventLog (0xAC);
            }

            /* When there is a Link state change, check the following:
             * 1. If the new state is the compliance state, enable the compliance pattern generation task.
             * 2. If the new state is any non-Ux state, enable the task that checks for SS connection disable.
             */

            tmp = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;

#ifdef CYU3P_DEBUG
            CyU3PUsbAddToEventLog (CYU3P_USB_LOG_LTSSM_CHG + tmp);
#endif

            /* If we have seen too many errors since the last timer interrupt, reset the USB link. Ensure that
               this is only done when the register value exceeds the stored value, to avoid the possibility that
               the hardware has cleared the register unexpectedly. */
            if (((USB3LNK->lnk_error_count & CY_U3P_UIB_LINK_ERROR_COUNT_MASK) > glUsbLinkErrorCount) &&
                        (((USB3LNK->lnk_error_count & CY_U3P_UIB_LINK_ERROR_COUNT_MASK) - glUsbLinkErrorCount)
                         >= CY_U3P_USBLNK_ERRCNT_LIMIT))
            {
                /* Turn off UIB Interrupts so as to have enough time to process the event. */
                UIB->intr_mask &= ~(CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT | CY_U3P_UIB_LNK_INT |
                        CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);

                glInSSReset = 0;        /* Make sure we do not get stuck in compliance state. */
                CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_LNK_ERR_LIMIT, CYU3P_EVENT_OR);

                CyU3PUsbAddToEventLog (0xAD);
            }

            switch (tmp)
            {
            case CY_U3P_UIB_LNK_STATE_RECOV_ACT:
            case CY_U3P_UIB_LNK_STATE_RECOV_CNFG:

                /* Set the NRDY_ALL bit. */
                USB3PROT->prot_cs = (USB3PROT->prot_cs & ~CY_U3P_UIB_SS_SETUP_CLR_BUSY) | CY_U3P_UIB_SS_NRDY_ALL;

                if (glRxValidMod)
                {
                    glRxValidMod = CyFalse;
                    CyFx3UsbWritePhyReg (0x1005, 0x0000);
                }

                if ((!glUsbUxWake) && (glUsbEvtCb != 0))
                    glUsbEvtCb (CY_U3P_USB_EVENT_LNK_RECOVERY, 0);
                break;

            case CY_U3P_UIB_LNK_STATE_U2:
                CyU3PBusyWait (1);
                tmp = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
                if ((tmp == CY_U3P_UIB_LNK_STATE_U2) && (!glRxValidMod))
                {
                    glRxValidMod = CyTrue;
                    CyFx3UsbWritePhyReg (0x1005, 0x0020);
                }

                /* Intentional fall-through. */
            case CY_U3P_UIB_LNK_STATE_U1:

                /* Set the NRDY_ALL bit. */
                USB3PROT->prot_cs = (USB3PROT->prot_cs & ~CY_U3P_UIB_SS_SETUP_CLR_BUSY) | CY_U3P_UIB_SS_NRDY_ALL;

                /* Deferred bit handling. Prevent U1/U2 entry for some time after device
                   wakes to U0 from U1/U2 states. */
                if ((!glUsbForceLPMAccept) && (!glUibDeviceInfo.forceLpmAccept))
                {
                    glUsbUxWake = CyTrue;
                    USB3LNK->lnk_device_power_control = CY_U3P_UIB_NO_U1 | CY_U3P_UIB_NO_U2;
                }

                if ((!glUibDeviceInfo.forceLpmAccept) && ((glLpmRqtCb == 0) ||
                            (!glLpmRqtCb ((CyU3PUsbLinkPowerMode)(CyU3PUsbLPM_U1 + (tmp - CY_U3P_UIB_LNK_STATE_U1)))) ||
                            (glUibEp0StatusPending)))
                {
                    CyU3PBusyWait (1);
                    if ((USB3LNK->lnk_device_power_control & CY_U3P_UIB_EXIT_LP) == 0)
                    {
                        tmp = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
                        if ((tmp == CY_U3P_UIB_LNK_STATE_U1) || (tmp == CY_U3P_UIB_LNK_STATE_U2))
                            USB3LNK->lnk_device_power_control |= CY_U3P_UIB_EXIT_LP;
                    }
                }

                if (glUibEp0StatusPending)
                    CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_TRY_UX_EXIT, CYU3P_EVENT_OR);
                break;

            case CY_U3P_UIB_LNK_STATE_U0:
                /* Clear the NRDY_ALL bit. */
                USB3PROT->prot_cs = (USB3PROT->prot_cs & ~(CY_U3P_UIB_SS_NRDY_ALL | CY_U3P_UIB_SS_SETUP_CLR_BUSY));
                glPollingRxEqSeen = CyFalse;

                /* Clear the LFPS_OBSERVE register. */
                USB3LNK->lnk_lfps_observe = 0;

                if (glUibDeviceInfo.ssHostResume)
                {
                    CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_USB3_U0_RESUME, CYU3P_EVENT_OR);
                    glUibDeviceInfo.ssHostResume = CyFalse;
                }

                if (glUsbUxWake)
                {
                    CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_UX_REENABLE, CYU3P_EVENT_OR);
                }
                break;

            case CY_U3P_UIB_LNK_STATE_U3:
                glUibDeviceInfo.ssHostResume = CyTrue;

                glRxValidMod = CyTrue;
                CyFx3UsbWritePhyReg (0x1005, 0x0020);

                CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_USB3_U3_SUSPEND, CYU3P_EVENT_OR);
                break;

            case CY_U3P_UIB_LNK_STATE_POLLING_RxEQ:
                glPollingRxEqSeen = CyTrue;
                /* Fall through to next case. */

            case CY_U3P_UIB_LNK_STATE_POLLING_LFPS:

                /* ZKR added to pass TD9.25 behind a SS hub */
                USB3PROT->prot_lmp_port_capability_timer    = CY_U3P_UIB_PROT_LMP_PORT_CAP_TIMER_VALUE;
                USB3PROT->prot_lmp_port_configuration_timer = CY_U3P_UIB_PROT_LMP_PORT_CFG_TIMER_VALUE;
                break;

            case CY_U3P_UIB_LNK_STATE_COMP:
                if (glUibDeviceInfo.ssCmdSeen)
                {
                    /* This is not expected to be a compliance mode entry. Treat this as equivalent to
                       LTSSM_DISCONNECT. */
                    CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USBSS_DISCONNECT);
                    CyU3PUsbSSDisConnecthandler ();
                }
                else
                {
                    /* Compliance mode : clear RESET LFPS detected bit before starting. */
                    USB3LNK->lnk_lfps_observe &= ~CY_U3P_UIB_RESET_DET;
                    CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_USB3_COMPLIANCE, CYU3P_EVENT_OR);
                    glInSSReset = 0;
                }
                break;
            }

            if ((tmp < CY_U3P_UIB_LNK_STATE_U0) || (tmp > CY_U3P_UIB_LNK_STATE_COMP))
            {
                if ((!glInCheckUsbDisconnect) && (!glUibDeviceInfo.ssHostResume))
                {
                    CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_USB3_WARM_RESET, CYU3P_EVENT_OR);
                }
            }
            else
            {
                glInSSReset = 0;
            }
        }

        if (lnkState & CY_U3P_UIB_LTSSM_RESET)
        {
            USB3PROT->prot_epo_cs1[0] &= ~CY_U3P_UIB_SSEPO_STALL;
            USB3PROT->prot_epi_cs1[0] &= ~CY_U3P_UIB_SSEPI_STALL;
            CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_LNK_INT, CYU3P_EVENT_OR);
            USB3LNK->lnk_intr_mask &= ~CY_U3P_UIB_LTSSM_RESET;

            CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USBSS_RESET);
        }

        if (lnkState & CY_U3P_UIB_LTSSM_CONNECT)
        {
            USB3PROT->prot_epo_cs1[0] &= ~CY_U3P_UIB_SSEPO_STALL;
            USB3PROT->prot_epi_cs1[0] &= ~CY_U3P_UIB_SSEPI_STALL;
            CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_LNK_INT, CYU3P_EVENT_OR);
            USB3LNK->lnk_intr_mask &= ~CY_U3P_UIB_LTSSM_CONNECT;

            CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USBSS_CONNECT);
        }

        if (lnkState & CY_U3P_UIB_LTSSM_DISCONNECT)
        {
            CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_LNK_INT, CYU3P_EVENT_OR);
            USB3LNK->lnk_intr_mask &= ~CY_U3P_UIB_LTSSM_DISCONNECT;
            CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USBSS_DISCONNECT);
        }
    }

    if (state & CY_U3P_UIB_PROT_INT)
    {
        devState = (USB3PROT->prot_intr & USB3PROT->prot_intr_mask);

        if ((devState & CY_U3P_UIB_TIMEOUT_PORT_CAP_EV) || (devState & CY_U3P_UIB_TIMEOUT_PORT_CFG_EV))
        {
            USB3LNK->lnk_ltssm_state   = 0x00002000; /* ZKR force SS.Disabled state */
            USB3PROT->prot_intr        = 0xFFFFFFFF; /* Clear all Protocol interrupts. */
        }
        if (devState & CY_U3P_UIB_LMP_PORT_CFG_EV)
        {
            USB3PROT->prot_lmp_port_configuration_timer = CY_U3P_UIB_TX_DISABLE | CY_U3P_UIB_RX_DISABLE;
            USB3PROT->prot_intr                         = CY_U3P_UIB_LMP_PORT_CFG_EV;
            glUibDeviceInfo.tDisabledCount = 0;
        }
        if (devState & CY_U3P_UIB_LMP_PORT_CAP_EV)
        {
            USB3PROT->prot_lmp_port_capability_timer = CY_U3P_UIB_TX_DISABLE | CY_U3P_UIB_RX_DISABLE;
            USB3PROT->prot_intr                      = CY_U3P_UIB_LMP_PORT_CAP_EV;
        }
        if (devState & CY_U3P_UIB_LMP_RCV_EV)
        {
            if (USB3PROT->prot_lmp_received & CY_U3P_UIB_FORCE_LINKPM_ACCEPT)
            {
                glUibDeviceInfo.forceLpmAccept = CyTrue;
                USB3LNK->lnk_device_power_control = CY_U3P_UIB_YES_U1 | CY_U3P_UIB_YES_U2 ;
            }
            else
            {
                glUibDeviceInfo.forceLpmAccept = CyFalse;
                if (!glUsbForceLPMAccept)
                {
                    if (glUibDeviceInfo.isLpmDisabled)
                        USB3LNK->lnk_device_power_control = CY_U3P_UIB_NO_U1 | CY_U3P_UIB_NO_U2;
                    else
                        USB3LNK->lnk_device_power_control = CY_U3P_UIB_AUTO_U1 | CY_U3P_UIB_AUTO_U2;
                }
            }
            USB3PROT->prot_intr = CY_U3P_UIB_LMP_RCV_EV;
        }
        if (devState & CY_U3P_UIB_SUTOK_EV)
        {
            glUibDeviceInfo.newCtrlRqt = CyTrue;
            USB3PROT->prot_epo_cs1[0] &= ~CY_U3P_UIB_SSEPO_STALL;
            USB3PROT->prot_epi_cs1[0] &= ~CY_U3P_UIB_SSEPI_STALL;

            glUibDeviceInfo.ssHostResume = CyFalse;
            glUibEp0StatusPending = CyTrue;
            CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_PROT_INT, CYU3P_EVENT_OR);
            UIB->intr_mask &= ~CY_U3P_UIB_PROT_INT;
            CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USBSS_CTRL);
        }
        if (devState & CY_U3P_UIB_STATUS_STAGE)
        {
            glUibStatusSendErdy         = CyFalse;
            glUibEp0StatusPending       = CyFalse;
            glUibDeviceInfo.ctrlAckDone = CyFalse;

            /* Enable transition to U1/U2, if not explicitly disabled by the application. */
            if ((!glUsbForceLPMAccept) && (!glUibDeviceInfo.forceLpmAccept) && (!glUibDeviceInfo.isLpmDisabled) &&
                    (!glUsbUxWake))
                USB3LNK->lnk_device_power_control = CY_U3P_UIB_AUTO_U1 | CY_U3P_UIB_AUTO_U2;

            if (glUibDeviceInfo.sendStatusEvent)
            {
                CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_PROT_INT, CYU3P_EVENT_OR);
                UIB->intr_mask &= ~CY_U3P_UIB_PROT_INT;
            }
            else
                USB3PROT->prot_intr = CY_U3P_UIB_STATUS_STAGE;
            CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USBSS_STATUS);
        }
        if (devState & CY_U3P_UIB_EP0_STALLED_EV)
        {
            USB3PROT->prot_epo_cs1[0] &= ~CY_U3P_UIB_SSEPO_STALL;
            USB3PROT->prot_epi_cs1[0] &= ~CY_U3P_UIB_SSEPI_STALL;
            USB3PROT->prot_intr = CY_U3P_UIB_EP0_STALLED_EV;
        }
        if (devState & CY_U3P_UIB_ITP_EV)
        {
            USB3PROT->prot_intr = CY_U3P_UIB_ITP_EV;
            if ((glUibDeviceInfo.sofEventEnable) && (glUsbEvtCb != 0))
            {
                glUsbEvtCb (CY_U3P_USB_EVENT_SOF_ITP, 0);
            }
        }
    }

    if (state & CY_U3P_UIB_DEV_CTL_INT)
    {
        devState = (UIB->dev_ctl_intr & UIB->dev_ctl_intr_mask);
        if (devState & CY_U3P_UIB_SUDAV)
        {
            glUibDeviceInfo.newCtrlRqt = CyTrue;
            UIB->dev_epo_cs[0] &= ~CY_U3P_UIB_EPO_STALL;
            UIB->dev_epi_cs[0] &= ~CY_U3P_UIB_EPI_STALL;
            UIB->dev_ctl_intr_mask &= ~CY_U3P_UIB_SUDAV;
        }

        if (devState & CY_U3P_UIB_URESET)
        {
            CyU3PDmaChannelReset (&glUibChHandle);
            CyU3PDmaChannelReset (&glUibChHandleOut);

            glUibDeviceInfo.inReset = 1;
            CyU3PUsbAckSetup ();
            UIB->dev_ctl_intr_mask &= ~CY_U3P_UIB_URESET;
            CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USB2_RESET);
        }

        if (devState & CY_U3P_UIB_SUSP)
        {
            /* USB 2.0 LPM-L1 workaround. */
            if (UIB->ohci_rh_port_status & CY_U3P_UIB_RHP_PORT_RESUME_FW)
            {
                UIB->phy_chirp = CY_U3P_UIB_OVERRIDE_FSM | 0x10;
                UIB->dev_pwr_cs |= CY_U3P_UIB_DEV_SUSPEND;

                /* Enable wake-up interrupts on D+/D- Change. */
                GCTLAON->wakeup_event = GCTLAON->wakeup_event;
                GCTLAON->wakeup_en = CY_U3P_GCTL_EN_UIB_DP | CY_U3P_GCTL_EN_UIB_DM;
                VIC->int_enable = (1 << CY_U3P_VIC_GCTL_CORE_VECTOR);

                /* Don't treat this as a regular suspend callback. Just clear the interrupt
                   so that the SUSPEND event callback is not generated. */
                UIB->dev_ctl_intr = CY_U3P_UIB_SUSP;
            }
            else
                UIB->dev_ctl_intr_mask &= ~CY_U3P_UIB_SUSP;

            CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USB2_SUSP);
        }

        if (devState & CY_U3P_UIB_HSGRANT)
        {
            UIB->dev_ctl_intr_mask &= ~CY_U3P_UIB_HSGRANT;
            CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USB2_HSGRANT);
        }
        if (devState & CY_U3P_UIB_ERRLIMIT)
            UIB->dev_ctl_intr_mask &= ~CY_U3P_UIB_ERRLIMIT;
        if (devState & CY_U3P_UIB_URESUME)
            UIB->dev_ctl_intr_mask &= ~CY_U3P_UIB_URESUME;
        if (devState & CY_U3P_UIB_STATUS_STAGE)
            UIB->dev_ctl_intr_mask &= ~CY_U3P_UIB_STATUS_STAGE;

        if (devState & CY_U3P_UIB_SOF)
        {
            UIB->dev_ctl_intr = CY_U3P_UIB_SOF;
            if ((glUsbEvtCb != NULL) && (glUibDeviceInfo.sofEventEnable))
            {
                glUsbEvtCb (CY_U3P_USB_EVENT_SOF_ITP, 0);
            }
        }

        /* Set the event as long as there is at least one interrupt other than SOF. */
        if (devState != CY_U3P_UIB_SOF)
            CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_DEV_CTRL_INT, CYU3P_EVENT_OR);
    }

    if (state & CY_U3P_UIB_PROT_EP_INT)
    {
        CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_PROT_EP_INT, CYU3P_EVENT_OR);
        UIB->intr_mask &= ~CY_U3P_UIB_PROT_EP_INT;
    }

    if (state & CY_U3P_UIB_DEV_EP_INT)
    {
        CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_EP_INT, CYU3P_EVENT_OR);
        UIB->intr_mask &= ~CY_U3P_UIB_DEV_EP_INT;
    }

    if (state & CY_U3P_UIB_EPM_URUN)
    {
        /* EPM underrun error. Raise an event to send a user callback. */
        UIB->intr_mask &= ~CY_U3P_UIB_EPM_URUN;
        CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_EPM_UNDERRUN, CYU3P_EVENT_OR);
    }

    if (state & CY_U3P_UIB_HOST_INT)
    {
        CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_HOST_INT, CYU3P_EVENT_OR);
        UIB->intr_mask &= ~CY_U3P_UIB_HOST_INT;
    }

    if (state & CY_U3P_UIB_HOST_EP_INT)
    {
        CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_HOST_EP_INT, CYU3P_EVENT_OR);
        UIB->intr_mask &= ~CY_U3P_UIB_HOST_EP_INT;
    }

    if (state & CY_U3P_UIB_EHCI_INT)
    {
        CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_EHCI_INT, CYU3P_EVENT_OR);
        UIB->intr_mask &= ~CY_U3P_UIB_EHCI_INT;
    }

    if (state & CY_U3P_UIB_OHCI_INT)
    {
        CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_OHCI_INT, CYU3P_EVENT_OR);
        UIB->intr_mask &= ~CY_U3P_UIB_OHCI_INT;
    }

    if (state & CY_U3P_UIB_OTG_INT)
    {
        CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_OTG_INT, CYU3P_EVENT_OR);
        UIB->intr_mask &= ~CY_U3P_UIB_OTG_INT;
    }

    if (state & CY_U3P_UIB_CHGDET_INT)
    {
        CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_CHGDET_INT, CYU3P_EVENT_OR);
        UIB->intr_mask &= ~CY_U3P_UIB_CHGDET_INT;
    }
}

void
CyU3PGctlPowerIntHandler (
        void)
{
    uint32_t state;

    state = (GCTL->iopwr_intr & GCTL->iopwr_intr_mask);

    if (state & glUibDeviceInfo.vbusDetectMode)
    {
        /* Clear the interrupt and raise an event for the UIB thread to process. */
        GCTL->iopwr_intr_mask = 0;
        GCTL->iopwr_intr      = CY_U3P_VBUS | CY_U3P_VBAT;

        CyU3PEventSet (&glUibEvent, CY_U3P_UIB_EVT_VBUS_CHANGE, CYU3P_EVENT_OR);
        glInSSReset = 0;
        glUibDeviceInfo.exitCompliance = CyTrue;
    }
}

/*[]*/

