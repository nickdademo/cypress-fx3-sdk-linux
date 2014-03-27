/*
## Cypress USB 3.0 Platform source file (cyfx3usb.c)
## ===========================
##
##  Copyright Cypress Semiconductor Corporation, 2011-2012,
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

#include "cyfx3bootloader.h"
#include "cyfx3device.h"
#include "cyfx3usb.h"

#include "cyfx3_api.h"
#include "lpp_regs.h"
#include "gctl_regs.h"
#include "gctlaon_regs.h"
#include "uib_regs.h"
#include "uibin_regs.h"
#include "usb3lnk_regs.h"
#include "usb3prot_regs.h"
#include "vic_regs.h"

#define CY_FX3_USB_EP0_PKT_SIZE             (64)   /* USB High Speed Ep0 Packet Size  */
#define CY_FX3_USB_REQ_DEVICE               (0)    /* Request targetted at device */
#define CY_FX3_USB_REQ_INTERFACE            (1)   /* The USB Target Interface */
#define CY_FX3_USB_REQ_ENDPOINT             (2)    /* Endpoint Request */
#define CY_FX3_USB_FEATURE_ENDPOINT_STALL   (0)    /* USB Standard EP Stall Feature selector */

/* Position of device address field in USB 3.0 LMP and TP. */
#define CY_FX3_BOOT_USB3_TP_DEVADDR_POS              (25)

/* Position of endpoint number field in USB 3.0 TP. */
#define CY_FX3_BOOT_USB3_TP_EPNUM_POS                (8)

#define CY_FX3_USB_PROT_LMP_PORT_CAP_TIMER_VALUE        (0x00000908)    /* 18.5 us */
#define CY_FX3_USB_PROT_LMP_PORT_CFG_TIMER_VALUE        (0x00000908)    /* 18.5 us */

/* Summary
   USB 3.0 packet type codes.
 */
typedef enum CyU3PUsb3PacketType
{
    CY_U3P_USB3_PACK_TYPE_LMP = 0x00,           /* Link Management Packet. */
    CY_U3P_USB3_PACK_TYPE_TP  = 0x04,           /* Transaction Packet. */
    CY_U3P_USB3_PACK_TYPE_DPH = 0x08,           /* Data Packet Header. */
    CY_U3P_USB3_PACK_TYPE_ITP = 0x0C            /* Isochronous Timestamp Packet. */
} CyU3PUsb3PacketType;

/* Summary
   USB 3.0 Transaction Packet Sub Type codes.
 */
typedef enum CyU3PUsb3TpSubType
{
    CY_U3P_USB3_TP_SUBTYPE_RES = 0,             /* Reserved. */
    CY_U3P_USB3_TP_SUBTYPE_ACK,                 /* ACK TP. */
    CY_U3P_USB3_TP_SUBTYPE_NRDY,                /* NRDY TP. */
    CY_U3P_USB3_TP_SUBTYPE_ERDY,                /* ERDY TP. */
    CY_U3P_USB3_TP_SUBTYPE_STATUS,              /* STATUS TP. */
    CY_U3P_USB3_TP_SUBTYPE_STALL,               /* STALL TP. */
    CY_U3P_USB3_TP_SUBTYPE_NOTICE,              /* DEV_NOTIFICATION TP. */
    CY_U3P_USB3_TP_SUBTYPE_PING,                /* PING TP. */
    CY_U3P_USB3_TP_SUBTYPE_PINGRSP              /* PING RESPONSE TP. */
} CyU3PUsb3TpSubType;

/* Summary
   Link state machine states.

   Description
   These are the following link states of interest to firmware.
 */
typedef enum CyFx3BootUsbLinkState_t
{
    USB_LNK_STATE_SSDISABLED = 0x00,     /* SS.Disabled */
    USB_LNK_STATE_RXDETECT_RES = 0x01,   /* Rx.Detect.Reset */
    USB_LNK_STATE_RXDETECT_ACT = 0x02,   /* Rx.Detect.Active */
    USB_LNK_STATE_POLLING_LFPS = 0x08,   /* Polling.LFPS */
    USB_LNK_STATE_POLLING_RxEQ = 0x09,   /* Polling.RxEq */
    USB_LNK_STATE_POLLING_ACT = 0x0A,    /* Polling.Active */
    USB_LNK_STATE_POLLING_IDLE = 0x0C,   /* Polling.Idle */
    USB_LNK_STATE_U0 = 0x10,             /* U0 - Active state */
    USB_LNK_STATE_U1 = 0x11,             /* U1 */
    USB_LNK_STATE_U2 = 0x12,             /* U2 */
    USB_LNK_STATE_U3 = 0x13,             /* U3 - Suspend state */
    USB_LNK_STATE_COMP = 0x17,           /* Compliance */
    USB_LNK_STATE_RECOV_ACT = 0x18,      /* Recovery.Active */
    USB_LNK_STATE_RECOV_CNFG = 0x19,     /* Recovery.Configuration */
    USB_LNK_STATE_RECOV_IDLE = 0x1A      /* Recovery.Idle */
} CyFx3BootUsbLinkState_t;

uint32_t glDescTableOffset = 0;

/* Summary:
   An instance of the descriptor pointer structure used to hold pointers to the
   various usb descriptors.
 */
CyU3PUsbDescrPtrs glUsbDescPtr;

/* The application setup callback function pointer */
CyFx3BootUSBSetupCb_t gSetupDataCbk;
CyFx3BootUsbSpeed_t glUsbSpeed;
CyFx3BootUSBEventCb_t gEvtCbk;

extern uint32_t glUsbMpllDefault;
CyBool_t glNoReenum = CyFalse;
CyBool_t glSsEnable = CyFalse;
volatile uint8_t glRxValidMod = CyFalse;
volatile uint8_t glUsbUxWake = CyFalse;
volatile uint8_t glInSSReset = CyFalse;
volatile uint8_t tDisabledCount = 0;
volatile uint8_t glInIsr = 0;
volatile uint8_t glNumDesc = 0;
volatile CyBool_t glLpmDisabled = CyFalse;
volatile CyBool_t glUibStatusSendErdy = CyFalse;

CyBool_t glEnableUsb3 = CyFalse;
CyBool_t glInCheckUsbDisconnect = CyFalse;
CyBool_t glUibEp0StatusPending = CyFalse; /* Pending status phase for USB-3 setup rqt. */
CyBool_t glUibForceLpmAccept = CyFalse;

extern CyBool_t glBootUsbIsOn;

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

#ifndef CY_USE_ARMCC
void
__nop (
        void)
{
    __asm__ volatile
        (
         "mov r0, r0\n\t"
        );
}
#endif
void
memCopy (
        uint8_t *d,
        uint8_t *s,
        int32_t cnt
        )
{
    int32_t i;
    for (i = 0; i < cnt; i++)
    {
        *d++ = *s++;
    }
}

void
CyFx3BootUsbLPMEnable (
        void)
{
    glLpmDisabled = CyFalse;
    if ((glUsbSpeed == CY_FX3_BOOT_SUPER_SPEED) && (glUibStatusSendErdy == CyFalse))
        USB3LNK->lnk_device_power_control = CY_U3P_UIB_AUTO_U1 | CY_U3P_UIB_AUTO_U2;
}

void
CyFx3BootUsbLPMDisable (
        void)
{
    if (glUsbSpeed == CY_FX3_BOOT_SUPER_SPEED)
        USB3LNK->lnk_device_power_control = CY_U3P_UIB_NO_U1 | CY_U3P_UIB_NO_U2;
    glLpmDisabled = CyTrue;
}

static void
ChangeUsbClockFreq (
        uint8_t pclk,
        uint8_t epmclk)
{
    uint8_t i;

    /* Turn off the clock first. */
    GCTL->uib_core_clk &= ~CY_U3P_GCTL_UIBCLK_CLK_EN;
    for (i = 20; i > 0; i--)
        __nop ();

    /* Update the divider fields, and then turn the clock back on. */
    GCTL->uib_core_clk  = (pclk << CY_U3P_GCTL_UIBCLK_PCLK_SRC_POS) | (epmclk << CY_U3P_GCTL_UIBCLK_EPMCLK_SRC_POS);
    GCTL->uib_core_clk |= CY_U3P_GCTL_UIBCLK_CLK_EN;

    for (i = 80; i > 0; i--)
        __nop ();
}

static void
CyFx3BootUsbPhyEnable (
        CyBool_t is_ss)
{
    UIB->otg_intr     = 0xFFFFFFFF;

    USB3LNK->lnk_intr = 0xFFFFFFFF;
    USB3LNK->lnk_intr_mask = CY_U3P_UIB_LGO_U3 | CY_U3P_UIB_LTSSM_CONNECT | CY_U3P_UIB_LTSSM_DISCONNECT |
        CY_U3P_UIB_LTSSM_RESET | CY_U3P_UIB_LTSSM_STATE_CHG;

    USB3PROT->prot_ep_intr_mask = 0;

    USB3PROT->prot_intr = 0xFFFFFFFF;
    USB3PROT->prot_intr_mask = (CY_U3P_UIB_SUTOK_EN | CY_U3P_UIB_STATUS_STAGE | CY_U3P_UIB_TIMEOUT_PORT_CAP_EN |
                                CY_U3P_UIB_TIMEOUT_PORT_CFG_EN | CY_U3P_UIB_LMP_RCV_EN | CY_U3P_UIB_LMP_PORT_CAP_EN |
                                CY_U3P_UIB_LMP_PORT_CFG_EN);

    UIB->dev_ctl_intr = 0xFFFFFFFF;
    UIB->dev_ctl_intr_mask = (CY_U3P_UIB_SUDAV | CY_U3P_UIB_URESET |
                            CY_U3P_UIB_SUSP | CY_U3P_UIB_URESUME | CY_U3P_UIB_HSGRANT);

    UIB->dev_ep_intr = 0xFFFFFFFF;
    UIB->dev_ep_intr_mask = 0;

    /* Enable the level 1 UIB interrupt mask. */
    UIB->intr_mask = CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT;
    VIC->int_enable |= (1 << USB_CoreInt);

    GCTLAON->control |= CY_U3P_GCTL_USB_POWER_EN;

    /* Enable the requested PHY, as USB 2.0 and 3.0 cannot function together as of now. */
    if (is_ss)
    {
        /* Allow automatic transition into U1 and U2 power states on host request. */
        glLpmDisabled = 0;
        USB3LNK->lnk_device_power_control = CY_U3P_UIB_AUTO_U1 | CY_U3P_UIB_AUTO_U2;

        CyFx3Usb3LnkSetup ();

        /* Set port config and capability timers to their initial values. */
        USB3PROT->prot_lmp_port_capability_timer    = CY_FX3_USB_PROT_LMP_PORT_CAP_TIMER_VALUE;
        USB3PROT->prot_lmp_port_configuration_timer = CY_FX3_USB_PROT_LMP_PORT_CFG_TIMER_VALUE;

        /* Turn on AUTO response to LGO_U3 command from host. */
        USB3LNK->lnk_compliance_pattern_8 |= CY_U3P_UIB_LFPS;
        USB3LNK->lnk_phy_conf              = 0x20000001;        /* Disable terminatons. */

        ChangeUsbClockFreq (2, 2);

        /* Force LTSSM into SS.Disabled state for 100us after the PHY is turned on. */
        USB3LNK->lnk_ltssm_state = (USB_LNK_STATE_SSDISABLED << CY_U3P_UIB_LTSSM_OVERRIDE_VALUE_POS) |
            CY_U3P_UIB_LTSSM_OVERRIDE_EN;
        UIB->otg_ctrl |= CY_U3P_UIB_SSDEV_ENABLE;
        CyFx3BootBusyWait (100);

        USB3LNK->lnk_conf = (USB3LNK->lnk_conf & ~CY_U3P_UIB_EPM_FIRST_DELAY_MASK) |
            (15 << CY_U3P_UIB_EPM_FIRST_DELAY_POS) | CY_U3P_UIB_LDN_DETECTION;
        USB3LNK->lnk_phy_mpll_status = 0x00310018 | CY_U3P_UIB_SSC_EN;

        /* Hardware work-arounds to control LFPS. */
        CyFx3UsbWritePhyReg (0x1010, 0x0080);
        CyFx3UsbWritePhyReg (0x1006, 0x0180);
        CyFx3UsbWritePhyReg (0x1024, 0x0080);

        USB3LNK->lnk_ltssm_state &= ~CY_U3P_UIB_LTSSM_OVERRIDE_EN;
        USB3LNK->lnk_phy_conf     = 0xE0000001; /* Enable USB 3.0 terminations. */

        CyFx3BootBusyWait (100);
        GCTL->uib_core_clk = CY_U3P_GCTL_UIBCLK_CLK_EN | (1 << CY_U3P_GCTL_UIBCLK_PCLK_SRC_POS) |
            (1 << CY_U3P_GCTL_UIBCLK_EPMCLK_SRC_POS);
    }
    else
    {
        /* USB 2.0 LPM-L1 workaround. */
        UIB->ehci_portsc = CY_U3P_UIB_WKOC_E;

        /* Disable EP0-IN and EP0-OUT. */
        UIB->dev_epi_cs[0] &= ~CY_U3P_UIB_EPI_VALID;
        UIB->dev_epo_cs[0] &= ~CY_U3P_UIB_EPO_VALID;

        CyFx3BootBusyWait (2);
        UIB->otg_ctrl = CY_U3P_UIB_DEV_ENABLE;
        CyFx3BootBusyWait (100);

        CyFx3Usb2PhySetup ();
        UIB->phy_clk_and_test = (CY_U3P_UIB_DATABUS16_8 | CY_U3P_UIB_VLOAD | CY_U3P_UIB_SUSPEND_N |
                CY_U3P_UIB_EN_SWITCH);
        CyFx3BootBusyWait (80);

        ChangeUsbClockFreq (2, 0);

        /* For USB 2.0 connections, enable pull-up on D+ pin. */
        UIB->dev_pwr_cs &= ~CY_U3P_UIB_DISCON;
    }
}

