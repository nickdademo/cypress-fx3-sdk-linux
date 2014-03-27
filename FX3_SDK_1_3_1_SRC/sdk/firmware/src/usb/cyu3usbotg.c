/*
 ## Cypress USB 3.0 Platform source file (cyu3usbotg.c)
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

/* This file defines the USB OTG mode APIs.
 */
#include <cyu3types.h>
#include <cyu3error.h>
#include <cyu3system.h>
#include <cyu3vic.h>
#include <cyu3usb.h>
#include <cyu3usbpp.h>
#include <cyu3usbhost.h>
#include <cyu3usbotg.h>
#include <cyu3lpp.h>
#include <cyu3regs.h>
#include <cyu3utils.h>

#include <cyfx3_api.h>

extern CyBool_t                 glIsOtgEnable;          /* Whether the OTG module has been started. */
extern CyBool_t                 glIsHnpEnable;
extern CyU3POtgPeripheralType_t glPeripheralType;
extern CyU3POtgConfig_t         glOtgInfo;
extern uint8_t glLppActive;

static uint32_t glOtgTimerPeriod = 0;

extern void
CyU3PUsbSetOTGIntHandler (
        void (*func_p) (void));
extern void
CyU3PUsbSetChgDetIntHandler (
        void (*func_p) (void));
extern void
CyU3PUsbPowerOn (
        void);

