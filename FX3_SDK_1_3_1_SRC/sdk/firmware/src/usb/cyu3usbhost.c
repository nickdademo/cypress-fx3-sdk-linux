/*
 ## Cypress USB 3.0 Platform source file (cyu3usbhost.c)
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

/* This file defines the USB host mode APIs.
 */
#include <cyu3types.h>
#include <cyu3error.h>
#include <cyu3system.h>
#include <cyfx3_api.h>
#include <cyu3mmu.h>
#include <cyu3vic.h>
#include <cyu3usbhost.h>
#include <cyu3usb.h>
#include <cyu3usbpp.h>
#include <cyu3usbotg.h>
#include <cyu3regs.h>
#include <cyu3utils.h>

/* The scheduler entry size for an EP in words. */
#define CY_U3P_USB_HOST_SHDL_ENTRY_SIZE         (3)
#define CY_U3P_USB_HOST_SDHL_RESP_MAX_ENTRY     (240)
#define CY_U3P_HOST_EP0_SCRATCH_MEM             (*(uvint32_t *)(0xe0032b00))

extern CyBool_t glIsHostEnabled;

static CyU3PEvent glHostEpEvent;
static uint8_t glPeriodEpCount = 0;
static uint8_t glAsyncEpCount = 0;
static uint16_t glHostRespIndex = 0;
static uint32_t glHostEpStatus[32];
static uint32_t glHostResp[CY_U3P_USB_HOST_SDHL_RESP_MAX_ENTRY];
static CyU3PUsbHostOpSpeed_t glUsbHostSpeed;
static CyU3PUsbHostPortStatus_t glUsbHostPortStatus;
static uint32_t glHostPendingEpXfer = 0;
static CyU3PUsbHostConfig_t glUsbHostCfg;
static CyU3PDmaChannel glHostEp0InChannel, glHostEp0OutChannel;
static uint8_t *glHostSetupPkt = NULL;
static uint8_t *glHostEp0Data = NULL;
static uint32_t glHostEpConfig = 0;
static uint32_t *glHostEpEntry[32];
/* 1 == setup phase,
 * 2 == data phase,
 * 3 == status phase,
 * 4 == error phase. */
static uint8_t glHostEp0Phase = 0;

extern void
CyU3PUsbSetHostIntHandler (
        void (*func_p) (void));
extern void
CyU3PUsbSetHostEPIntHandler (
        void (*func_p) (void));
extern void
CyU3PUsbSetEHCIIntHandler (
        void (*func_p) (void));
extern void
CyU3PUsbSetOHCIIntHandler (
        void (*func_p) (void));
extern void
CyU3PUsbPowerOn (
        void);

