/*
## Cypress USB 3.0 Platform source file (cyu3usbutil.c)
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

/* File containing infrequently used USB utility functions.
 * Moved here to help reduce memory footprint. */

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

extern CyU3PUsbEpEvtCb_t glUsbEpCb;
extern uint32_t          glUsbEpEvtMask;
extern uint32_t          glUsbEvtEnabledEps;
extern void
CyU3PUsbEnableEpInterrupts (
        uint8_t  ep,
        uint32_t eventMask);

/* The function returns the NAK/STALL status of the specified EP */
CyU3PReturnStatus_t
CyU3PUsbGetEpCfg (uint8_t ep,CyBool_t *isNak, CyBool_t *isStall)
{
    uint32_t val;
    uint8_t tmp = ep & 0x0F;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if  ((ep & 0x7F) > 15)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (isNak)
    {
        if (ep & 0x80)
        {
            if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
            {
                val = (USB3PROT->prot_epi_cs1[tmp] & CY_U3P_UIB_SSEPI_NRDY);
            }
            else
            {
                val = (UIB->dev_epi_cs[tmp] & CY_U3P_UIB_EPI_NAK);
            }
        }
        else
        {
            if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
            {
                val = (USB3PROT->prot_epo_cs1[tmp] & CY_U3P_UIB_SSEPO_NRDY);
            }
            else
            {
                val = (UIB->dev_epo_cs[tmp] & CY_U3P_UIB_EPO_NAK);
            }
        }
        *isNak = CyFalse;
        if (val)
        {
            *isNak = CyTrue;
        }
    }
    if (isStall)
    {
        if (ep & 0x80)
        {
            if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
            {
                val = (USB3PROT->prot_epi_cs1[tmp] & CY_U3P_UIB_SSEPI_STALL);
            }
            else
            {
                val = (UIB->dev_epi_cs[tmp] & CY_U3P_UIB_EPI_STALL);
            }
        }
        else
        {
            if (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED)
            {
                val = (USB3PROT->prot_epo_cs1[tmp] & CY_U3P_UIB_SSEPO_STALL);
            }
            else
            {
                val = (UIB->dev_epo_cs[tmp] & CY_U3P_UIB_EPO_STALL);
            }
        }
        *isStall = CyFalse;
        if (val)
        {
            *isStall = CyTrue;
        }
    }
    return CY_U3P_SUCCESS;
}

/* Returns the status of the USB connection. */
CyBool_t
CyU3PGetConnectState (void)
{
    CyBool_t ret;

    if (glUibDeviceInfo.usbState >= CY_U3P_USB_CONNECTED)
    {
        ret = CyTrue;
    }
    else
    {
        ret = CyFalse;
    }
    return ret;
}