static void
CyFx3BootUsbPhyDisable (
        CyBool_t is_ss)
{
    glUibStatusSendErdy = CyFalse;

    /* De-register USB related interrupts and clear any pending interrupts. */
    VIC->int_clear = (1 << (USB_CoreInt));

    UIB->dev_ctl_intr = 0xFFFFFFFF;
    UIB->dev_ep_intr  = 0xFFFFFFFF;
    UIB->otg_intr     = 0xFFFFFFFF;

    /* Turn off the PHY that is active. Also disable the pull-up on D+, if in USB 2.0 mode. */
    if (is_ss)
    {
        USB3LNK->lnk_intr   = 0xFFFFFFFF;
        USB3PROT->prot_intr = 0xFFFFFFFF;

        CyFx3UsbWritePhyReg (0x1005, 0x0000);

        /* Change EPM config to full speed */
        ChangeUsbClockFreq (2, 2);

        CyFx3BootBusyWait (2);
        GCTLAON->control &= ~CY_U3P_GCTL_USB_POWER_EN;

        /* Disable usb3 functionality */
        UIB->otg_ctrl &= ~CY_U3P_UIB_SSDEV_ENABLE;
        CyFx3BootBusyWait (100); /* 2 */
        UIB->otg_ctrl &= ~CY_U3P_UIB_SSEPM_ENABLE;
        CyFx3BootBusyWait (100);

        USB3LNK->lnk_phy_conf        &= 0x1FFFFFFF;
        USB3LNK->lnk_phy_mpll_status  = glUsbMpllDefault;
    }
    else
    {
        UIB->dev_pwr_cs |= CY_U3P_UIB_DISCON;
        CyFx3BootBusyWait (10);

        ChangeUsbClockFreq (2, 2);

        CyFx3BootBusyWait (10);
        GCTLAON->control &= ~CY_U3P_GCTL_USB_POWER_EN;
        UIB->otg_ctrl &= ~CY_U3P_UIB_DEV_ENABLE;
    }
}

static void
HandleVbusOffIntr (
        void)
{
    if (glUsbSpeed == CY_FX3_BOOT_SUPER_SPEED)
    {
        CyFx3BootUsbPhyDisable (CyTrue);
    }
    else
    {
        /* Make sure that the 3.0 PHY has been turned off at this stage. */
        if (UIB->otg_ctrl & CY_U3P_UIB_SSDEV_ENABLE)
        {
            UIB->otg_ctrl &= ~CY_U3P_UIB_SSDEV_ENABLE;
            CyFx3BootBusyWait (2);
            UIB->intr_mask = 0;
            CyFx3BootBusyWait (1);
            USB3LNK->lnk_phy_conf        &= 0x1FFFFFFF;
            USB3LNK->lnk_phy_mpll_status  = glUsbMpllDefault;
            CyFx3BootBusyWait (1);
            UIB->intr_mask = (CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT);
        }

        CyFx3BootUsbPhyDisable (CyFalse);
    }

    glUsbSpeed          = CY_FX3_BOOT_NOT_CONNECTED;
    tDisabledCount      = 0;
    glUibForceLpmAccept = 0;

    if (gEvtCbk)
        gEvtCbk (CY_FX3_BOOT_USB_DISCONNECT);
}

void
CyFx3BootUsbVbusIntrHandler (
        void)
{
    /* Disable and clear the interrupt first. */
    GCTL->iopwr_intr_mask = 0;
    GCTL->iopwr_intr = (CY_U3P_VBUS);

    if ((GCTL->iopower & CY_U3P_VBUS) != 0)
    {
        /* If the USB PHY is already on, turn it off and then turn it on. */
        if ((UIB->otg_ctrl & (CY_U3P_UIB_DEV_ENABLE | CY_U3P_UIB_SSDEV_ENABLE)) != 0)
            HandleVbusOffIntr ();

        glEnableUsb3   = glSsEnable;
        tDisabledCount = 0;

        glInSSReset = CyFalse;
        CyFx3BootUsbPhyEnable (glSsEnable);
    }
    else
    {
        if ((UIB->otg_ctrl & (CY_U3P_UIB_DEV_ENABLE | CY_U3P_UIB_SSDEV_ENABLE)) != 0)
            HandleVbusOffIntr ();
    }

    /* Re-enable the interrupt. */
    GCTL->iopwr_intr_mask = (CY_U3P_VBUS);
}

static void
CyFx3BootUsbFallBackToUsb2 (
		void)
{
    /* If Vbus is not available, do not do anything here. */
    if ((GCTL->iopower & CY_U3P_VBUS) == 0)
        return;

    /* USB 2.0 is already on, just turn off the 3.0 PHY. */
    if (UIB->otg_ctrl & CY_U3P_UIB_DEV_ENABLE)
    {
        /* Force the link state machine into SS.Disabled. */
        USB3LNK->lnk_ltssm_state = (USB_LNK_STATE_SSDISABLED << CY_U3P_UIB_LTSSM_OVERRIDE_VALUE_POS) |
            CY_U3P_UIB_LTSSM_OVERRIDE_EN;
        CyFx3BootBusyWait (2);

        UIB->otg_ctrl &= ~(CY_U3P_UIB_SSDEV_ENABLE | CY_U3P_UIB_SSEPM_ENABLE);
        CyFx3BootBusyWait (2);

        /* Need to disable interrupts while updating the MPLL. */
        UIB->intr_mask = 0;
        CyFx3BootBusyWait (1);

        USB3LNK->lnk_phy_conf        &= 0x1FFFFFFF;
        USB3LNK->lnk_phy_mpll_status  = glUsbMpllDefault;
        CyFx3BootBusyWait (1);

        UIB->intr_mask = (CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT);
        ChangeUsbClockFreq (2, 0);
        return;
    }

    CyFx3UsbWritePhyReg (0x1005, 0x0000);

    /* Force the link state machine into SS.Disabled. */
    USB3LNK->lnk_ltssm_state = (USB_LNK_STATE_SSDISABLED << CY_U3P_UIB_LTSSM_OVERRIDE_VALUE_POS) |
        CY_U3P_UIB_LTSSM_OVERRIDE_EN;

    /* Keep track of the number of times the 3.0 link training has failed. */
    tDisabledCount++;
    glUibStatusSendErdy = CyFalse;

    /* Change EPM config to full speed */
    ChangeUsbClockFreq (2, 2);
    CyFx3BootBusyWait (2);

    /* Switch the EPM to USB 2.0 mode, turn off USB 3.0 PHY and remove Rx Termination. */
    UIB->otg_ctrl &= ~CY_U3P_UIB_SSDEV_ENABLE;
    CyFx3BootBusyWait (2);
    UIB->otg_ctrl &= ~CY_U3P_UIB_SSEPM_ENABLE;

    UIB->intr_mask = 0;
    USB3LNK->lnk_phy_conf        &= 0x1FFFFFFF;
    USB3LNK->lnk_phy_mpll_status  = glUsbMpllDefault;

    /* Power cycle the PHY blocks. */
    GCTLAON->control &= ~CY_U3P_GCTL_USB_POWER_EN;
    CyFx3BootBusyWait (5);
    GCTLAON->control |= CY_U3P_GCTL_USB_POWER_EN;
    CyFx3BootBusyWait (10);

    /* Clear USB 2.0 interrupts. */
    UIB->dev_ctl_intr = 0xFFFFFFFF;
    UIB->dev_ep_intr  = 0xFFFFFFFF;
    UIB->otg_intr     = 0xFFFFFFFF;

    /* Clear and disable USB 3.0 interrupts. */
    USB3LNK->lnk_intr_mask   = 0x00000000;
    USB3LNK->lnk_intr        = 0xFFFFFFFF;
    USB3PROT->prot_intr_mask = 0x00000000;
    USB3PROT->prot_intr      = 0xFFFFFFFF;

    UIB->intr_mask = (CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT);

    /* Disable EP0-IN and EP0-OUT (USB-2). */
    UIB->dev_epi_cs[0] &= ~CY_U3P_UIB_EPI_VALID;
    UIB->dev_epo_cs[0] &= ~CY_U3P_UIB_EPO_VALID;

    glUsbSpeed = CY_FX3_BOOT_FULL_SPEED;

    /* Enable USB 2.0 PHY. */
    CyFx3BootBusyWait (2);
    UIB->otg_ctrl |= CY_U3P_UIB_DEV_ENABLE;

    CyFx3BootBusyWait (100);
    CyFx3Usb2PhySetup ();
    UIB->phy_clk_and_test = (CY_U3P_UIB_DATABUS16_8 | CY_U3P_UIB_VLOAD | CY_U3P_UIB_SUSPEND_N |
            CY_U3P_UIB_EN_SWITCH);
    CyFx3BootBusyWait (80);

    ChangeUsbClockFreq (2, 0);

    /* For USB 2.0 connections, enable pull-up on D+ pin. */
    UIB->dev_pwr_cs &= ~CY_U3P_UIB_DISCON;
}