/* Start usb host mode. */
CyU3PReturnStatus_t
CyU3PUsbHostStart (CyU3PUsbHostConfig_t *hostCfg)
{
    /* Verify that the part in use can support the USB 2.0 host functionality. */
    if (!CyFx3DevIsOtgSupported ())
        return CY_U3P_ERROR_NOT_SUPPORTED;

    if (glIsHostEnabled)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    /* Parameter validity. */
    if (hostCfg == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    /* Check if host mode is allowed. */
    if ((!CyU3POtgIsHostMode ()) || (CyU3PUsbIsStarted ()))
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    /* Register the handlers for Host interrupt events. */
    CyU3PUsbSetHostIntHandler (CyU3PUsbHostIntHandler);
    CyU3PUsbSetHostEPIntHandler (CyU3PUsbHostEpIntHandler);
    CyU3PUsbSetEHCIIntHandler (CyU3PUsbHostEhciIntHandler);
    CyU3PUsbSetOHCIIntHandler (CyU3PUsbHostOhciIntHandler);

    /* Enable the USB PHY connections. */
    GCTLAON->control &= ~CY_U3P_GCTL_ANALOG_SWITCH;

    /* Enable and set the EPM clock to bus clock (100MHz). */
    GCTL->uib_core_clk = (CY_U3P_GCTL_UIBCLK_CLK_EN | CY_U3P_GCTL_UIB_CORE_CLK_DEFAULT);
    GCTLAON->control |= CY_U3P_GCTL_USB_POWER_EN;

    /* Reset and enable the UIB block only if in host only mode. */
    if (CyU3POtgGetMode () == CY_U3P_OTG_MODE_HOST_ONLY)
    {
        CyU3PUsbPowerOn ();
    }

    /* Clear port ownership. */
    UIB->ehci_configflag = 0;

    /* Enable host mode. */
    UIB->otg_ctrl &= CY_U3P_UIB_OTG_ENABLE;
    UIB->otg_ctrl |= CY_U3P_UIB_HOST_ENABLE;
    /* Reset and enable the PHY. */
    UIB->phy_clk_and_test = (CY_U3P_UIB_DATABUS16_8 | CY_U3P_UIB_VLOAD |
            CY_U3P_UIB_RESET | CY_U3P_UIB_EN_SWITCH);
    CyU3PBusyWait (10);
    UIB->phy_clk_and_test &= ~CY_U3P_UIB_RESET;
    UIB->phy_clk_and_test |= CY_U3P_UIB_SUSPEND_N;
    CyU3PBusyWait (100);
    /* Switch EPM clock to 120MHz. */
    GCTL->uib_core_clk &= ~CY_U3P_GCTL_UIBCLK_EPMCLK_SRC_MASK;
    CyU3PBusyWait (10);

    /* Enable D+ and D- pulldown. */
    UIB->otg_ctrl |= (CY_U3P_UIB_DP_PD_EN | CY_U3P_UIB_DM_PD_EN);
    CyU3PBusyWait (10);

    /* Initialize global variables. */
    glPeriodEpCount = glAsyncEpCount = 0;
    glUsbHostPortStatus = 0;
    glUsbHostSpeed = CY_U3P_USB_HOST_HIGH_SPEED;
    CyU3PMemSet ((uint8_t *)glHostEpEntry, 0, 32 * 4);

    /* Load the default configuration. Also disable deactivate on IN EP short packet. */
    UIB->host_cs = (CY_U3P_UIB_HOST_CS_DEFAULT & ~CY_U3P_UIB_DEACTIVATE_ON_IN_EP_SHORT_PKT);

    /* Copy and save the config information. */
    glUsbHostCfg = *hostCfg;

    /* Create an EP event group for transfer complete notification. */
    CyU3PEventCreate (&glHostEpEvent);

    /* Mark the controller as active. */
    glIsHostEnabled = CyTrue;

    UIB->ehci_usbsts = CY_U3P_UIB_EHCI_USBSTS_PORT_CHNG_DET;
    UIB->ehci_usbintr = CY_U3P_UIB_PORT_CHANGE_DET_IE;

    UIB->host_ep_intr = ~0;
    UIB->host_ep_intr_mask = 0;

    /* Make sure EHCI is the owner of the port. */
    UIB->ehci_configflag = CY_U3P_UIB_CF;
    /* Wait for line voltage to stabilize. */
    CyU3PThreadSleep (100);

    UIB->intr_mask |= (CY_U3P_UIB_HOST_INT | CY_U3P_UIB_HOST_EP_INT | CY_U3P_UIB_EHCI_INT);

    CyU3PUibSocketInit ();

    CyU3PVicEnableInt (CY_U3P_VIC_UIB_DMA_VECTOR);
    CyU3PVicEnableInt (CY_U3P_VIC_UIB_CORE_VECTOR);

    return CY_U3P_SUCCESS;
}

/* Stop usb host mode. */
CyU3PReturnStatus_t
CyU3PUsbHostStop (
        void)
{
    if (!glIsHostEnabled)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Disable interrupts. */
    UIB->intr_mask &= ~(CY_U3P_UIB_HOST_INT | CY_U3P_UIB_HOST_EP_INT | CY_U3P_UIB_EHCI_INT);
    CyU3PVicDisableInt (CY_U3P_VIC_UIB_DMA_VECTOR);

    /* Disable D+ and D- pulldown. */
    UIB->otg_ctrl &= ~(CY_U3P_UIB_DP_PD_EN | CY_U3P_UIB_DM_PD_EN);
    CyU3PBusyWait (10);
    UIB->otg_ctrl &= CY_U3P_UIB_OTG_ENABLE;

    CyU3PEventDestroy (&glHostEpEvent);

    /* Disable the UIB block only if in host only mode. */
    if (CyU3POtgGetMode () == CY_U3P_OTG_MODE_HOST_ONLY)
    {
        CyU3PVicDisableInt (CY_U3P_VIC_UIB_CORE_VECTOR);
        UIB->power &= ~CY_U3P_UIB_RESETN;
        CyU3PBusyWait (10);

        /* Disable the UIB clock. */
        GCTL->uib_core_clk &= ~CY_U3P_GCTL_UIBCLK_CLK_EN;
        GCTLAON->control &= ~CY_U3P_GCTL_USB_POWER_EN;
    }

    glIsHostEnabled = CyFalse;

    return CY_U3P_SUCCESS;
}

static CyU3PReturnStatus_t
CyU3PUsbHostOhciPortEnable (
        void)
{
    UIB->host_cs = (UIB->host_cs & ~CY_U3P_UIB_FRAME_FIT_OFFSET_MASK)
        | (190 << CY_U3P_UIB_FRAME_FIT_OFFSET_POS);

    /* Set OHCI controller as the owner of the port. */
    UIB->ehci_portsc |= CY_U3P_UIB_PORT_OWNER;
    UIB->ohci_control = (UIB->ohci_control & ~CY_U3P_UIB_HCFS_MASK)
       | (0x02 << CY_U3P_UIB_HCFS_POS);

    /* Enable the RH port. */
    UIB->ohci_rh_port_status = CY_U3P_UIB_RHP_PES;
    CyU3PBusyWait (10);

    /* Check if the device is connected. */
    if ((UIB->ohci_rh_port_status & CY_U3P_UIB_RHP_CCS) == 0)
    {
        return CY_U3P_ERROR_FAILURE;
    }
    UIB->ohci_rh_port_status |= CY_U3P_UIB_RHP_CSC;

    /* Reset the port. */
    UIB->ohci_rh_port_status |= CY_U3P_UIB_RHP_PRS;
    while (UIB->ohci_rh_port_status & CY_U3P_UIB_RHP_PRS);
    UIB->ohci_rh_port_status |= CY_U3P_UIB_RHP_PRSC;

    /* Enable the controller. */
    UIB->ohci_command_status |= CY_U3P_UIB_RS;

    /* Identify the speed of operation. */
    if (UIB->ohci_rh_port_status & CY_U3P_UIB_RHP_LSDA)
    {
        glUsbHostSpeed = CY_U3P_USB_HOST_LOW_SPEED;
    }
    else
    {
        glUsbHostSpeed = CY_U3P_USB_HOST_FULL_SPEED;
    }

    /* Enable the interrupt for RH change after clearing all interrupts. */
    UIB->ohci_rh_port_status |= (CY_U3P_UIB_RHP_CSC | CY_U3P_UIB_RHP_PESC |
            CY_U3P_UIB_RHP_PSSC | CY_U3P_UIB_RHP_OCIC | CY_U3P_UIB_RHP_PRSC);
    UIB->ohci_interrupt_enable = (CY_U3P_UIB_RHSC | CY_U3P_UIB_MIE);

    UIB->intr_mask |= CY_U3P_UIB_OHCI_INT;

    return CY_U3P_SUCCESS;
}

/* Control port status - enable, power on. */
CyU3PReturnStatus_t
CyU3PUsbHostPortEnable (
        void)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    if (!glIsHostEnabled)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }
    if (glUsbHostPortStatus & CY_U3P_USB_HOST_PORT_STAT_ENABLED)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    /* Disable both EHCI and OHCI interrupts. Only the correct interrupt
     * needs to be enabled at the end of the call. */
    UIB->intr_mask &= ~(CY_U3P_UIB_EHCI_INT | CY_U3P_UIB_OHCI_INT);

    /* Initialize variables. */
    glPeriodEpCount = 0;
    glAsyncEpCount = 0;
    UIB->host_ep_intr_mask = 0;
    UIB->host_active_ep = 0;
    UIB->host_ep_intr = ~0;
    glHostPendingEpXfer = 0;
    glHostEpConfig = 0;

    /* Make sure that all EP events are cleared. */
    CyU3PEventSet (&glHostEpEvent, 0, CYU3P_EVENT_AND);

    /* Disable the schedulers. */
    UIB->ehci_usbcmd &= ~(CY_U3P_UIB_EHCI_USBCMD_PER_SHDL_EN | 
            CY_U3P_UIB_EHCI_USBCMD_ASYNC_SHDL_EN);
    UIB->ohci_control &= ~(CY_U3P_UIB_PLE | CY_U3P_UIB_IE |
            CY_U3P_UIB_CLE | CY_U3P_UIB_BLE);

    /* Reset the EPM. */
    UIB->iepm_cs |= (CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);
    CyU3PBusyWait(10);
    UIB->iepm_cs &= ~(CY_U3P_UIB_EPM_FLUSH | CY_U3P_UIB_EPM_MUX_RESET);

    /* Initialize the response register. */
    glHostRespIndex = 0;
    UIB->host_resp_base = (uint32_t)glHostResp;
    CyU3PMemSet ((uint8_t *)glHostResp, 0, sizeof (glHostResp));
    CyU3PMemSet ((uint8_t *)glHostEpStatus, 0, sizeof (glHostEpStatus));
    UIB->host_resp_cs = (CY_U3P_USB_HOST_SDHL_RESP_MAX_ENTRY |
            ((CY_U3P_USB_HOST_SDHL_RESP_MAX_ENTRY / 2) << CY_U3P_UIB_LIMIT_POS));

    /* Wait for the port to get started. */
    if ((UIB->ehci_portsc & CY_U3P_UIB_PORT_CONNECT_C) == 0)
    {
        status = CY_U3P_ERROR_FAILURE;
    }
    UIB->ehci_portsc |= CY_U3P_UIB_PORT_CONNECT_C;

    /* Check if the device is connected. */
    if ((UIB->ehci_portsc & CY_U3P_UIB_PORT_CONNECT) == 0)
    {
        status = CY_U3P_ERROR_FAILURE;
    }

    /* Reset port. */
    while (UIB->ehci_portsc & CY_U3P_UIB_PORT_RESET);
    UIB->ehci_portsc |= CY_U3P_UIB_PORT_RESET;
    /* Wait for line voltage to stabilize. */
    CyU3PThreadSleep (100);
    UIB->ehci_portsc &= ~CY_U3P_UIB_PORT_RESET;
    while (UIB->ehci_portsc & CY_U3P_UIB_PORT_RESET);

    /* Check if the port got enabled. If not then the
     * device attached is not high speed, try OHCI. */
    if ((UIB->ehci_portsc & CY_U3P_UIB_PORT_EN) == 0)
    {
        /* Change owner to OHCI and initialize port. */
        status = CyU3PUsbHostOhciPortEnable ();
    }
    else
    {
        /* This is a successful EHCI initialization.
         * Start the EHCI controller. */
        UIB->host_cs = (UIB->host_cs & ~CY_U3P_UIB_FRAME_FIT_OFFSET_MASK)
            | (212 << CY_U3P_UIB_FRAME_FIT_OFFSET_POS);
        UIB->ehci_usbcmd |= CY_U3P_UIB_EHCI_USBCMD_RS;
        glUsbHostSpeed = CY_U3P_USB_HOST_HIGH_SPEED;
        UIB->intr_mask &= ~CY_U3P_UIB_OHCI_INT;
        UIB->ehci_usbsts = CY_U3P_UIB_EHCI_USBSTS_PORT_CHNG_DET;
        UIB->ehci_usbintr |= CY_U3P_UIB_PORT_CHANGE_DET_IE;
        UIB->intr_mask |= CY_U3P_UIB_EHCI_INT;
    }

    if (status == CY_U3P_SUCCESS)
    {
        glUsbHostPortStatus |= CY_U3P_USB_HOST_PORT_STAT_ENABLED;

        /* Clear pointers. */
        CyU3PMemSet ((uint8_t *)glHostEpEntry, 0, 32 * 4);
        /* Make the device address to go back to zero. */
        CyU3PUsbHostSetDeviceAddress (0);
    }

    return status;
}

/* Get the port status. */
CyU3PReturnStatus_t
CyU3PUsbHostGetPortStatus (
        CyU3PUsbHostPortStatus_t *portStatus,
        CyU3PUsbHostOpSpeed_t    *portSpeed)
{
    if (!glIsHostEnabled)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }
    if (portStatus != NULL)
    {
        *portStatus = glUsbHostPortStatus;
    }
    if (portSpeed != NULL)
    {
        *portSpeed = glUsbHostSpeed;
    }

    return CY_U3P_SUCCESS;
}