CyU3PReturnStatus_t
CyU3PUsbSetXfer (uint8_t ep, uint16_t count)
{
    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if  ((ep & 0x7F) > 15)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (ep & 0x80)
    {
        UIB->dev_epi_xfer_cnt[ep & 0x7f] = count;
    }
    else
    {
        UIB->dev_epo_xfer_cnt[ep] = count;
    }
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PSetEpPacketSize (
        uint8_t ep, uint16_t maxPktSize)
{
    uint8_t index = ep & 0x7F;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if ((index == 0) || (index > 0x0F) || (maxPktSize > 1024))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (ep & 0x80)
    {
        UIB->eepm_endpoint[index] = maxPktSize;
    }
    else
    {
        UIB->iepm_endpoint[index] = maxPktSize;
    }

    return CY_U3P_SUCCESS;
}

/* Function to un/remap stream socket combination */
CyU3PReturnStatus_t
CyU3PUsbChangeMapping (uint8_t ep, uint8_t socketNum,  CyBool_t remap, uint16_t newstreamId, uint8_t newep)
{
    uint8_t epnum = (ep & 0x7F);

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if (epnum > 15)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (ep & 0x80)
    {
        if (USB3PROT->prot_epi_mapped_stream[socketNum] & CY_U3P_UIB_ENABLE)
        {
            USB3PROT->prot_epi_mapped_stream[socketNum] |= CY_U3P_UIB_UNMAP;
            USB3PROT->prot_epi_mapped_stream[socketNum] &= ~CY_U3P_UIB_ENABLE;
            while(!(USB3PROT->prot_epi_mapped_stream[socketNum] & CY_U3P_UIB_UNMAPPED));
            USB3PROT->prot_epi_mapped_stream[socketNum] &= ~CY_U3P_UIB_UNMAP;
        }

        if(remap)
        {
            USB3PROT->prot_epi_mapped_stream[socketNum] = (CY_U3P_UIB_ENABLE |
                    ((newep & 0x7f) << CY_U3P_UIB_EP_NUMBER_POS) | newstreamId);
        }
    }
    else
    {
        if (USB3PROT->prot_epo_mapped_stream[socketNum] & CY_U3P_UIB_ENABLE)
        {
            USB3PROT->prot_epo_mapped_stream[socketNum] |= CY_U3P_UIB_UNMAP;
            USB3PROT->prot_epo_mapped_stream[socketNum] &= ~CY_U3P_UIB_ENABLE;
            while(!(USB3PROT->prot_epo_mapped_stream[socketNum] & CY_U3P_UIB_UNMAPPED));
            USB3PROT->prot_epo_mapped_stream[socketNum] &= ~CY_U3P_UIB_UNMAP;
        }

        if(remap)
        {
            USB3PROT->prot_epo_mapped_stream[socketNum] = (CY_U3P_UIB_ENABLE |
                    ((newep & 0x7f) << CY_U3P_UIB_EP_NUMBER_POS) | newstreamId);
        }
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbGetDevProperty (
        CyU3PUsbDevProperty  type,
        uint32_t            *buf)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if (glUibDeviceInfo.usbState < CY_U3P_USB_CONNECTED)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    if ((buf == 0) || (type > CY_U3P_USB_PROP_SYS_EXIT_LAT))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED)
    {
        switch (type)
        {
        case CY_U3P_USB_PROP_DEVADDR:
            *buf = USB3PROT->prot_cs & CY_U3P_UIB_SS_DEVICEADDR_MASK;
            break;

        case CY_U3P_USB_PROP_LINKSTATE:
            *buf = USB3LNK->lnk_ltssm_state & CY_U3P_UIB_LTSSM_STATE_MASK;
            break;

        case CY_U3P_USB_PROP_ITPINFO:
            *buf = USB3PROT->prot_framecnt;
            break;

        case CY_U3P_USB_PROP_SYS_EXIT_LAT:
            CyU3PMemCopy ((uint8_t *)buf, (uint8_t *)glUibSelBuffer, 6);
            break;

        default:
            status = CY_U3P_ERROR_BAD_ARGUMENT;
            break;
        }
    }
    else
    {
        switch (type)
        {
        case CY_U3P_USB_PROP_DEVADDR:
            *buf = (UIB->dev_cs & CY_U3P_UIB_DEVICEADDR_MASK) >> CY_U3P_UIB_DEVICEADDR_POS;
            break;

        case CY_U3P_USB_PROP_FRAMECNT:
            *buf = UIB->dev_framecnt;
            break;

        default:
            status = CY_U3P_ERROR_BAD_ARGUMENT;
            break;
        }
    }

    return status;
}

void
CyU3PUsbRegisterEpEvtCallback (
        CyU3PUsbEpEvtCb_t cbFunc,
        uint32_t          eventMask,
        uint16_t          outEpMask,
        uint16_t          inEpMask)
{
    uint8_t i;

    /* EP interrupts for EP-0 are not to be enabled. */
    outEpMask &= 0xFFFE;
    inEpMask  &= 0xFFFE;

    /* Store the parameters. */
    glUsbEpCb          = cbFunc;
    glUsbEpEvtMask     = eventMask;
    glUsbEvtEnabledEps = (((uint32_t)outEpMask << 16) | (uint32_t)inEpMask);

    UIB->dev_ep_intr_mask       |= glUsbEvtEnabledEps;
    USB3PROT->prot_ep_intr_mask |= glUsbEvtEnabledEps;

    for (i = 1; i < 16; i++)
    {
        if (outEpMask & (1 << i))
        {
            CyU3PUsbEnableEpInterrupts (i, eventMask);
        }

        if (inEpMask & (1 << i))
        {
            CyU3PUsbEnableEpInterrupts (0x80 | i, eventMask);
        }
    }
}

CyU3PReturnStatus_t
CyU3PUsbEnableITPEvent (
        CyBool_t enable)
{
    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        return CY_U3P_ERROR_NOT_STARTED;

    if (enable != glUibDeviceInfo.sofEventEnable)
    {
        glUibDeviceInfo.sofEventEnable = enable;

        /* Turn on/off the relevant interrupts. */
        if (enable)
        {
            UIB->dev_ctl_intr        |= CY_U3P_UIB_SOF;
            UIB->dev_ctl_intr_mask   |= CY_U3P_UIB_SOF;
            USB3PROT->prot_intr      |= CY_U3P_UIB_ITP_EV;
            USB3PROT->prot_intr_mask |= CY_U3P_UIB_ITP_EN;
        }
        else
        {
            UIB->dev_ctl_intr_mask   &= ~CY_U3P_UIB_SOF;
            USB3PROT->prot_intr_mask &= ~CY_U3P_UIB_ITP_EN;
        }
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbDoRemoteWakeup (
        void)
{
    /* Check if the USB connection state allows 2.0 remote wake signalling. */
    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        return CY_U3P_ERROR_NOT_STARTED;
    if ((glUibDeviceInfo.usbState != CY_U3P_USB_CONNECTED) || (glUibDeviceInfo.usbSpeed == CY_U3P_SUPER_SPEED))
        return CY_U3P_ERROR_OPERN_DISABLED;
    if ((glUibDeviceInfo.enumMethod == CY_U3P_USBENUM_WB) &&
            ((glUibDeviceInfo.usbDeviceStat & CY_U3P_USB_DEVSTAT_REMOTEWAKE) == 0))
        return CY_U3P_ERROR_OPERN_DISABLED;

    UIB->dev_pwr_cs |= CY_U3P_UIB_SIGRSUME;
    CyU3PThreadSleep (15);
    UIB->dev_pwr_cs &= ~CY_U3P_UIB_SIGRSUME;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbForceFullSpeed (
        CyBool_t enable)
{
    if (glUibDeviceInfo.usbState == CY_U3P_USB_INACTIVE)
        return CY_U3P_ERROR_NOT_STARTED;
    if (glUibDeviceInfo.usbState >= CY_U3P_USB_VBUS_WAIT)
        return CY_U3P_ERROR_OPERN_DISABLED;

    if (enable)
        UIB->dev_pwr_cs |= CY_U3P_UIB_FORCE_FS;
    else
        UIB->dev_pwr_cs &= ~CY_U3P_UIB_FORCE_FS;

    return CY_U3P_SUCCESS;
}