CyU3PReturnStatus_t
CyU3POtgStart (
        CyU3POtgConfig_t *cfg)
{
    /* Verify that the part in use supports the USB OTG functionality. */
    if (!CyFx3DevIsOtgSupported ())
        return CY_U3P_ERROR_NOT_SUPPORTED;

    if (glIsOtgEnable)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    /* This call is not allowed if the device mode is already active. */
    if ((CyU3PUsbIsStarted ()) || (glSdk_UsbIsOn))
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    /* Check for parameter validity. */
    if (cfg == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if (cfg->otgMode >= CY_U3P_OTG_NUM_MODES)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((cfg->cb == NULL) && (cfg->otgMode != CY_U3P_OTG_MODE_CARKIT_PPORT) &&
                (cfg->otgMode != CY_U3P_OTG_MODE_CARKIT_UART))
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if (cfg->chargerMode >= CY_U3P_OTG_CHARGER_DETECT_NUM_MODES)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Register the handlers for OTG interrupt events. */
    CyU3PUsbSetOTGIntHandler (CyU3PUsbOtgIntHandler);
    CyU3PUsbSetChgDetIntHandler (CyU3PUsbChgdetIntHandler);

    /* Reset the connected peripheral to invalid entry. */
    glPeripheralType = CY_U3P_OTG_TYPE_DISABLED;
    /* Reset the state machine variables. */
    glIsHnpEnable = CyFalse;

    if (cfg->otgMode == CY_U3P_OTG_MODE_OTG)
    {
        /* Enable the USB PHY connections. */
        GCTLAON->control &= ~CY_U3P_GCTL_ANALOG_SWITCH;

        /* Enable and set the EPM clock to bus clock (100MHz). */
        GCTL->uib_core_clk = (CY_U3P_GCTL_UIBCLK_CLK_EN | CY_U3P_GCTL_UIB_CORE_CLK_DEFAULT);
        GCTLAON->control |= CY_U3P_GCTL_USB_POWER_EN;
        CyU3PBusyWait (100);

        CyU3PUsbPowerOn ();

        /* Enable OTG and charger detection interrupts. */
        UIB->otg_intr = ~CY_U3P_UIB_OTG_INTR_DEFAULT;
        UIB->otg_intr_mask = CY_U3P_UIB_OTG_TIMER_TIMEOUT;
        UIB->chgdet_intr = ~CY_U3P_UIB_CHGDET_INTR_DEFAULT;
        UIB->chgdet_intr_mask = CY_U3P_UIB_OTG_ID_CHANGE;

        /* Enable OTG mode and charger detection. */
        UIB->otg_ctrl = (CY_U3P_UIB_OTG_ENABLE);

        /* Enable ID pin detection. */
	UIB->chgdet_ctrl = CY_U3P_UIB_ACA_ENABLE;

        /* Enable the UIB interrupts. */
        UIB->intr = ~CY_U3P_UIB_INTR_DEFAULT;
        UIB->intr_mask = (CY_U3P_UIB_OTG_INT | CY_U3P_UIB_CHGDET_INT);
        CyU3PVicEnableInt (CY_U3P_VIC_UIB_CORE_VECTOR);

        /* Enable the VBUS interrupts. */
        GCTL->iopwr_intr = ~CY_U3P_GCTL_IOPWR_INTR_DEFAULT;
        GCTL->iopwr_intr_mask = CY_U3P_VBUS;
        glUibDeviceInfo.vbusDetectMode = CY_U3P_VBUS;
        CyU3PVicEnableInt (CY_U3P_VIC_GCTL_PWR_VECTOR);
    }
    else if (cfg->otgMode == CY_U3P_OTG_MODE_CARKIT_PPORT)
    {
        /* Enable the USB PHY connections. */
        GCTLAON->control &= ~CY_U3P_GCTL_ANALOG_SWITCH;

        /* Enable and set the EPM clock to bus clock (100MHz). */
        GCTL->uib_core_clk = (CY_U3P_GCTL_UIBCLK_CLK_EN | CY_U3P_GCTL_UIB_CORE_CLK_DEFAULT);
        GCTLAON->control |= CY_U3P_GCTL_USB_POWER_EN;
        CyU3PBusyWait (100);

        CyU3PUsbPowerOn ();

        UIB->otg_ctrl = CY_U3P_UIB_OTG_CTRL_DEFAULT;
        GCTL->iomatrix |= CY_U3P_CARKIT;
        UIB->chgdet_ctrl = CY_U3P_UIB_CARKIT;
    }
    else if (cfg->otgMode == CY_U3P_OTG_MODE_CARKIT_UART)
    {
        /* Verify if the configuration is possible. */
        if ((CyFx3DevIOIsSib8BitWide (1)) || (CyFx3DevIOIsUartConfigured ()))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }

        /* Enable the USB PHY connections. */
        GCTLAON->control &= ~CY_U3P_GCTL_ANALOG_SWITCH;

        /* Enable and set the EPM clock to bus clock (100MHz). */
        GCTL->uib_core_clk = (CY_U3P_GCTL_UIBCLK_CLK_EN | CY_U3P_GCTL_UIB_CORE_CLK_DEFAULT);
        GCTLAON->control |= CY_U3P_GCTL_USB_POWER_EN;
        CyU3PBusyWait (100);

        CyU3PUsbPowerOn ();

        UIB->otg_ctrl = CY_U3P_UIB_OTG_CTRL_DEFAULT;
        GCTL->iomatrix &= ~CY_U3P_CARKIT;
        UIB->chgdet_ctrl = CY_U3P_UIB_CARKIT;
    }
    else
    {
        /* Do nothing. */
    }

    /* Save the current OTG configuration. */
    glOtgInfo = *cfg;
    glIsOtgEnable = CyTrue;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3POtgStop (void)
{
    if (!glIsOtgEnable)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Do not disable the block if any of the modes are running. */
    if ((CyU3PUsbIsStarted ()) || (CyU3PUsbHostIsStarted ()))
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    /* Disable all interrupts. */
    UIB->intr_mask = 0;
    CyU3PVicDisableInt (CY_U3P_VIC_UIB_CORE_VECTOR);
    CyU3PVicDisableInt (CY_U3P_VIC_UIB_DMA_VECTOR);

    /* Reset all state machine variables to default. */
    glOtgInfo.otgMode = CY_U3P_OTG_MODE_DEVICE_ONLY;
    glOtgInfo.chargerMode = CY_U3P_OTG_CHARGER_DETECT_ACA_MODE;
    glOtgInfo.cb = NULL;
    glPeripheralType = CY_U3P_OTG_TYPE_DISABLED;
    glIsHnpEnable = CyFalse;
    UIB->chgdet_ctrl = CY_U3P_UIB_CHGDET_CTRL_DEFAULT;
    UIB->otg_ctrl = CY_U3P_UIB_OTG_CTRL_DEFAULT;

    /* Disable the UIB block. */
    UIB->power &= ~CY_U3P_UIB_RESETN;
    CyU3PBusyWait (10);

    /* Disable the UIB clock. */
    GCTL->uib_core_clk &= ~CY_U3P_GCTL_UIBCLK_CLK_EN;
    GCTLAON->control &= ~CY_U3P_GCTL_USB_POWER_EN;

    /* Update the state variable. */
    glIsOtgEnable = CyFalse;

    return CY_U3P_SUCCESS;
}

static void
CyU3POtgSetupPhy (void)
{
    if (CyU3POtgIsHostMode ())
    {
        /* Enable and set the EPM clock to bus clock (100MHz). */
        GCTL->uib_core_clk = (CY_U3P_GCTL_UIBCLK_CLK_EN | CY_U3P_GCTL_UIB_CORE_CLK_DEFAULT);

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

        /* Make sure EHCI is the owner of the port. */
        UIB->ehci_configflag = CY_U3P_UIB_CF;
        CyU3PBusyWait (10);
    }
    if (CyU3POtgIsDeviceMode ())
    {
        UIB->otg_ctrl &= CY_U3P_UIB_OTG_ENABLE;
        UIB->otg_ctrl |= CY_U3P_UIB_DEV_ENABLE;

        /* Enable and set the EPM clock to bus clock (100MHz). */
        GCTL->uib_core_clk = (CY_U3P_GCTL_UIBCLK_CLK_EN |
                CY_U3P_GCTL_UIB_CORE_CLK_DEFAULT);
        CyU3PBusyWait (100);

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
        UIB->otg_ctrl &= ~CY_U3P_UIB_DEV_ENABLE;
    }
}

CyU3PReturnStatus_t
CyU3POtgSrpStart (uint32_t repeatInterval)
{
    if (!glIsOtgEnable)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }
    if ((repeatInterval > CY_U3P_OTG_SRP_MAX_REPEAT_INTERVAL) ||
            (repeatInterval == 0))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Check if device mode is enabled. */
    if ((!CyU3POtgIsDeviceMode ()) || (CyU3PUsbIsStarted ()) || (CyU3PUsbHostIsStarted ()))
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    /* Check if there is any valid session. */
    if (GCTL->iopower & glUibDeviceInfo.vbusDetectMode)
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    CyU3POtgSetupPhy ();

    /* Update the required timer period value. */
    glOtgTimerPeriod = (uint32_t)(repeatInterval * 32);

    /* Initialize the OTG timer with the corresponding repeat period. */
    UIB->otg_timer = glOtgTimerPeriod;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3POtgSrpAbort (void)
{
    if (!glIsOtgEnable)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check if device mode is enabled. */
    if ((!CyU3POtgIsDeviceMode ()) || (CyU3PUsbHostIsStarted ()))
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    /* Disable the timer. */
    glOtgTimerPeriod = 0;

    /* Disable the OTG timer interrupt. */
    UIB->otg_timer = 0;

    return CY_U3P_SUCCESS;
}

static CyU3PReturnStatus_t
CyU3POtgIssueSrp (void)
{
    uint32_t regVal;

    if (!glIsOtgEnable)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check if device mode is enabled. */
    if ((!CyU3POtgIsDeviceMode ()) || (CyU3PUsbIsStarted ()) || (CyU3PUsbHostIsStarted ()))
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    /* Check if there is any valid session. */
    if (GCTL->iopower & glUibDeviceInfo.vbusDetectMode)
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    /* This is a hack as there is no way to send a pulse on the VBUS.
     * Here we are first configuring the PHY as a device and doing a DP
     * pull-up to send a high. To send a low, the PHY is configured as host
     * and then DP is pulled down. This seems to work without the VBUS. */

    /* Enable CHG_VBUS. */
    UIB->otg_ctrl |= CY_U3P_UIB_CHG_VBUS;
    CyU3PThreadSleep (10);

    /* Pullup DP */
    regVal = UIB->otg_ctrl & (CY_U3P_UIB_OTG_ENABLE | CY_U3P_UIB_CHG_VBUS);
    UIB->otg_ctrl = (regVal | CY_U3P_UIB_DEV_ENABLE | CY_U3P_UIB_DP_PU_EN);
    CyU3PThreadSleep (10);
    /* Pull down DP */
    UIB->otg_ctrl = (regVal | CY_U3P_UIB_HOST_ENABLE | CY_U3P_UIB_DP_PD_EN);
    CyU3PThreadSleep (10);
    /* Set the PHY to be disconnected. */
    UIB->otg_ctrl = regVal;

    /* Disable CHG_VBUS. */
    UIB->otg_ctrl &= ~CY_U3P_UIB_CHG_VBUS;
    CyU3PThreadSleep (10);

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3POtgRequestHnp (CyBool_t isEnable)
{
    if (!glIsOtgEnable)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if (glOtgInfo.otgMode != CY_U3P_OTG_MODE_OTG)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if (!CyU3PUsbIsStarted ())
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Mark the GET_STATUS information to update the host request flag. */
    glUibDeviceInfo.isHnpRequested = isEnable;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3POtgHnpEnable (CyBool_t isEnable)
{
    if (!glIsOtgEnable)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if (glOtgInfo.otgMode != CY_U3P_OTG_MODE_OTG)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if ((CyU3PUsbIsStarted ()) || (CyU3PUsbHostIsStarted ()))
    {
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    glIsHnpEnable = isEnable;

    return CY_U3P_SUCCESS;
}

void
CyU3PUsbOtgIntHandler (void)
{
    uint32_t state;
    CyU3PReturnStatus_t status;

    state = (UIB->otg_intr & UIB->otg_intr_mask);
    /* Clear the interrupts. */
    UIB->otg_intr = state;

    if (state & (CY_U3P_UIB_SRP_DP_INT | CY_U3P_UIB_SRP_VBUS_INT))
    {
        if (glOtgInfo.cb)
        {
            glOtgInfo.cb (CY_U3P_OTG_SRP_DETECT, 0);
        }
    }

    if (state & CY_U3P_UIB_OTG_TIMER_TIMEOUT)
    {
        /* Currently the timer is used only for repeating the SRP.
         * Run this only if the timer is not aborted and there is
         * no valid VBUS available. */
        if (GCTL->iopower & glUibDeviceInfo.vbusDetectMode)
        {
            glOtgTimerPeriod = 0;
        }
        if (glOtgTimerPeriod != 0)
        {
            status = CyU3POtgIssueSrp ();
            if (status != CY_U3P_SUCCESS)
            {
                glOtgTimerPeriod = 0;
            }
        }

        /* Re-load the timer. */
        UIB->otg_timer = glOtgTimerPeriod;
    }

    /* Re-enable the OTG interrupt. */
    UIB->intr_mask |= CY_U3P_UIB_OTG_INT;
}

CyBool_t
CyU3POtgIsVBusValid (void)
{
    if (GCTL->iopower & glUibDeviceInfo.vbusDetectMode)
    {
        return CyTrue;
    }

    return CyFalse;
}

void
CyU3PUsbChgdetIntHandler (void)
{
    uint32_t state;
    uint32_t temp;

    state = (UIB->chgdet_intr & UIB->chgdet_intr_mask);
    /* Clear the interrupts. */
    UIB->chgdet_intr = state;

    if (state & CY_U3P_UIB_OTG_ID_CHANGE)
    {
        /* Identify the newly attached device. */
        temp = ((UIB->chgdet_ctrl & CY_U3P_UIB_ACA_OTG_ID_VALUE_MASK) >>
            CY_U3P_UIB_ACA_OTG_ID_VALUE_POS) + 1;
        /* Since the motorola EMU detection has the same codes as ACA in
         * the register and has an offset of 3 in the enumeration, update
         * accordingly. */
        if ((glOtgInfo.chargerMode == CY_U3P_OTG_CHARGER_DETECT_MOT_EMU)
                && (temp > CY_U3P_OTG_TYPE_B_CABLE))
        {
            temp += 3;
        }

        /* Save the current operation mode and invoke the callback
         * only if the type is different. This is to avoid repeated
         * callbacks due to debounce. */
        if (temp != glPeripheralType)
        {
            glPeripheralType = (CyU3POtgPeripheralType_t)temp;
            /* Disable the role reversal flag. */
            glIsHnpEnable = CyFalse;
            /* Clear the session request flag. */
            glUibDeviceInfo.isHnpRequested = CyFalse;
            /* Disable the timer. */
            glOtgTimerPeriod = 0;
            UIB->otg_timer = 0;
            
            /* Enable the correct PHY mode. */
            CyU3POtgSetupPhy ();
            /* Enable SRP interrupts only if in host mode. */
            if ((CyU3POtgIsHostMode ()) && (!CyU3POtgIsVBusValid ()))
            {
                UIB->otg_intr = (CY_U3P_UIB_SRP_DP_INT | CY_U3P_UIB_SRP_VBUS_INT);
                UIB->otg_intr_mask |= (CY_U3P_UIB_SRP_DP_INT | CY_U3P_UIB_SRP_VBUS_INT);
            }
            else
            {
                UIB->otg_intr = (CY_U3P_UIB_SRP_DP_INT | CY_U3P_UIB_SRP_VBUS_INT);
                UIB->otg_intr_mask &= ~(CY_U3P_UIB_SRP_DP_INT | CY_U3P_UIB_SRP_VBUS_INT);
            }

            /* Invoke the event handler. */
            if (glOtgInfo.cb)
            {
                glOtgInfo.cb (CY_U3P_OTG_PERIPHERAL_CHANGE, glPeripheralType);
            }
        }
    }

    UIB->intr_mask |= CY_U3P_UIB_CHGDET_INT;
}

/* [] */