static void
CyU3PHostMyEp0DmaCb (CyU3PDmaChannel *ch,
        CyU3PDmaCbType_t type,
        CyU3PDmaCBInput_t *input)
{
    uint32_t status = CY_U3P_SUCCESS;
    CyBool_t dir = (glHostSetupPkt[0] & 0x80) ? CyTrue : CyFalse;
    uint16_t size = CY_U3P_MAKEWORD(glHostSetupPkt[7], glHostSetupPkt[6]);
    CyU3PDmaBuffer_t buf_p;

    /* This callback happens after setup phase and also status phase. */
    switch (type)
    {
        case CY_U3P_DMA_CB_SEND_CPLT:
            if (glHostEp0Phase == 1) /* Setup phase. */
            {
                if (dir) /* IN data. */
                {
                    /* Queue the data phase if there is any data to be received. */
                    if (size)
                    {
                        buf_p.buffer = glHostEp0Data;
                        buf_p.count = 0;
                        /* Align to 16 byte boundary. */
                        buf_p.size = ((size + 0x0F) & ~0x0F);
                        buf_p.status = 0;
                        status = CyU3PDmaChannelSetupRecvBuffer (&glHostEp0InChannel, &buf_p);
                    }
                    /* Queue the OUT status phase. */
                    if (status == CY_U3P_SUCCESS)
                    {
                        buf_p.buffer = glHostSetupPkt;
                        buf_p.count = 0;
                        buf_p.size = 16;
                        buf_p.status = 0;
                        status = CyU3PDmaChannelSetupSendBuffer (&glHostEp0OutChannel, &buf_p);
                    }
                    if (status == CY_U3P_SUCCESS)
                    {
                        glHostEp0Phase = 3;
                    }
                }
                else /* Out data. */
                {
                    /* In this case, we can queue both data and status phase together. */
                    if (size)
                    {
                        buf_p.buffer = glHostEp0Data;
                        buf_p.count = size;
                        /* Align to 16 byte boundary. */
                        buf_p.size = ((size + 0x0F) & ~0x0F);
                        buf_p.status = 0;
                        status = CyU3PDmaChannelSetupSendBuffer (&glHostEp0OutChannel, &buf_p);
                    }
                    if (status == CY_U3P_SUCCESS)
                    {
                        buf_p.buffer = glHostSetupPkt;
                        buf_p.count = 0;
                        buf_p.size = 16;
                        buf_p.status = 0;
                        status = CyU3PDmaChannelSetupRecvBuffer (&glHostEp0InChannel, &buf_p);
                    }
                    if (status == CY_U3P_SUCCESS)
                    {
                        glHostEp0Phase = 3;
                    }
                }
            }
            else /* OUT status phase. */
            {
                glHostEp0Phase = 0;
                if ((dir) && (size))
                {
                    status = CyU3PDmaChannelWaitForCompletion (&glHostEp0InChannel, CYU3P_NO_WAIT);
                    if (status == CY_U3P_ERROR_TIMEOUT)
                    {
                        /* EP0 buffer has to be wrapped up if receive has not completed. */
                        status = CyU3PDmaChannelSetWrapUp (&glHostEp0InChannel);
                    }
                }
            }
            break;

        case CY_U3P_DMA_CB_RECV_CPLT:
            if (!dir) /* IN status phase. */
            {
                glHostEp0Phase = 0;
            }
            break;

        default:
            status = CY_U3P_ERROR_FAILURE;
            break;

    }

    if (status != CY_U3P_SUCCESS)
    {
        glHostEp0Phase = 4;
        CyU3PDmaChannelReset (&glHostEp0OutChannel);
        CyU3PDmaChannelReset (&glHostEp0InChannel);
        CyU3PUsbHostEpAbort (0);
    }
}

static CyU3PReturnStatus_t
CyU3PMyHostEp0DmaConfig (CyBool_t isEnable)
{
    uint32_t status = CY_U3P_SUCCESS;
    CyU3PDmaChannelConfig_t dmaCfg;

    /* Since the channels are used only for over-ride mode
     * do not allocate any buffers. */
    dmaCfg.size = 64;
    dmaCfg.count = 0;
    dmaCfg.prodHeader = 0;
    dmaCfg.prodFooter = 0;
    dmaCfg.consHeader = 0;
    dmaCfg.prodAvailCount = 0;
    dmaCfg.dmaMode    = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.cb = CyU3PHostMyEp0DmaCb;

    /* Clear tracker variables. */
    glHostSetupPkt = NULL;
    glHostEp0Data = NULL;
    glHostEp0Phase = 0;

    if (isEnable)
    {
        /* Create the egress pipe. */
        dmaCfg.prodSckId = CY_U3P_CPU_SOCKET_PROD;
        dmaCfg.consSckId = CY_U3P_UIB_SOCKET_CONS_0;
        dmaCfg.notification = (CY_U3P_DMA_CB_SEND_CPLT);
        status = CyU3PDmaChannelCreate (&glHostEp0OutChannel,
                CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaCfg);

        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        /* Create the ingress pipe. */
        dmaCfg.prodSckId = CY_U3P_UIB_SOCKET_PROD_0;
        dmaCfg.consSckId = CY_U3P_CPU_SOCKET_CONS;
        dmaCfg.notification = (CY_U3P_DMA_CB_RECV_CPLT);
        status = CyU3PDmaChannelCreate (&glHostEp0InChannel,
                CY_U3P_DMA_TYPE_MANUAL_IN, &dmaCfg);
        if (status != CY_U3P_SUCCESS)
        {
            CyU3PDmaChannelDestroy (&glHostEp0OutChannel);
            return status;
        }
    }
    else
    {
        CyU3PDmaChannelDestroy (&glHostEp0OutChannel);
        CyU3PDmaChannelDestroy (&glHostEp0InChannel);
    }

    return status;
}

/* Disable the port. */
CyU3PReturnStatus_t
CyU3PUsbHostPortDisable (
        void)
{
    if (!(glUsbHostPortStatus & CY_U3P_USB_HOST_PORT_STAT_ENABLED))
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Disable the EHCI and OHCI interrupts. */
    UIB->intr_mask &= ~(CY_U3P_UIB_OHCI_INT | CY_U3P_UIB_EHCI_INT);

    /* Make sure EHCI is the owner of the port. */
    UIB->ehci_configflag = 0;
    CyU3PThreadSleep (10);
    UIB->ehci_configflag = CY_U3P_UIB_CF;
    /* Wait for line voltage to stabilize. */
    CyU3PThreadSleep (100);

    /* Clear any interrupts that got set and then
     * re-enable the EHCI interrupts only if the port has
     * not detected any connection. */

    /* Clear the interrupts only if the port does not have a
     * connection. Otherwise this might very well be a connect
     * interrupt after the disconnect. */
    if ((UIB->ehci_portsc & CY_U3P_UIB_PORT_CONNECT) == 0)
    {
        UIB->ehci_usbsts = CY_U3P_UIB_EHCI_USBSTS_PORT_CHNG_DET;
    }

    /* Remove EP0 dma channels if required. */
    if (!glUsbHostCfg.ep0LowLevelControl)
    {
        CyU3PMyHostEp0DmaConfig (CyFalse);
    }

    glPeriodEpCount = 0;
    glAsyncEpCount = 0;
    UIB->host_ep_intr_mask = 0;
    UIB->host_active_ep = 0;
    UIB->host_ep_intr = ~0;
    glHostPendingEpXfer = 0;
    glHostEpConfig = 0;

    /* Since interrupt will not be generated to waiting threads
     * on CyU3PUsbHostEpWaitForCompletion, we need to make sure
     * that all these threads are woken up. */
    CyU3PEventSet (&glHostEpEvent, ~0, CYU3P_EVENT_AND);

    /* Disable the schedulers. */
    UIB->ehci_usbcmd &= ~(CY_U3P_UIB_EHCI_USBCMD_PER_SHDL_EN | 
            CY_U3P_UIB_EHCI_USBCMD_ASYNC_SHDL_EN);
    UIB->ohci_control &= ~(CY_U3P_UIB_PLE | CY_U3P_UIB_IE |
            CY_U3P_UIB_CLE | CY_U3P_UIB_BLE);

    glUsbHostPortStatus &= ~CY_U3P_USB_HOST_PORT_STAT_ENABLED;
    glUsbHostSpeed = CY_U3P_USB_HOST_HIGH_SPEED;

    /* Re-enable the interrupt. */
    UIB->ehci_usbintr |= CY_U3P_UIB_PORT_CHANGE_DET_IE;

    UIB->intr_mask |= CY_U3P_UIB_EHCI_INT;

    /* Make the device address to go back to zero. */
    CyU3PUsbHostSetDeviceAddress (0);

    /* Clear pointers. */
    CyU3PMemSet ((uint8_t *)glHostEpEntry, 0, 32 * 4);

    return CY_U3P_SUCCESS;
}

/* Reset port - This is same as disable and re-enabling the port. */
CyU3PReturnStatus_t
CyU3PUsbHostPortReset (
        void)
{
    uint32_t status;

    if (!glIsHostEnabled)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    CyU3PUsbHostPortDisable ();
    /* Clear the interrupt. */
    UIB->ehci_usbsts = CY_U3P_UIB_EHCI_USBSTS_PORT_CHNG_DET;
    CyU3PThreadSleep (100);
    status = CyU3PUsbHostPortEnable ();

    return status;
}