void
CyFx3BootUsbSendCompliancePatterns (
        void)
{
    uint8_t next_cp = 1;
    uint32_t state;

    /* Return from this loop if LTSSM is not currently in Compliance state. */
    state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
    if (state != USB_LNK_STATE_COMP)
        return;

    /* Force LTSSM to stay in compliance state. */
    USB3LNK->lnk_ltssm_state = (USB_LNK_STATE_COMP << CY_U3P_UIB_LTSSM_OVERRIDE_VALUE_POS) |
        CY_U3P_UIB_LTSSM_OVERRIDE_EN;

    /* While the device is in compliance state, keep updating the compliance patterns as required. */
    do
    {
        if (USB3LNK->lnk_lfps_observe & CY_U3P_UIB_PING_DET)
        {
            /* Update the compliance pattern and sleep for 1 second before clearing the PING detected bit. */
            USB3LNK->lnk_compliance_pattern_0 = glUsb3CompliancePatterns[next_cp];

            CyFx3BootBusyWait (1000);
            USB3LNK->lnk_lfps_observe &= ~CY_U3P_UIB_PING_DET;

            /* Special case handling for CP 4 (LFPS signalling). */
            if (next_cp == 4)
            {
                while (((USB3LNK->lnk_lfps_observe & (CY_U3P_UIB_RESET_DET | CY_U3P_UIB_PING_DET)) == 0) &&
                        ((GCTL->iopower & CY_U3P_VBUS) != 0))
                {
                    USB3LNK->lnk_phy_conf |= CY_U3P_UIB_TXDETECTRX_LB_OVR;
                    CyFx3BootBusyWait (10);
                    USB3LNK->lnk_phy_conf &= ~CY_U3P_UIB_TXDETECTRX_LB_OVR;
                    CyFx3BootBusyWait (30);
                }
            }

            next_cp = (next_cp + 1) % 9;
        }

        if ((USB3LNK->lnk_lfps_observe & CY_U3P_UIB_RESET_DET) || ((GCTL->iopower & CY_U3P_VBUS) == 0))
        {
            USB3LNK->lnk_lfps_observe &= ~CY_U3P_UIB_RESET_DET;
            USB3LNK->lnk_compliance_pattern_0 = CY_U3P_UIB_LNK_COMPLIANCE_PATTERN_0_DEFAULT;
            USB3LNK->lnk_ltssm_state = 0;
            break;
        }

    }
    while (1);
}

void
CyFx3BootUsbCheckUsb3Disconnect (
        void)
{
    uint16_t trials = 8000;
    uint32_t state;

    if ((UIB->otg_ctrl & CY_U3P_UIB_SSDEV_ENABLE) == 0)
    {
        glInCheckUsbDisconnect = CyFalse;
        return;
    }

    state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
    if ((state >= USB_LNK_STATE_U0) && (state <= USB_LNK_STATE_COMP))
    {
        glInSSReset = CyFalse;
        glInCheckUsbDisconnect = CyFalse;
        return;
    }

    while (trials--)
    {
        CyFx3BootBusyWait (100);
        state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
        if ((glInSSReset == CyFalse) || (state >= USB_LNK_STATE_U0) && (state <= USB_LNK_STATE_COMP))
        {
            glInSSReset = 0;
            break;
        }
    }

    if (glInSSReset != 0)
    {
        CyFx3BootUsbFallBackToUsb2 ();
    }

    glInCheckUsbDisconnect = CyFalse;
    return;
}

static void
CyFx3BootUsbReEnableUsb3 (
        void)
{
    UIB->intr_mask = 0;
    /* Make sure that all relevant USB 3.0 interrupts are enabled. */
    USB3LNK->lnk_intr = 0xFFFFFFFF;
    USB3LNK->lnk_intr_mask = CY_U3P_UIB_LGO_U3 | CY_U3P_UIB_LTSSM_CONNECT | CY_U3P_UIB_LTSSM_DISCONNECT |
        CY_U3P_UIB_LTSSM_RESET | CY_U3P_UIB_LTSSM_STATE_CHG;
    USB3PROT->prot_intr = 0xFFFFFFFF;
    USB3PROT->prot_intr_mask = (CY_U3P_UIB_SUTOK_EN | CY_U3P_UIB_STATUS_STAGE | CY_U3P_UIB_TIMEOUT_PORT_CAP_EN |
                                CY_U3P_UIB_TIMEOUT_PORT_CFG_EN | CY_U3P_UIB_LMP_RCV_EN | CY_U3P_UIB_LMP_PORT_CAP_EN |
                                CY_U3P_UIB_LMP_PORT_CFG_EN);

    /* Allow automatic transition into U1 and U2 power states on host request. */
    glLpmDisabled = 0;
    USB3LNK->lnk_device_power_control = CY_U3P_UIB_AUTO_U1 | CY_U3P_UIB_AUTO_U2;

    CyFx3Usb3LnkSetup ();

    /* Set port config and capability timers to their initial values. */
    USB3PROT->prot_lmp_port_capability_timer    = CY_FX3_USB_PROT_LMP_PORT_CAP_TIMER_VALUE;
    USB3PROT->prot_lmp_port_configuration_timer = CY_FX3_USB_PROT_LMP_PORT_CFG_TIMER_VALUE;

    /* Turn on AUTO response to LGO_U3 command from host. */
    USB3LNK->lnk_compliance_pattern_8 |= CY_U3P_UIB_LFPS;
    USB3LNK->lnk_phy_conf = 0x20000001;     /* Disable terminations. */

    /* Switch EPM clock to USB 3.0 mode. */
    ChangeUsbClockFreq (1, 0);
    CyFx3BootBusyWait (10);

    /* Force LTSSM into SS.Disabled state for 100us after the PHY is turned on. */
    USB3LNK->lnk_ltssm_state = (USB_LNK_STATE_SSDISABLED << CY_U3P_UIB_LTSSM_OVERRIDE_VALUE_POS) |
        CY_U3P_UIB_LTSSM_OVERRIDE_EN;
    UIB->otg_ctrl |= CY_U3P_UIB_SSDEV_ENABLE;
    CyFx3BootBusyWait (100);

    USB3LNK->lnk_conf = (USB3LNK->lnk_conf & ~CY_U3P_UIB_EPM_FIRST_DELAY_MASK) |
        (12 << CY_U3P_UIB_EPM_FIRST_DELAY_POS) | CY_U3P_UIB_LDN_DETECTION;
    USB3LNK->lnk_phy_mpll_status   = 0x00310018 | CY_U3P_UIB_SSC_EN;

    /* Hardware work-arounds: Control LFPS behavior. */
    CyFx3UsbWritePhyReg (0x1010, 0x0080);
    CyFx3UsbWritePhyReg (0x1006, 0x0180);
    CyFx3UsbWritePhyReg (0x1024, 0x0080);

    USB3LNK->lnk_ltssm_state &= ~CY_U3P_UIB_LTSSM_OVERRIDE_EN;
    USB3LNK->lnk_phy_conf = 0xE0000001;

    CyFx3BootBusyWait (20);
    UIB->intr_mask = CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT;
}

