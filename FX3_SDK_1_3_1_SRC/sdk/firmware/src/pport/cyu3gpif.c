/*
 ## Cypress USB 3.0 Platform source file (cyu3gpif.c)
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

/*@@GPIF Functions
 * This file contains functions that configure, control and check the status
 * of the GPIF interface on the USB 3.0 device.
 */

#include <cyu3pib.h>
#include <cywbpib.h>
#include <cyu3gpif.h>
#include <cyu3error.h>
#include <cyu3regs.h>
#include <cyu3utils.h>
#include <cyu3system.h>
#include <cyfx3_api.h>

extern CyU3PGpifEventCb_t  glGpifEvtCb;                /* GPIF event callback. */
extern CyU3PGpifSMIntrCb_t glGpifSMCb;                 /* GPIF state machine callback. */

/* Register the GPIF event callback. */
void
CyU3PGpifRegisterCallback (
        CyU3PGpifEventCb_t cbFunc)
{
    glGpifEvtCb = cbFunc;
}

void
CyU3PGpifRegisterSMIntrCallback (
        CyU3PGpifSMIntrCb_t cb)
{
    glGpifSMCb = cb;
}

CyU3PReturnStatus_t
CyU3PGpifLoad (
        const CyU3PGpifConfig_t *conf)
{
    uint16_t index, entry;
    uvint32_t *addr = (uvint32_t *)CY_U3P_PIB_GPIF_BUS_CONFIG_ADDRESS;

    /* Parameter checks. */
    if ((conf == 0) || ((conf->stateCount == 0) && (conf->functionCount == 0) && (conf->regCount == 0)))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }


    /* Verify that the GPIF has not been enabled yet. */
    if ((PIB->gpif_config & CY_U3P_GPIF_CONF_ENABLE) || (PIB->gpif_waveform_ctrl_stat & CY_U3P_GPIF_WAVEFORM_VALID))
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    if (!CyFx3DevIsGpifConfigurable ())
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    /* Check if GPIF is configured to be 32 bits wide on a part that has a 16-bit GPIF. */
    if ((conf->regCount >= 2) && (conf->regData != NULL) &&
            ((conf->regData[1] & CY_U3P_GPIF_BUS_WIDTH_MASK) > 0x04))
    {
        if (!CyFx3DevIsGpif32Supported ())
            return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if (conf->stateCount)
    {
        if (conf->stateData == 0)
            return CY_U3P_ERROR_BAD_ARGUMENT;

        for (index = 0; index < conf->stateCount; index++)
        {
            /* No look-up table means direct mapping from array index to state number. */
            entry = (conf->statePosition != 0) ? conf->statePosition[index] : index;

            PIB->gpif_left[index].waveform0  = conf->stateData[entry].leftData[0];
            PIB->gpif_left[index].waveform1  = conf->stateData[entry].leftData[1];
            PIB->gpif_left[index].waveform2  = conf->stateData[entry].leftData[2];

            PIB->gpif_right[index].waveform0 = conf->stateData[entry].rightData[0];
            PIB->gpif_right[index].waveform1 = conf->stateData[entry].rightData[1];
            PIB->gpif_right[index].waveform2 = conf->stateData[entry].rightData[2];
        }
    }

    if (conf->functionCount)
    {
        if ((conf->functionData == 0) || (conf->functionCount > CYU3P_GPIF_NUM_TRANS_FNS))
            return CY_U3P_ERROR_BAD_ARGUMENT;
        for (index = 0; index < conf->functionCount; index++)
            PIB->gpif_function[index] = conf->functionData[index];
    }

    if (conf->regCount)
    {
        if (conf->regData == 0)
            return CY_U3P_ERROR_BAD_ARGUMENT;
        for (index = 1; index < conf->regCount; index++)
            *addr++ = conf->regData[index];

        /* The GPIF config register is initialized at the end. */
        PIB->gpif_config = conf->regData[0];
    }

    return CY_U3P_SUCCESS;
}