/* Suspend port. */
CyU3PReturnStatus_t
CyU3PUsbHostPortSuspend (
        void)
{
    if (glUsbHostPortStatus != CY_U3P_USB_HOST_PORT_STAT_ACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }
    if ((UIB->host_active_ep) || (glHostPendingEpXfer))
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    if (glUsbHostSpeed == CY_U3P_USB_HOST_HIGH_SPEED)
    {
        UIB->ehci_portsc |= CY_U3P_UIB_PORT_SUSPEND;
    }
    else
    {
        UIB->ohci_rh_port_status |= CY_U3P_UIB_RHP_PSS;
    }

    glUsbHostPortStatus |= CY_U3P_USB_HOST_PORT_STAT_SUSPENDED;

    return CY_U3P_SUCCESS;
}

/* Resume port. */
CyU3PReturnStatus_t
CyU3PUsbHostPortResume (
        void)
{
    if ((glUsbHostPortStatus & CY_U3P_USB_HOST_PORT_STAT_ACTIVE) !=
            CY_U3P_USB_HOST_PORT_STAT_ACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }
    if ((glUsbHostPortStatus & CY_U3P_USB_HOST_PORT_STAT_SUSPENDED) == 0)
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    if (glUsbHostSpeed == CY_U3P_USB_HOST_HIGH_SPEED)
    {
        UIB->ehci_portsc &= ~CY_U3P_UIB_PORT_SUSPEND;
    }
    else
    {
        UIB->ohci_rh_port_status &= ~CY_U3P_UIB_RHP_PSS;
    }

    glUsbHostPortStatus &= ~CY_U3P_USB_HOST_PORT_STAT_SUSPENDED;

    return CY_U3P_SUCCESS;
}