void
HandleUsbLnkInterrupt (
        void)
{
    uint32_t devState;
    uint32_t tmp;

    /* Check what interrupts are active, and then clear them. */
    devState = USB3LNK->lnk_intr & USB3LNK->lnk_intr_mask;
    USB3LNK->lnk_intr = devState;

    if (devState & CY_U3P_UIB_LGO_U3)
    {
        /* Unconditionally accept entry into U3 state. */
        USB3LNK->lnk_device_power_control |= CY_U3P_UIB_TX_LAU;
    }

    if (devState & CY_U3P_UIB_LTSSM_STATE_CHG)
    {
        uint32_t tmp = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
        switch (tmp)
        {
            case USB_LNK_STATE_U3:
                glRxValidMod = CyTrue;
                CyFx3UsbWritePhyReg (0x1005, 0x0020);
                if (gEvtCbk)
                {
                    gEvtCbk  (CY_FX3_BOOT_USB_SUSPEND);
                }
                break;

            case USB_LNK_STATE_RECOV_ACT:
            case USB_LNK_STATE_RECOV_CNFG:
                /* Ensure that device cannot send and ERDY as soon as it enters U0. */
                USB3PROT->prot_cs = (USB3PROT->prot_cs & ~CY_U3P_UIB_SS_SETUP_CLR_BUSY) | CY_U3P_UIB_SS_NRDY_ALL;

                if (glRxValidMod)
                {
                    glRxValidMod = CyFalse;
                    CyFx3UsbWritePhyReg (0x1005, 0x0000);
                }
                break;

            case USB_LNK_STATE_U2:
                glRxValidMod = CyTrue;
                CyFx3UsbWritePhyReg (0x1005, 0x0020);
            case USB_LNK_STATE_U1:
                /* Ensure that device cannot send and ERDY as soon as it enters U0. */
                USB3PROT->prot_cs = (USB3PROT->prot_cs & ~CY_U3P_UIB_SS_SETUP_CLR_BUSY) | CY_U3P_UIB_SS_NRDY_ALL;

                /* Deferred bit handling. Prevent U1/U2 entry for some time after device
                   wakes to U0 from U1/U2 states. Also keep the LGO_U1/U2 interrupts turned off
                   for this duration to prevent the device from being swamped with interrupts. */
                glUsbUxWake = CyTrue;
                USB3LNK->lnk_device_power_control = CY_U3P_UIB_NO_U1 | CY_U3P_UIB_NO_U2;
                break;

            case USB_LNK_STATE_U0:
                if (glUsbUxWake)
                {
                    /* Wait for some time before enabling U1/U2 entry again. */
                    glUsbUxWake = CyFalse;
                    CyFx3BootBusyWait (250);
                }

                /* Clear the global NAK and allow transfers to proceed. */
                USB3PROT->prot_cs = (USB3PROT->prot_cs & ~(CY_U3P_UIB_SS_NRDY_ALL | CY_U3P_UIB_SS_SETUP_CLR_BUSY));

                if ((!glLpmDisabled) && (!glUibStatusSendErdy))
                    USB3LNK->lnk_device_power_control = CY_U3P_UIB_AUTO_U1 | CY_U3P_UIB_AUTO_U2;
                break;

            case USB_LNK_STATE_POLLING_LFPS:
            case USB_LNK_STATE_POLLING_RxEQ:
                USB3PROT->prot_lmp_port_capability_timer    = CY_FX3_USB_PROT_LMP_PORT_CAP_TIMER_VALUE;
                USB3PROT->prot_lmp_port_configuration_timer = CY_FX3_USB_PROT_LMP_PORT_CFG_TIMER_VALUE;
                break;

            case USB_LNK_STATE_COMP:
                /* Compliance mode : clear RESET LFPS detected bit before starting. */
                USB3LNK->lnk_lfps_observe &= ~CY_U3P_UIB_RESET_DET;
                if (gEvtCbk)
                {
                    gEvtCbk (CY_FX3_BOOT_USB_COMPLIANCE);
                }
                glInSSReset = CyFalse;
                break;
        }

        if ((tmp < USB_LNK_STATE_U0) || (tmp > USB_LNK_STATE_COMP))
        {
            if (!glInCheckUsbDisconnect)
            {
                glInSSReset = CyTrue;
                glInCheckUsbDisconnect = CyTrue;
                if (gEvtCbk)
                {
                    gEvtCbk (CY_FX3_BOOT_USB_IN_SS_DISCONNECT);
                }
            }
        }
        else
        {
            glInSSReset = CyFalse;
        }
    }

    if (devState & CY_U3P_UIB_LTSSM_RESET)
    {
        USB3PROT->prot_epo_cs1[0] &= ~CY_U3P_UIB_SSEPO_STALL;
        USB3PROT->prot_epi_cs1[0] &= ~CY_U3P_UIB_SSEPI_STALL;

        if (gEvtCbk)
        {
            gEvtCbk  (CY_FX3_BOOT_USB_RESET);
        }
    }

    if (devState & CY_U3P_UIB_LTSSM_CONNECT)
    {
        if (UIB->otg_ctrl & CY_U3P_UIB_DEV_ENABLE)
        {
            /* Check whether we go to compliance state or Polling.RxEQ state from here. We should
               turn off the USB 2.0 PHY iff the LTSSM moves to the Polling.RxEQ state. */
            tmp = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
            while (tmp == USB_LNK_STATE_POLLING_LFPS)
            {
                CyFx3BootBusyWait (1);
                tmp = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
            }

            if (tmp == USB_LNK_STATE_COMP)
            {
                /* USB 3.0 link training has failed. We can turn off the USB 3.0 PHY and
                   proceed with USB 2.0 enumeration. */
                UIB->otg_ctrl &= ~(CY_U3P_UIB_SSDEV_ENABLE | CY_U3P_UIB_SSEPM_ENABLE);
                CyFx3BootBusyWait (2);

                /* Need to disable interrupts while updating the MPLL. */
                UIB->intr_mask = 0;
                CyFx3BootBusyWait (1);

                USB3LNK->lnk_phy_conf        &= 0x1FFFFFFF;
                USB3LNK->lnk_phy_mpll_status  = glUsbMpllDefault;
                CyFx3BootBusyWait (1);

                USB3LNK->lnk_intr   = USB3LNK->lnk_intr;
                USB3PROT->prot_intr = USB3PROT->prot_intr;
                UIB->intr_mask      = (CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT);
                ChangeUsbClockFreq (2, 0);

                return;
            }
            else
            {
                UIB->dev_pwr_cs |= CY_U3P_UIB_DISCON;
                UIB->otg_ctrl &= ~CY_U3P_UIB_DEV_ENABLE;            /* Disable USB 2.0 PHY. */
                UIB->dev_ctl_intr = UIB->dev_ctl_intr;
            }
        }

        USB3PROT->prot_epo_cs1[0] &= ~CY_U3P_UIB_SSEPO_STALL;
        USB3PROT->prot_epi_cs1[0] &= ~CY_U3P_UIB_SSEPI_STALL;

        USB3LNK->lnk_phy_tx_trim = 0x0B569011; /* 0x09529015; TX amplitude = 900mV, De-emp = 17 #only for FX3 */

        CyFx3UsbWritePhyReg (0x1006, 0x0180);   /* RX_EQ modifications by GML #only for FX3 */
        CyFx3UsbWritePhyReg (0x1024, 0x0080);
        CyFx3UsbWritePhyReg (0x1010, 0x0080);

        /* Switch EPM clock to USB 3.0 mode. */
        ChangeUsbClockFreq (1, 1);

        USB3LNK->lnk_phy_conf = 0xE0000001;
        glUsbSpeed = CY_FX3_BOOT_SUPER_SPEED;
        if (glNoReenum)
        {
            ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->usbSpeed = CY_FX3_BOOT_SUPER_SPEED;
        }

        UIB->otg_ctrl |= CY_U3P_UIB_SSEPM_ENABLE;           /* Switch EPMs for USB-SS. */

        /* Reset the EPM mux. */
        UIB->iepm_cs |= (CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
        CyFx3BootBusyWait (1);
        UIB->iepm_cs &= ~(CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
        CyFx3BootBusyWait (1);

        /* Enable USB 3.0 control eps. */
        USB3PROT->prot_epi_cs1[0] |= CY_U3P_UIB_SSEPI_VALID;
        UIB->eepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */
        USB3PROT->prot_epo_cs1[0] |= CY_U3P_UIB_SSEPO_VALID;
        UIB->iepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */

        if (gEvtCbk)
        {
            gEvtCbk  (CY_FX3_BOOT_USB_CONNECT);
        }
    }

    if (devState & CY_U3P_UIB_LTSSM_DISCONNECT)
    {
        /* If we still have VBUS, try to connect in USB 2.0 mode. */
        if ((GCTL->iopower & CY_U3P_VBUS) != 0)
        {
            if (UIB->otg_ctrl & CY_U3P_UIB_DEV_ENABLE)
            {
                /* If the 2.0 PHY is already on, simply turn off the USB 3.0 PHY. */
                UIB->otg_ctrl &= ~(CY_U3P_UIB_SSDEV_ENABLE | CY_U3P_UIB_SSEPM_ENABLE);
                CyFx3BootBusyWait (2);

                /* Need to disable interrupts while updating the MPLL. */
                UIB->intr_mask = 0;
                CyFx3BootBusyWait (1);

                USB3LNK->lnk_phy_conf        &= 0x1FFFFFFF;
                USB3LNK->lnk_phy_mpll_status  = glUsbMpllDefault;
                CyFx3BootBusyWait (1);

                UIB->intr_mask = (CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT);
                ChangeUsbClockFreq (2, 0);
            }
            else
            {
                glUsbSpeed = CY_FX3_BOOT_FULL_SPEED;
                tDisabledCount++;
                if (glNoReenum)
                {
                    ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->usbSpeed = CY_FX3_BOOT_FULL_SPEED;
                }
                CyFx3BootUsbPhyDisable (CyTrue);
                CyFx3BootUsbPhyEnable (CyFalse);
            }
        }
        else
        {
            CyFx3BootUsbPhyDisable (CyTrue);
            /* Clear the interrupt and re-enable the mask. */
            GCTL->iopwr_intr      = (CY_U3P_VBUS);
            GCTL->iopwr_intr_mask = (CY_U3P_VBUS);
            if (gEvtCbk)
            {
                gEvtCbk  (CY_FX3_BOOT_USB_DISCONNECT);
            }
        }

        glInSSReset = CyFalse;
    }
}

static void
UsbDelayFunction (
        uint16_t delay)
{
    while (delay--)
    {
        CyFx3BootBusyWait (1);
        /* If there are pending USB link interrupts, handle them. */
        if ((glInIsr) && (UIB->intr & CY_U3P_UIB_LNK_INT))
        {
            HandleUsbLnkInterrupt ();
        }
    }
}

static void
GetUsb3LinkActive (
        void)
{
    uint32_t state;

    /* Make sure that the EP0 is not stalled as of now. */
    if (glUsbSpeed == CY_FX3_BOOT_SUPER_SPEED)
    {
        USB3PROT->prot_epi_cs1[0] &= ~CY_U3P_UIB_SSEPI_STALL;
        USB3PROT->prot_epo_cs1[0] &= ~CY_U3P_UIB_SSEPO_STALL;

        glUibStatusSendErdy = CyTrue;
        USB3LNK->lnk_device_power_control = CY_U3P_UIB_NO_U1 | CY_U3P_UIB_NO_U2;

        state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
        while (state != USB_LNK_STATE_U0)
        {
            if ((state == USB_LNK_STATE_U1) || (state == USB_LNK_STATE_U2))
            {
                USB3LNK->lnk_device_power_control |= CY_U3P_UIB_EXIT_LP;
            }
            UsbDelayFunction (10);
            state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
        }
    }
}

static CyBool_t
IsNewCtrlRqtReceived (
        void)
{
    if ((UIB->dev_ctl_intr & CY_U3P_UIB_SUDAV) || (USB3PROT->prot_intr & CY_U3P_UIB_SUTOK_EV))
        return CyTrue;
    return CyFalse;
}

static void
CyFx3BootDisableEp0Sockets (
        void)
{
    PSCK_T s;

    s = (PSCK_T)&UIB->sck[0];
    s->status &= ~(CY_U3P_LPP_GO_ENABLE | CY_U3P_LPP_WRAPUP);
    s->intr    = 0xFF;
    while (s->status & CY_U3P_LPP_ENABLED)
        __nop ();

    s = (PSCK_T)&UIBIN->sck[0];
    s->status &= ~(CY_U3P_LPP_GO_ENABLE | CY_U3P_LPP_WRAPUP);
    s->intr    = 0xFF;
    while (s->status & CY_U3P_LPP_ENABLED)
        __nop ();

    /* Flush EP0-IN as well. */
    UIB->eepm_endpoint[0] |= CY_U3P_UIB_SOCKET_FLUSH;
    CyFx3BootBusyWait (10);
    UIB->eepm_endpoint[0] &= ~CY_U3P_UIB_SOCKET_FLUSH;
}

/*
   usb_isr
 */
#ifdef CY_USE_ARMCC
void usb_isr (void) __irq
#else
void __attribute__((interrupt("FIQ"))) usb_isr (
        void)
#endif
{
    int ii;
    uint32_t devState;
    uint32_t state;
    uint32_t prevIntrMask = 0;

    glInIsr = 1;

    if (GCTL->iopwr_intr & CY_U3P_VBUS)
    {
        CyFx3BootUsbVbusIntrHandler ();
        glInIsr = 0;
        return;
    }

    if ((GCTLAON->wakeup_event & GCTLAON->wakeup_en) != 0)
    {
        GCTLAON->wakeup_en = 0;
        UIB->phy_chirp = 0;
        UIB->dev_pwr_cs &= ~CY_U3P_UIB_DEV_SUSPEND;
        GCTLAON->wakeup_event = GCTLAON->wakeup_event;

        VIC->int_clear = (1 << GCTL_Int);
    }

    /* Clear the interrupts */
    state = (UIB->intr & UIB->intr_mask);
    UIB->intr = state; /* UIB->intr; */

    /* Mask the interrupts */
    prevIntrMask = UIB->intr_mask;
    UIB->intr_mask = 0;

    if (state & CY_U3P_UIB_LNK_INT)
    {
        HandleUsbLnkInterrupt ();
    }

    if (state & CY_U3P_UIB_PROT_INT)
    {
        devState = (USB3PROT->prot_intr & USB3PROT->prot_intr_mask);
        USB3PROT->prot_intr = devState; /* USB3PROT->prot_intr; */

        if ((devState & CY_U3P_UIB_TIMEOUT_PORT_CAP_EV) || (devState & CY_U3P_UIB_TIMEOUT_PORT_CFG_EV))
        {
            USB3LNK->lnk_ltssm_state   = 0x00002000; /* Force SS.Disabled state */
        }
        if (devState & CY_U3P_UIB_LMP_PORT_CFG_EV)
        {
            USB3PROT->prot_lmp_port_configuration_timer = CY_U3P_UIB_TX_DISABLE | CY_U3P_UIB_RX_DISABLE;
            USB3PROT->prot_intr                         = CY_U3P_UIB_LMP_PORT_CFG_EV;
            tDisabledCount = 0;

        }
        if (devState & CY_U3P_UIB_LMP_PORT_CAP_EV)
        {
            USB3PROT->prot_lmp_port_capability_timer = CY_U3P_UIB_TX_DISABLE | CY_U3P_UIB_RX_DISABLE;
            USB3PROT->prot_intr                         = CY_U3P_UIB_LMP_PORT_CAP_EV;
        }

        if (devState & CY_U3P_UIB_LMP_RCV_EV)
        {
            if (USB3PROT->prot_lmp_received & CY_U3P_UIB_FORCE_LINKPM_ACCEPT)
            {
                glUibForceLpmAccept = CyTrue;
                USB3LNK->lnk_device_power_control = CY_U3P_UIB_YES_U1 | CY_U3P_UIB_YES_U2;
            }
            else
            {
                glUibForceLpmAccept = CyFalse;
                if ((!glLpmDisabled) && (!glUibStatusSendErdy))
                    USB3LNK->lnk_device_power_control = CY_U3P_UIB_AUTO_U1 | CY_U3P_UIB_AUTO_U2;
                else
                    USB3LNK->lnk_device_power_control = CY_U3P_UIB_NO_U1 | CY_U3P_UIB_NO_U2;
            }
        }
        if (devState & CY_U3P_UIB_SUTOK_EV)
        {
            /* clear stall */
            USB3PROT->prot_epi_cs1[0] = USB3PROT->prot_epo_cs1[0] = CY_U3P_UIB_SSEPO_VALID;

            glUibStatusSendErdy   = CyFalse;
            glUibEp0StatusPending = CyTrue;

            /* Make sure that the EP0 sockets are disabled. */
            CyFx3BootDisableEp0Sockets ();

            /* Set the expected transfer count. */
            if (USB3PROT->prot_setupdat0 & 0x80)
                UIB->dev_epi_xfer_cnt[0] = (USB3PROT->prot_setupdat1 >> 16);
            else
                UIB->dev_epo_xfer_cnt[0] = (USB3PROT->prot_setupdat1 >> 16);

            if (gSetupDataCbk)
            {
                gSetupDataCbk (USB3PROT->prot_setupdat0, USB3PROT->prot_setupdat1);
            }
        }

        if (devState & CY_U3P_UIB_STATUS_STAGE)
        {
            glUibStatusSendErdy   = CyFalse;
            glUibEp0StatusPending = CyFalse;
            if (!glLpmDisabled)
                USB3LNK->lnk_device_power_control = CY_U3P_UIB_AUTO_U1 | CY_U3P_UIB_AUTO_U2;
        }
    }

    if (state & CY_U3P_UIB_DEV_CTL_INT)
    {
        devState = (UIB->dev_ctl_intr & UIB->dev_ctl_intr_mask);

        /* Clear all the interrupts */
        UIB->dev_ctl_intr = devState; /* UIB->dev_ctl_intr; */

        if (devState & CY_U3P_UIB_SUDAV)
        {
            UIB->dev_epo_cs[0] &= ~CY_U3P_UIB_EPO_STALL;
            UIB->dev_epi_cs[0] &= ~CY_U3P_UIB_EPI_STALL;

            if (glSsEnable && (tDisabledCount < 3))
            {
                glEnableUsb3 = CyTrue;
            }

            /* Make sure to reset the EP0 DMA channels at this stage. */
            CyFx3BootDisableEp0Sockets ();

            /* Set the expected transfer count. */
            if (UIB->dev_setupdat0 & 0x80)
                UIB->dev_epi_xfer_cnt[0] = (UIB->dev_setupdat1 >> 16);
            else
                UIB->dev_epo_xfer_cnt[0] = (UIB->dev_setupdat1 >> 16);

            if (gSetupDataCbk)
            {
                gSetupDataCbk (UIB->dev_setupdat0, UIB->dev_setupdat1);
            }
        }

        if (devState & CY_U3P_UIB_SUSP)
        {
            uint32_t pol = 0;

            if (UIB->ohci_rh_port_status & CY_U3P_UIB_RHP_PORT_RESUME_FW)
            {
                UIB->phy_chirp = CY_U3P_UIB_OVERRIDE_FSM | 0x10;
                UIB->dev_pwr_cs |= CY_U3P_UIB_DEV_SUSPEND;

		if ((UIB->otg_ctrl & CY_U3P_UIB_DP) == 0)
		{
                    pol |= (CY_U3P_GCTL_POL_UIB_DP);
		}

		if ((UIB->otg_ctrl & CY_U3P_UIB_DM) == 0)
		{
                    pol |= (CY_U3P_GCTL_POL_UIB_DM);
		}

                /* Enable wake-up interrupts on D+/D- Change. */
                GCTLAON->wakeup_event = GCTLAON->wakeup_event;
                GCTLAON->wakeup_polarity = 0x100;
                GCTLAON->wakeup_en = CY_U3P_GCTL_EN_UIB_DP | CY_U3P_GCTL_EN_UIB_DM;
                VIC->int_enable |= (1 << GCTL_Int);
            }

            if (gEvtCbk)
            {
                gEvtCbk  (CY_FX3_BOOT_USB_SUSPEND);
            }
        }

        if (devState & CY_U3P_UIB_URESUME)
        {
            if (gEvtCbk)
            {
                gEvtCbk  (CY_FX3_BOOT_USB_RESUME);
            }
        }

        if (devState & CY_U3P_UIB_URESET)
        {
            CyFx3BootUsbAckSetup ();
            glUsbSpeed = CY_FX3_BOOT_FULL_SPEED;
            if (glNoReenum)
            {
                ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->usbSpeed = CY_FX3_BOOT_FULL_SPEED;
            }

            for (ii=1; ii<16; ii++) /* clear STALL bits          */
            {
                UIB->dev_epi_cs[ii] &= ~CY_U3P_UIB_EPI_STALL;
                UIB->dev_epo_cs[ii] &= ~CY_U3P_UIB_EPI_STALL;
            }

            if (gEvtCbk)
            {
                gEvtCbk  (CY_FX3_BOOT_USB_RESET);
            }

            /* Reset the EPM mux. */
            UIB->iepm_cs |= (CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
            CyFx3BootBusyWait (1);
            UIB->iepm_cs &= ~(CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
            CyFx3BootBusyWait (1);

            UIB->eepm_endpoint[0] = 0x40;
            UIB->iepm_endpoint[0] = 0x40;

            /* Enable EP0-IN and EP0-OUT. */
            UIB->dev_epi_cs[0] &= ~CY_U3P_UIB_EPI_PAYLOAD_MASK;
            UIB->dev_epo_cs[0] &= ~CY_U3P_UIB_EPI_PAYLOAD_MASK;

            UIB->dev_epi_cs[0] |= (CY_U3P_UIB_EPI_VALID | 64);
            UIB->dev_epo_cs[0] |= (CY_U3P_UIB_EPO_VALID | 64);

            /* If USB-SS functionality is enabled, we need to retry for a SS connection on getting USB bus reset. */
            if (glEnableUsb3)
            {
                if (tDisabledCount >= 3)
                    glEnableUsb3 = CyFalse;

                if ((UIB->otg_ctrl & CY_U3P_UIB_SSDEV_ENABLE) == 0)
                    CyFx3BootUsbReEnableUsb3 ();
            }
        }

        if (devState & CY_U3P_UIB_HSGRANT)
        {
            if (UIB->dev_pwr_cs & CY_U3P_UIB_HSM)
            {
                glUsbSpeed = CY_FX3_BOOT_HIGH_SPEED;
                if (glNoReenum)
                {
                    ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->usbSpeed = CY_FX3_BOOT_HIGH_SPEED;
                }
            }
        }
    }

    /* Restore the UIB interrupt mask */
    UIB->intr_mask = prevIntrMask;
    glInIsr = 0;
}

/* Summary:
   A one shot dma setup and transfer function.
 */
CyFx3BootErrorCode_t
CyFx3BootUsbDmaXferData (
        uint8_t epNum,
        uint32_t address,
        uint32_t length ,
        uint32_t timeout
        )
{
    uint8_t direction = epNum & 0x80;
    PSCK_T s;
    uint16_t id, sc, ch;
    PDSCR_T dscr = DSCR(0);
    uint32_t size = length;
    uint32_t *buf;

    CyFx3BootErrorCode_t status;

    if ((address < CY_FX3_BOOT_SYSMEM_BASE) || (address >= CY_FX3_BOOT_SYSMEM_END))
        return CY_FX3_BOOT_ERROR_INVALID_DMA_ADDR;

    if (((epNum & 0x0F) == 0) && (IsNewCtrlRqtReceived ()))
        return CY_FX3_BOOT_ERROR_ABORTED;

    buf = (uint32_t*)address;
    if (direction)
        s = (PSCK_T)&UIB->sck[epNum & 0x0F];
    else
        s = (PSCK_T)&UIBIN->sck[epNum & 0x0F];

    id = ((uint32_t)s>>16) & 0x1f;     /* use bit16-20 to decode Device ID */
    ch = ((uint32_t)s>>7) & 0x1f;  /* use bit7-11  to decode channel number */

    s->status = CY_U3P_LPP_SCK_STATUS_DEFAULT;
    while (s->status & CY_U3P_LPP_ENABLED)
       __nop ();
    s->status = CY_U3P_LPP_SCK_STATUS_DEFAULT | CY_U3P_LPP_UNIT;
    s->intr   = 0xFF;                      /* clear all previous interrupt */
    s->dscr   = DSCR_ADDR(dscr);
    s->size   = 1;
    s->count  = 0;

    dscr->buffer = (uint32_t)buf;
    sc = (size & 0xf) ? (size & cSizeMask) + 0x10 : (size & cSizeMask);
    if (sc == 0)
        sc = 16;

    if (direction) /* 1=TX: SYSMEM to device     */
    {
        dscr->sync = (  (ch << CY_U3P_LPP_CONS_SCK_POS) | (id << CY_U3P_LPP_CONS_IP_POS) |
                (CPU_SCK_NUM << CY_U3P_LPP_PROD_SCK_POS) | (CPU_IP_NUM << CY_U3P_LPP_PROD_IP_POS) |
                (CY_U3P_LPP_EN_CONS_EVENT|CY_U3P_LPP_EN_CONS_INT)   | (CY_U3P_LPP_EN_PROD_EVENT|CY_U3P_LPP_EN_PROD_INT)
                );
        dscr->size = ((size & (cTX_EN-1)) << CY_U3P_LPP_BYTE_COUNT_POS) | sc | CY_U3P_LPP_BUFFER_OCCUPIED;
    }
    else   /* 0=RX: Device to SYSMEM          */
    {
        dscr->sync = (  (CPU_SCK_NUM << CY_U3P_LPP_CONS_SCK_POS) | (CPU_IP_NUM << CY_U3P_LPP_CONS_IP_POS) |
                (ch << CY_U3P_LPP_PROD_SCK_POS) | (id << CY_U3P_LPP_PROD_IP_POS) |
                (CY_U3P_LPP_EN_CONS_EVENT|CY_U3P_LPP_EN_CONS_INT) | (CY_U3P_LPP_EN_PROD_EVENT|CY_U3P_LPP_EN_PROD_INT)
                );
        dscr->size = sc;
    }

    dscr->chain = (0xFFFFFFFF);

    s->status |= CY_U3P_LPP_GO_ENABLE;
    status = CY_FX3_BOOT_ERROR_TIMEOUT;

    do
    {
        if (s->intr & CY_U3P_LPP_ERROR)
        {
            status = CY_FX3_BOOT_ERROR_XFER_FAILURE;
            break;
        }

        UsbDelayFunction (100);

        if (direction)
        {
            if (s->intr & CY_U3P_LPP_CONSUME_EVENT)
            {
                status = CY_FX3_BOOT_SUCCESS;
                break;
            }
        }
        else
        {
            if (s->intr & CY_U3P_LPP_PRODUCE_EVENT)
            {
                status = CY_FX3_BOOT_SUCCESS;
                break;
            }
        }

        if ((timeout != CY_FX3_BOOT_NO_WAIT) && (timeout != CY_FX3_BOOT_WAIT_FOREVER))
        {
            timeout --;
        }

        if (((epNum & 0x0F) == 0) && (IsNewCtrlRqtReceived ()))
        {
            /* This request has been aborted due to a new control request. Just reset
               the USB socket and return an error. */
            s->status &= ~(CY_U3P_LPP_GO_ENABLE | CY_U3P_LPP_WRAPUP);
            s->intr    = 0xFF;
            while (s->status & CY_U3P_LPP_ENABLED)
                __nop ();

            /* Flush EP0-IN as well. */
            UIB->eepm_endpoint[0] |= CY_U3P_UIB_SOCKET_FLUSH;
            CyFx3BootBusyWait (10);
            UIB->eepm_endpoint[0] &= ~CY_U3P_UIB_SOCKET_FLUSH;

            status = CY_FX3_BOOT_ERROR_ABORTED;
            break;
        }

    } while (timeout > 0);

    return status;
}

/* Summary:
   This function returns the speed at which the USB is operating.
 */
CyFx3BootUsbSpeed_t
CyFx3BootUsbGetSpeed (void)
{
    return glUsbSpeed;
}

/* the function returns the NAK/STALL status of the specified EP */
CyFx3BootErrorCode_t
CyFx3BootUsbGetEpCfg (
        uint8_t ep,
        CyBool_t *isNak,
        CyBool_t *isStall
        )
{
    uint32_t val, val1;
    uint8_t tmp = ep & 0x0F;

    if (!isNak && !isStall)
    {
        return CY_FX3_BOOT_ERROR_NULL_POINTER;
    }

    if (ep & 0x80)
    {
        if (glUsbSpeed == CY_FX3_BOOT_SUPER_SPEED)
        {
            val = (USB3PROT->prot_epi_cs1[tmp] & CY_U3P_UIB_SSEPI_NRDY);
            val1 = (USB3PROT->prot_epi_cs1[tmp] & CY_U3P_UIB_SSEPI_STALL);
        }
        else
        {
            val = (UIB->dev_epi_cs[tmp] & CY_U3P_UIB_EPI_NAK);
            val1 = (UIB->dev_epi_cs[tmp] & CY_U3P_UIB_EPI_STALL);
        }
    }
    else
    {
        if (glUsbSpeed == CY_FX3_BOOT_SUPER_SPEED)
        {
            val = (USB3PROT->prot_epo_cs1[tmp] & CY_U3P_UIB_SSEPO_NRDY);
            val1 = (USB3PROT->prot_epo_cs1[tmp] & CY_U3P_UIB_SSEPO_STALL);
        }
        else
        {
            val = (UIB->dev_epo_cs[tmp] & CY_U3P_UIB_EPO_NAK);
            val1 = (UIB->dev_epo_cs[tmp] & CY_U3P_UIB_EPO_STALL);
        }
    }

    if (isNak)
    {
        *isNak = val > 0 ? CyTrue : CyFalse;
    }
    if (isStall)
    {
        *isStall = val1 > 0 ? CyTrue : CyFalse;
    }

    return CY_FX3_BOOT_SUCCESS;
}

CyFx3BootErrorCode_t
CyFx3BootUsbSetClrFeature (
        uint32_t              sc,
        CyBool_t              isConfigured,
        CyFx3BootUsbEp0Pkt_t *pEp0)
{
    uint8_t feature = pEp0->bVal0;

    if (pEp0->bmReqType == CY_FX3_USB_REQ_DEVICE)
    {
        if (CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED)
        {
            if (((feature == 48) || (feature == 49) || (feature == 50)) && (isConfigured))
                return CY_FX3_BOOT_SUCCESS;
        }
        else
        {
            if ((feature == 1) || ((feature == 2) && (sc != 0)))
                return CY_FX3_BOOT_SUCCESS;
        }
    }

    if ((pEp0->bmReqType == CY_FX3_USB_REQ_ENDPOINT) && (feature == 0))
    {
        if (sc)
        {
            CyFx3BootUsbStall (pEp0->bIdx0, CyTrue, CyFalse);
        }
        else
        {
            CyFx3BootUsbStall (pEp0->bIdx0, CyFalse, CyTrue);
        }
        return CY_FX3_BOOT_SUCCESS;
    }

    if (pEp0->bmReqType == CY_FX3_USB_REQ_INTERFACE)
    {
        if (CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED)
        {
            if ((isConfigured) && (feature == 0))
            {
                return CY_FX3_BOOT_SUCCESS;
            }
        }
    }

    return CY_FX3_BOOT_ERROR_FAILURE;
}

/* Summary:
   This function returns the pointer to the USB descriptors. The application will dereference this pointer
   to retrieve the values of the various supported descriptors to handle the standard requests.
 */
CyU3PUsbDescrPtrs *
CyFx3BootUsbGetDesc (void)
{
    return &glUsbDescPtr;
}

/* Function to set the descriptor */
CyFx3BootErrorCode_t
CyFx3BootUsbSetDesc (
        CyU3PUSBSetDescType_t descType,
        uint8_t desc_index,
        uint8_t *desc)
{
    CyFx3BootErrorCode_t ret = CY_FX3_BOOT_SUCCESS;
    uint16_t length = 0;

    switch (descType)
    {
        case CY_U3P_USB_SET_SS_DEVICE_DESCR:
            glUsbDescPtr.usbSSDevDesc_p = (uint8_t *)desc;
            length = desc[0];
            break;

        case CY_U3P_USB_SET_HS_DEVICE_DESCR:
            glUsbDescPtr.usbDevDesc_p = (uint8_t *)desc;
            length = desc[0];
            break;

        case CY_U3P_USB_SET_SS_BOS_DESCR:
            glUsbDescPtr.usbSSBOSDesc_p = (uint8_t *)desc;
            length = ((desc[3] << 8) | (desc[2]));
            break;

        case CY_U3P_USB_SET_DEVQUAL_DESCR:
            glUsbDescPtr.usbDevQualDesc_p = (uint8_t *)desc;
            length = desc[0];
            break;

        case CY_U3P_USB_SET_SS_CONFIG_DESCR:
            glUsbDescPtr.usbSSConfigDesc_p = (uint8_t *)desc;
            length = ((desc[3] << 8) | (desc[2]));
            break;

        case CY_U3P_USB_SET_FS_CONFIG_DESCR:
            glUsbDescPtr.usbFSConfigDesc_p  = (uint8_t *)desc;
            /* Move the config pointer to the new location. */
            glUsbDescPtr.usbConfigDesc_p   = glUsbDescPtr.usbFSConfigDesc_p;
            length = ((desc[3] << 8) | (desc[2]));
            break;

        case CY_U3P_USB_SET_HS_CONFIG_DESCR:
            glUsbDescPtr.usbHSConfigDesc_p  = (uint8_t *)desc;
            /* Move the config pointer to the new location. */
            glUsbDescPtr.usbOtherSpeedConfigDesc_p = glUsbDescPtr.usbHSConfigDesc_p;
            length = ((desc[3] << 8) | (desc[2]));
            break;

        case CY_U3P_USB_SET_STRING_DESCR:
            if (desc_index > CY_FX3_USB_MAX_STRING_DESC_INDEX)
            {
                ret = CY_FX3_BOOT_ERROR_BAD_ARGUMENT; /* Bad Index */
            }
            else
            {
                glUsbDescPtr.usbStringDesc_p[desc_index] = (uint8_t *)desc;
                length = desc[0];
            }
            break;

        default:
            ret = CY_FX3_BOOT_ERROR_BAD_DESCRIPTOR_TYPE; /* Bad Descriptor Type. */
    }

    if (glNoReenum && (ret == CY_FX3_BOOT_SUCCESS))
    {
        length = (length + 3) & 0xFFFC;
        if ((((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->length + length) > (0x1000 - 8))
            ret = CY_FX3_BOOT_ERROR_MEMORY_ERROR;
        else
        { 
            memCopy ((uint8_t *)glDescTableOffset, desc, length);
            ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->descPtrs[glNumDesc].descType  = (uint8_t)descType;
            ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->descPtrs[glNumDesc].descIndex = desc_index;
            ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->descPtrs[glNumDesc++].descPtr = (uint32_t *)glDescTableOffset;
            ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->numDesc                       = glNumDesc;

            glDescTableOffset += length;
            ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->length += length;
        }
    }
    else
    {
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->length = 0;
    }

    return ret;
}

static void
CyFx3BootUsbSendErdy (
        uint8_t  ep,
        uint16_t bulkStream)
{
    uint32_t usb_tp[3] = {0};
    uint8_t  epNum = (ep & 0x0F);
    uint8_t  epDir = (ep & 0x80);

    uint32_t tmp = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;

    if (tmp != USB_LNK_STATE_U0)
        return;

    /* Compute the values for the ERDY TP. */
    usb_tp[0] = ((USB3PROT->prot_cs & CY_U3P_UIB_SS_DEVICEADDR_MASK) << CY_FX3_BOOT_USB3_TP_DEVADDR_POS) |
        CY_U3P_USB3_PACK_TYPE_TP;
    usb_tp[1] = 0x00010000 | CY_U3P_USB3_TP_SUBTYPE_ERDY | epDir | (epNum << CY_FX3_BOOT_USB3_TP_EPNUM_POS);

    if (epDir)
    {
        if ((USB3PROT->prot_epi_cs1[epNum] & CY_U3P_UIB_SSEPI_STREAM_EN) != 0)
            usb_tp[2] = bulkStream;
    }
    else
    {
        if ((USB3PROT->prot_epo_cs1[epNum] & CY_U3P_UIB_SSEPO_STREAM_EN) != 0)
            usb_tp[2] = bulkStream;
    }

    /* Send the ERDY TP to the host. */
    CyFx3Usb3SendTP (usb_tp);
}

/* This function does not have any more work to do, as the EP0 handling scheme has been changed. */
void
CyFx3BootUsbEp0StatusCheck (
        void)
{
    uint32_t state;

    if (CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED)
    {
        /* If USB link is in U0, ensure that the NAK-ALL bit is cleared. */
        state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
        if (state == USB_LNK_STATE_U0)
            USB3PROT->prot_cs = (USB3PROT->prot_cs & ~(CY_U3P_UIB_SS_NRDY_ALL | CY_U3P_UIB_SS_SETUP_CLR_BUSY));
    }
}

/* set / clear the stall on the EP */
void
CyFx3BootUsbStall (
        uint8_t ep,
        CyBool_t stall,
        CyBool_t toggle
        )
{
    uint8_t tmp = (ep & 0x0F);
    uint32_t seqClear = 0;
    uint32_t x;

    if (tmp == 0)
    {
        if ((!stall) || (IsNewCtrlRqtReceived ()))
            return;

        if (CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED)
        {
            GetUsb3LinkActive ();
            USB3PROT->prot_epi_cs1[0] |= CY_U3P_UIB_SSEPI_STALL;
            USB3PROT->prot_epo_cs1[0] |= CY_U3P_UIB_SSEPO_STALL;

            x = VIC->int_enable;
            VIC->int_clear = 0xFFFFFFFF;
            CyFx3BootBusyWait (1);
            USB3PROT->prot_cs |= CY_U3P_UIB_SS_SETUP_CLR_BUSY;
            VIC->int_enable = x;

            UsbDelayFunction (10);

            glUibStatusSendErdy = CyFalse;
            if (!glLpmDisabled)
                USB3LNK->lnk_device_power_control = CY_U3P_UIB_AUTO_U1 | CY_U3P_UIB_AUTO_U2;
        }
        else
        {
            UIB->dev_epi_cs[0] |= CY_U3P_UIB_EPI_STALL;
            UIB->dev_epo_cs[0] |= CY_U3P_UIB_EPO_STALL;
            UIB->dev_cs |= CY_U3P_UIB_SETUP_CLR_BUSY;
        }

        glUibEp0StatusPending = CyFalse;
        return;
    }

    if (CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED)
    {
        /* USB-SS case. Use the PROT register space. */
        if ((ep & 0x80) != 0)
        {
            /* IN endpoint. */
            if (stall)
            {
                /* Set the stall bit. */
                x = USB3PROT->prot_epi_cs1[tmp];
                x |= (CY_U3P_UIB_SSEPI_STALL | CY_U3P_UIB_SSEPI_VALID);
                USB3PROT->prot_epi_cs1[tmp] = x;

                    CyFx3BootUsbSendErdy (ep, (uint16_t)USB3PROT->prot_epi_mapped_stream[tmp]);
            }
            else
            {
                /* Clear the stall bit and reset the endpoint. */
                x = USB3PROT->prot_epi_cs1[tmp];
                x |= (CY_U3P_UIB_SSEPI_EP_RESET | CY_U3P_UIB_SSEPI_VALID);
                USB3PROT->prot_epi_cs1[tmp] = x;
                CyFx3BootBusyWait (1);
                x = ((x & ~(CY_U3P_UIB_SSEPI_STALL | CY_U3P_UIB_SSEPI_EP_RESET)) | CY_U3P_UIB_SSEPI_VALID);
                USB3PROT->prot_epi_cs1[tmp] = x;
            }
        }
        else
        {
            /* OUT endpoint. */
            if (stall)
            {
                /* Set the stall bit. */
                x = USB3PROT->prot_epo_cs1[tmp] | CY_U3P_UIB_SSEPO_STALL | CY_U3P_UIB_SSEPO_VALID;
                USB3PROT->prot_epo_cs1[tmp] = x;

                    CyFx3BootUsbSendErdy (ep, (uint16_t)USB3PROT->prot_epo_mapped_stream[tmp]);
            }
            else
            {
                /* Clear the stall bit and reset the endpoint. */
                x = USB3PROT->prot_epo_cs1[tmp] | CY_U3P_UIB_SSEPO_EP_RESET | CY_U3P_UIB_SSEPO_VALID;
                USB3PROT->prot_epo_cs1[tmp] = x;
                CyFx3BootBusyWait (1);
                x = (x & ~(CY_U3P_UIB_SSEPO_STALL | CY_U3P_UIB_SSEPO_EP_RESET)) | CY_U3P_UIB_SSEPO_VALID;
                USB3PROT->prot_epo_cs1[tmp] = x;
            }
        }
    }
    else
    {
        /* USB 2.0 case. Use the DEV register space. */
        if ((ep & 0x80) != 0)
        {
            /* IN endpoint. */
            if (stall)
            {
                /* Set the stall bit. */
                UIB->dev_epi_cs[tmp] |= CY_U3P_UIB_EPI_STALL;
            }
            else
            {
                /* Clear the stall bit. */
                UIB->dev_epi_cs[tmp] &= ~CY_U3P_UIB_EPI_STALL;
            }
        }
        else
        {
            /* OUT endpoint. */
            if (stall)
            {
                /* Set the stall bit. */
                UIB->dev_epo_cs[tmp] |= CY_U3P_UIB_EPO_STALL;
            }
            else
            {
                /* Clear the stall bit. */
                UIB->dev_epo_cs[tmp] &= ~CY_U3P_UIB_EPO_STALL;
            }
        }
    }

    /* Toggle control is only relevant when clearing the Stall bit. */
    if ((toggle) && (!stall))
    {
        if (CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED)
        {
            seqClear = tmp | CY_U3P_UIB_COMMAND;
            if (ep & 0x80)
                seqClear |= CY_U3P_UIB_DIR;

            USB3PROT->prot_seq_num = seqClear;
            while ((USB3PROT->prot_seq_num & CY_U3P_UIB_SEQ_VALID) == 0);
        }
        else
        {
            /* Check the endpoint direction. */
            if (ep & 0x80)
            {
                /* Set the direction */
                tmp |= CY_U3P_UIB_IO;
            }

            UIB->dev_toggle = tmp;
            while (!(UIB->dev_toggle & CY_U3P_UIB_TOGGLE_VALID));

            /* Reset the Data toggle to 0 */
            tmp |= CY_U3P_UIB_R;
            UIB->dev_toggle = tmp;

            while (!(UIB->dev_toggle & CY_U3P_UIB_TOGGLE_VALID));
        }
    }

    return;
}

void
CyFx3BootUsbAckSetup (
        void)
{
    uint32_t x;

    if (IsNewCtrlRqtReceived ())
        return;

    if (CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED)
    {
        /* Make sure that the USB 3.0 link is moved to U0 before preparing
           the device for the status handshake. */
        GetUsb3LinkActive ();

        USB3PROT->prot_epi_cs1[0] &= ~CY_U3P_UIB_SSEPI_STALL;
        USB3PROT->prot_epo_cs1[0] &= ~CY_U3P_UIB_SSEPO_STALL;

        x = VIC->int_enable;
        VIC->int_clear = 0xFFFFFFFF;
        CyFx3BootBusyWait (1);
        USB3PROT->prot_cs |= CY_U3P_UIB_SS_SETUP_CLR_BUSY;
        VIC->int_enable = x;
    }
    else
    {
        UIB->dev_epo_cs[0] &= ~CY_U3P_UIB_EPO_STALL;
        UIB->dev_epi_cs[0] &= ~CY_U3P_UIB_EPI_STALL;
        UIB->dev_cs |= CY_U3P_UIB_SETUP_CLR_BUSY;
    }
}

void
CyFx3BootUsbConnect (
        CyBool_t connect,
        CyBool_t ssEnable
        )
{
    if (!connect)
    {
        if (glUsbSpeed == CY_FX3_BOOT_SUPER_SPEED)
        {
            USB3PROT->prot_cs = (USB3PROT->prot_cs & ~CY_U3P_UIB_SS_NRDY_ALL) | CY_U3P_UIB_SS_SETUP_CLR_BUSY;
            while (!(USB3PROT->prot_intr & CY_U3P_UIB_STATUS_STAGE));
        }
        else if ((glUsbSpeed == CY_FX3_BOOT_FULL_SPEED) || (glUsbSpeed == CY_FX3_BOOT_HIGH_SPEED))
        {
            UIB->dev_cs |= CY_U3P_UIB_SETUP_CLR_BUSY;     /* Clear before enable */
            while (!(UIB->dev_ctl_intr & CY_U3P_UIB_STATUS_STAGE));
        }

        if (!glNoReenum)
        {
            /* Disable the PHY */
            CyFx3BootUsbPhyDisable (glUsbSpeed == CY_FX3_BOOT_SUPER_SPEED);
        }

        VIC->int_clear = ((1 << USB_CoreInt) | (1 << IOPWR_Int));

        UIB->dev_ctl_intr = 0xFFFFFFFF;
        UIB->dev_ep_intr  = 0xFFFFFFFF;
        UIB->otg_intr     = 0xFFFFFFFF;

        USB3LNK->lnk_intr   = 0xFFFFFFFF;
        USB3PROT->prot_intr = 0xFFFFFFFF;
        GCTL->iopwr_intr_mask = 0;

        tDisabledCount = 0;
    }
    else
    {
        if (glNoReenum)
        {
            ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->ssConnect = ssEnable;
        }

        /* Store the desired USB connection speed for later use. */
        glSsEnable     = ssEnable;
        glEnableUsb3   = ssEnable;
        tDisabledCount = 0;

        /* Enable the Vbus detection interrupt at this stage. */
        GCTL->iopwr_intr      = 0xFFFFFFFF;
        GCTL->iopwr_intr_mask = CY_U3P_VBUS;
        VIC->int_enable |= (1 << (IOPWR_Int));

        /* If Vbus is already ON or if we are working off Vbat, the PHY can be turned on here. */
        if ((GCTL->iopower & CY_U3P_VBUS) != 0)
        {
            CyFx3BootUsbPhyEnable (ssEnable);
        }
        else
        {
            glUsbSpeed = CY_FX3_BOOT_NOT_CONNECTED;
            if (glNoReenum)
            {
                ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->usbSpeed = CY_FX3_BOOT_NOT_CONNECTED;
            }
        }
    }
}

/* Summary: Function to register the callback for handling the usb setup
 * packets.
 */
void
CyFx3BootRegisterSetupCallback (
        CyFx3BootUSBSetupCb_t callback)
{
    gSetupDataCbk = callback;
}

/* Sets up the end point configuration for transferring data */
CyFx3BootErrorCode_t
CyFx3BootUsbSetEpConfig (
        uint8_t ep,
        CyFx3BootUsbEpConfig_t *epinfo)
{
    uint8_t epnum = ep & 0x7F;
    uint16_t pcktSize;
    uint32_t epCs = 0;
    uint32_t protEpCs1 = 0;
    uint32_t protEpCs2 = 0;

    /* Check for parameter validity. */
    if (!epinfo)
    {
        return CY_FX3_BOOT_ERROR_NULL_POINTER;
    }

    if ((epinfo->epType & 0x03) != CY_FX3_BOOT_USB_EP_BULK)
    {
        return CY_FX3_BOOT_ERROR_NOT_SUPPORTED;
    }

    /* Update the endpoint type. EPI and EPO masks are same. */
    epCs = ((uint32_t)(epinfo->epType & 0x03) << CY_U3P_UIB_EPI_TYPE_POS);
    protEpCs2 = 0x02; /*CyU3PSSEpTypeMap[epinfo->epType & 0x03];*/

    /* Setting the burstlen field for SS. EPI and EPO masks are same. */
    protEpCs2 |= ((uint32_t)(epinfo->burstLen & 0x0F) << CY_U3P_UIB_SSEPI_MAXBURST_POS);

    /* If the USB connection is already active, set the payload according to the connection speed. */
    pcktSize = ((epinfo->pcktSize != 0) && (epinfo->pcktSize <= 0x400)) ? epinfo->pcktSize : 0x400;
    switch (glUsbSpeed)
    {
        case CY_FX3_BOOT_HIGH_SPEED:
            if (pcktSize > 0x200)
            {
                pcktSize = 0x200;
            }
            break;

        case CY_FX3_BOOT_FULL_SPEED:
            if (pcktSize > 1023)
            {
                pcktSize = 1023;
            }
            else
            {
                if (pcktSize > 0x40)
                {
                    pcktSize = 0x40;
                }
            }
            break;

        default:
            /* Super speed. All settings are already correct. */
            break;
    }

    /* Update the HS packet size information. Both EPI and EPO have the same mask. */
    epCs |= (pcktSize & CY_U3P_UIB_EPI_PAYLOAD_MASK);

    if (ep & 0x80) /* USB IN EP */
    {
        if (epinfo->enable == CyFalse)
        {
            UIB->dev_epi_cs[epnum] &=  ~CY_U3P_UIB_EPI_VALID;
            USB3PROT->prot_epi_cs1[epnum] &= ~CY_U3P_UIB_SSEPI_VALID;

            return CY_FX3_BOOT_SUCCESS;
        }

        UIB->eepm_endpoint[epnum]     = pcktSize;
        UIB->dev_epi_cs[epnum]        = (epCs | CY_U3P_UIB_EPI_VALID);
        USB3PROT->prot_epi_cs1[epnum] = (protEpCs1 | CY_U3P_UIB_SSEPI_VALID);
        USB3PROT->prot_epi_cs2[epnum] = protEpCs2;
    }
    else /* USB OUT EP */
    {
        if (epinfo->enable == CyFalse)
        {
            UIB->dev_epo_cs[epnum] &=  ~CY_U3P_UIB_EPO_VALID;
            USB3PROT->prot_epo_cs1[epnum] &= ~CY_U3P_UIB_SSEPI_VALID;

            return CY_FX3_BOOT_SUCCESS;
        }

        UIB->iepm_endpoint[epnum]     = pcktSize;
        UIB->dev_epo_cs[epnum]        = (epCs | CY_U3P_UIB_EPO_VALID);
        USB3PROT->prot_epo_cs1[epnum] = (protEpCs1 | CY_U3P_UIB_SSEPO_VALID);
        USB3PROT->prot_epo_cs2[epnum] = protEpCs2;
    }

    return CY_FX3_BOOT_SUCCESS;
}

void
CyFx3BootUsbVBattEnable (
        CyBool_t enable
        )
{
    uint32_t tmp = 0;

    if (enable)
    {
        GCTLAON->control |= CY_U3P_GCTL_USB_VBAT_EN;
    }
    else
    {
        /* Cannot directly turn off Vbat enable. Need to switch to Carkit mode, turn off Vbat enable
           and then turn off Carkit mode. */
        tmp = GCTL->iopower;
        GCTL->iopower = (GCTL->iopower & ~(CY_U3P_REG_CARKIT_SEL_MASK | CY_U3P_REG_CARKIT_EN)) |
            CY_U3P_REG_CARKIT_EN | (2 << CY_U3P_REG_CARKIT_SEL_POS);

        GCTLAON->control &= ~CY_U3P_GCTL_USB_VBAT_EN;
        CyFx3BootBusyWait (100);

        GCTL->iopower = tmp;
    }
}

CyFx3BootErrorCode_t
CyFx3BootUsbLoadDescs (
        void
        )
{
    uint8_t i = 0;
    uint8_t count = ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->numDesc;
    CyFx3BootErrorCode_t ret;
    CyFx3BootUsbDescTable_t *descTable_p = (CyFx3BootUsbDescTable_t *)glUsbDescPtrs;

    for (i = 0; i < count; i++)
    {
        ret = CyFx3BootUsbSetDesc ((CyU3PUSBSetDescType_t)descTable_p->descPtrs[i].descType,
                                descTable_p->descPtrs[i].descIndex,
                                (uint8_t *)descTable_p->descPtrs[i].descPtr);
        if (ret != CY_FX3_BOOT_SUCCESS)
        {
            return ret;
        }
    }

    return CY_FX3_BOOT_SUCCESS;
}

CyFx3BootErrorCode_t
CyFx3BootUsbNoReEnum (
        void)
{
    uint32_t length = 0;
    CyFx3BootErrorCode_t status = CY_FX3_BOOT_SUCCESS;

    /* All relevant structure checks are done by the CyFx3BootDeviceInit function itself. */
    if (!glBootUsbIsOn)
    {
        /* Make sure that the USB block is off at this stage. */
        UIB->otg_ctrl &= ~CY_U3P_UIB_SSDEV_ENABLE;
        CyFx3BootBusyWait (2);
        UIB->otg_ctrl &= ~CY_U3P_UIB_SSEPM_ENABLE;

        UIB->otg_ctrl &= ~CY_U3P_UIB_DEV_ENABLE;
        UIB->dev_pwr_cs |= CY_U3P_UIB_DISCON;

        /* Disable the UIB Core clock */
        GCTL->uib_core_clk &= ~(CY_U3P_GCTL_UIBCLK_CLK_EN);
        CyFx3BootBusyWait (2);

        return CY_FX3_BOOT_ERROR_NOT_SUPPORTED;
    }

    /* Copy the length and set glNoReenum to false as this would avoid copying the descriptors
     * in the table region again.
     * */
    length = ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->length;
    glNoReenum = CyFalse;

    status = CyFx3BootUsbLoadDescs ();
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->length = length;
        return status;
    }

    ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->length = length;
    glUsbSpeed = (CyFx3BootUsbSpeed_t)((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->usbSpeed;
    glNoReenum = CyTrue;
    glSsEnable = (((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->ssConnect ? CyTrue : CyFalse);

    /* Enable the interrupt masks and the UIB Core interrupt */
    USB3LNK->lnk_intr      = 0xFFFFFFFF;
    USB3LNK->lnk_intr_mask = CY_U3P_UIB_LGO_U3 | CY_U3P_UIB_LTSSM_CONNECT | CY_U3P_UIB_LTSSM_DISCONNECT |
        CY_U3P_UIB_LTSSM_RESET | CY_U3P_UIB_LTSSM_STATE_CHG;

    USB3PROT->prot_intr      = 0xFFFFFFFF;
    USB3PROT->prot_intr_mask = (CY_U3P_UIB_SUTOK_EN | CY_U3P_UIB_STATUS_STAGE | CY_U3P_UIB_TIMEOUT_PORT_CAP_EN |
            CY_U3P_UIB_TIMEOUT_PORT_CFG_EN | CY_U3P_UIB_LMP_RCV_EN | CY_U3P_UIB_LMP_PORT_CAP_EN |
            CY_U3P_UIB_LMP_PORT_CFG_EN);
    USB3PROT->prot_ep_intr_mask = 0;

    UIB->dev_ctl_intr = 0xFFFFFFFF;
    UIB->dev_ctl_intr_mask = (CY_U3P_UIB_SUDAV | CY_U3P_UIB_URESET | CY_U3P_UIB_SUSP |
            CY_U3P_UIB_URESUME | CY_U3P_UIB_HSGRANT);
    UIB->dev_ep_intr = 0xFFFFFFFF;
    UIB->dev_ep_intr_mask = 0;

    /* Enable the Vbus detection interrupt at this stage. */
    GCTL->iopwr_intr      = 0xFFFFFFFF;
    GCTL->iopwr_intr_mask = CY_U3P_VBUS;

    /* Enable the level 1 UIB interrupt mask. */
    UIB->intr_mask = CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT;

    VIC->int_clear  = 0xFFFFFFFF;
    VIC->int_select = (1 << USB_CoreInt) | (1 << IOPWR_Int) | (1 << GCTL_Int);
    VIC->int_enable = (1 << USB_CoreInt) | (1 << IOPWR_Int) | (1 << GCTL_Int);

    /* Assume that RxValid has been changed at this stage. */
    glRxValidMod = CyTrue;

    return CY_FX3_BOOT_ERROR_NO_REENUM_REQUIRED;
}

CyFx3BootErrorCode_t
CyFx3BootUsbStart (
        CyBool_t noReEnum,
        CyFx3BootUSBEventCb_t cb
        )
{
    uint8_t ep = 0;
    uint32_t tmp;

    glRxValidMod = CyFalse;
    glUsbUxWake = CyFalse;
    glInSSReset = CyFalse;

    tDisabledCount = 0;
    glEnableUsb3   = CyFalse;
    gEvtCbk = cb;

    glInCheckUsbDisconnect = CyFalse;
    glUibStatusSendErdy    = CyFalse;
    glUibEp0StatusPending  = CyFalse;
    glUibForceLpmAccept    = CyFalse;

    /* Clear the contents of the REENUM_CONTROL location. */
    (*(uint32_t *)(0x40002FFC)) = 0x00;
    if (noReEnum)
    {
        if (CyFx3BootUsbNoReEnum () == CY_FX3_BOOT_ERROR_NO_REENUM_REQUIRED)
        {
            return CY_FX3_BOOT_ERROR_NO_REENUM_REQUIRED;
        }
    }

    glUsbSpeed = CY_FX3_BOOT_NOT_CONNECTED;
    glNoReenum = noReEnum;

    if (glNoReenum)
    {
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->numDesc        = 0;
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->usbSpeed       = CY_FX3_BOOT_NOT_CONNECTED;
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->signature      = CY_WB_SIGNATURE;
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->bootSignature  = CY_FX3_BOOTER_SIGNATURE;
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->revision       = CY_FX3_BOOTER_REV_1_3_1;
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->ssConnect      = CyFalse;
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->switchToBooter = 0;

        /* Set the Descriptor table offset */
        tmp = sizeof (CyFx3BootUsbDescTable_t);

        /* Make it 4 byte aligned */
        if (tmp % 4)
        {
            tmp += (4 - (tmp % 4));
        }

        glNumDesc         = 0;
        glDescTableOffset = glUsbDescPtrs + tmp;
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->length = tmp;
    }
    else
    {
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->length = 0;
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->signature = 0xFFFFFFFF;
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->bootSignature = 0xFFFFFFFF;
        ((CyFx3BootUsbDescTable_t *)glUsbDescPtrs)->leaveGpioOn = 0;
    }

    ChangeUsbClockFreq (2, 2);
    GCTLAON->control = ((CY_U3P_GCTL_CONTROL_DEFAULT & ~CY_U3P_GCTL_ANALOG_SWITCH) |
                    CY_U3P_GCTL_WAKEUP_CPU_INT | CY_U3P_GCTL_WAKEUP_CLK);

    CyFx3UsbPowerOn ();

    UIB->otg_ctrl = CY_U3P_UIB_OTG_CTRL_DEFAULT;
    UIB->phy_clk_and_test = (CY_U3P_UIB_DATABUS16_8 | CY_U3P_UIB_VLOAD | CY_U3P_UIB_SUSPEND_N |
                            CY_U3P_UIB_EN_SWITCH);

    /* De-activate the pull-up on the D+ pin to disconnect from USB 2.0 host. */
    UIB->dev_pwr_cs |= CY_U3P_UIB_DISCON;

    /* Initialize the USB interrupts and the masks */
    GCTLAON->wakeup_en       = 0;
    GCTLAON->wakeup_polarity = 0;

    /* Link_phy_conf enable Rx terminations */
    USB3LNK->lnk_phy_conf   = 0xE0000001;
    USB3LNK->lnk_error_conf = 0xFFFFFFFF;
    USB3LNK->lnk_intr       = 0xFFFFFFFF;
    USB3LNK->lnk_intr_mask = CY_U3P_UIB_LGO_U3 | CY_U3P_UIB_LTSSM_CONNECT
                                | CY_U3P_UIB_LTSSM_DISCONNECT | CY_U3P_UIB_LTSSM_RESET |
                                CY_U3P_UIB_LTSSM_STATE_CHG;
    USB3PROT->prot_ep_intr_mask = 0;
    USB3PROT->prot_intr = 0xFFFFFFFF;
    /* enable the intr for receiving setup token */
    USB3PROT->prot_intr_mask = (CY_U3P_UIB_SUTOK_EN | CY_U3P_UIB_STATUS_STAGE | CY_U3P_UIB_TIMEOUT_PORT_CAP_EN |
                                CY_U3P_UIB_TIMEOUT_PORT_CFG_EN | CY_U3P_UIB_LMP_RCV_EN | CY_U3P_UIB_LMP_PORT_CAP_EN |
                                CY_U3P_UIB_LMP_PORT_CFG_EN);

    /* USB 2.0 device. */
    CyFx3Usb2PhySetup ();
    UIB->dev_ctl_intr = 0xFFFFFFFF;
    UIB->dev_ctl_intr_mask = (CY_U3P_UIB_SUDAV | CY_U3P_UIB_URESET | CY_U3P_UIB_SUSP |
            CY_U3P_UIB_URESUME | CY_U3P_UIB_HSGRANT);
    UIB->dev_ep_intr = 0xFFFFFFFF;
    UIB->dev_ep_intr_mask = 0;

    USB3PROT->prot_epi_cs1[0] = USB3PROT->prot_epo_cs1[0] = CY_U3P_UIB_SSEPO_VALID;
    USB3PROT->prot_epi_cs2[0] = USB3PROT->prot_epo_cs2[0] = 0x43;

    /* Configure USB 2.0 device EP0. */
    UIB->dev_epi_cs[0] = 0x4040;
    UIB->eepm_endpoint[0] = 0x40;
    /* configure endpoint 0 out */
    UIB->dev_epo_cs[0] = 0x4040;
    UIB->iepm_endpoint[0] = 0x40;

    /* Disable all EPs except EP0 */
    for (ep = 1; ep < 16; ep++)
    {
        UIB->dev_epo_cs[ep] &= ~CY_U3P_UIB_EPO_VALID;
        USB3PROT->prot_epo_cs1[ep] = 0;
        UIB->dev_epi_cs[ep] &= ~CY_U3P_UIB_EPI_VALID;
        USB3PROT->prot_epi_cs1[ep] = 0;
    }

    /* Disable all UIB interrupts at this stage. */
    UIB->intr_mask = 0;

    VIC->int_clear  = 0xFFFFFFFF;
    VIC->int_select = (1 << USB_CoreInt) | (1 << IOPWR_Int) | (1 << GCTL_Int);

    return CY_FX3_BOOT_SUCCESS;
}

/* [] */