/* Function to initialize the GPIF configuration registers. */
CyU3PReturnStatus_t
CyU3PGpifConfigure (
        uint8_t         numRegs,
        const uint32_t *regData)
{
    uvint32_t *addr = (uvint32_t *)CY_U3P_PIB_GPIF_BUS_CONFIG_ADDRESS;
    int index;

    if ((numRegs == 0) || (regData == 0))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (!CyFx3DevIsGpifConfigurable ())
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    /* Check if GPIF is configured to be 32 bits wide on a part that has a 16-bit GPIF. */
    if ((numRegs > 1) && ((regData[1] & CY_U3P_GPIF_BUS_WIDTH_MASK) > 0x04))
    {
        if (!CyFx3DevIsGpif32Supported ())
            return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    /* Verify that the GPIF has not been enabled yet. */
    if (PIB->gpif_config & CY_U3P_GPIF_CONF_ENABLE)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    /* Copy the register data into the registers. */
    for (index = 1; index < numRegs; index++)
    {
        *addr++ = regData[index];
    }

    /* Init the GPIF_CONFIG register as the last step. */
    CY_U3P_PIB_GPIF_CONFIG = regData[0];

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpifSMStart (
        uint8_t stateIndex,
        uint8_t initialAlpha)
{
    uint32_t numdss;
    uint32_t gpifstat;
    uint32_t switchVal;

    /* State checks. */
    if (((PIB->gpif_config & CY_U3P_GPIF_CONF_ENABLE) == 0) ||
            ((PIB->gpif_left[stateIndex].waveform2 & CY_U3P_GPIF2_VALID) == 0))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    if (PIB->gpif_waveform_ctrl_stat & CY_U3P_GPIF_WAVEFORM_VALID)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    /* Mark the waveform as valid. */
    CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT = ((CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT & ~CY_U3P_GPIF_ALPHA_INIT_MASK) |
            (initialAlpha << CY_U3P_GPIF_ALPHA_INIT_POS) | CY_U3P_GPIF_WAVEFORM_VALID);

    /* Switch to the desired state and start execution. */
    switchVal = CY_U3P_PIB_GPIF_WAVEFORM_SWITCH;
    switchVal = ((switchVal & 0xFF00003A) | (stateIndex << CY_U3P_GPIF_DESTINATION_STATE_POS));

    CY_U3P_PIB_GPIF_WAVEFORM_SWITCH  = switchVal;
    CY_U3P_PIB_GPIF_WAVEFORM_SWITCH |= (CY_U3P_GPIF_WAVEFORM_SWITCH | CY_U3P_GPIF_SWITCH_NOW);

    gpifstat = CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT;
    numdss   = (CY_U3P_PIB_GPIF_BUS_CONFIG2 & CY_U3P_GPIF_STATE_FROM_CTRL_MASK);
    switch (numdss)
    {
    case 1:
        CY_U3P_PIB_GPIF_WAVEFORM_SWITCH = ((switchVal & 0xFF7F003A) | CY_U3P_GPIF_WAVEFORM_SWITCH |
                CY_U3P_GPIF_SWITCH_NOW | ((gpifstat & 0x80000000u) >> 8));
        break;
    case 2:
        CY_U3P_PIB_GPIF_WAVEFORM_SWITCH = ((switchVal & 0xFF3F003A) | CY_U3P_GPIF_WAVEFORM_SWITCH |
                CY_U3P_GPIF_SWITCH_NOW | ((gpifstat & 0xC0000000u) >> 8));
        break;
    case 3:
        CY_U3P_PIB_GPIF_WAVEFORM_SWITCH = ((switchVal & 0xFF1F003A) | CY_U3P_GPIF_WAVEFORM_SWITCH |
                CY_U3P_GPIF_SWITCH_NOW | ((gpifstat & 0xE0000000u) >> 8));
        break;
    default:
        break;
    }

    return CY_U3P_SUCCESS;
}

const uint32_t glConstGpifDefaults[76] = {
    CY_U3P_PIB_GPIF_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_BUS_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_BUS_CONFIG2_DEFAULT,
    CY_U3P_PIB_GPIF_AD_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_STATUS_DEFAULT,
    CY_U3P_PIB_GPIF_INTR_DEFAULT,
    CY_U3P_PIB_GPIF_INTR_MASK_DEFAULT,
    CY_U3P_PIB_GPIF_SERIAL_IN_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_SERIAL_OUT_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_DIRECTION_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_DEFAULT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_POLARITY_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_TOGGLE_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_BUS_SELECT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_COUNT_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_COUNT_RESET_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_COUNT_LIMIT_DEFAULT,
    CY_U3P_PIB_GPIF_ADDR_COUNT_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_ADDR_COUNT_RESET_DEFAULT,
    CY_U3P_PIB_GPIF_ADDR_COUNT_LIMIT_DEFAULT,
    CY_U3P_PIB_GPIF_STATE_COUNT_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_STATE_COUNT_LIMIT_DEFAULT,
    CY_U3P_PIB_GPIF_DATA_COUNT_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_DATA_COUNT_RESET_DEFAULT,
    CY_U3P_PIB_GPIF_DATA_COUNT_LIMIT_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_COMP_VALUE_DEFAULT,
    CY_U3P_PIB_GPIF_CTRL_COMP_MASK_DEFAULT,
    CY_U3P_PIB_GPIF_DATA_COMP_VALUE_DEFAULT,
    CY_U3P_PIB_GPIF_DATA_COMP_MASK_DEFAULT,
    CY_U3P_PIB_GPIF_ADDR_COMP_VALUE_DEFAULT,
    CY_U3P_PIB_GPIF_ADDR_COMP_MASK_DEFAULT,
    CY_U3P_PIB_GPIF_DATA_CTRL_DEFAULT,
    CY_U3P_PIB_GPIF_INGRESS_DATA_DEFAULT,
    CY_U3P_PIB_GPIF_INGRESS_DATA_DEFAULT,
    CY_U3P_PIB_GPIF_INGRESS_DATA_DEFAULT,
    CY_U3P_PIB_GPIF_INGRESS_DATA_DEFAULT,
    CY_U3P_PIB_GPIF_EGRESS_DATA_DEFAULT,
    CY_U3P_PIB_GPIF_EGRESS_DATA_DEFAULT,
    CY_U3P_PIB_GPIF_EGRESS_DATA_DEFAULT,
    CY_U3P_PIB_GPIF_EGRESS_DATA_DEFAULT,
    CY_U3P_PIB_GPIF_INGRESS_ADDRESS_DEFAULT,
    CY_U3P_PIB_GPIF_INGRESS_ADDRESS_DEFAULT,
    CY_U3P_PIB_GPIF_INGRESS_ADDRESS_DEFAULT,
    CY_U3P_PIB_GPIF_INGRESS_ADDRESS_DEFAULT,
    CY_U3P_PIB_GPIF_EGRESS_ADDRESS_DEFAULT,
    CY_U3P_PIB_GPIF_EGRESS_ADDRESS_DEFAULT,
    CY_U3P_PIB_GPIF_EGRESS_ADDRESS_DEFAULT,
    CY_U3P_PIB_GPIF_EGRESS_ADDRESS_DEFAULT,
    CY_U3P_PIB_GPIF_THREAD_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_THREAD_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_THREAD_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_THREAD_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_LAMBDA_STAT_DEFAULT,
    CY_U3P_PIB_GPIF_ALPHA_STAT_DEFAULT,
    CY_U3P_PIB_GPIF_BETA_STAT_DEFAULT,
    CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT_DEFAULT,
    CY_U3P_PIB_GPIF_WAVEFORM_SWITCH_DEFAULT,
    CY_U3P_PIB_GPIF_WAVEFORM_SWITCH_TIMEOUT_DEFAULT,
    CY_U3P_PIB_GPIF_CRC_CONFIG_DEFAULT,
    CY_U3P_PIB_GPIF_CRC_DATA_DEFAULT,
    CY_U3P_PIB_GPIF_BETA_DEASSERT_DEFAULT
};

/* Function to disable the GPIF hardware. */
void
CyU3PGpifDisable (
        CyBool_t forceReload)
{
    uint32_t i;

    /* Pause the state machine to stop operation and then disable it. */
    PIB->gpif_waveform_ctrl_stat |= CY_U3P_GPIF_PAUSE;
    CyU3PBusyWait (10);
    PIB->gpif_waveform_ctrl_stat  = CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT_DEFAULT;

    if (forceReload)
    {
        PIB->gpif_config = CY_U3P_PIB_GPIF_CONFIG_DEFAULT;

        /* Clear all existing state information from the waveform memory. */
        for (i = 0; i < CYU3P_GPIF_NUM_STATES; i++)
        {
            PIB->gpif_left[i].waveform2  = 0;
            PIB->gpif_right[i].waveform2 = 0;
        }

        /* Reset all GPIF registers to their default values. */
        CyU3PGpifConfigure (76, glConstGpifDefaults);
    }
}

/* Maximum number of distinct states in the GPIF state machine (DSS dependent). */
const uint8_t glGpifConstMaxState[4] = {
    0xFF, 0x7F, 0x3F, 0x1F
};

CyU3PReturnStatus_t
CyU3PGpifSMSwitch (
        uint16_t fromState,
        uint16_t toState,
        uint16_t endState,
        uint8_t  initialAlpha,
        uint32_t switchTimeout)
{
    uint32_t switchVal = 0;
    uint32_t numdss;
    uint32_t gpifstat;
    uint8_t  curState;

    /* Parameter and state checks. */
    if (toState >= CYU3P_GPIF_NUM_STATES)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Removing the condition check for valid state because it is not possible to read GPIF
       waveform registers while GPIF is running. */
    if ((PIB->gpif_config & CY_U3P_GPIF_CONF_ENABLE) == 0)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    /* Find the DSS level of the state machine. */
    numdss = (CY_U3P_PIB_GPIF_BUS_CONFIG2 & CY_U3P_GPIF_STATE_FROM_CTRL_MASK);

    /* Update the initial Alpha values for the next state switch. */
    CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT = ((CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT & ~CY_U3P_GPIF_ALPHA_INIT_MASK) |
            (initialAlpha << CY_U3P_GPIF_ALPHA_INIT_POS) | CY_U3P_GPIF_WAVEFORM_VALID);

    switchVal = (toState << CY_U3P_GPIF_DESTINATION_STATE_POS) | CY_U3P_GPIF_WAVEFORM_SWITCH;

    if (fromState < CYU3P_GPIF_NUM_STATES)
    {
        /* Check if the state machine is already in the fromState or a mirror. */
        curState = (PIB->gpif_waveform_ctrl_stat & CY_U3P_GPIF_CURRENT_STATE_MASK) >> CY_U3P_GPIF_CURRENT_STATE_POS;
        if ((curState & glGpifConstMaxState[numdss]) == (fromState & glGpifConstMaxState[numdss]))
        {
            switchVal |= CY_U3P_GPIF_SWITCH_NOW;
        }
        else
        {
            /* The timeout setting is assumed to be valid if a fromState is specified. */
            switchVal |= ((fromState << CY_U3P_GPIF_TERMINAL_STATE_POS) | (1 << CY_U3P_GPIF_TIMEOUT_MODE_POS));
            PIB->gpif_waveform_switch_timeout = switchTimeout;
        }
    }
    else
    {
        /* Set the switch immediately flag. */
        switchVal |= CY_U3P_GPIF_SWITCH_NOW;
    }

    /* If a valid endState is specified, set this information and enable the DONE event. */
    if (endState < CYU3P_GPIF_NUM_STATES)
    {
        switchVal |= ((endState << CY_U3P_GPIF_DONE_STATE_POS) | CY_U3P_GPIF_DONE_ENABLE);
        PIB->gpif_intr_mask |= CY_U3P_GPIF_INTR_GPIF_DONE;
    }

    /* Write into the GPIF register to request the switch. */
    PIB->gpif_waveform_switch = switchVal;

    gpifstat = CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT;
    switch (numdss)
    {
    case 1:
        CY_U3P_PIB_GPIF_WAVEFORM_SWITCH = ((switchVal & 0xFF7FFFFF) | ((gpifstat & 0x80000000u) >> 8));
        break;
    case 2:
        CY_U3P_PIB_GPIF_WAVEFORM_SWITCH = ((switchVal & 0xFF3FFFFF) | ((gpifstat & 0xC0000000u) >> 8));
        break;
    case 3:
        CY_U3P_PIB_GPIF_WAVEFORM_SWITCH = ((switchVal & 0xFF1FFFFF) | ((gpifstat & 0xE0000000u) >> 8));
        break;
    default:
        break;
    }

    return CY_U3P_SUCCESS;
}

void
CyU3PGpifControlSWInput (
        CyBool_t set)
{
    if (set)
    {
        CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT |= CY_U3P_GPIF_CPU_LAMBDA;
    }
    else
    {
        CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT &= ~CY_U3P_GPIF_CPU_LAMBDA;
    }
}

CyU3PReturnStatus_t
CyU3PGpifSocketConfigure (
        uint8_t            threadIndex,
        CyU3PDmaSocketId_t socketNum,
        uint16_t           watermark,
        CyBool_t           flagOnData,
        uint8_t            burst)
{
    uint32_t regVal = 0;

    /* Parameter checks. */
    if ((threadIndex >= CYU3P_PIB_THREAD_COUNT) || (burst > CYU3P_PIB_MAX_BURST_SETTING) ||
            (socketNum < CY_U3P_PIB_SOCKET_0) || (socketNum > CY_U3P_PIB_SOCKET_31))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* In PP-mode, only thread 0 can be used. */
    if (((PIB->gpif_config & CY_U3P_GPIF_CONF_PP_MODE) != 0) && (threadIndex != 0))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Disable the thread before enabling it with the correct configuration. */
    PIB->gpif_thread_config[threadIndex] &= ~CY_U3P_GPIF_ENABLE;
    CyU3PBusyWait (1);

    /* Compute and update the register value. */
    regVal = (socketNum & 0xFF) | (burst << CY_U3P_GPIF_BURST_SIZE_POS) |
        ((watermark << CY_U3P_GPIF_WATERMARK_POS) & CY_U3P_GPIF_WATERMARK_MASK) | CY_U3P_GPIF_ENABLE;
    if (flagOnData)
    {
        regVal |= CY_U3P_GPIF_WM_CFG;
    }

    PIB->gpif_thread_config[threadIndex] = regVal;
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpifOutputConfigure (
        uint8_t           ctrlPin,
        CyU3PGpifOutput_t opFlag,
        CyBool_t          isActiveLow)
{
    if ((ctrlPin > 15) || ((opFlag > CYU3P_GPIF_OP_ALPHA3) && (opFlag < CYU3P_GPIF_OP_BETA0)) ||
            ((opFlag > CYU3P_GPIF_OP_BETA3) && (opFlag < CYU3P_GPIF_OP_THR0_READY)) ||
            (opFlag > CYU3P_GPIF_OP_PPDRQ))
        return CY_U3P_ERROR_BAD_ARGUMENT;

    /* Update the signal connectivity for this control. */
    PIB->gpif_ctrl_bus_select[ctrlPin] = opFlag;

    /* Set the polarity for this pin. */
    PIB->gpif_ctrl_bus_polarity = (PIB->gpif_ctrl_bus_polarity & (~(1 << ctrlPin))) | (isActiveLow << ctrlPin);

    /* Configure the pin as an output. */
    PIB->gpif_ctrl_bus_direction = (PIB->gpif_ctrl_bus_direction & (~(3 << (ctrlPin * 2)))) | (1 << (ctrlPin * 2));

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpifGetSMState (
        uint8_t *curState_p)
{
    if (curState_p == 0)
        return CY_U3P_ERROR_BAD_ARGUMENT;

    if ((PIB->gpif_config & CY_U3P_GPIF_CONF_ENABLE) == 0)
        return CY_U3P_ERROR_NOT_CONFIGURED;

    *curState_p = (PIB->gpif_waveform_ctrl_stat & CY_U3P_GPIF_CURRENT_STATE_MASK) >> CY_U3P_GPIF_CURRENT_STATE_POS;
    return CY_U3P_SUCCESS;
}

/*[]*/