/* Get the current frame number. */
CyU3PReturnStatus_t
CyU3PUsbHostGetFrameNumber (
        uint32_t *frameNumber)
{
    if (frameNumber == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if (glUsbHostPortStatus != CY_U3P_USB_HOST_PORT_STAT_ACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if (glUsbHostSpeed == CY_U3P_USB_HOST_HIGH_SPEED)
    {
        *frameNumber = UIB->ehci_frindex & CY_U3P_UIB_FRINDEX_MASK;
    }
    else
    {
        *frameNumber = UIB->ohci_fm_number & CY_U3P_UIB_FN_MASK;
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbHostGetDeviceAddress (
        uint8_t *devAddr)
{
    if (devAddr == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if (glUsbHostPortStatus != CY_U3P_USB_HOST_PORT_STAT_ACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    *devAddr = (UIB->host_cs & CY_U3P_UIB_DEV_ADDR_MASK);

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbHostSetDeviceAddress (
        uint8_t devAddr)
{
    if (devAddr > CY_U3P_UIB_DEV_ADDR_MASK)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (glUsbHostPortStatus != CY_U3P_USB_HOST_PORT_STAT_ACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    UIB->host_cs = (UIB->host_cs & ~CY_U3P_UIB_DEV_ADDR_MASK) |
        (devAddr & CY_U3P_UIB_DEV_ADDR_MASK);

    return CY_U3P_SUCCESS;
}

static CyU3PReturnStatus_t
CyU3PUsbHostEpFindEntry (
        uint8_t ep,
        uint32_t **entry)
{
    uint8_t epIndex, temp;
    uint32_t *ptr;
    CyBool_t flag = CyFalse;

    epIndex = (ep & 0x0F);
    if ((ep != 0) && (!(ep & 0x80)))
    {
        /* OutEp. */
        epIndex |= 0x10;
    }

    /* Locate the scheduler entry for the Ep. */
    if (UIB->host_shdl_cs & CY_U3P_UIB_ASYNC_SHDL_STATUS)
    {
        ptr = (uint32_t *)&UIB->ehci_shdl[32];
    }
    else
    {
        ptr = (uint32_t *)UIB->ehci_shdl;
    }

    for (temp = 0; temp < glPeriodEpCount; temp++)
    {
        if ((*ptr & CY_U3P_UIB_EHCI0_EPND_MASK) == epIndex)
        {
            flag = CyTrue;
            break;
        }
        ptr += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
    }
    for (temp = 0; ((temp < glAsyncEpCount) && (!flag)); temp++)
    {
        if ((*ptr & CY_U3P_UIB_EHCI0_EPND_MASK) == epIndex)
        {
            flag = CyTrue;
            break;
        }
        ptr += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
    }

    if (!flag)
    {
        *entry = NULL;
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    *entry = ptr;

    return CY_U3P_SUCCESS;
}

static __inline void
CyU3PUsbHostFormatEntry (
        uint8_t ep,
        CyU3PUsbHostEpConfig_t *cfg,
        uint32_t *entry)
{
    CyBool_t isAsync;

    isAsync = ((cfg->type == CY_U3P_USB_EP_CONTROL) ||
            (cfg->type == CY_U3P_USB_EP_BULK)) ? CyTrue : CyFalse;

    /* Create the scheduler entry. */
    entry[0] = (ep & 0x0F);
    if (((ep & 0x80) == 0) && (ep != 0))
    {
        entry[0] |= 0x10;
    }
    entry[0] |= ((cfg->type << CY_U3P_UIB_EHCI0_EPT_POS)
            & CY_U3P_UIB_EHCI0_EPT_MASK);

    /* Mark end of list. */
    if (((!isAsync) && (glPeriodEpCount == 0)) ||
            ((isAsync) && (glAsyncEpCount == 0)))
    {
        entry[0] |= CY_U3P_UIB_EHCI0_T;
    }

    /* Set the EP to halt by default. */
    entry[0] |= CY_U3P_UIB_EHCI0_HALT;

    /* Set the EP to receive / transmit only
     * when there is buffer available. */
    if (cfg->type == CY_U3P_USB_EP_ISO)
    {
        entry[0] |= CY_U3P_UIB_OHCI0_ISO_EPM;
    }
    entry[0] |= ((cfg->mult << CY_U3P_UIB_EHCI0_MULT_POS)
        & CY_U3P_UIB_EHCI0_MULT_MASK);
    if (glUsbHostSpeed == CY_U3P_USB_HOST_HIGH_SPEED)
    {
        /* Send only ping token for EHCI OUT EP. */
        /*entry[0] |= CY_U3P_UIB_EHCI0_PING;*/
        /* Set UFRAME_SMASK as 1 always. */
        if (cfg->type == CY_U3P_USB_EP_INTR)
        {
            entry[0] |= (1 << CY_U3P_UIB_OHCI0_UFRAME_SMASK_POS);
        }
    }

    entry[1] = ((cfg->maxPktSize << CY_U3P_UIB_EHCI1_MAX_PKT_SIZE_POS)
        & CY_U3P_UIB_EHCI1_MAX_PKT_SIZE_MASK);
    /* Also update the EPM packet size. */
    if ((ep & 0x80) || (ep == 0))
    {
        UIB->iepm_endpoint[ep & 0x0F] = (cfg->fullPktSize & CY_U3P_UIB_PACKET_SIZE_MASK);
    }
    if ((ep & 0x80) == 0)
    {
        UIB->eepm_endpoint[ep & 0x0F] = (cfg->fullPktSize & CY_U3P_UIB_PACKET_SIZE_MASK);
    }
    entry[1] |= ((cfg->pollingRate << CY_U3P_UIB_EHCI1_POLLING_RATE_POS)
        & CY_U3P_UIB_EHCI1_POLLING_RATE_MASK);
    entry[1] |= (1 << CY_U3P_UIB_EHCI1_RESP_RATE_POS);
    if (cfg->isStreamMode)
    {
        entry[1] |= CY_U3P_UIB_OHCI1_BYPASS_ERROR;
    }
    entry[2] = 0;
    if (cfg->isStreamMode)
    {
        entry[2] |= CY_U3P_UIB_EHCI2_TRNS_MODE;
    }
}

/* Create and configure EP. */
CyU3PReturnStatus_t
CyU3PUsbHostEpAdd (
        uint8_t ep,
        CyU3PUsbHostEpConfig_t *cfg)
{
    CyBool_t isAsync;
    CyBool_t isUpper;
    CyBool_t isFirstTime;
    uint32_t *standby, *active;
    uint32_t entry[CY_U3P_USB_HOST_SHDL_ENTRY_SIZE], temp;
    CyU3PReturnStatus_t status;

    if (glUsbHostPortStatus != CY_U3P_USB_HOST_PORT_STAT_ACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    temp = 1 << (ep & 0x0F);
    if (ep & 0x80)
    {
        temp <<= 16;
    }
    if (ep == 0)
    {
        temp |= (1 << 16);
    }
    if (UIB->host_ep_intr_mask & temp)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }
    if (cfg == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    isAsync = ((cfg->type == CY_U3P_USB_EP_CONTROL) ||
            (cfg->type == CY_U3P_USB_EP_BULK)) ? CyTrue : CyFalse;

    /* Parameter validation. */
    if (((ep & 0x7F) > 0x0F) || (ep == 0x80))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (cfg->type > CY_U3P_USB_EP_INTR)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((cfg->mult == 0) || ((cfg->mult > 1) && (isAsync)) ||
            ((cfg->mult > 3) && (!isAsync)))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (cfg->fullPktSize < cfg->maxPktSize)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (((ep & 0x80) == 0) && (cfg->isStreamMode))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Create EP0 dma channel if required. */
    if ((ep == 0) && (!glUsbHostCfg.ep0LowLevelControl))
    {
        if (cfg->maxPktSize != cfg->fullPktSize)
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
        status = CyU3PMyHostEp0DmaConfig (CyTrue);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }
    }

    /* Make sure that both the periodic and async queues use the same
     * memory region. They can be different only if this is the first
     * EP initialization. */
    if (((UIB->host_shdl_cs & CY_U3P_UIB_PERI_SHDL_STATUS) &&
            ((UIB->host_shdl_cs & CY_U3P_UIB_ASYNC_SHDL_STATUS) == 0)) ||
            (((UIB->host_shdl_cs & CY_U3P_UIB_PERI_SHDL_STATUS) == 0) &&
             (UIB->host_shdl_cs & CY_U3P_UIB_ASYNC_SHDL_STATUS)))
    {
        UIB->host_shdl_cs |= (CY_U3P_UIB_PERI_SHDL_CHNG);
        while (UIB->host_shdl_cs & CY_U3P_UIB_PERI_SHDL_CHNG);
    }
    /* Identify the standby scheduler memory. */
    if (UIB->host_shdl_cs & CY_U3P_UIB_ASYNC_SHDL_STATUS)
    {
        standby = (uint32_t *)UIB->ehci_shdl;
        active = (uint32_t *)&UIB->ehci_shdl[32];
        isUpper = CyFalse;
    }
    else
    {
        standby = (uint32_t *)&UIB->ehci_shdl[32];
        active = (uint32_t *)UIB->ehci_shdl;
        isUpper = CyTrue;
    }

    /* Format scheduler entry. */
    CyU3PUsbHostFormatEntry (ep, cfg, entry);

    if (((!isAsync) && (glPeriodEpCount == 0)) ||
            ((isAsync) && (glAsyncEpCount == 0)))
    {
        isFirstTime = CyTrue;
    }
    else
    {
        isFirstTime = CyFalse;
    }

    /* The standby memory region should be recreated from the active
     * region. New EPs are always added at the beginning of the list
     * and the existing entries are then copied over. */

    /* Periodic EP. */
    if (!isAsync)
    {
        /* Load the new scheduler entry. */
        CyU3PMemCopy32 (standby, entry, CY_U3P_USB_HOST_SHDL_ENTRY_SIZE);
        standby += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
    }
    /* Copy the existing periodic scheduler entries. */
    if (glPeriodEpCount)
    {
        CyU3PMemCopy32 (standby, active, glPeriodEpCount *
                CY_U3P_USB_HOST_SHDL_ENTRY_SIZE);
        standby += glPeriodEpCount * CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
        active += glPeriodEpCount * CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
    }
    /* Async EP. */
    if (isAsync)
    {
        /* Load the new scheduler entry. */
        CyU3PMemCopy32 (standby, entry, CY_U3P_USB_HOST_SHDL_ENTRY_SIZE);
        standby += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
    }
    /* Copy the existing async scheduler entries. */
    if (glAsyncEpCount)
    {
        CyU3PMemCopy32 (standby, active, glAsyncEpCount *
                CY_U3P_USB_HOST_SHDL_ENTRY_SIZE);
    }

    /* Update the count. */
    if (isAsync)
    {
        glAsyncEpCount++;
    }
    else
    {
        glPeriodEpCount++;
    }

    /* Now update the entry pointer for all EPs. */
    standby = (isUpper) ? (uint32_t *)&UIB->ehci_shdl[32] : (uint32_t *)UIB->ehci_shdl;
    for (temp = 0; temp < (glAsyncEpCount + glPeriodEpCount); temp++)
    {
        glHostEpEntry[(*standby) & 0x1F] = standby;
        standby+= CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
    }

    /* Make sure all memory writes completed. */
    CyU3PSysBarrierSync ();

    /* Update the scheduler entry pointer. */
    temp = UIB->host_shdl_cs;
    if (isUpper)
    {
        temp &= ~CY_U3P_UIB_BULK_CNTRL_PTR1_MASK;
        temp |= (((glPeriodEpCount + 32) * CY_U3P_USB_HOST_SHDL_ENTRY_SIZE)
                << CY_U3P_UIB_BULK_CNTRL_PTR1_POS);
    }
    else
    {
        temp &= ~CY_U3P_UIB_BULK_CNTRL_PTR0_MASK;
        temp |= ((glPeriodEpCount * CY_U3P_USB_HOST_SHDL_ENTRY_SIZE)
                << CY_U3P_UIB_BULK_CNTRL_PTR0_POS);
    }
    UIB->host_shdl_cs = temp;

    /* Change the schduler memory to the standby region. */
    UIB->host_shdl_cs |= (CY_U3P_UIB_ASYNC_SHDL_CHNG | CY_U3P_UIB_PERI_SHDL_CHNG);
    while (UIB->host_shdl_cs & (CY_U3P_UIB_PERI_SHDL_CHNG |
                CY_U3P_UIB_ASYNC_SHDL_CHNG));

    /* Enable the scheduler. */
    if (isFirstTime)
    {
        if (glUsbHostSpeed == CY_U3P_USB_HOST_HIGH_SPEED)
        {
            UIB->ehci_usbcmd |= (isAsync) ? CY_U3P_UIB_EHCI_USBCMD_ASYNC_SHDL_EN :
                CY_U3P_UIB_EHCI_USBCMD_PER_SHDL_EN;
        }
        else
        {
            UIB->ohci_control |= (isAsync) ? (CY_U3P_UIB_CLE | CY_U3P_UIB_BLE) :
                (CY_U3P_UIB_PLE | CY_U3P_UIB_IE);
        }
    }

    /* Enable interrupts for the EP. */
    temp = 1 << (ep & 0x0F);
    if (ep & 0x80)
    {
        temp <<= 16;
    }
    if (ep == 0)
    {
        temp |= (1 << 16);
    }

    UIB->host_ep_intr = temp;
    if (!cfg->isStreamMode)
    {
        UIB->host_ep_intr_mask |= temp;
    }
    glHostEpConfig |= temp;
    
    return CY_U3P_SUCCESS;
}

/* Reset EP. */
CyU3PReturnStatus_t
CyU3PUsbHostEpReset (
        uint8_t ep)
{
    uint32_t tmp;
    uint32_t epMask;
    uint8_t epIndex = (ep & 0x0F);

    if (glUsbHostPortStatus != CY_U3P_USB_HOST_PORT_STAT_ACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }
    if (((ep & 0x7F) > 0x0F) || (ep == 0x80))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    epMask = 1 << (ep & 0x0F);
    if (ep & 0x80)
    {
        epMask <<= 16;
    }
    if (ep == 0)
    {
        epMask |= (1 << 16);
    }

    if (glHostPendingEpXfer & epMask)
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    if (!(glHostEpConfig & epMask))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    /* Reset the Data toggle to 0 */
    if (epIndex != 0)
    {
        tmp = epIndex;
        /* Check the endpoint direction. */
        if (ep & 0x80)
        {
            /* Set the direction */
            tmp |= CY_U3P_UIB_IO;
        }
        UIB->host_toggle = tmp;
        while (!(UIB->host_toggle & CY_U3P_UIB_TOGGLE_VALID));
        tmp |= CY_U3P_UIB_R;
        UIB->host_toggle = tmp;
        while (!(UIB->host_toggle & CY_U3P_UIB_TOGGLE_VALID));

        tmp = 0;
        UIB->host_toggle = tmp;
        while (!(UIB->host_toggle & CY_U3P_UIB_TOGGLE_VALID));
    }

    /* Flush the EPM. */
    if ((ep & 0x80) == 0)
    {
        UIB->eepm_endpoint[epIndex] |= CY_U3P_UIB_SOCKET_FLUSH;
        CyU3PBusyWait (1);
        UIB->eepm_endpoint[epIndex] &= ~CY_U3P_UIB_SOCKET_FLUSH;
    }
    if ((ep & 0x80) || (ep == 0))
    {
        UIB->iepm_endpoint[epIndex] |= CY_U3P_UIB_SOCKET_FLUSH;
        CyU3PBusyWait (1);
        UIB->iepm_endpoint[epIndex] &= ~CY_U3P_UIB_SOCKET_FLUSH;
    }

    return CY_U3P_SUCCESS;
}

/* Destroy and disable EP. */
CyU3PReturnStatus_t
CyU3PUsbHostEpRemove (
        uint8_t ep)
{
    CyBool_t isAsync;
    CyBool_t isUpper;
    uint32_t *standby, *active;
    uint32_t temp;
    uint32_t status;
    uint8_t epIndex;

    if (glUsbHostPortStatus != CY_U3P_USB_HOST_PORT_STAT_ACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }
    if (((ep & 0x7F) > 0x0F) || (ep == 0x80))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    status = CyU3PUsbHostEpFindEntry (ep, &active);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Remove the EP0 channels if required. */
    if ((ep == 0) && (!glUsbHostCfg.ep0LowLevelControl))
    {
        CyU3PMyHostEp0DmaConfig (CyFalse);
    }

    temp = ((*active & CY_U3P_UIB_EHCI0_EPT_MASK) >>
        CY_U3P_UIB_EHCI0_EPT_POS);
    isAsync = ((temp == CY_U3P_USB_EP_CONTROL) ||
            (temp == CY_U3P_USB_EP_BULK)) ? CyTrue : CyFalse;

    /* Identify the standby scheduler memory. */
    if (UIB->host_shdl_cs & CY_U3P_UIB_ASYNC_SHDL_STATUS)
    {
        standby = (uint32_t *)UIB->ehci_shdl;
        active = (uint32_t *)&UIB->ehci_shdl[32];
        isUpper = CyFalse;
    }
    else
    {
        standby = (uint32_t *)&UIB->ehci_shdl[32];
        active = (uint32_t *)UIB->ehci_shdl;
        isUpper = CyTrue;
    }

    epIndex = (ep & 0x0F);
    if ((ep != 0) && (!(ep & 0x80)))
    {
        /* OutEp. */
        epIndex |= 0x10;
    }

    /* Disable interrupts for the EP and then disable it. */
    temp = 1 << (ep & 0x0F);
    if (ep & 0x80)
    {
        temp <<= 16;
    }
    if (ep == 0)
    {
        temp |= (1 << 16);
    }
    UIB->host_ep_intr_mask &= ~temp;
    UIB->host_active_ep &= ~temp;
    UIB->host_ep_intr = temp;
    glHostPendingEpXfer &= ~temp;
    glHostEpConfig &= ~temp;

    /* The standby memory region should be recreated from the active
     * region. Existing entries are copied to standby memory and required EP
     * is removed. If this is a periodic EP, then the Async pointer must be
     * decremented. If this is the last EP in the list, then the last bit
     * must be moved to the just previous one. If this is the only EP
     * in either periodic or async, the corresponding scheduler must be
     * disabled. */
    if (isAsync)
    {
        /* Copy the periodic list completely. */
        for (temp = 0; temp < glPeriodEpCount; temp++)
        {
            CyU3PMemCopy32 (standby, active, CY_U3P_USB_HOST_SHDL_ENTRY_SIZE);
            active += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
            standby += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
        }
        /* Now copy the async list except for the corresponding entry. */
        for (temp = 0; temp < glAsyncEpCount; temp++)
        {
            if ((*active & CY_U3P_UIB_EHCI0_EPND_MASK) == epIndex)
            {
                /* Check if this is the last entry. */
                if ((temp == (glAsyncEpCount - 1)) && (temp != 0))
                {
                    *(standby - CY_U3P_USB_HOST_SHDL_ENTRY_SIZE) |= CY_U3P_UIB_EHCI0_T;
                }

                active += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
                continue;
            }
            CyU3PMemCopy32 (standby, active, CY_U3P_USB_HOST_SHDL_ENTRY_SIZE);
            active += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
            standby += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
        }

        /* Decrement the async EP count. */
        glAsyncEpCount--;
    }
    else
    {
        /* Copy the periodic list except for the corresponding entry. */
        for (temp = 0; temp < glPeriodEpCount; temp++)
        {
            if ((*active & CY_U3P_UIB_EHCI0_EPND_MASK) == epIndex)
            {
                /* Check if this is the last entry. */
                if ((temp == (glPeriodEpCount - 1)) && temp != 0)
                {
                    *(standby - CY_U3P_USB_HOST_SHDL_ENTRY_SIZE) |= CY_U3P_UIB_EHCI0_T;
                }

                active += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
                continue;
            }
            CyU3PMemCopy32 (standby, active, CY_U3P_USB_HOST_SHDL_ENTRY_SIZE);
            active += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
            standby += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
        }
        /* Copy the async list completely. */
        for (temp = 0; temp < glAsyncEpCount; temp++)
        {
            CyU3PMemCopy32 (standby, active, CY_U3P_USB_HOST_SHDL_ENTRY_SIZE);
            active += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
            standby += CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
        }

        /* Decrement the periodic EP count. */
        glPeriodEpCount--;
    }

    /* Now update the entry pointer for all EPs. */
    standby = (isUpper) ? (uint32_t *)&UIB->ehci_shdl[32] : (uint32_t *)UIB->ehci_shdl;
    for (temp = 0; temp < (glAsyncEpCount + glPeriodEpCount); temp++)
    {
        glHostEpEntry[(*standby) & 0x1F] = standby;
        standby+= CY_U3P_USB_HOST_SHDL_ENTRY_SIZE;
    }

    /* Clear the entry pointer. */
    glHostEpEntry[epIndex] = 0;

    /* Disable scheduler operation if the corresponding count has reached zero. */
    if (glPeriodEpCount == 0)
    {
        if (glUsbHostSpeed == CY_U3P_USB_HOST_HIGH_SPEED)
        {
            UIB->ehci_usbcmd &= ~CY_U3P_UIB_EHCI_USBCMD_PER_SHDL_EN;
        }
        else
        {
            UIB->ohci_control &= ~(CY_U3P_UIB_PLE | CY_U3P_UIB_IE);
        }

    }
    if (glAsyncEpCount == 0)
    {
        if (glUsbHostSpeed == CY_U3P_USB_HOST_HIGH_SPEED)
        {
            UIB->ehci_usbcmd &= ~CY_U3P_UIB_EHCI_USBCMD_ASYNC_SHDL_EN;
        }
        else
        {
            UIB->ohci_control &= ~(CY_U3P_UIB_CLE | CY_U3P_UIB_BLE);
        }
    }

    /* Make sure all memory writes completed. */
    CyU3PSysBarrierSync ();

    /* Update the scheduler entry pointer. */
    temp = UIB->host_shdl_cs;
    if (isUpper)
    {
        temp &= ~CY_U3P_UIB_BULK_CNTRL_PTR1_MASK;
        temp |= (((glPeriodEpCount + 32) * CY_U3P_USB_HOST_SHDL_ENTRY_SIZE)
                << CY_U3P_UIB_BULK_CNTRL_PTR1_POS);
    }
    else
    {
        temp &= ~CY_U3P_UIB_BULK_CNTRL_PTR0_MASK;
        temp |= ((glPeriodEpCount * CY_U3P_USB_HOST_SHDL_ENTRY_SIZE)
                << CY_U3P_UIB_BULK_CNTRL_PTR0_POS);
    }
    UIB->host_shdl_cs = temp;

    /* Make sure all memory writes completed. */
    CyU3PSysBarrierSync ();

    /* Change the schduler memory to the standby region. */
    UIB->host_shdl_cs |= (CY_U3P_UIB_ASYNC_SHDL_CHNG | CY_U3P_UIB_PERI_SHDL_CHNG);
    while (UIB->host_shdl_cs & (CY_U3P_UIB_PERI_SHDL_CHNG |
                CY_U3P_UIB_ASYNC_SHDL_CHNG));

    return CY_U3P_SUCCESS;
}

/* Set transfer. */
CyU3PReturnStatus_t
CyU3PUsbHostEpSetXfer (
        uint8_t ep,
        CyU3PUsbHostEpXferType_t type,
        uint32_t count)
{
    uint32_t epMask;
    uint32_t temp;
    uint32_t *entry;
    CyU3PReturnStatus_t status;

    temp = (ep & 0x0F);
    epMask = 1 << temp;
    if (ep & 0x80)
    {
        epMask <<= 16;
    }
    /* For EP0 both IN and OUT bits must be used. */
    if (ep == 0)
    {
        epMask |= (1 << 16);
    }

    if (glUsbHostPortStatus != CY_U3P_USB_HOST_PORT_STAT_ACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }
    if (((ep & 0x7F) > 0x0F) || (ep == 0x80))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((UIB->host_active_ep & epMask) ||
            (glHostPendingEpXfer & epMask))
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }
    /* Check for parameter validity. */
    if (((ep != 0) && (type != CY_U3P_USB_HOST_EPXFER_NORMAL)) ||
            ((ep == 0) && (type == CY_U3P_USB_HOST_EPXFER_NORMAL)))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    status = CyU3PUsbHostEpFindEntry (ep, &entry);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Clear the event flags. */
    CyU3PEventSet (&glHostEpEvent, ~epMask, CYU3P_EVENT_AND);

    /* Update the scheduler memory. */
    /* Clear the EP HALT and ZLPEN bits. */
    entry[0] &= ~(CY_U3P_UIB_EHCI0_HALT | CY_U3P_UIB_EHCI0_ZPLEN);
    /* ZPLEN should be turned on for activating ZLP transfer
     * when the transfer count is zero. This should not be set
     * for ISO / control endpoints or stream mode of operation. */
    temp = (entry[0] & CY_U3P_UIB_EHCI0_EPT_MASK) >> CY_U3P_UIB_EHCI0_EPT_POS;
    if ((count == 0) && ((entry[2] & CY_U3P_UIB_EHCI2_TRNS_MODE) == 0)
            && (temp != CY_U3P_USB_EP_ISO) && (temp != CY_U3P_USB_EP_CONTROL))
    {
        entry[0] |= CY_U3P_UIB_EHCI0_ZPLEN;
    }
    temp = entry[1];
    temp &= ~CY_U3P_UIB_EHCI1_EP0_CODE_MASK;
    temp |= ((type << CY_U3P_UIB_EHCI1_EP0_CODE_POS)
            & CY_U3P_UIB_EHCI1_EP0_CODE_MASK);
    entry[1] = temp;
    entry[2] &= ~CY_U3P_UIB_EHCI2_TOTAL_BYTE_COUNT_MASK;
    entry[2] |= (count << CY_U3P_UIB_EHCI2_TOTAL_BYTE_COUNT_POS);

    /* Clear the previous status. */
    temp = (ep & 0x0F);
    if ((ep != 0) && (!(ep & 0x80)))
    {
        /* OutEp. */
        temp |= 0x10;
    }
    glHostEpStatus[temp] = 0;

    CyU3PSysBarrierSync ();

    /* EP0 requires that the socket should be active
     * and have at least the setup packet queued
     * before enabling the EP. So additional API call
     * is required to activate EP0. Other EPs are 
     * activated at the end of the call. */
    glHostPendingEpXfer |= epMask;
    if (ep != 0)
    {
        /* Enable the EP. */
        UIB->host_active_ep |= epMask;
    }

    return status;
}

CyU3PReturnStatus_t
CyU3PUsbHostSendSetupRqt (
        uint8_t *setupPkt,
        uint8_t *data_p)
{
    uint16_t size;
    uint32_t status = CY_U3P_SUCCESS;
    CyU3PUsbHostEpXferType_t type;
    CyU3PDmaBuffer_t buf_p;

    if (glUsbHostCfg.ep0LowLevelControl)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }
    if (glUsbHostPortStatus != CY_U3P_USB_HOST_PORT_STAT_ACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }
    /* Check for parameter validity. */
    if (setupPkt == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    /* Initialize tracker variables. */
    glHostEp0Phase = 0;
    glHostSetupPkt = setupPkt;
    glHostEp0Data = data_p;

    type = (glHostSetupPkt[0] & 0x80) ? CY_U3P_USB_HOST_EPXFER_SETUP_IN_DATA :
        CY_U3P_USB_HOST_EPXFER_SETUP_OUT_DATA;
    size = CY_U3P_MAKEWORD(glHostSetupPkt[7], glHostSetupPkt[6]);
    if (size == 0)
    {
        type = CY_U3P_USB_HOST_EPXFER_SETUP_NO_DATA;
    }
    else if (data_p == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    /* Clean any previous errors in the pipe. */
    CyU3PDmaChannelReset (&glHostEp0InChannel);
    status = CyU3PDmaChannelWaitForCompletion (&glHostEp0OutChannel, CYU3P_NO_WAIT);
    if ((status != CY_U3P_SUCCESS) && (status != CY_U3P_ERROR_NOT_STARTED))
    {
        CyU3PDmaChannelReset (&glHostEp0OutChannel);
    }

    status = CyU3PUsbHostEpSetXfer (0, type, size);

    if (status == CY_U3P_SUCCESS)
    {
        buf_p.buffer = glHostSetupPkt;
        buf_p.count = 8;
        buf_p.size  = 32;
        buf_p.status = 0;
        status = CyU3PDmaChannelSetupSendBuffer (&glHostEp0OutChannel, &buf_p);
        if (status != CY_U3P_SUCCESS)
        {
            status = CY_U3P_ERROR_DMA_FAILURE;
        }
    }
    if (status == CY_U3P_SUCCESS)
    {
        glHostEp0Phase = 1;
        status = CyU3PUsbHostEp0BeginXfer ();
    }

    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDmaChannelReset (&glHostEp0OutChannel);
        CyU3PDmaChannelReset (&glHostEp0InChannel);
    }

    return status;
}

CyU3PReturnStatus_t
CyU3PUsbHostEp0BeginXfer (
        void)
{
    uint32_t epMask = 0x00010001;

    if (glUsbHostPortStatus != CY_U3P_USB_HOST_PORT_STAT_ACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if ((UIB->host_active_ep & epMask) ||
            ((glHostPendingEpXfer & epMask) == 0))
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    UIB->host_active_ep |= epMask;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PUsbHostEpWaitForCompletion (
        uint8_t ep, 
        CyU3PUsbHostEpStatus_t *epStatus,
        uint32_t waitOption)
{
    uint32_t status;
    uint32_t temp, epMask;

    if (glUsbHostPortStatus != CY_U3P_USB_HOST_PORT_STAT_ACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }
    if (((ep & 0x7F) > 0x0F) || (ep == 0x80))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    temp = (ep & 0x0F);
    epMask = 1 << temp;
    if (ep & 0x80)
    {
        epMask <<= 16;
    }

    if (ep == 0)
    {
        epMask = 0x00010001;
    }

    if (!(glHostEpConfig & epMask))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    /* Check for cases with no active transfer. */
    if ((glHostPendingEpXfer & epMask) == 0)
    {
        status = CyU3PEventGet (&glHostEpEvent, epMask, CYU3P_EVENT_AND, 
            &temp, CYU3P_NO_WAIT);
        if (status != CY_U3P_SUCCESS)
        {
            return CY_U3P_ERROR_INVALID_SEQUENCE;
        }
    }
    else
    {
        status = CyU3PEventGet (&glHostEpEvent, epMask, CYU3P_EVENT_AND, 
                &temp, waitOption);
    }

    temp = ((ep == 0) || (ep & 0x80)) ? glHostEpStatus[ep & 0x0F] :
        glHostEpStatus[ep + 16];

    if (status != CY_U3P_SUCCESS)
    {
        status = CY_U3P_ERROR_TIMEOUT;
    }

    if (epStatus != NULL)
    {
        *epStatus = temp;
    }

    if ((status == CY_U3P_SUCCESS) && (temp & CY_U3P_USB_HOST_EPS_HALT))
    {
        status = CY_U3P_ERROR_STALLED;
    }

    return status;
}

/* Read and update the response memory. This function is not
 * re-entrant and so should not be nested or called from
 * multiple threads. This is because glHostRespIndex is read
 * and updated here. */
static void
CyU3PUsbHostUpdateResponse (
        void)
{
    uint8_t  epIndex;
    uint16_t index;
    uint32_t *curPtr, *endPtr;

    /* There can be two senerios:
     * 1. The start index is less than the current index.
     * In this case we need to read from start to current.
     * 2. The start index is greater than the current index.
     * In this case we need to read from start index to
     * end and then from beginning to current index. */
    index = ((UIB->host_resp_cs & CY_U3P_UIB_WR_PTR_MASK) >>
            CY_U3P_UIB_WR_PTR_POS);

    if (index == glHostRespIndex)
    {
        /* No entry has been created. Just return. */
        return;
    }

    if (index > glHostRespIndex)
    {
        endPtr = glHostResp + index;
    }
    else
    {
        endPtr = glHostResp + CY_U3P_USB_HOST_SDHL_RESP_MAX_ENTRY;
    }

    for (curPtr = glHostResp + glHostRespIndex; curPtr < endPtr; curPtr++)
    {
        epIndex = ((*curPtr) & CY_U3P_UIB_EHCI0_EPND_MASK);
        if (epIndex == 0x10)
        {
            epIndex = 0;
        }
        glHostEpStatus[epIndex] = *curPtr;
    }

    if (index < glHostRespIndex)
    {
        endPtr = glHostResp + index;
        for (curPtr = glHostResp; curPtr < endPtr; curPtr++)
        {
            epIndex = ((*curPtr) & CY_U3P_UIB_EHCI0_EPND_MASK);
            if (epIndex == 0x10)
            {
                epIndex = 0;
            }
            glHostEpStatus[epIndex] = *curPtr;
        }
    }

    /* Update the current index variable. */
    glHostRespIndex = index;
}

/* Abort transfer. */
CyU3PReturnStatus_t
CyU3PUsbHostEpAbort (
        uint8_t ep)
{
    uint32_t epMask;
    uint32_t *entry;
    uint32_t status = CY_U3P_SUCCESS;

    if (glUsbHostPortStatus != CY_U3P_USB_HOST_PORT_STAT_ACTIVE)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }
    if (((ep & 0x7F) > 0x0F) || (ep == 0x80))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    epMask = 1 << (ep & 0x0F);
    if (ep & 0x80)
    {
        epMask <<= 16;
    }
    if (ep == 0)
    {
        epMask |= (1 << 16);
    }

    /* Disable the EP and mark the scheduler entry as halted. */
    UIB->host_active_ep &= ~epMask;
    glHostPendingEpXfer &= ~epMask;
    status = CyU3PUsbHostEpFindEntry (ep, &entry);
    if (status == CY_U3P_SUCCESS)
    {
        *entry |= CY_U3P_UIB_EHCI0_HALT;
    }

    return status;
}

/* Event handler functions. */
void
CyU3PUsbHostIntHandler (
        void)
{
    uint8_t index;
    uint32_t val;

    /* The only interrupt defined is the response limit error. */
    if (UIB->host_resp_cs & CY_U3P_UIB_LIM_ERROR)
    {
        /* Update the EP status information. */
        CyU3PUsbHostUpdateResponse ();

        /* Update the response limit for the next interrupt. */
        index = ((UIB->host_resp_cs & CY_U3P_UIB_WR_PTR_MASK) >>
                CY_U3P_UIB_WR_PTR_POS);

        val = (UIB->host_resp_cs & ~(CY_U3P_UIB_LIMIT_MASK | CY_U3P_UIB_LIM_ERROR));
        if (index < (CY_U3P_USB_HOST_SDHL_RESP_MAX_ENTRY / 2))
        {
            val |= ((CY_U3P_USB_HOST_SDHL_RESP_MAX_ENTRY / 2) << CY_U3P_UIB_LIMIT_POS);
        }
        else
        {
            val |= (CY_U3P_USB_HOST_SDHL_RESP_MAX_ENTRY << CY_U3P_UIB_LIMIT_POS);
        }

        UIB->host_resp_cs = val;
    }

    UIB->intr_mask |= CY_U3P_UIB_HOST_INT;
}

void
CyU3PUsbHostEpIntHandler (
        void)
{
    uint8_t i = 0, epIndex = 0;
    uint32_t state;

    state = UIB->host_ep_intr & UIB->host_ep_intr_mask;
    UIB->host_ep_intr = state;
    /* Disable the EPs. */
    UIB->host_active_ep &= ~state;
    /* Clear pending flags. */
    glHostPendingEpXfer &= ~state;

    /* Update response data. */
    CyU3PUsbHostUpdateResponse ();

    /* Scan EP0. */
    if (state & 0x00010001)
    {
        /* HACK: Due to hardware fault, the stall condition
         * has to be retrieved from the scheduler memory. */
        if ((*glHostEpEntry[0]) & CY_U3P_UIB_EHCI0_HALT)
        {
            glHostEpStatus[0] |= CY_U3P_USB_HOST_EPS_HALT;
            if (!glUsbHostCfg.ep0LowLevelControl)
            {
                CyU3PDmaChannelReset (&glHostEp0OutChannel);
                CyU3PDmaChannelReset (&glHostEp0InChannel);
                CyU3PUsbHostEpReset (0);
            }
        }
        if (glUsbHostCfg.xferCb != NULL)
        {
            glUsbHostCfg.xferCb (i, glHostEpStatus[0]);
        }
    }

    /* Scan OUT EPs. */
    for (i = 1; i < 16; i++)
    {
        if (state & (1 << i))
        {
            epIndex = 16 + i;
            /* HACK: Due to hardware fault, the stall condition
             * has to be retrieved from the scheduler memory. */
            if ((*glHostEpEntry[epIndex]) & CY_U3P_UIB_EHCI0_HALT)
            {
                glHostEpStatus[epIndex] |= CY_U3P_USB_HOST_EPS_HALT;
            }
            if (glUsbHostCfg.xferCb != NULL)
            {
                glUsbHostCfg.xferCb (i, glHostEpStatus[epIndex]);
            }
        }
    }
    /* Scan IN EPs. */
    for (i = 17; i < 32; i++)
    {
        if (state & (1 << i))
        {
            epIndex = i - 16;
            /* HACK: Due to hardware fault, the stall condition
             * has to be retrieved from the scheduler memory. */
            if (*glHostEpEntry[epIndex] & CY_U3P_UIB_EHCI0_HALT)
            {
                glHostEpStatus[epIndex] |= CY_U3P_USB_HOST_EPS_HALT;
            }

            if (glUsbHostCfg.xferCb != NULL)
            {
                glUsbHostCfg.xferCb ((0x80 | epIndex), glHostEpStatus[epIndex]);
            }
        }
    }

    /* Set event to wake sleeping threads. */
    CyU3PEventSet (&glHostEpEvent, state, CYU3P_EVENT_OR);

    /* Re-enable the EP interrupts. */
    UIB->intr_mask |= CY_U3P_UIB_HOST_EP_INT;
}

void
CyU3PUsbHostEhciIntHandler (
        void)
{
    uint32_t state;

    state = UIB->ehci_usbsts & UIB->ehci_usbintr;
    /* Clear the interrupts and re-enable the EHCI interrupts. */
    UIB->ehci_usbsts |= state;
    UIB->intr_mask |= CY_U3P_UIB_EHCI_INT;

    if (state & CY_U3P_UIB_PORT_CHANGE_DET_IE)
    {
        /* Identify whether this is a connect or a disconnect. */
        if (UIB->ehci_portsc & CY_U3P_UIB_PORT_CONNECT)
        {
            glUsbHostPortStatus |= CY_U3P_USB_HOST_PORT_STAT_CONNECTED;
            if (glUsbHostCfg.eventCb)
            {
                glUsbHostCfg.eventCb (CY_U3P_USB_HOST_EVENT_CONNECT, 0);
            }
        }
        else
        {
            glUsbHostPortStatus &= ~CY_U3P_USB_HOST_PORT_STAT_CONNECTED;
            /* We are chosing to disable the port when-ever there
             * is a disconnect. This means that the port will revert
             * to EHCI control on disconnect. */
            CyU3PThreadSleep (1);
            CyU3PUsbHostPortDisable ();
            if (glUsbHostCfg.eventCb)
            {
                glUsbHostCfg.eventCb (CY_U3P_USB_HOST_EVENT_DISCONNECT, 0);
            }
        }
    }
}

void
CyU3PUsbHostOhciIntHandler (
        void)
{
    uint32_t state, portStatus;

    state = UIB->ohci_interrupt_status & UIB->ohci_interrupt_enable;
    /* Clear all OHCI interrupts. */
    portStatus = UIB->ohci_rh_port_status;
    UIB->ohci_rh_port_status |= CY_U3P_UIB_RHP_CSC;
    UIB->intr_mask |= CY_U3P_UIB_OHCI_INT;

    /* Since the port connection detection is always handled by EHCI
     * controller, we will reach here only if this is an OHCI disconnect. */
    if ((state & CY_U3P_UIB_RHSC) && (portStatus & CY_U3P_UIB_RHP_CSC))
    {
        if (portStatus & CY_U3P_UIB_RHP_CCS)
        {
            glUsbHostPortStatus |= CY_U3P_USB_HOST_PORT_STAT_CONNECTED;
            if (glUsbHostCfg.eventCb)
            {
                glUsbHostCfg.eventCb (CY_U3P_USB_HOST_EVENT_CONNECT, 0);
            }
        }
        else
        {
            glUsbHostPortStatus &= ~CY_U3P_USB_HOST_PORT_STAT_CONNECTED;
            /* We are chosing to disable the port when-ever there
             * is a disconnect. This means that the port will revert
             * to EHCI control on disconnect. */
            CyU3PUsbHostPortDisable ();
            if (glUsbHostCfg.eventCb)
            {
                glUsbHostCfg.eventCb (CY_U3P_USB_HOST_EVENT_DISCONNECT, 0);
            }
        }
    }
}

/* [] */

