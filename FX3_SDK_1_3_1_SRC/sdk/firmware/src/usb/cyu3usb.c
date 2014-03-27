/*
## Cypress USB 3.0 Platform source file (cyu3usb.c)
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

#include <cyu3types.h>
#include <cyu3dma.h>
#include <cyu3error.h>
#include <cyu3protocol.h>
#include <cyu3vic.h>
#include <cyu3regs.h>
#include <cyu3usbpp.h>
#include <cyu3socket.h>
#include <cyu3utils.h>
#include <cyu3system.h>
#include <cyu3usbhost.h>
#include <cyu3usbotg.h>

#include <cyfx3_api.h>

/* Timeout duration (ms) for a EP0 transfer request. */
#define CY_U3P_EP0_XFER_TIMEOUT         (5000)
/* Period (ms) at which the LTSSM state should be checked while waiting for a EP0 transfer to complete. */
#define CY_U3P_EP0_CHECK_INCR           (100)

/* Timer configuration for Watchdog reset. */
#define CYU3P_WATCHDOG_MODE             (2)

CyU3PDmaChannel glUibChHandle;                    /* In channel handle for ep0 */
CyU3PDmaChannel glUibChHandleOut;                 /* Out channel handle for ep0 */
CyU3PUsbEpInfo  glPcktSizeIn[16];
CyU3PUsbEpInfo  glPcktSizeOut[16];

extern CyU3PTimer        glUibStatusTimer;
extern CyBool_t          glUibStatusSendErdy;
extern CyBool_t          glUibEp0StatusPending;
extern CyU3PUsbEpEvtCb_t glUsbEpCb;
extern uint32_t          glUsbEpEvtMask;
extern uint32_t          glUsbEvtEnabledEps;

       CyBool_t          glUsbForceLPMAccept = CyFalse;

/* Array that maps standard EP type definitions to those for the USB-SS PROT layer. */
const uint8_t CyU3PSSEpTypeMap[4] =
{
    3,  /* Control EP. */
    0,  /* ISO EP. */
    2,  /* Bulk EP. */
    1   /* Intr EP. */
};

CyBool_t
CyU3PUsbIsStarted (void)
{
    if (glUibDeviceInfo.usbState != CY_U3P_USB_INACTIVE)
    {
        return CyTrue;
    }

    return CyFalse;
}

void
CyU3PUsbPowerOn (
        void)
{
    CyFx3UsbPowerOn ();
}

static void
CyU3PUibInit (
        void)
{
    uint8_t ep = 0;

    /* Enable the Power regulators*/
    GCTLAON->wakeup_en       = 0;
    GCTLAON->wakeup_polarity = 0;

    /* TODO: NO Charger interrupts are enabled. */

    /* Link_phy_conf enable Rx terminations */ 
    USB3LNK->lnk_phy_conf   = 0xE0000001;
    USB3LNK->lnk_error_conf = 0xFFFFFFFF;
    USB3LNK->lnk_intr       = 0xFFFFFFFF;
    USB3LNK->lnk_intr_mask  = CY_U3P_UIB_LGO_U3 | CY_U3P_UIB_LTSSM_CONNECT |
        CY_U3P_UIB_LTSSM_DISCONNECT | CY_U3P_UIB_LTSSM_RESET | CY_U3P_UIB_LTSSM_STATE_CHG;

    USB3PROT->prot_ep_intr_mask = 0;
    USB3PROT->prot_intr         = 0xFFFFFFFF;
    USB3PROT->prot_intr_mask    = (CY_U3P_UIB_STATUS_STAGE | CY_U3P_UIB_SUTOK_EN | CY_U3P_UIB_EP0_STALLED_EN |
            CY_U3P_UIB_TIMEOUT_PORT_CAP_EN | CY_U3P_UIB_TIMEOUT_PORT_CFG_EN | CY_U3P_UIB_LMP_RCV_EN |
            CY_U3P_UIB_LMP_PORT_CAP_EN | CY_U3P_UIB_LMP_PORT_CFG_EN);


    /* USB 2.0 device. */
    CyFx3Usb2PhySetup ();
    UIB->dev_ctl_intr = 0xFFFFFFFF;
    UIB->dev_ctl_intr_mask = (CY_U3P_UIB_SUDAV | CY_U3P_UIB_URESET | CY_U3P_UIB_SUSP |
            CY_U3P_UIB_URESUME | CY_U3P_UIB_HSGRANT | CY_U3P_UIB_STATUS_STAGE);
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

    /* Flush and disable all EPs except EP0 */
    for (ep = 1; ep < 16; ep++)
    {
        CyU3PUsbFlushEp (ep);
        UIB->dev_epo_cs[ep] &= ~CY_U3P_UIB_EPO_VALID;
        USB3PROT->prot_epo_cs1[ep] = 0;
        CyU3PUsbFlushEp (ep | 0x80);
        UIB->dev_epi_cs[ep] &= ~CY_U3P_UIB_EPI_VALID;
        USB3PROT->prot_epi_cs1[ep] = 0;
    }

    /* Disable all UIB interrupts at this stage. */
    UIB->intr_mask &= ~(CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT | CY_U3P_UIB_LNK_INT |
            CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);

    /* Enable the Vbus detection interrupt at this stage. */
    GCTL->iopwr_intr      = 0xFFFFFFFF;
    GCTL->iopwr_intr_mask = CY_U3P_VBUS;
    glUibDeviceInfo.vbusDetectMode = CY_U3P_VBUS;
    CyU3PVicEnableInt (CY_U3P_VIC_GCTL_PWR_VECTOR);

    /* If a USB event callback has been registered, and VBus is already on; send an event notification. */
    if ((glUsbEvtCb != 0) && ((GCTL->iopower & CY_U3P_VBUS) != 0))
    {
        glUsbEvtCb (CY_U3P_USB_EVENT_VBUS_VALID, 0x00);
    }
}

CyU3PReturnStatus_t
CyU3PUsbVBattEnable (
        CyBool_t enable)
{
    uint32_t tmp;

    if (CyU3POtgGetMode () == CY_U3P_OTG_MODE_DEVICE_ONLY)
    {
        if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        {
            return CY_U3P_ERROR_NOT_STARTED;
        }

        if ((glUibDeviceInfo.usbState == CY_U3P_USB_CONNECTED) || (glUibDeviceInfo.usbState == CY_U3P_USB_VBUS_WAIT))
        {
            return CY_U3P_ERROR_INVALID_SEQUENCE;
        }
    }
    else
    {
        if ((CyU3PUsbIsStarted ()) || (CyU3PUsbHostIsStarted ()))
        {
            return CY_U3P_ERROR_INVALID_SEQUENCE;
        }
    }

    if (enable)
    {
        glUibDeviceInfo.vbat_Power = CyTrue;
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
        CyU3PBusyWait (100);

        GCTL->iopower = tmp;
        glUibDeviceInfo.vbat_Power = CyFalse;
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbCreateDmaChannels (
        void)
{
    CyU3PReturnStatus_t status;
    CyU3PDmaChannelConfig_t dmaConfig;

    /* setup the DMA for EP 0 IN and EP0 OUT */
    dmaConfig.size        = 512;
    dmaConfig.count       = 1;
    dmaConfig.prodAvailCount = 0;
    dmaConfig.dmaMode     = CY_U3P_DMA_MODE_BYTE;
    dmaConfig.prodHeader  = 0;
    dmaConfig.prodFooter  = 0;
    dmaConfig.consHeader  = 0;
    dmaConfig.cb             = 0;
    dmaConfig.notification = 0;
    dmaConfig.prodSckId    = CY_U3P_CPU_SOCKET_PROD;
    dmaConfig.consSckId      = CY_U3P_UIB_SOCKET_CONS_0;

    status = CyU3PDmaChannelCreate (&glUibChHandle, CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaConfig);
    if (status != CY_U3P_SUCCESS)
    {
        return (CY_U3P_ERROR_CHANNEL_CREATE_FAILED);
    }

    /* The cache control for this channel has to be done internally. */
    if (glIsDCacheEnabled)
    {
        status = CyU3PDmaChannelCacheControl (&glUibChHandle, CyTrue);
        if (status != CY_U3P_SUCCESS)
        {
            CyU3PDmaChannelDestroy (&glUibChHandle);
            return (CY_U3P_ERROR_CHANNEL_CREATE_FAILED);
        }
    }

    /* creating out channel for usb */
    dmaConfig.prodSckId   = CY_U3P_UIB_SOCKET_PROD_0;
    dmaConfig.consSckId   = CY_U3P_CPU_SOCKET_CONS;
    status = CyU3PDmaChannelCreate (&glUibChHandleOut, CY_U3P_DMA_TYPE_MANUAL_IN, &dmaConfig);

    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDmaChannelDestroy (&glUibChHandle);
        return (CY_U3P_ERROR_CHANNEL_CREATE_FAILED);
    }

    /* The cache control for this channel has to be done internally. */
    if (glIsDCacheEnabled)
    {
        status = CyU3PDmaChannelCacheControl (&glUibChHandleOut, CyTrue);
        if (status != CY_U3P_SUCCESS)
        {
            CyU3PDmaChannelDestroy (&glUibChHandle);
            CyU3PDmaChannelDestroy (&glUibChHandleOut);
            return (CY_U3P_ERROR_CHANNEL_CREATE_FAILED);
        }
    }

    return status;
}

/* Global bitmap indicating if the application and booter can switch control back forth between 'em. */
uint32_t glSwitchToBooter = 0;

#define CY_U3P_USB_SDK_SWITCH_ENABLE     (0x01)
#define CY_U3P_USB_BOOTER_SWITCH_ENABLE  (0x02)

/* This function enables/disables the switching the control back to the FX3
 * 2-stage bootloader from the final application. */
void
CyU3PUsbSetBooterSwitch (
        CyBool_t enable)
{
    if (enable)
    {
        glSwitchToBooter |= CY_U3P_USB_SDK_SWITCH_ENABLE;
    }
    else
    {
        glSwitchToBooter &= ~CY_U3P_USB_SDK_SWITCH_ENABLE;
    }
}

CyU3PReturnStatus_t
CyU3PUsbGetBooterVersion (
        uint8_t *major_p,
        uint8_t *minor_p,
        uint8_t *patch_p)
{
    CyU3PUsbDescTable_t *state_p = (CyU3PUsbDescTable_t *)CYU3P_DEVSTATE_LOCATION;
    uint32_t temp, checksum;

    if ((major_p == 0) || (minor_p == 0) || (patch_p == 0))
        return CY_U3P_ERROR_BAD_ARGUMENT;

    if (state_p->signature != CY_USBDEV_SIGNATURE)
        return CY_U3P_ERROR_FAILURE;

    /* Work-around for checksum calculation bug in SDK 1.2.x and older. */
    if ((state_p->bootSignature == CY_USB_BOOTER_SIGNATURE) && (state_p->revision >= CY_USB_BOOTER_REV_1_3_0))
        temp = state_p->length;
    else
        temp = state_p->length & 0xFF;
    if ((state_p->length == 0) || (state_p->length > (0x1000 - 8)))
        return CY_U3P_ERROR_FAILURE;

    CyU3PComputeChecksum ((uint32_t *)CYU3P_DEVSTATE_LOCATION, temp, &checksum);
    if (checksum != *((uint32_t *)(CYU3P_DEVSTATE_LOCATION + state_p->length)))
        return CY_U3P_ERROR_FAILURE;

    temp = state_p->revision;
    if ((temp >= CY_USB_BOOTER_REV_1_1_1) && (temp <= CY_USB_BOOTER_REV_1_3_0))
    {
        *major_p = (uint8_t)(temp >> 16);
        *minor_p = (uint8_t)(temp >> 8);
        *patch_p = (uint8_t)(temp);
    }
    else
    {
        *major_p = 1;
        *minor_p = 1;
        *patch_p = 0;
    }

    return CY_U3P_SUCCESS;
}

extern void jump(uint32_t add);
extern CyBool_t
CyU3PAreLppsOff (
        uint32_t retainGpioState);

CyU3PReturnStatus_t
CyU3PUsbJumpBackToBooter (
        uint32_t address)
{
    CyU3PUsbDescTable_t *state_p = (CyU3PUsbDescTable_t *)CYU3P_DEVSTATE_LOCATION;
    uint32_t temp1 = 0;
    uint32_t temp2 = 0;

    /* Check if both the application and the booter can switch control back. */
    temp1 = (CY_U3P_USB_SDK_SWITCH_ENABLE | CY_U3P_USB_BOOTER_SWITCH_ENABLE);

    if (glSwitchToBooter == temp1)
    {
        /* Make sure that LPP blocks have been turned off at this stage. */
        if (CyU3PAreLppsOff (state_p->leaveGpioOn) == CyFalse)
            return CY_U3P_ERROR_INVALID_SEQUENCE;

        GCTL->iopwr_intr      = 0xFFFFFFFF;
        GCTL->iopwr_intr_mask = 0;

        /* Disable the UIB Interrupts */
        UIB->intr_mask &= ~(CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT | CY_U3P_UIB_LNK_INT |
            CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);

        /* Disable the Caches */
        CyU3PDeviceCacheControl (0, 0, 0);

        /* Clean up */
        CyU3PDmaChannelDestroy (&glUibChHandle);
        CyU3PDmaChannelDestroy (&glUibChHandleOut);

        VIC->int_clear = 0xFFFFFFFF;

        /* Set the flag in the booter descriptor table indicating that the 
           control is being transferred from the application. */
        state_p->switchToBooter = 1;

        /* Update the current USB connection speed in the descriptor table. */
        state_p->usbSpeed = CyU3PUsbGetSpeed ();

        /* Recompute the check sum and store it in the descriptor table.
         * SDK 1.2.3 and older have a checksum bug that needs to be accounted for. */
        if (state_p->revision >= CY_USB_BOOTER_REV_1_3_0)
        temp1 = state_p->length;
        else
            temp1 = state_p->length & 0xFF;
        CyU3PComputeChecksum ((uint32_t *)CYU3P_DEVSTATE_LOCATION, temp1, &temp2);
        *(uint32_t *)(CYU3P_DEVSTATE_LOCATION + state_p->length) = temp2;

        /* Value indicating that the booter is to jump to main instead of __main. */
        (*(uint32_t *)(0x40002FFC)) = 0xB0;

        /* Disable the Watchdog Timers */
        temp2 = 0;
        temp1 = GCTLAON->watchdog_cs;
        CyU3PBusyWait (10);

        /* If watchdog 0 is in reset mode, it is allowed to continue. Otherwise it is forcible disabled. */
        if (((temp1 & CY_U3P_GCTL_MODE0_MASK) != CY_U3P_GCTL_MODE0_MASK) &&
                ((temp1 & CY_U3P_GCTL_MODE0_MASK) != CYU3P_WATCHDOG_MODE))
            temp2 = CY_U3P_GCTL_MODE0_MASK;

        if ((temp1 & CY_U3P_GCTL_MODE1_MASK) != CY_U3P_GCTL_MODE1_MASK)
            temp2 |= CY_U3P_GCTL_MODE1_MASK;

        if (temp2 != 0)
        {
            GCTLAON->watchdog_cs = temp1 | temp2;
            CyU3PBusyWait (10);
        }

        /* Reset the EPM mux, and then re-enable EP0. */
        UIB->iepm_cs |= (CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
        CyU3PBusyWait (1);
        UIB->iepm_cs &= ~(CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
        CyU3PBusyWait (1);

        if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
        {
            USB3PROT->prot_epi_cs1[0] |= CY_U3P_UIB_SSEPI_VALID;
            UIB->eepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */
            USB3PROT->prot_epo_cs1[0] |= CY_U3P_UIB_SSEPO_VALID;
            UIB->iepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */
        }
        else
        {
            UIB->eepm_endpoint[0] = 0x40;
            UIB->iepm_endpoint[0] = 0x40;
            UIB->dev_epi_cs[0] |= CY_U3P_UIB_EPI_VALID;
            UIB->dev_epo_cs[0] |= CY_U3P_UIB_EPO_VALID;
        }

        /* Jump to the reset handler of the Booter */      
        jump (address);
    }
    else
    {
        /* Operation not enabled. */
        return CY_U3P_ERROR_OPERN_DISABLED;
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t 
CyU3PUsbLoadBootloaderDescs (
        CyU3PUsbDescTable_t *state_p)
{
    CyU3PReturnStatus_t ret;
    uint8_t i = 0;
    uint8_t count = state_p->numDesc;

    for (i = 0; i < count; i++)
    {
        ret = CyU3PUsbSetDesc ((CyU3PUSBSetDescType_t)(state_p->descPtrs[i].descType),
                state_p->descPtrs[i].descIndex, (uint8_t *)(state_p->descPtrs[i].descPtr));
        if (ret != CY_U3P_SUCCESS)
        {
            return ret;
        }
    }

    return CY_U3P_SUCCESS;
}

void
CyU3PSetUsbCoreClock (
        uint8_t pclk,
        uint8_t epmclk)
{
    uint32_t mask;
    uint32_t i;

    /* We need to disable interrupts while doing this. */
    mask = CyU3PVicDisableAllInterrupts ();

    /* Turn the UIB clock off first. */
    GCTL->uib_core_clk &= ~(CY_U3P_GCTL_UIBCLK_CLK_EN);
    for (i = 20; i > 0; i--)
        __nop ();

    /* Set the clock dividers as required, and then turn the clock ON again. */
    GCTL->uib_core_clk  = (pclk << CY_U3P_GCTL_UIBCLK_PCLK_SRC_POS) | (epmclk << CY_U3P_GCTL_UIBCLK_EPMCLK_SRC_POS);
    GCTL->uib_core_clk |= CY_U3P_GCTL_UIBCLK_CLK_EN;
    for (i = 80; i > 0; i--)
        __nop ();

    /* Now restore the interrupts that were on. */
    CyU3PVicEnableInterrupts (mask);
}

CyBool_t
CyFx3IsUsbActive (
        void)
{
    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        return CyFalse;
    return CyTrue;
}

extern CyBool_t glSdk_UsbIsOn;
extern CyBool_t glRxValidMod;

CyU3PReturnStatus_t
CyU3PUsbStart (
        void)
{
    CyU3PReturnStatus_t status;

    /* If the UIB thread is not started, fail the USB start call. */
    if (!glUibThreadStarted)
    {
        return CY_U3P_ERROR_MEMORY_ERROR;
    }

    if (glUibDeviceInfo.usbState != CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    /* Check if the USB module is already started by the
     * first stage boot-loader. This prevents re-enumeration of the device.
     * OTG mode is not possible in this configuration. */
    if (glSdk_UsbIsOn)
    {
        CyU3PUsbDescTable_t *state_p = (CyU3PUsbDescTable_t *)CYU3P_DEVSTATE_LOCATION;

        /* Make sure that all UIB sockets are disabled before creating any DMA channels. */
        CyU3PUibSocketInit ();

        status = CyU3PUsbCreateDmaChannels ();
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        glUibDeviceInfo.usbState = CY_U3P_USB_STARTED;
        glUibDeviceInfo.usbSpeed = state_p->usbSpeed;
        status = CyU3PUsbLoadBootloaderDescs (state_p);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* Assume that SET_CONFIG is complete. Otherwise, we could not have got here. */
        glUibDeviceInfo.usbState = CY_U3P_USB_ESTABLISHED;

        /* Assume that the RxValid bit in the PHY has been turned off. Otherwise, we can have a failure if the
           link is currently in U1/U2 state.
         */
        if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
            glRxValidMod = CyTrue;

        if (state_p->bootSignature == CY_USB_BOOTER_SIGNATURE)
        {
            if (state_p->revision >= CY_USB_BOOTER_REV_1_1_1)
            {
                glUibDeviceInfo.enableSS = (state_p->ssConnect) ? CyTrue : CyFalse;
            }

            if (state_p->revision >= CY_USB_BOOTER_REV_1_2_0)
            {
                /* This version of the bootloader supports switching control back from the application. */
                glSwitchToBooter |= CY_U3P_USB_BOOTER_SWITCH_ENABLE;
            }
        }
        else
        {
            /* Structure has no information on whether the user wanted USB 3.0. Assume that 3.0 connection is
               required. */
            glUibDeviceInfo.enableSS = CyTrue;
        }

        /* Enable the interrupt masks and the UIB Core interrupt */
        USB3LNK->lnk_intr      = 0xFFFFFFFF;
        USB3LNK->lnk_intr_mask = CY_U3P_UIB_LGO_U3 |
            CY_U3P_UIB_LTSSM_CONNECT | CY_U3P_UIB_LTSSM_DISCONNECT | CY_U3P_UIB_LTSSM_RESET |
            CY_U3P_UIB_LTSSM_STATE_CHG;

        USB3PROT->prot_intr      = 0xFFFFFFFF;
        USB3PROT->prot_intr_mask = (CY_U3P_UIB_STATUS_STAGE | CY_U3P_UIB_SUTOK_EN | CY_U3P_UIB_EP0_STALLED_EN |
                CY_U3P_UIB_TIMEOUT_PORT_CAP_EN | CY_U3P_UIB_TIMEOUT_PORT_CFG_EN | CY_U3P_UIB_LMP_RCV_EN |
                CY_U3P_UIB_LMP_PORT_CAP_EN | CY_U3P_UIB_LMP_PORT_CFG_EN);
        USB3PROT->prot_ep_intr_mask = 0;

        UIB->dev_ctl_intr = 0xFFFFFFFF;
        UIB->dev_ctl_intr_mask = (CY_U3P_UIB_SUDAV | CY_U3P_UIB_URESET | CY_U3P_UIB_SUSP |
                CY_U3P_UIB_URESUME | CY_U3P_UIB_HSGRANT | CY_U3P_UIB_STATUS_STAGE);
        UIB->dev_ep_intr = 0xFFFFFFFF;
        UIB->dev_ep_intr_mask = 0;

        /* Enable the level 1 UIB interrupt mask. */
        UIB->intr_mask = CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT | 
            CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN;

        CyU3PVicEnableInt (CY_U3P_VIC_UIB_DMA_VECTOR);
        CyU3PVicEnableInt (CY_U3P_VIC_UIB_CORE_VECTOR);

        /* Ensure that the endpoints are enabled with the correct connection speed. */
        CyU3PUsbEpPrepare (CyU3PUsbGetSpeed ());

        /* If we do not need to switch back to the booter, corrupt the state data structure. */
        if (!(glSwitchToBooter & CY_U3P_USB_BOOTER_SWITCH_ENABLE))
        {
            state_p->signature = 0xFFFFFFFF;
            state_p->length    = 0;
        }

        GCTL->iopwr_intr      = 0xFFFFFFFF;
        GCTL->iopwr_intr_mask = CY_U3P_VBUS;
        glUibDeviceInfo.vbusDetectMode = CY_U3P_VBUS;
        CyU3PVicEnableInt (CY_U3P_VIC_GCTL_PWR_VECTOR);

        return CY_U3P_ERROR_NO_REENUM_REQUIRED;
    }

    /* Make sure any USB device PHYs are turned off. */
    UIB->otg_ctrl &= ~CY_U3P_UIB_SSDEV_ENABLE;
    CyU3PBusyWait (2);
    UIB->otg_ctrl &= ~CY_U3P_UIB_SSEPM_ENABLE;

    UIB->otg_ctrl &= ~CY_U3P_UIB_DEV_ENABLE;
    CyU3PDisconUsbPins ();

    /* Disable the UIB core clock */
    GCTL->uib_core_clk &= ~(CY_U3P_GCTL_UIBCLK_CLK_EN);
    CyU3PBusyWait (2);

    /* Check whether device mode operation is allowed. */
    if ((!CyU3POtgIsDeviceMode ()) || (CyU3PUsbHostIsStarted ()))
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    CyU3PSetUsbCoreClock (2, 2);
    GCTLAON->control   = ((CY_U3P_GCTL_CONTROL_DEFAULT & ~CY_U3P_GCTL_ANALOG_SWITCH) | CY_U3P_GCTL_WAKEUP_CPU_INT |
            CY_U3P_GCTL_WAKEUP_CLK);

    /* Initialize UIB block only if in device only mode. */
    if (CyU3POtgGetMode () == CY_U3P_OTG_MODE_DEVICE_ONLY)
    {
        CyU3PUsbPowerOn ();
        UIB->otg_ctrl = CY_U3P_UIB_OTG_CTRL_DEFAULT;
    }

    UIB->phy_clk_and_test = (CY_U3P_UIB_DATABUS16_8 | CY_U3P_UIB_VLOAD | CY_U3P_UIB_SUSPEND_N | CY_U3P_UIB_EN_SWITCH);

    /* Make sure the USB pins are disconnected. */
    CyU3PDisconUsbPins ();

    /* Initialize the descriptor pointers to default values. */
    CyU3PUsbDescInit ();

    status = CyU3PUsbCreateDmaChannels ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    glUibDeviceInfo.usbState = CY_U3P_USB_STARTED;

    /* Work-around for compatibility reasons. If a setup callback has already been registered and
       application (not fast) enumeration mode is desired, treat the configuration as having been
       completed. */
    if (glUibDeviceInfo.enumMethod == CY_U3P_USBENUM_PPORT)
    {
        glUibDeviceInfo.usbState = CY_U3P_USB_CONFIGURED;
    }
    else
    {
        glUibDeviceInfo.usbState = CY_U3P_USB_WAITING_FOR_DESC;
    }

    /* intialize intrs and masks */
    CyU3PUibInit ();

    return status;
}

static void
GetUsbLinkActive (
        void)
{
    uint32_t state;

    /* Make sure that any EP stall is cleared up at this stage. */
    if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
    {
        USB3PROT->prot_epi_cs1[0] &= ~CY_U3P_UIB_SSEPI_STALL;
        USB3PROT->prot_epo_cs1[0] &= ~CY_U3P_UIB_SSEPO_STALL;

        /* If the Link is in U0/U1, try and get it back to U0 before moving on. */
        if (!glUibDeviceInfo.isLpmDisabled)
        {
            state = USB3LNK->lnk_ltssm_state &  CY_U3P_UIB_LTSSM_STATE_MASK;
            if ((state == CY_U3P_UIB_LNK_STATE_U1) || (state == CY_U3P_UIB_LNK_STATE_U2))
                CyU3PUsbSetLinkPowerState (CyU3PUsbLPM_U0);
        }
    }
}

static CyBool_t
CyU3PUsbIsNewCtrlRqtRecvd (
        void)
{
    if ((glUibDeviceInfo.newCtrlRqt) || (UIB->dev_ctl_intr & CY_U3P_UIB_SUDAV) ||
            (USB3PROT->prot_intr & CY_U3P_UIB_SUTOK_EV))
        return CyTrue;
    return CyFalse;
}

void
CyU3PUsbAckSetup (
        void)
{
    uint32_t mask;

    CyU3PMutexGet (&glUibLock, CYU3P_WAIT_FOREVER);
    GetUsbLinkActive ();

    /* If a new CTRL request has been received, don't do anything here. */
    if (CyU3PUsbIsNewCtrlRqtRecvd ())
    {
        CyU3PMutexPut (&glUibLock);
        return;
    }

    /* Clear stalls on both EP0-OUT and EP0-IN and
     * then clear the SETUP busy flag to allow
     * the rest of the transfer to proceed. */
    glUibDeviceInfo.ackPending = CyFalse;
    if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
    {
        USB3PROT->prot_epi_cs1[0] &= ~CY_U3P_UIB_SSEPI_STALL;
        USB3PROT->prot_epo_cs1[0] &= ~CY_U3P_UIB_SSEPO_STALL;

        mask = CyU3PVicDisableAllInterrupts ();
        CyU3PBusyWait (1);
        USB3PROT->prot_cs = (USB3PROT->prot_cs | CY_U3P_UIB_SS_SETUP_CLR_BUSY);
        CyU3PVicEnableInterrupts (mask);

        UIB->eepm_endpoint[0] = 0x200;

        CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USBSS_ACKSETUP);

        if (!glUibDeviceInfo.isLpmDisabled)
        {
            glUibStatusSendErdy = CyTrue;
            CyU3PTimerModify (&glUibStatusTimer, 100, 0);
            CyU3PTimerStart (&glUibStatusTimer);
        }
    }
    else
    {
        UIB->dev_cs |= CY_U3P_UIB_SETUP_CLR_BUSY;
        UIB->eepm_endpoint[0] = 0x40;
    }

    CyU3PMutexPut (&glUibLock);
}

/* This function is used to send data from EPO to host */
CyU3PReturnStatus_t
CyU3PUsbSendEP0Data (uint16_t count, uint8_t *buffer)
{
    CyU3PDmaBuffer_t buf_p;
    uint32_t delay = CY_U3P_EP0_XFER_TIMEOUT;
    uint8_t ret;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* in case when a invalid buffer has been provided */
    if (buffer == NULL)
    {
        return (CY_U3P_ERROR_BAD_ARGUMENT);
    }

    CyU3PMutexGet (&glUibLock, CYU3P_WAIT_FOREVER);

    /* Check if a USB reset or a new control request has already aborted this request. */
    UIB->dev_ctl_intr_mask &= ~CY_U3P_UIB_URESET;
    if ((glUibDeviceInfo.inReset) || (CyU3PUsbIsNewCtrlRqtRecvd ()))
    {
        UIB->dev_ctl_intr_mask |= CY_U3P_UIB_URESET;
        CyU3PMutexPut (&glUibLock);
        return CY_U3P_SUCCESS;
    }
    UIB->dev_ctl_intr_mask |= CY_U3P_UIB_URESET;

    /* in case when a number of bytes to be transferred are more that 
       what is asked by USB, send only what USB request for. */
    if (UIB->dev_epi_xfer_cnt[0] < count)
    {
        count = UIB->dev_epi_xfer_cnt[0];
    }

    /* Make sure we are in U0 state before proceeding. */
    GetUsbLinkActive ();

    buf_p.count  = count;
    buf_p.size   = ((count + 15) & 0xFFF0);
    buf_p.buffer = buffer;
    buf_p.status = 0;
    ret = CyU3PDmaChannelSetupSendBuffer (&glUibChHandle, &buf_p);

    /* We can release the lock, now that the transfer has been queued. */
    CyU3PMutexPut (&glUibLock);

    if (ret == CY_U3P_SUCCESS)
    {
        /* Clear the busy bit. */
        if (glUibDeviceInfo.ackPending != CyFalse)
        CyU3PUsbAckSetup ();

        while (delay != 0)
        {
            ret = CyU3PDmaChannelWaitForCompletion (&glUibChHandle, CY_U3P_EP0_CHECK_INCR);
            if (ret == CY_U3P_ERROR_TIMEOUT)
            {
                if (CyU3PUsbIsNewCtrlRqtRecvd ())
                {
                    return CY_U3P_SUCCESS;
                }

                delay -= CY_U3P_EP0_CHECK_INCR;
            }
            else
                break;
        }

        if (ret != CY_U3P_SUCCESS)
        {
            /* Need to NAK EP0 before resetting the channel. */
            CyU3PMutexGet (&glUibLock, CYU3P_WAIT_FOREVER);
            CyU3PUsbSetEpNak (0x80, CyTrue);
            CyU3PBusyWait (100);
            CyU3PDmaChannelReset (&glUibChHandle);
            CyU3PUsbFlushEp (0x80);
            CyU3PUsbSetEpNak (0x80, CyFalse);
            CyU3PMutexPut (&glUibLock);
        }
    }

    return ret;
}

/* The function is used to get data from EPO */
CyU3PReturnStatus_t
CyU3PUsbGetEP0Data (uint16_t size, uint8_t *buffer, uint16_t* readCount)
{
    CyU3PDmaBuffer_t buf_p;
    uint32_t delay = CY_U3P_EP0_XFER_TIMEOUT;
    uint8_t ret;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if (buffer == NULL)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    CyU3PMutexGet (&glUibLock, CYU3P_WAIT_FOREVER);

    /* Round size up to a multiple of 16 bytes. */
    size = (size + 0x0F) & 0xFFF0;

    /* Check if a USB reset or a new control request has already aborted this request. */
    UIB->dev_ctl_intr_mask &= ~CY_U3P_UIB_URESET;
    if ((glUibDeviceInfo.inReset) || (CyU3PUsbIsNewCtrlRqtRecvd ()))
    {
        UIB->dev_ctl_intr_mask |= CY_U3P_UIB_URESET;
        CyU3PMutexPut (&glUibLock);
        return CY_U3P_ERROR_XFER_CANCELLED;
    }
    UIB->dev_ctl_intr_mask |= CY_U3P_UIB_URESET;

    /* Make sure we are in U0 state before proceeding. */
    GetUsbLinkActive ();

    buf_p.size = size;
    buf_p.buffer = buffer;
    buf_p.status = 0;
    ret = CyU3PDmaChannelSetupRecvBuffer (&glUibChHandleOut, &buf_p);

    /* Release the lock once the request has been queued. */
    CyU3PMutexPut (&glUibLock);

    if (ret == CY_U3P_SUCCESS)
    {
        /* Clear the busy bit. */
        if (glUibDeviceInfo.ackPending != CyFalse)
            CyU3PUsbAckSetup ();

        while (delay != 0)
        {
            ret = CyU3PDmaChannelWaitForRecvBuffer (&glUibChHandleOut, &buf_p, CY_U3P_EP0_CHECK_INCR);
            if (ret == CY_U3P_ERROR_TIMEOUT)
            {
                if (CyU3PUsbIsNewCtrlRqtRecvd ())
                {
                    return CY_U3P_ERROR_XFER_CANCELLED;
                }

                delay -= CY_U3P_EP0_CHECK_INCR;
            }
            else
                break;
        }

        if (ret != CY_U3P_SUCCESS)
        {
            /* NAK EP0 before resetting the channel. */
            CyU3PMutexGet (&glUibLock, CYU3P_WAIT_FOREVER);
            CyU3PUsbSetEpNak (0x00, CyTrue);
            CyU3PBusyWait (100);
            CyU3PDmaChannelReset (&glUibChHandleOut);
            CyU3PUsbSetEpNak (0x00, CyFalse);
            CyU3PMutexPut (&glUibLock);
        }
        else
        {
            if (readCount != NULL)
                *readCount = buf_p.count;
        }
    }

    return ret;
}

void
CyU3PUsbPhyEnable (
        CyBool_t is_ss)
{
    uint32_t mask;

    CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USB2_PHY_ON + is_ss);

    /* Clear all interrupts related to USB and enable the interrupt vectors. */
    UIB->dev_ctl_intr = 0xFFFFFFFF;
    UIB->dev_ep_intr  = 0xFFFFFFFF;
    CyU3PVicEnableInt (CY_U3P_VIC_UIB_DMA_VECTOR);
    CyU3PVicEnableInt (CY_U3P_VIC_UIB_CORE_VECTOR);

    /* Enable the USB device mode interrupts. */
    UIB->intr_mask |= (CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT | 
	CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);

    GCTLAON->control |= CY_U3P_GCTL_USB_POWER_EN;

    /* Enable the requested PHY, as USB 2.0 and 3.0 cannot function together as of now. */
    if (is_ss)
    {
        glUsbLinkErrorCount   = USB3LNK->lnk_error_count;
        glUibEp0StatusPending    = CyFalse;

        /* Make sure that all relevant USB 3.0 interrupts are enabled. */
	USB3LNK->lnk_intr = 0xFFFFFFFF;
	USB3LNK->lnk_intr_mask = CY_U3P_UIB_LGO_U3 |
	    CY_U3P_UIB_LTSSM_CONNECT | CY_U3P_UIB_LTSSM_DISCONNECT | CY_U3P_UIB_LTSSM_RESET |
            CY_U3P_UIB_LTSSM_STATE_CHG;
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

        /* Shorten U3 exit LFPS duration based on operating clock. */
        USB3LNK->lnk_phy_tx_trim           = glUsb3TxTrimVal;
        CyFx3Usb3LnkSetup ();

        /* Set port config and capability timers to their initial values. */
        USB3PROT->prot_lmp_port_capability_timer    = CY_U3P_UIB_PROT_LMP_PORT_CAP_TIMER_VALUE;
        USB3PROT->prot_lmp_port_configuration_timer = CY_U3P_UIB_PROT_LMP_PORT_CFG_TIMER_VALUE;

        /* Turn on AUTO response to LGO_U3 command from host. */
        USB3LNK->lnk_compliance_pattern_8 |= CY_U3P_UIB_LFPS;
        glUibDeviceInfo.hpTimeoutCnt       = 0;
        USB3LNK->lnk_phy_conf              = 0x20000001;        /* Disable terminations. */

        CyU3PSetUsbCoreClock (2, 2);

        /* Disable all interrupts for this duration. */
        mask = CyU3PVicDisableAllInterrupts ();
        glPollingRxEqSeen = CyFalse;

        /* Force LTSSM into SS.Disabled state for 100us after the PHY is turned on. */
        USB3LNK->lnk_ltssm_state = (CY_U3P_UIB_LNK_STATE_SSDISABLED << CY_U3P_UIB_LTSSM_OVERRIDE_VALUE_POS) |
            CY_U3P_UIB_LTSSM_OVERRIDE_EN;
        UIB->otg_ctrl |= CY_U3P_UIB_SSDEV_ENABLE;
        CyU3PBusyWait (100);

        USB3LNK->lnk_conf = (USB3LNK->lnk_conf & ~CY_U3P_UIB_EPM_FIRST_DELAY_MASK) |
            (15 << CY_U3P_UIB_EPM_FIRST_DELAY_POS) | CY_U3P_UIB_LDN_DETECTION;
        USB3LNK->lnk_phy_mpll_status = 0x00310018 | CY_U3P_UIB_SSC_EN;

        /* Hardware work-arounds to control LFPS. */
        CyFx3UsbWritePhyReg (0x1010, 0x0080);
        CyFx3UsbWritePhyReg (0x1006, 0x0180);
        CyFx3UsbWritePhyReg (0x1024, 0x0080);

        CyU3PVicEnableInterrupts (mask);

        USB3LNK->lnk_ltssm_state &= ~CY_U3P_UIB_LTSSM_OVERRIDE_EN;
        USB3LNK->lnk_phy_conf     = 0xE0000001; /* Enable USB 3.0 terminations. */

        CyU3PBusyWait (100);
        GCTL->uib_core_clk = CY_U3P_GCTL_UIBCLK_CLK_EN | (1 << CY_U3P_GCTL_UIBCLK_PCLK_SRC_POS) |
            (1 << CY_U3P_GCTL_UIBCLK_EPMCLK_SRC_POS);
    }
    else
    {
        if (glUibDeviceInfo.usb2Disable)
            return;

        glUibDeviceInfo.usbSpeed = CY_U3P_FULL_SPEED;

        /* USB 2.0 LPM-L1 workaround. */
        UIB->ehci_portsc = CY_U3P_UIB_WKOC_E;
        glUibDeviceInfo.isLpmDisabled = CyFalse;
 
        /* Disable EP0-IN and EP0-OUT. */
        UIB->dev_epi_cs[0] &= ~CY_U3P_UIB_EPI_VALID;
        UIB->dev_epo_cs[0] &= ~CY_U3P_UIB_EPO_VALID;

        CyU3PBusyWait (2);
        /* Clear everything except OTG_ENABLE. */
        UIB->otg_ctrl &= CY_U3P_UIB_OTG_ENABLE;
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
}

void
CyU3PUsbPhyDisable (
        CyBool_t is_ss)
{
    CyU3PUsbAddToEventLog (CYU3P_USB_LOG_USB2_PHY_OFF + is_ss);

    glUibStatusSendErdy = CyFalse;
    CyU3PTimerStop (&glUibStatusTimer);

    /* De-register USB related interrupts and clear any pending interrupts. */
    CyU3PVicDisableInt (CY_U3P_VIC_UIB_DMA_VECTOR);

    /* Disable the USB device mode interrupts. */
    UIB->intr_mask &= ~(CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT | 
	CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);

    UIB->dev_ctl_intr = 0xFFFFFFFF;
    UIB->dev_ep_intr  = 0xFFFFFFFF;

    /* Reset the EP-0 DMA channels. */
    CyU3PDmaChannelReset (&glUibChHandle);
    CyU3PDmaChannelReset (&glUibChHandleOut);

    /* Turn off the PHY that is active. Also disable the pull-up on D+, if in USB 2.0 mode. */
    if (is_ss)
    {
        /* Make sure that the driver thread exits the compliance polling loop. */
        glUibDeviceInfo.exitCompliance = CyTrue;
        CyU3PThreadRelinquish ();

        USB3LNK->lnk_intr   = 0xFFFFFFFF;
        USB3PROT->prot_intr = 0xFFFFFFFF;

        glUsbLinkErrorCount   = USB3LNK->lnk_error_count;
        glUibEp0StatusPending    = CyFalse;

        CyFx3UsbWritePhyReg (0x1005, 0x0000);

        /* Change EPM config to full speed */
        CyU3PSetUsbCoreClock (2, 2);
        CyU3PBusyWait (2);

        /* Disable usb3 functionality */
        UIB->otg_ctrl &= ~CY_U3P_UIB_SSDEV_ENABLE;
        CyU3PBusyWait (2);
        UIB->otg_ctrl &= ~CY_U3P_UIB_SSEPM_ENABLE;

        USB3LNK->lnk_phy_conf        &= 0x1FFFFFFF;
        USB3LNK->lnk_phy_mpll_status  = glUsbMpllDefault;
    }
    else
    {
        CyU3PDisconUsbPins ();
        CyU3PSetUsbCoreClock (2, 2);
        CyU3PBusyWait (2);

        /* Clear all bits except for OTG_ENABLE. */
        UIB->otg_ctrl &= CY_U3P_UIB_OTG_ENABLE;
    }

    GCTLAON->control &= ~CY_U3P_GCTL_USB_POWER_EN;
    glUibDeviceInfo.usbSpeed = CY_U3P_NOT_CONNECTED;

    /* The OTG functionality requires USB_POWER to be on. */
    if (CyU3POtgIsStarted ())
    {
        CyU3PBusyWait (100);
        GCTLAON->control |= CY_U3P_GCTL_USB_POWER_EN;
    }
}

/* This function is called to connect / disconnect the USB device from the Host. */
CyU3PReturnStatus_t
CyU3PConnectState (
        CyBool_t connect,
        CyBool_t ssEnable)
{
    uint16_t ret;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* If USB 3.0 is not supported by the device, drop down to USB 2.0 implicitly. */
    if (!CyFx3DevIsUsb3Supported ())
        ssEnable = CyFalse;

    ret = CY_U3P_SUCCESS;
    CyU3PMutexGet (&glUibLock, CYU3P_WAIT_FOREVER);

    if (!connect)
    {
        if (glUibDeviceInfo.usbState >= CY_U3P_USB_VBUS_WAIT)
        {
            if (glUibDeviceInfo.usbState >= CY_U3P_USB_CONNECTED)
            {
                CyU3PUsbPhyDisable (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED);
            }

            glUibDeviceInfo.usbState       = CY_U3P_USB_CONFIGURED;
            glUibDeviceInfo.isConnected    = CyFalse;
            glUibDeviceInfo.tDisabledCount = 0;
        }

        glUibDeviceInfo.isLpmDisabled = CyFalse;
    }
    else
    {
        switch (glUibDeviceInfo.usbState)
        {
        case CY_U3P_USB_VBUS_WAIT:
        case CY_U3P_USB_CONNECTED:
        case CY_U3P_USB_ESTABLISHED:
            ret = CY_U3P_ERROR_ALREADY_STARTED; 
            break;

        case CY_U3P_USB_CONFIGURED:
            {
                /* If Fast enumeration is in use, all config descriptors need to be available. */
                if ((glUibDeviceInfo.enumMethod != CY_U3P_USBENUM_PPORT) && (
                            (glUibDescrPtrs.usbSSConfigDesc_p == NULL) ||
                            (glUibDescrPtrs.usbHSConfigDesc_p == NULL) ||
                            (glUibDescrPtrs.usbFSConfigDesc_p == NULL)
                            ))
                {
                    CyU3PMutexPut (&glUibLock);
                    return CY_U3P_ERROR_NOT_CONFIGURED;
                }

                if (ssEnable)
                {
                    USB3LNK->lnk_error_count = 0;
                    glUsbLinkErrorCount      = 0;
                    glUsbUserLinkErrorCount  = 0;
                }
                else
                {
                    if (glUibDeviceInfo.usb2Disable)
                    {
                        CyU3PMutexPut (&glUibLock);
                        return CY_U3P_ERROR_OPERN_DISABLED;
                    }
                }

                /* Store the desired USB connection speed for later use. */
                glUibDeviceInfo.enableSS       = ssEnable;
                glUibDeviceInfo.tDisabledCount = 0;

                /* If Vbus is already on or if we are working off Vbat, the PHY can be turned on here. */
                if (CyU3PUsbCanConnect ())
                {
                    /* Variables for handling USB 3.0 retry attempts. */
                    if (ssEnable)
                    {
                        glUibDeviceInfo.ssHostResume = CyFalse;
                        glUibDeviceInfo.enableUsb3   = CyTrue;
                        glUibDeviceInfo.ssCmdSeen    = CyFalse;
                    }
                    else
                        glUibDeviceInfo.enableUsb3   = CyFalse;

                    CyU3PUsbPhyEnable (ssEnable);
                    glUibDeviceInfo.usbState = CY_U3P_USB_CONNECTED;
                }
                else
                {
                    glUibDeviceInfo.usbState = CY_U3P_USB_VBUS_WAIT;
                }
            }
            break;

        default:
            ret = CY_U3P_ERROR_NOT_CONFIGURED;
            break;
        }
    }

    CyU3PMutexPut (&glUibLock);
    return ret;
}

/* Set descriptor for FX3 functionality. */
CyU3PReturnStatus_t
CyU3PUsbSetDesc (
        CyU3PUSBSetDescType_t desc_type,
        uint8_t desc_index,
        uint8_t * desc)
{
    CyU3PReturnStatus_t ret=0;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if (desc == 0)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    switch (desc_type)
    {
        case CY_U3P_USB_SET_SS_DEVICE_DESCR:
            glUibDescrPtrs.usbSSDevDesc_p = (uint8_t *)desc;
            break;
        case CY_U3P_USB_SET_HS_DEVICE_DESCR:
            glUibDescrPtrs.usbDevDesc_p = (uint8_t *)desc;
            break;

        case CY_U3P_USB_SET_SS_BOS_DESCR:
            glUibDescrPtrs.usbSSBOSDesc_p = (uint8_t *)desc;
            break;

        case CY_U3P_USB_SET_DEVQUAL_DESCR:
            glUibDescrPtrs.usbDevQualDesc_p = (uint8_t *)desc;
            break;

        case CY_U3P_USB_SET_SS_CONFIG_DESCR:
            glUibDescrPtrs.usbSSConfigDesc_p = (uint8_t *)desc;
            if ((desc[7] & 0x40) != 0)
                glUibDeviceInfo.usbDeviceStat = CY_U3P_USB_DEVSTAT_SELFPOWER;
            else
                glUibDeviceInfo.usbDeviceStat = 0;
            break;

        case CY_U3P_USB_SET_FS_CONFIG_DESCR:
            glUibDescrPtrs.usbFSConfigDesc_p  = (uint8_t *)desc;
            if ((desc[7] & 0x40) != 0)
                glUibDeviceInfo.usbDeviceStat = CY_U3P_USB_DEVSTAT_SELFPOWER;
            else
                glUibDeviceInfo.usbDeviceStat = 0;
            break;

        case CY_U3P_USB_SET_HS_CONFIG_DESCR:
            glUibDescrPtrs.usbHSConfigDesc_p  = (uint8_t *)desc;
            if ((desc[7] & 0x40) != 0)
                glUibDeviceInfo.usbDeviceStat = CY_U3P_USB_DEVSTAT_SELFPOWER;
            else
                glUibDeviceInfo.usbDeviceStat = 0;
            break;

        case CY_U3P_USB_SET_STRING_DESCR:
            if (desc_index > CY_U3P_MAX_STRING_DESC_INDEX)
                ret = CY_U3P_ERROR_BAD_INDEX;
            glUibDescrPtrs.usbStringDesc_p[desc_index] = (uint8_t *)desc;
            break;

        case CY_U3P_USB_SET_OTG_DESCR:
            glUibDescrPtrs.usbOtgDesc_p  = (uint8_t *)desc;
            break;

        default:
            return (CY_U3P_ERROR_BAD_DESCRIPTOR_TYPE);
    }
    if ((glUibDescrPtrs.usbDevDesc_p != NULL) && (glUibDescrPtrs.usbStringDesc_p[0] != NULL))
    {
        if (glUibDeviceInfo.usbState < CY_U3P_USB_CONFIGURED)
            glUibDeviceInfo.usbState = CY_U3P_USB_CONFIGURED;
    }
    return ret;

}

/* disconnect from usb,disabling channel for EP 0 */
CyU3PReturnStatus_t
CyU3PUsbStop ()
{
    CyU3PReturnStatus_t ret = 0;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    glUibDeviceInfo.usbState = CY_U3P_USB_INACTIVE;
    CyU3PVicDisableInt (CY_U3P_VIC_UIB_DMA_VECTOR);

    if (CyU3POtgGetMode () == CY_U3P_OTG_MODE_DEVICE_ONLY)
    {
        CyU3PVicDisableInt (CY_U3P_VIC_UIB_CORE_VECTOR);

        /* Disable VBus detection at this stage. */
        GCTL->iopwr_intr = 0xFFFFFFFF;
        GCTL->iopwr_intr_mask = 0;
        glUibDeviceInfo.vbusDetectMode = 0;
        CyU3PVicDisableInt (CY_U3P_VIC_GCTL_PWR_VECTOR);
    }

    UIB->intr_mask &= ~(CY_U3P_UIB_DEV_CTL_INT | CY_U3P_UIB_DEV_EP_INT | 
	CY_U3P_UIB_LNK_INT | CY_U3P_UIB_PROT_INT | CY_U3P_UIB_PROT_EP_INT | CY_U3P_UIB_EPM_URUN);

    /* Make sure the USB connection is disconnected. */
    if (glUibDeviceInfo.usbSpeed != CY_U3P_SUPER_SPEED)
    {
        CyU3PDisconUsbPins ();
    }

    /* Destroy the EP-0 DMA channels. */
    ret = CyU3PDmaChannelDestroy (&glUibChHandle);
    if (ret == CY_U3P_SUCCESS)
    {
        ret = CyU3PDmaChannelDestroy (&glUibChHandleOut);
        if (ret != CY_U3P_SUCCESS)
        {
            return CY_U3P_ERROR_CHANNEL_DESTROY_FAILED;
        }
    }
    else
    {
        return CY_U3P_ERROR_CHANNEL_DESTROY_FAILED;
    }

    /* Clear all bits except for OTG_ENABLE. */
    UIB->otg_ctrl &= CY_U3P_UIB_OTG_ENABLE;

    /* Disable UIB block only if in device only mode. */
    if (CyU3POtgGetMode () == CY_U3P_OTG_MODE_DEVICE_ONLY)
    {
        GCTL->uib_core_clk &= ~(CY_U3P_GCTL_UIBCLK_CLK_EN);
        UIB->power &= ~CY_U3P_UIB_RESETN;
        CyU3PBusyWait (10);
    }

    return ret;
}

CyU3PReturnStatus_t
CyU3PUsbSendErdy (
        uint8_t  ep,
        uint16_t bulkStream)
{
    uint32_t usb_tp[3] = {0};
    uint8_t  epNum = (ep & 0x0F);
    uint8_t  epDir = (ep & 0x80);

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if ((CyU3PUsbGetSpeed () != CY_U3P_SUPER_SPEED) ||
            ((USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK) != CY_U3P_UIB_LNK_STATE_U0))
    {
        return CY_U3P_ERROR_OPERN_DISABLED;
    }

    if  ((ep & 0x7F) > 15)
    {
        return (CY_U3P_ERROR_BAD_ARGUMENT);
    }

    /* Compute the values for the ERDY TP. */
    usb_tp[0] = ((USB3PROT->prot_cs & CY_U3P_UIB_SS_DEVICEADDR_MASK) << CY_U3P_USB3_TP_DEVADDR_POS) |
        CY_U3P_USB3_PACK_TYPE_TP;
    usb_tp[1] = 0x00010000 | CY_U3P_USB3_TP_SUBTYPE_ERDY | epDir | (epNum << CY_U3P_USB3_TP_EPNUM_POS);

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
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbSendNrdy (
        uint8_t  ep,
        uint16_t bulkStream)
{
    uint32_t usb_tp[3] = {0};
    uint8_t  epNum = (ep & 0x0F);
    uint8_t  epDir = (ep & 0x80);

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if ((CyU3PUsbGetSpeed () != CY_U3P_SUPER_SPEED) ||
            ((USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK) != CY_U3P_UIB_LNK_STATE_U0))
    {
        return CY_U3P_ERROR_OPERN_DISABLED;
    }

    if  ((ep & 0x7F) > 15)
    {
        return (CY_U3P_ERROR_BAD_ARGUMENT);
    }

    /* Compute the values for the NRDY TP. */
    usb_tp[0] = ((USB3PROT->prot_cs & CY_U3P_UIB_SS_DEVICEADDR_MASK) << CY_U3P_USB3_TP_DEVADDR_POS) |
        CY_U3P_USB3_PACK_TYPE_TP;
    usb_tp[1] = CY_U3P_USB3_TP_SUBTYPE_NRDY | epDir | (epNum << CY_U3P_USB3_TP_EPNUM_POS);

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

    /* Send the NRDY TP to the host. */
    CyFx3Usb3SendTP (usb_tp);
    return CY_U3P_SUCCESS;
}


/* set / clear the stall on the EP */
CyU3PReturnStatus_t
CyU3PUsbStall (uint8_t ep, CyBool_t stall, CyBool_t toggle)
{
    uint8_t tmp = (ep & 0x0F);
    uint32_t seqClear = 0;
    uint32_t x, mask;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if  ((ep & 0x7F) > 15)
    {
        return (CY_U3P_ERROR_BAD_ARGUMENT);
    }

    if ((tmp == 0) && (!stall))
    {
        return (CY_U3P_ERROR_BAD_ARGUMENT);
    }

    if (glUibDeviceInfo.usbSpeed == CY_U3P_NOT_CONNECTED)
        return CY_U3P_SUCCESS;

    CyU3PMutexGet (&glUibLock, CYU3P_WAIT_FOREVER);

    if (tmp == 0)
    {
        /* Don't do anything if the next control request has already been received. */
        if (CyU3PUsbIsNewCtrlRqtRecvd ())
        {
            CyU3PMutexPut (&glUibLock);
            return CY_U3P_SUCCESS;
        }

        if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
        {
            USB3PROT->prot_epi_cs1[0] |= CY_U3P_UIB_SSEPI_STALL;
            USB3PROT->prot_epo_cs1[0] |= CY_U3P_UIB_SSEPO_STALL;

            mask = CyU3PVicDisableAllInterrupts ();
            CyU3PBusyWait (1);
            USB3PROT->prot_cs  = (USB3PROT->prot_cs | CY_U3P_UIB_SS_SETUP_CLR_BUSY);
            CyU3PVicEnableInterrupts (mask);

            glUibStatusSendErdy   = CyFalse;
            glUibEp0StatusPending = CyFalse;
            CyU3PTimerStop (&glUibStatusTimer);
        }
        else
        {
            UIB->dev_epi_cs[0] |= CY_U3P_UIB_EPI_STALL;
            UIB->dev_epo_cs[0] |= CY_U3P_UIB_EPO_STALL;
            UIB->dev_cs |= CY_U3P_UIB_SETUP_CLR_BUSY;
        }

        CyU3PMutexPut (&glUibLock);
        return CY_U3P_SUCCESS;
    }

    if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
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

                /* Send an ERDY so that the host initiates a transfer and sees the stall TP. */
                    CyU3PUsbSendErdy (ep, (uint16_t)USB3PROT->prot_epi_mapped_stream[tmp]);
            }
            else
            {
                /* Clear the stall bit and reset the endpoint. */
                x = USB3PROT->prot_epi_cs1[tmp];
                x |= (CY_U3P_UIB_SSEPI_EP_RESET | CY_U3P_UIB_SSEPI_VALID);
                USB3PROT->prot_epi_cs1[tmp] = x;
                CyU3PBusyWait (1);
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

                /* Send an ERDY so that the host initiates a transfer and sees the stall TP. */
                    CyU3PUsbSendErdy (ep, (uint16_t)USB3PROT->prot_epo_mapped_stream[tmp]);
            }
            else
            {
                /* Clear the stall bit and reset the endpoint. */
                x = USB3PROT->prot_epo_cs1[tmp] | CY_U3P_UIB_SSEPO_EP_RESET | CY_U3P_UIB_SSEPO_VALID;
                USB3PROT->prot_epo_cs1[tmp] = x;
                CyU3PBusyWait (1);
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
        if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
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

    CyU3PMutexPut (&glUibLock);
    return CY_U3P_SUCCESS;
}

void
CyU3PUsbEnableEpInterrupts (
        uint8_t  ep,
        uint32_t eventMask)
{
    uint8_t  epIndex = ep & 0x0F;
    uint32_t epCs;

    if (ep & 0x80)
    {
        epCs = UIB->dev_epi_cs[epIndex];
        if (epCs & CY_U3P_UIB_EPI_VALID)
        {
            if (eventMask & CYU3P_USBEP_NAK_EVT)
                epCs |= CY_U3P_UIB_EPI_BNAK_MASK;
            if (eventMask & CYU3P_USBEP_ZLP_EVT)
                epCs |= CY_U3P_UIB_EPI_ZERO_MASK;
            if (eventMask & CYU3P_USBEP_SLP_EVT)
                epCs |= CY_U3P_UIB_EPI_SHORT_MASK;
            if (eventMask & CYU3P_USBEP_ISOERR_EVT)
                epCs |= CY_U3P_UIB_EPI_ISOERR_MASK;

            UIB->dev_epi_cs[epIndex] = epCs;
        }

        epCs = USB3PROT->prot_epi_cs1[epIndex];
        if (epCs & CY_U3P_UIB_SSEPI_VALID)
        {
            if (eventMask & CYU3P_USBEP_NAK_EVT)
                epCs |= CY_U3P_UIB_SSEPI_FLOWCONTROL_MASK;
            if (eventMask & CYU3P_USBEP_ZLP_EVT)
                epCs |= CY_U3P_UIB_SSEPI_ZERO_MASK;
            if (eventMask & CYU3P_USBEP_SLP_EVT)
                epCs |= CY_U3P_UIB_SSEPI_SHORT_MASK;
            if (eventMask & CYU3P_USBEP_SS_RETRY_EVT)
                epCs |= CY_U3P_UIB_SSEPI_RETRY_MASK;
            if (eventMask & CYU3P_USBEP_SS_SEQERR_EVT)
                epCs |= CY_U3P_UIB_SSEPI_OOSERR_MASK;
            if (eventMask & CYU3P_USBEP_SS_STREAMERR_EVT)
                epCs |= CY_U3P_UIB_SSEPI_STREAM_ERROR_MASK;

            USB3PROT->prot_epi_cs1[epIndex] = epCs;
        }
    }
    else
    {
        epCs = UIB->dev_epo_cs[epIndex];
        if (epCs & CY_U3P_UIB_EPO_VALID)
        {
            if (eventMask & CYU3P_USBEP_NAK_EVT)
                epCs |= CY_U3P_UIB_EPO_BNAK_MASK;
            if (eventMask & CYU3P_USBEP_ZLP_EVT)
                epCs |= CY_U3P_UIB_EPO_ZERO_MASK;
            if (eventMask & CYU3P_USBEP_SLP_EVT)
                epCs |= CY_U3P_UIB_EPO_SHORT_MASK;
            if (eventMask & CYU3P_USBEP_ISOERR_EVT)
                epCs |= CY_U3P_UIB_EPO_ISOERR_MASK;

            UIB->dev_epo_cs[epIndex] = epCs;
        }

        epCs = USB3PROT->prot_epo_cs1[epIndex];
        if (epCs & CY_U3P_UIB_SSEPO_VALID)
        {
            if (eventMask & CYU3P_USBEP_NAK_EVT)
                epCs |= CY_U3P_UIB_SSEPO_FLOWCONTROL_MASK;
            if (eventMask & CYU3P_USBEP_ZLP_EVT)
                epCs |= CY_U3P_UIB_SSEPO_ZERO_MASK;
            if (eventMask & CYU3P_USBEP_SLP_EVT)
                epCs |= CY_U3P_UIB_SSEPO_SHORT_MASK;
            if (eventMask & CYU3P_USBEP_SS_RETRY_EVT)
                epCs |= CY_U3P_UIB_SSEPO_RETRY_MASK;
            if (eventMask & CYU3P_USBEP_SS_SEQERR_EVT)
                epCs |= CY_U3P_UIB_SSEPO_OOSERR_MASK;
            if (eventMask & CYU3P_USBEP_SS_STREAMERR_EVT)
                epCs |= CY_U3P_UIB_SSEPO_STREAM_ERROR_MASK;

            USB3PROT->prot_epo_cs1[epIndex] = epCs;
        }
    }
}

/* Function to map socket to a stream. */
CyU3PReturnStatus_t
CyU3PUsbMapStream (uint8_t ep, uint8_t socketNum, uint16_t streamId)
{
    uint8_t epnum = (ep & 0x7F);

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if  (epnum > 15)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (ep & 0x80)
    {
        USB3PROT->prot_epi_mapped_stream[socketNum] = (CY_U3P_UIB_ENABLE |
                (epnum << CY_U3P_UIB_EP_NUMBER_POS) | streamId);
    }
    else
    {
        USB3PROT->prot_epo_mapped_stream[socketNum] = (CY_U3P_UIB_ENABLE |
                (epnum << CY_U3P_UIB_EP_NUMBER_POS) | streamId);
    }

    return CY_U3P_SUCCESS;
}

/* Sets up the end point configuration for transferring data */
CyU3PReturnStatus_t
CyU3PSetEpConfig (uint8_t ep, CyU3PEpConfig_t *epinfo)
{
    uint8_t epnum = ep & 0x7F;
    uint16_t pcktSize = 0;
    uint32_t epCs = 0;
    uint32_t protEpCs1 = 0;
    uint32_t protEpCs2 = 0;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (epinfo == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    /* Check for other parameter validity only if this is EP enable. */
    if (epinfo->enable)
    {
        if ((epinfo->pcktSize > 0x400) || (epnum > 15))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
        if ((epinfo->epType == CY_U3P_USB_EP_CONTROL) || (epinfo->epType > CY_U3P_USB_EP_INTR))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
        if ((epinfo->burstLen > 16) || ((epinfo->burstLen > 1) && (epinfo->pcktSize < 1024) &&
                    (epinfo->pcktSize != 0)))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }

        /* Update the endpoint type. EPI and EPO masks are same. */
        epCs = ((uint32_t)(epinfo->epType & 0x03) << CY_U3P_UIB_EPI_TYPE_POS);
        protEpCs2 = CyU3PSSEpTypeMap[epinfo->epType & 0x03];

        /* Setting the burstlen field for SS. EPI and EPO masks are same. */
        if (epinfo->burstLen)
        {
            protEpCs2 |= ((uint32_t)((epinfo->burstLen - 1) & 0x0F) << CY_U3P_UIB_SSEPI_MAXBURST_POS);
        }
        else
            epinfo->burstLen = 1;

        /* Setting the ISO pkts field. EPI and EPO masks are same. */
        if ((epinfo->epType == CY_U3P_USB_EP_ISO) && (epinfo->enable))
        {
            epCs |= ((uint32_t)(epinfo->isoPkts & 0x03) << CY_U3P_UIB_EPI_ISOINPKS_POS);
            protEpCs2 |= (((epinfo->burstLen * epinfo->isoPkts) & 0x3F) << CY_U3P_UIB_SSEPI_ISOINPKS_POS);

            if ((epinfo->isoPkts > 1) && (epnum != 3) && (epnum != 7))
            {
                return CY_U3P_ERROR_INVALID_CONFIGURATION;
            }
        }

        /* If the USB connection is already active, set the payload according to the connection speed. */
        pcktSize = ((epinfo->pcktSize != 0) && (epinfo->pcktSize <= 0x400)) ? epinfo->pcktSize : 0x400;
        if (glUibDeviceInfo.usbState > CY_U3P_USB_CONNECTED)
        {
            switch (glUibDeviceInfo.usbSpeed)
            {
                case CY_U3P_HIGH_SPEED:
                    if ((epinfo->epType == CY_U3P_USB_EP_BULK) && (pcktSize > 0x200))
                    {
                        pcktSize = 0x200;
                    }
                    break;

                case CY_U3P_FULL_SPEED:
                    if ((epinfo->epType == CY_U3P_USB_EP_ISO) && (pcktSize > 1023))
                    {
                        pcktSize = 1023;
                    }
                    else
                    {
                        if ((epinfo->epType != CY_U3P_USB_EP_ISO) && (pcktSize > 0x40))
                        {
                            pcktSize = 0x40;
                        }
                    }
                    break;

                default:
                    /* Super speed. All settings are already correct. */
                    break;
            }
        }

        /* Update the HS packet size information. Both EPI and EPO have the same mask. */
        epCs |= (pcktSize & CY_U3P_UIB_EPI_PAYLOAD_MASK);
    }

    CyU3PMutexGet (&glUibLock, CYU3P_WAIT_FOREVER);

    if (ep & 0x80) /* USB IN EP */
    {
        if (!epinfo->enable)
        {
            UIB->dev_epi_cs[epnum]        = CY_U3P_UIB_DEV_EPI_CS_DEFAULT;
            USB3PROT->prot_epi_cs1[epnum] = CY_U3P_UIB_PROT_EPI_CS1_DEFAULT;

            USB3PROT->prot_ep_intr_mask &= ~(1 << epnum);
            UIB->dev_ep_intr_mask       &= ~(1 << epnum);

            /* Populate data structure */
            glPcktSizeIn[epnum].valid = CyFalse;
            glPcktSizeIn[epnum].mapped = CyFalse;

            CyU3PMutexPut (&glUibLock);
            return CY_U3P_SUCCESS;
        }
 
        /* Populate data structure. */
        glPcktSizeIn[epnum].valid  = CyTrue;
        glPcktSizeIn[epnum].mapped = CyTrue;
        glPcktSizeIn[epnum].size   = epinfo->pcktSize;

        /* Update the EEPM to maximum packet size. */
        UIB->eepm_endpoint[epnum] = pcktSize;

        /* Map to the default stream in case of bulk streaming EP. */
        if ((epinfo->epType == CY_U3P_USB_EP_BULK) && (epinfo->streams > 0))
        {
            /* The ep's are mapped to the sockets with same no.  */
            CyU3PUsbMapStream (ep, epnum, 1);
            protEpCs1 = CY_U3P_UIB_SSEPI_STREAM_ERROR_STALL_EN |
                CY_U3P_UIB_SSEPI_STREAM_EN | CY_U3P_UIB_SSEPI_STREAMNRDY_MASK;
        }

        USB3PROT->prot_ep_intr_mask |= (1 << epnum);
        protEpCs1 |= CY_U3P_UIB_SSEPI_RETRY_MASK | CY_U3P_UIB_SSEPI_OOSERR_MASK | CY_U3P_UIB_SSEPI_DBTERM_MASK;

        UIB->dev_epi_cs[epnum]        = (epCs | CY_U3P_UIB_EPI_VALID);
        USB3PROT->prot_epi_cs1[epnum] = (protEpCs1 | CY_U3P_UIB_SSEPI_VALID);
        USB3PROT->prot_epi_cs2[epnum] = protEpCs2;

        /* Enable desired EP interrupts. */
        if ((glUsbEpEvtMask != 0) && (glUsbEvtEnabledEps & (1 << epnum)))
        {
            UIB->dev_ep_intr_mask       |= (1 << epnum);
            USB3PROT->prot_ep_intr_mask |= (1 << epnum);
            CyU3PUsbEnableEpInterrupts (ep, glUsbEpEvtMask);
        }
    }
    else /* USB OUT EP */
    {
        if (!epinfo->enable)
        {
            UIB->dev_epo_cs[epnum]        = CY_U3P_UIB_DEV_EPO_CS_DEFAULT;
            USB3PROT->prot_epo_cs1[epnum] = CY_U3P_UIB_PROT_EPO_CS1_DEFAULT;

            UIB->dev_ep_intr_mask       &= ~(1 << (16 + epnum));
            USB3PROT->prot_ep_intr_mask &= ~(1 << (16 + epnum));

            /* Populate data structure */
            glPcktSizeOut[epnum].valid = CyFalse;
            glPcktSizeOut[epnum].mapped = CyFalse;

            CyU3PMutexPut (&glUibLock);
            return CY_U3P_SUCCESS;
        }

        /* Populate data structure. */
        glPcktSizeOut[epnum].valid  = CyTrue;
        glPcktSizeOut[epnum].mapped = CyTrue;
        glPcktSizeOut[epnum].size   = epinfo->pcktSize;

        /* Update the IEPM to maximum packet size. */
        UIB->iepm_endpoint[epnum] = (UIB->iepm_endpoint[epnum] & CY_U3P_UIB_EOT_EOP) | pcktSize;

        /* Map to the default stream in case of bulk streaming EP. */
        if ((epinfo->epType == CY_U3P_USB_EP_BULK) && (epinfo->streams > 0))
        {
            /* The ep's are mapped to the sockets with same no. */
            CyU3PUsbMapStream(ep, epnum, 1);
            USB3PROT->prot_ep_intr_mask  |= (1 << (16 + epnum));
            protEpCs1 = CY_U3P_UIB_SSEPO_STREAM_ERROR_STALL_EN |
                CY_U3P_UIB_SSEPO_STREAM_EN | CY_U3P_UIB_SSEPO_STREAMNRDY_MASK;
        }

        UIB->dev_epo_cs[epnum]        = (epCs | CY_U3P_UIB_EPO_VALID);
        USB3PROT->prot_epo_cs1[epnum] = (protEpCs1 | CY_U3P_UIB_SSEPO_VALID);
        USB3PROT->prot_epo_cs2[epnum] = protEpCs2;

        /* Enable desired EP interrupts. */
        if ((glUsbEpEvtMask != 0) && ((glUsbEvtEnabledEps & (1 << (16 + epnum)))))
        {
            UIB->dev_ep_intr_mask       |= (1 << (16 + epnum));
            USB3PROT->prot_ep_intr_mask |= (1 << (16 + epnum));
            CyU3PUsbEnableEpInterrupts (ep, glUsbEpEvtMask);
        }
    }

    /* If the USB connection is already active; reset the EP, flush the EPM and clear the sequence number. */
    if (glUibDeviceInfo.usbState >= CY_U3P_USB_CONNECTED)
    {
        CyU3PUsbResetEp (ep);
        CyU3PUsbFlushEp (ep);
        CyU3PUsbStall (ep, CyFalse, CyTrue);
    }

    CyU3PMutexPut (&glUibLock);
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbEPSetBurstMode (
        uint8_t  ep,
        CyBool_t burstEnable)
{
    uint8_t idx = ep & 0x7F;

    if ((idx != 0) && (idx < 16))
    {
        CyU3PDmaSetUsbSocketMult (ep, burstEnable);
        return CY_U3P_SUCCESS;
    }
    return CY_U3P_ERROR_BAD_ARGUMENT;
}

CyU3PReturnStatus_t
CyU3PUsbFlushEp (uint8_t ep)
{
    uint8_t epnum = ep & 0x7F;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if  (epnum > 15)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    CyU3PMutexGet (&glUibLock, CYU3P_WAIT_FOREVER);

    if (ep & 0x80)
    {
        UIB->eepm_endpoint[epnum] |= CY_U3P_UIB_SOCKET_FLUSH;
        CyU3PBusyWait (10);
        UIB->eepm_endpoint[epnum] &= ~CY_U3P_UIB_SOCKET_FLUSH;
    }
    else
    {
        UIB->iepm_endpoint[epnum] |= CY_U3P_UIB_SOCKET_FLUSH;
        CyU3PBusyWait (10);
        UIB->iepm_endpoint[epnum] &= ~CY_U3P_UIB_SOCKET_FLUSH;
    }

    CyU3PMutexPut (&glUibLock);

    return CY_U3P_SUCCESS;
}

/* Function to reset an endpoint. This clears sticky bits e.g retry bit, flowcontrol bit. */
CyU3PReturnStatus_t
CyU3PUsbResetEp (uint8_t ep)
{
    uint8_t epnum = (ep & 0x7F);
    uint32_t x;
	
    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if  (epnum > 15)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
	
    CyU3PMutexGet (&glUibLock, CYU3P_WAIT_FOREVER);

    /* Clear if there is any stream error */
    if (CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED)
    {
        USB3PROT->prot_stream_error_status |= CY_U3P_UIB_ERROR_DETECTED;

        if (ep & 0x80)
        {
            x = USB3PROT->prot_epi_cs1[epnum];
            x |= (CY_U3P_UIB_SSEPI_EP_RESET | CY_U3P_UIB_SSEPI_VALID);
            USB3PROT->prot_epi_cs1[epnum] = x;
            CyU3PBusyWait (2);
            x = ((x & ~CY_U3P_UIB_SSEPI_EP_RESET) | CY_U3P_UIB_SSEPI_VALID);
            USB3PROT->prot_epi_cs1[epnum] = x;
        }
        else
        {
            x = USB3PROT->prot_epo_cs1[epnum] | CY_U3P_UIB_SSEPO_EP_RESET | CY_U3P_UIB_SSEPO_VALID;
            USB3PROT->prot_epo_cs1[epnum] = x;
            CyU3PBusyWait (2);
            x = (x & ~CY_U3P_UIB_SSEPO_EP_RESET) | CY_U3P_UIB_SSEPO_VALID;
            USB3PROT->prot_epo_cs1[epnum] = x;
        }
    }

    CyU3PMutexPut (&glUibLock);
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbResetEndpointMemories (
        void)
{
    CyU3PUSBSpeed_t curSpeed;
    uint32_t mask;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        return CY_U3P_ERROR_NOT_STARTED;

    /* Wait until there are no pending EP0 transfers. */
    while ((glUibEp0StatusPending) || (glUibDeviceInfo.newCtrlRqt))
        CyU3PThreadRelinquish ();

    CyU3PMutexGet (&glUibLock, CYU3P_WAIT_FOREVER);

    /* Set the NAK-ALL bit. */
    curSpeed = CyU3PUsbGetSpeed ();
    if (curSpeed == CY_U3P_SUPER_SPEED)
    {
        USB3PROT->prot_cs = (USB3PROT->prot_cs & ~CY_U3P_UIB_SS_SETUP_CLR_BUSY) | CY_U3P_UIB_SS_NRDY_ALL;
    }
    else
        UIB->dev_cs      |= CY_U3P_UIB_NAKALL;
    CyU3PBusyWait (50);

    /* Disable all interrupts. */
    mask = CyU3PVicDisableAllInterrupts ();

    UIB->iepm_cs |= (CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
    CyU3PBusyWait (1);
    UIB->iepm_cs &= ~(CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
    CyU3PBusyWait (1);

    if (curSpeed == CY_U3P_SUPER_SPEED)
    {
        USB3PROT->prot_epi_cs1[0] |= CY_U3P_UIB_SSEPI_VALID;
        UIB->eepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */
        USB3PROT->prot_epo_cs1[0] |= CY_U3P_UIB_SSEPO_VALID;
        UIB->iepm_endpoint[0] = 0x200;                      /* Control EP transfer size is 512 bytes. */
    }
    else
    {
        UIB->eepm_endpoint[0] = 0x40;
        UIB->iepm_endpoint[0] = 0x40;
        UIB->dev_epi_cs[0] |= CY_U3P_UIB_EPI_VALID;
        UIB->dev_epo_cs[0] |= CY_U3P_UIB_EPO_VALID;
    }

    if (curSpeed != CY_U3P_NOT_CONNECTED)
        CyU3PUsbEpPrepare (CyU3PUsbGetSpeed ());

    /* Re-enable interrupts. */
    CyU3PVicEnableInterrupts (mask);

    /* Clear the NAK-ALL bit. */
    if (curSpeed == CY_U3P_SUPER_SPEED)
    {
        USB3PROT->prot_cs = (USB3PROT->prot_cs & ~(CY_U3P_UIB_SS_NRDY_ALL | CY_U3P_UIB_SS_SETUP_CLR_BUSY));
    }
    else
        UIB->dev_cs &= ~CY_U3P_UIB_NAKALL;

    CyU3PMutexPut (&glUibLock);
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbGetEpSeqNum (
        uint8_t  ep,
        uint8_t *seqnum_p)
{
    uint8_t  idx = ep & 0x7F;
    uint32_t arg = (uint32_t)idx;

    if (((idx == 0) || (idx > 15)) || (seqnum_p == 0))
        return CY_U3P_ERROR_BAD_ARGUMENT;
    if (CyU3PUsbGetSpeed () != CY_U3P_SUPER_SPEED)
        return CY_U3P_ERROR_INVALID_SEQUENCE;

    if (ep & 0x80)
        arg |= CY_U3P_UIB_DIR;

    USB3PROT->prot_seq_num = arg;
    while ((USB3PROT->prot_seq_num & CY_U3P_UIB_SEQ_VALID) == 0);

    *seqnum_p = ((USB3PROT->prot_seq_num & CY_U3P_UIB_SEQUENCE_NUMBER_MASK) >> CY_U3P_UIB_SEQUENCE_NUMBER_POS);
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbSetEpSeqNum (
        uint8_t ep,
        uint8_t seqnum)
{
    uint8_t  idx = ep & 0x7F;
    uint32_t arg = (uint32_t)idx;

    if (((idx == 0) || (idx > 15)) || (seqnum >= 32))
        return CY_U3P_ERROR_BAD_ARGUMENT;
    if (CyU3PUsbGetSpeed () != CY_U3P_SUPER_SPEED)
        return CY_U3P_ERROR_INVALID_SEQUENCE;

    if (ep & 0x80)
        arg |= CY_U3P_UIB_DIR;

    USB3PROT->prot_seq_num = arg | (seqnum << CY_U3P_UIB_SEQUENCE_NUMBER_POS) | CY_U3P_UIB_COMMAND;
    while ((USB3PROT->prot_seq_num & CY_U3P_UIB_SEQ_VALID) == 0);

    return CY_U3P_SUCCESS;
}

CyU3PUSBSpeed_t
CyU3PUsbGetSpeed (
        void)
{
    return ((CyU3PUSBSpeed_t)glUibDeviceInfo.usbSpeed);
}

CyU3PReturnStatus_t
CyU3PUsbGetLinkPowerState (
        CyU3PUsbLinkPowerMode *mode_p)
{
    uint8_t state;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        return CY_U3P_ERROR_NOT_STARTED;

    if (glUibDeviceInfo.usbState < CY_U3P_USB_CONNECTED)
        return CY_U3P_ERROR_NOT_CONFIGURED;

    if (CyU3PUsbGetSpeed () != CY_U3P_SUPER_SPEED)
        return CY_U3P_ERROR_OPERN_DISABLED;

    if (mode_p == 0)
        return CY_U3P_ERROR_BAD_ARGUMENT;

    if (glUibDeviceInfo.ssCompliance)
    {
        *mode_p = CyU3PUsbLPM_COMP;
        return CY_U3P_SUCCESS;
    }

    state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
    switch (state)
    {
    case CY_U3P_UIB_LNK_STATE_U0:
        *mode_p = CyU3PUsbLPM_U0;
        break;
    case CY_U3P_UIB_LNK_STATE_U1:
        *mode_p = CyU3PUsbLPM_U1;
        break;
    case CY_U3P_UIB_LNK_STATE_U2:
        *mode_p = CyU3PUsbLPM_U2;
        break;
    case CY_U3P_UIB_LNK_STATE_U3:
        *mode_p = CyU3PUsbLPM_U3;
        break;
    case CY_U3P_UIB_LNK_STATE_COMP:
        *mode_p = CyU3PUsbLPM_COMP;
        break;
    default:
        *mode_p = CyU3PUsbLPM_Unknown;
        break;
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbSetLinkPowerState (
        CyU3PUsbLinkPowerMode link_mode)
{
    CyU3PReturnStatus_t ret = CY_U3P_ERROR_OPERN_DISABLED;
    uint8_t state;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        return CY_U3P_ERROR_NOT_STARTED;

    if (glUibDeviceInfo.usbState < CY_U3P_USB_CONNECTED)
        return CY_U3P_ERROR_NOT_CONFIGURED;

    if (CyU3PUsbGetSpeed () != CY_U3P_SUPER_SPEED)
        return CY_U3P_ERROR_OPERN_DISABLED;

    state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;

    switch (link_mode)
    {
    case CyU3PUsbLPM_U0:
        /* Do nothing if the EXIT_LP bit is already set, or if the link state is not {U1,U2,U3}. */
        if ((USB3LNK->lnk_device_power_control & CY_U3P_UIB_EXIT_LP) == 0)
        {
            state = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
            if ((state >= CY_U3P_UIB_LNK_STATE_U1) && (state <= CY_U3P_UIB_LNK_STATE_U3))
            {
                USB3LNK->lnk_device_power_control |= CY_U3P_UIB_EXIT_LP;
            }
        }
        ret = CY_U3P_SUCCESS;
        break;

    case CyU3PUsbLPM_U1:
        if ((state == CY_U3P_UIB_LNK_STATE_U0) && ((glUibDeviceInfo.usbDeviceStat & CY_U3P_USB_DEVSTAT_U1ENABLE) != 0))
        {
            USB3LNK->lnk_device_power_control |= CY_U3P_UIB_TX_U1;
            ret = CY_U3P_SUCCESS;
        }
        break;
    case CyU3PUsbLPM_U2:
        if ((state == CY_U3P_UIB_LNK_STATE_U0) && ((glUibDeviceInfo.usbDeviceStat & CY_U3P_USB_DEVSTAT_U2ENABLE) != 0))
        {
            USB3LNK->lnk_device_power_control |= CY_U3P_UIB_TX_U2;
            ret = CY_U3P_SUCCESS;
        }
        break;
    default:
        ret = CY_U3P_ERROR_BAD_ARGUMENT;
        break;
    }

    return ret;
}

/* This function sets/clears the NAK on the EP */
CyU3PReturnStatus_t
CyU3PUsbSetEpNak (
        uint8_t  ep,
        CyBool_t nak)
{
    uint32_t x;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if  ((ep & 0x7F) > 15)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    CyU3PMutexGet (&glUibLock, CYU3P_WAIT_FOREVER);

    if (nak)
    {
        if (ep & 0x80)
        {
            if(glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
            {
                x = USB3PROT->prot_epi_cs1[ep & 0x0F];
                x |= (CY_U3P_UIB_SSEPI_NRDY | CY_U3P_UIB_SSEPI_VALID);
                USB3PROT->prot_epi_cs1[ep & 0x0F] = x;
            }
            else
            {
                UIB->dev_epi_cs[ep & 0x0F] |= CY_U3P_UIB_EPI_NAK;
            }
        }
        else
        {
            if(glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
            {
                x = USB3PROT->prot_epo_cs1[ep & 0x0F] | CY_U3P_UIB_SSEPO_NRDY | CY_U3P_UIB_SSEPO_VALID;
                USB3PROT->prot_epo_cs1[ep & 0x0F] = x;
            }
            else
            {
                UIB->dev_epo_cs[ep & 0x0F] |= CY_U3P_UIB_EPO_NAK;
            }
        }
    }
    else
    {
        if (ep & 0x80)
        {
            if(glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
            {
                x = USB3PROT->prot_epi_cs1[ep & 0x0F];
                x = ((x & ~(CY_U3P_UIB_SSEPI_NRDY)) | CY_U3P_UIB_SSEPI_VALID);
                USB3PROT->prot_epi_cs1[ep & 0x0F] = x;
            }
            else
            {
                UIB->dev_epi_cs[ep & 0x0F] &= ~CY_U3P_UIB_EPI_NAK;
            }
        }
        else
        {
            if(glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
            {
                x = (USB3PROT->prot_epo_cs1[ep & 0x0F] & ~CY_U3P_UIB_SSEPO_NRDY) | CY_U3P_UIB_SSEPO_VALID;
                USB3PROT->prot_epo_cs1[ep & 0x0F] = x;
            }
            else
            {
                UIB->dev_epo_cs[ep & 0x0F] &= ~CY_U3P_UIB_EPO_NAK;
            }
        }
    }

    CyU3PMutexPut (&glUibLock);
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbSendDevNotification (
        uint8_t  notificationType,
        uint32_t param0,
        uint32_t param1)
{
    uint32_t usb_tp[3] = {0};

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        return CY_U3P_ERROR_NOT_STARTED;
    if ((glUibDeviceInfo.usbState < CY_U3P_USB_CONNECTED) || (glUibDeviceInfo.usbSpeed != CY_U3P_SUPER_SPEED))
        return CY_U3P_ERROR_OPERN_DISABLED;
    if ((USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK) != CY_U3P_UIB_LNK_STATE_U0)
        return CY_U3P_ERROR_OPERN_DISABLED;

    usb_tp[0] = ((USB3PROT->prot_cs & CY_U3P_UIB_SS_DEVICEADDR_MASK) << CY_U3P_USB3_TP_DEVADDR_POS) |
        CY_U3P_USB3_PACK_TYPE_TP;
    usb_tp[1] = CY_U3P_USB3_TP_SUBTYPE_NOTICE | ((notificationType & 0x0F) << 4) | ((param0 & 0xFFFFFF) << 8);
    usb_tp[2] = param1;

    /* Send the DEV_NOTIFICATION TP to the host. */
    CyFx3Usb3SendTP (usb_tp);
    return CY_U3P_SUCCESS;
}

extern CyBool_t glUsbUxWake;
CyU3PReturnStatus_t
CyU3PUsbLPMEnable (
        void)
{
    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        return CY_U3P_ERROR_NOT_STARTED;
    if (glUibDeviceInfo.usbState < CY_U3P_USB_CONNECTED)
        return CY_U3P_ERROR_OPERN_DISABLED;

    glUibDeviceInfo.isLpmDisabled = CyFalse;
    UIB->ehci_portsc = CY_U3P_UIB_WKOC_E;

    if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
    {
        if ((!glUibDeviceInfo.ctrlAckDone) && (!glUsbUxWake))
            USB3LNK->lnk_device_power_control = CY_U3P_UIB_AUTO_U1 | CY_U3P_UIB_AUTO_U2;
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbForceLPMAccept (
        CyBool_t enable)
{
    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        return CY_U3P_ERROR_NOT_STARTED;
    if ((glUibDeviceInfo.usbState < CY_U3P_USB_CONNECTED) || (glUibDeviceInfo.usbSpeed != CY_U3P_SUPER_SPEED))
        return CY_U3P_ERROR_OPERN_DISABLED;

    if (enable)
    {
        glUsbForceLPMAccept               = CyTrue;
        glUibDeviceInfo.isLpmDisabled     = CyFalse;
        USB3LNK->lnk_device_power_control = CY_U3P_UIB_YES_U1 | CY_U3P_UIB_YES_U2;
    }
    else
    {
        glUsbForceLPMAccept               = CyFalse;
        USB3LNK->lnk_device_power_control = CY_U3P_UIB_AUTO_U1 | CY_U3P_UIB_AUTO_U2;
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbLPMDisable (
        void)
{
    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        return CY_U3P_ERROR_NOT_STARTED;
    if (glUibDeviceInfo.usbState < CY_U3P_USB_CONNECTED)
        return CY_U3P_ERROR_OPERN_DISABLED;

    glUibDeviceInfo.isLpmDisabled = CyTrue;
    UIB->ehci_portsc &= ~CY_U3P_UIB_WKOC_E;
    if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
    {
        USB3LNK->lnk_device_power_control = CY_U3P_UIB_NO_U1 | CY_U3P_UIB_NO_U2;
    }

    return CY_U3P_SUCCESS;
}

void
CyU3PUsbEnableEPPrefetch (
        void)
{
    glUibDeviceInfo.enablePrefetch = CyTrue;
    CyFx3UsbDmaPrefetchEnable (CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED);
}

CyU3PReturnStatus_t
CyU3PUsbSetEpPktMode (
        uint8_t  ep,
        CyBool_t pktMode)
{
    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        return CY_U3P_ERROR_NOT_STARTED;

    if (((ep & 0xF0) != 0) || (ep == 0))
        return CY_U3P_ERROR_BAD_ARGUMENT;

    if (pktMode)
        UIB->iepm_endpoint[ep] |=  CY_U3P_UIB_EOT_EOP;
    else
        UIB->iepm_endpoint[ep] &= ~CY_U3P_UIB_EOT_EOP;
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbSetTxSwing (
        uint32_t swing)
{
    if (swing >= 0x80)
        return CY_U3P_ERROR_BAD_ARGUMENT;

    glUsb3TxTrimVal = ((glUsb3TxTrimVal & ~CY_U3P_UIB_PCS_TX_SWING_FULL_MASK) |
        (swing << CY_U3P_UIB_PCS_TX_SWING_FULL_POS));
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbSetTxDeemphasis (
        uint32_t value)
{
    if (value > 0x1F)
        return CY_U3P_ERROR_BAD_ARGUMENT;

    glUsb3TxTrimVal = ((glUsb3TxTrimVal & ~CY_U3P_UIB_PCS_TX_DEEMPH_3P5DB_MASK) |
        (value << CY_U3P_UIB_PCS_TX_DEEMPH_3P5DB_POS));
    return CY_U3P_SUCCESS;
}

extern CyU3PReturnStatus_t
CyU3PUsbControlVBusDetect (
        CyBool_t enable,
        CyBool_t useVbatt)
{
    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if ((glUibDeviceInfo.usbState == CY_U3P_USB_CONNECTED) || (glUibDeviceInfo.usbState == CY_U3P_USB_VBUS_WAIT))
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    if (enable)
    {
        GCTL->iopwr_intr      = 0xFFFFFFFF;
        GCTL->iopwr_intr_mask = CY_U3P_VBUS;
        glUibDeviceInfo.vbusDetectMode = CY_U3P_VBUS;
        CyU3PVicEnableInt (CY_U3P_VIC_GCTL_PWR_VECTOR);
    }
    else
    {
        if (useVbatt)
        {
            GCTL->iopwr_intr      = 0xFFFFFFFF;
            GCTL->iopwr_intr_mask = CY_U3P_VBAT;
            glUibDeviceInfo.vbusDetectMode = CY_U3P_VBAT;
            CyU3PVicEnableInt (CY_U3P_VIC_GCTL_PWR_VECTOR);
        }
        else
        {
            GCTL->iopwr_intr_mask = 0x0;
            GCTL->iopwr_intr      = 0xFFFFFFFF;
            glUibDeviceInfo.vbusDetectMode = 0;
            CyU3PVicDisableInt (CY_U3P_VIC_GCTL_PWR_VECTOR);
        }
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbControlUsb2Support (
        CyBool_t enable)
{
    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if ((glUibDeviceInfo.usbState == CY_U3P_USB_CONNECTED) || (glUibDeviceInfo.usbState == CY_U3P_USB_VBUS_WAIT))
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    glUibDeviceInfo.usb2Disable = !enable;
    return CY_U3P_SUCCESS;
}

/* OTG Stuff: Moved here for footprint reduction. */
CyBool_t                 glIsOtgEnable    = CyFalse;
CyBool_t                 glIsHnpEnable    = CyFalse;
CyU3POtgPeripheralType_t glPeripheralType = CY_U3P_OTG_TYPE_DISABLED;
CyU3POtgConfig_t         glOtgInfo = 
{
    CY_U3P_OTG_MODE_DEVICE_ONLY,
    CY_U3P_OTG_CHARGER_DETECT_ACA_MODE,
    NULL
};

CyBool_t
CyU3POtgIsStarted (void)
{
    return glIsOtgEnable;
}

CyU3POtgPeripheralType_t
CyU3POtgGetPeripheralType (void)
{
    return glPeripheralType;
}

CyU3POtgMode_t
CyU3POtgGetMode (void)
{
    return glOtgInfo.otgMode;
}

CyBool_t
CyU3POtgIsDeviceMode (void)
{
    CyBool_t flag = CyFalse;

    /* Device mode is allowed only in DEVICE_ONLY and OTG modes. */
    if ((glOtgInfo.otgMode == CY_U3P_OTG_MODE_DEVICE_ONLY) ||
            (glOtgInfo.otgMode == CY_U3P_OTG_MODE_OTG))
    {
        switch (glPeripheralType)
        {
            case CY_U3P_OTG_TYPE_DISABLED:
                /* In case of OTG mode, detection needs to
                 * complete before enabling device mode. */
                if (glOtgInfo.otgMode == CY_U3P_OTG_MODE_OTG)
                {
                    return CyFalse;
                }
                /* Deliberate fall through. */
            case CY_U3P_OTG_TYPE_B_CABLE:
            case CY_U3P_OTG_TYPE_ACA_B_CHG:
            case CY_U3P_OTG_TYPE_ACA_C_CHG:
                flag = CyTrue;
                break;

            default:
                flag = CyFalse;
                break;
        }

        /* Invert the result if HNP is in progress. */
        if (glIsHnpEnable)
        {
            flag = (flag) ? CyFalse : CyTrue;
        }
    }

    return flag;
}

CyBool_t
CyU3POtgIsHostMode (void)
{
    CyBool_t flag = CyFalse;

    if ((glOtgInfo.otgMode == CY_U3P_OTG_MODE_HOST_ONLY) ||
            (glOtgInfo.otgMode == CY_U3P_OTG_MODE_OTG))
    {
        switch (glPeripheralType)
        {
            case CY_U3P_OTG_TYPE_DISABLED:
                /* In case of OTG mode, detection needs to
                 * complete before enabling host mode. */
                if (glOtgInfo.otgMode == CY_U3P_OTG_MODE_OTG)
                {
                    return CyFalse;
                }
                /* Deliberate fall through. */
            case CY_U3P_OTG_TYPE_A_CABLE:
            case CY_U3P_OTG_TYPE_ACA_A_CHG:
                flag = CyTrue;
                break;

            default:
                flag = CyFalse;
                break;
        }

        /* Invert the result if HNP is in progress. */
        if (glIsHnpEnable)
        {
            flag = (flag) ? CyFalse : CyTrue;
        }
    }

    return flag;
}

CyBool_t
CyU3POtgIsHnpEnabled (
        void)
{
    return glIsHnpEnable;
}

void
CyU3PUsbOtgVbusChangeHandler (
        void)
{
    CyBool_t vbus = CyFalse;

    if (CyU3PUsbCanConnect ())
    {
        vbus = CyTrue;

        /* Once the VBUS is turned on, disable the SRP detect interrupts. */
        UIB->otg_intr = (CY_U3P_UIB_SRP_DP_INT | CY_U3P_UIB_SRP_VBUS_INT);
        UIB->otg_intr_mask &= ~(CY_U3P_UIB_SRP_DP_INT | CY_U3P_UIB_SRP_VBUS_INT);
    }
    else
    {
        /* Re-enable SRP interrupts if in host mode. */
        if (CyU3POtgIsHostMode ())
        {
            UIB->otg_intr = (CY_U3P_UIB_SRP_DP_INT | CY_U3P_UIB_SRP_VBUS_INT);
            UIB->otg_intr_mask |= (CY_U3P_UIB_SRP_DP_INT | CY_U3P_UIB_SRP_VBUS_INT);
        }
    }


    if (glOtgInfo.cb)
    {
        glOtgInfo.cb (CY_U3P_OTG_VBUS_VALID_CHANGE, vbus);
    }
}

/* USB Host Stuff: Moved here for footprint reduction. */
CyBool_t glIsHostEnabled = CyFalse;

CyBool_t
CyU3PUsbHostIsStarted (
        void)
{
    return glIsHostEnabled;
}

/* [] */

