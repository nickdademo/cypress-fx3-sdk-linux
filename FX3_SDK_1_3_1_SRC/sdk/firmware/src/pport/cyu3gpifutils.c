
/*
 ## Cypress USB 3.0 Platform source file (cyu3gpifutils.c)
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
 * This file contains GPIF utility functions that are not commonly used in FX3 applications.
 */

#include <cyu3pib.h>
#include <cywbpib.h>
#include <cyu3gpif.h>
#include <cyu3error.h>
#include <cyu3regs.h>
#include <cyu3utils.h>
#include <cyu3system.h>
#include <cyfx3_api.h>

/* Function to load the GPIF waveform data into the state machine memory. */
CyU3PReturnStatus_t
CyU3PGpifWaveformLoad (
        uint8_t            firstState,
        uint16_t           stateCnt,
        uint8_t           *stateDataMap,
        CyU3PGpifWaveData *transitionData)
{
    uint16_t index, entry;

    /* Parameter checks. */
    if ((transitionData == 0) || ((firstState + stateCnt) > CYU3P_GPIF_NUM_STATES))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (!CyFx3DevIsGpifConfigurable ())
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    /* Verify that the GPIF has not been enabled yet. */
    if ((PIB->gpif_config & CY_U3P_GPIF_CONF_ENABLE) || (PIB->gpif_waveform_ctrl_stat & CY_U3P_GPIF_WAVEFORM_VALID))
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    /* Copy the data from the input arrays into the waveform memory. */
    for (index = 0; index < stateCnt; index++)
    {
        /* No look-up table means direct mapping. */
        entry = (stateDataMap != 0) ? stateDataMap[index] : index;

        PIB->gpif_left[index + firstState].waveform0  = transitionData[entry].leftData[0];
        PIB->gpif_left[index + firstState].waveform1  = transitionData[entry].leftData[1];
        PIB->gpif_left[index + firstState].waveform2  = transitionData[entry].leftData[2];

        PIB->gpif_right[index + firstState].waveform0 = transitionData[entry].rightData[0];
        PIB->gpif_right[index + firstState].waveform1 = transitionData[entry].rightData[1];
        PIB->gpif_right[index + firstState].waveform2 = transitionData[entry].rightData[2];
    }

    return CY_U3P_SUCCESS;
}

/* Function to initialize GPIF configuration registers. */
CyU3PReturnStatus_t
CyU3PGpifRegisterConfig (
        uint16_t numRegs,
        uint32_t regData[][2])
{
    uint16_t index;

    /* Verify that the parameters are good and that the SM is not currently running. */
    if ((numRegs == 0) || (regData == 0))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (PIB->gpif_config & CY_U3P_GPIF_CONF_ENABLE)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    if (!CyFx3DevIsGpifConfigurable ())
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    /* Check if GPIF is configured to be 32 bits wide on a part that has a 16-bit GPIF. */
    for (index = 0; index < numRegs; index++)
    {
        if ((regData[index][0] == CY_U3P_PIB_GPIF_BUS_CONFIG_ADDRESS) &&
                ((regData[index][1] & CY_U3P_GPIF_BUS_WIDTH_MASK) > 0x04))
        {
            if (!CyFx3DevIsGpif32Supported ())
                return CY_U3P_ERROR_NOT_SUPPORTED;
        }
    }

    /* Initialize all registers with the desired values. */
    for (index = 0; index < numRegs; index++)
    {
        *((uvint32_t *)regData[index][0]) = regData[index][1];
    }

    return CY_U3P_SUCCESS;
}

/* Function to initialize the GPIF transition function registers. */
CyU3PReturnStatus_t
CyU3PGpifInitTransFunctions (
        uint16_t *fnTable)
{
    int index;

    if (fnTable == 0)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (!CyFx3DevIsGpifConfigurable ())
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    /* Verify that the GPIF has not been enabled yet. */
    if (PIB->gpif_config & CY_U3P_GPIF_CONF_ENABLE)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    /* Copy the transition function data into the registers. */
    for (index = 0; index < CYU3P_GPIF_NUM_TRANS_FNS; index++)
    {
        PIB->gpif_function[index] = (uint32_t)fnTable[index];
    }

    return CY_U3P_SUCCESS;
}

void
CyU3PGpifInitCtrlCounter (  
        uint16_t initValue,
        uint16_t limit,
        CyBool_t reload,
        CyBool_t upCount,
        uint8_t  outputBit)
{
    uint32_t cfgVal = 0;

    /* Make sure that the counter is stopped. */
    PIB->gpif_ctrl_count_config = 0;

    /* Set the initial value and the limit. */
    PIB->gpif_ctrl_count_reset = initValue;
    PIB->gpif_ctrl_count_limit = limit;

    /* Set the counter configuration as desired. */
    cfgVal = CY_U3P_GPIF_CC_SW_RESET | CY_U3P_GPIF_CC_ENABLE |
        ((outputBit << CY_U3P_GPIF_CC_CONNECT_POS) & CY_U3P_GPIF_CC_CONNECT_MASK);
    if (reload)
    {
        cfgVal |= CY_U3P_GPIF_CC_RELOAD;
    }
    if (upCount)
    {
        cfgVal |= CY_U3P_GPIF_CC_DOWN_UP;
    }

    PIB->gpif_ctrl_count_config = cfgVal;
}

void
CyU3PGpifInitAddrCounter (  
        uint32_t initValue,
        uint32_t limit,
        CyBool_t reload,
        CyBool_t upCount,
        uint8_t  increment)
{
    uint32_t cfgVal = 0;

    /* Make sure that the counter is stopped. */
    PIB->gpif_addr_count_config = 0;

    /* Set the initial value and the limit. */
    PIB->gpif_addr_count_reset = initValue;
    PIB->gpif_addr_count_limit = limit;

    /* Set the counter configuration as desired. */
    cfgVal = (increment <<  CY_U3P_GPIF_AC_INCREMENT_POS) | CY_U3P_GPIF_AC_SW_RESET | CY_U3P_GPIF_AC_ENABLE;
    if (reload)
    {
        cfgVal |= CY_U3P_GPIF_AC_RELOAD;
    }
    if (upCount)
    {
        cfgVal |= CY_U3P_GPIF_AC_DOWN_UP;
    }

    PIB->gpif_addr_count_config = cfgVal;
}

void
CyU3PGpifInitDataCounter (  
        uint32_t initValue,
        uint32_t limit,
        CyBool_t reload,
        CyBool_t upCount,
        uint8_t  increment)
{
    uint32_t cfgVal = 0;

    /* Make sure that the counter is stopped. */
    PIB->gpif_data_count_config = 0;

    /* Set the initial value and the limit. */
    PIB->gpif_data_count_reset = initValue;
    PIB->gpif_data_count_limit = limit;

    /* Set the counter configuration as desired. */
    cfgVal = (increment <<  CY_U3P_GPIF_DC_INCREMENT_POS) | CY_U3P_GPIF_DC_SW_RESET | CY_U3P_GPIF_DC_ENABLE;
    if (reload)
    {
        cfgVal |= CY_U3P_GPIF_DC_RELOAD;
    }
    if (upCount)
    {
        cfgVal |= CY_U3P_GPIF_DC_DOWN_UP;
    }

    PIB->gpif_data_count_config = cfgVal;
}

CyU3PReturnStatus_t
CyU3PGpifInitComparator (
        CyU3PGpifComparatorType type,
        uint32_t                value,
        uint32_t                mask)
{
    switch (type)
    {
        case CYU3P_GPIF_COMP_CTRL:
            PIB->gpif_ctrl_comp_value = (value & CY_U3P_GPIF_CC_VALUE_MASK);
            PIB->gpif_ctrl_comp_mask  = (mask & CY_U3P_GPIF_CC_MASK_MASK);
            break;

        case CYU3P_GPIF_COMP_ADDR:
            PIB->gpif_addr_comp_value = value;
            PIB->gpif_addr_comp_mask  = mask;
            break;

        case CYU3P_GPIF_COMP_DATA:
            PIB->gpif_data_comp_value = value;
            PIB->gpif_data_comp_mask  = mask;
            break;

        default:
            return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpifSMControl (
        CyBool_t pause)
{
    if ((CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT & CY_U3P_GPIF_WAVEFORM_VALID) == 0)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    if (pause)
    {
        CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT |= CY_U3P_GPIF_PAUSE;
    }
    else
    {
        CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT &= ~CY_U3P_GPIF_PAUSE;
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpifReadDataWords (
        uint32_t  threadIndex,
        CyBool_t  selectThread,
        uint32_t  numWords,
        uint32_t *buffer_p,
        uint32_t  waitOption)
{
    CyU3PReturnStatus_t status;
    uint32_t tmp, mask;

    if (threadIndex >= CYU3P_PIB_THREAD_COUNT)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if ((PIB->gpif_config & CY_U3P_GPIF_CONF_PP_MODE) != 0)
    {
        return CY_U3P_ERROR_FAILURE;
    }

    /* If the thread has to be activated, check if software based thread selection
     * is permitted; and do so.
     */
    if (selectThread)
    {
        if (((PIB->gpif_config & CY_U3P_GPIF_CONF_THREAD_IN_STATE) != 0) ||
                ((PIB->gpif_ad_config & CY_U3P_GPIF_AIN_DATA) == 0))
        {
            return CY_U3P_ERROR_FAILURE;
        }

        PIB->gpif_ad_config = (PIB->gpif_ad_config & ~CY_U3P_GPIF_DATA_THREAD_MASK) |
            (threadIndex << CY_U3P_GPIF_DATA_THREAD_POS);
    }

    /* If the register already has data, read the first word from it directly without waiting for an event. */
    if (PIB->gpif_status & (1 << (CY_U3P_GPIF_STATUS_IN_DATA_VALID_POS + threadIndex)))
    {
        *buffer_p++ = PIB->gpif_ingress_data[threadIndex];
        PIB->gpif_data_ctrl = (1 << threadIndex);
        numWords--;
    }

    if (numWords)
    {
        /* Make sure that the IN_DATA_READY interrupt for this thread has been enabled. */
        PIB->gpif_intr_mask |= (1 << (CY_U3P_GPIF_INTR_IN_DATA_VALID_POS + threadIndex));

        /* For each word, wait for the data ready event and copy the data into the buffer. */
        mask = (1 << (CY_U3P_PIB_THR0_INDATA_EVT_POS + threadIndex));
        while (numWords--)
        {
            status = CyU3PEventGet (&glPibEvt, mask, CYU3P_EVENT_OR_CLEAR, &tmp, waitOption);
            if (status != CY_U3P_SUCCESS)
            {
                return status;
            }

            /* Read the data into the buffer and clear the data valid flag. */
            *buffer_p++ = PIB->gpif_ingress_data[threadIndex];
            PIB->gpif_data_ctrl = (1 << threadIndex);
        }
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpifWriteDataWords (
        uint32_t  threadIndex,
        CyBool_t  selectThread,
        uint32_t  numWords,
        uint32_t *buffer_p,
        uint32_t  waitOption)
{
    CyU3PReturnStatus_t status;
    uint32_t tmp, mask;

    if (threadIndex >= CYU3P_PIB_THREAD_COUNT)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if ((PIB->gpif_config & CY_U3P_GPIF_CONF_PP_MODE) != 0)
    {
        return CY_U3P_ERROR_FAILURE;
    }

    /* If the thread has to be activated, check if software based thread selection
     * is permitted; and do so.
     */
    if (selectThread)
    {
        if (((PIB->gpif_config & CY_U3P_GPIF_CONF_THREAD_IN_STATE) != 0) ||
                ((PIB->gpif_ad_config & CY_U3P_GPIF_AIN_DATA) == 0))
        {
            return CY_U3P_ERROR_FAILURE;
        }

        PIB->gpif_ad_config = (PIB->gpif_ad_config & ~CY_U3P_GPIF_DATA_THREAD_MASK) |
            (threadIndex << CY_U3P_GPIF_DATA_THREAD_POS);
    }

    /* If the register is already free, we can write the first word without waiting for an event. */
    if (PIB->gpif_status & (1 << (CY_U3P_GPIF_STATUS_EG_DATA_EMPTY_POS + threadIndex)))
    {
        PIB->gpif_egress_data[threadIndex] = *buffer_p++;
        PIB->gpif_data_ctrl = (1 << (CY_U3P_GPIF_EG_DATA_VALID_POS + threadIndex));
        numWords--;
    }

    if (numWords)
    {
        /* Make sure that the EG_DATA_EMPTY interrupt for this thread has been enabled. */
        PIB->gpif_intr_mask |= (1 << (CY_U3P_GPIF_INTR_EG_DATA_EMPTY_POS + threadIndex));

        /* For each word, wait for the empty event and then copy the data. */
        mask = (1 << (CY_U3P_PIB_THR0_EGDATA_EVT_POS + threadIndex));
        while (numWords--)
        {
            status = CyU3PEventGet (&glPibEvt, mask, CYU3P_EVENT_OR_CLEAR, &tmp, waitOption);
            if (status != CY_U3P_SUCCESS)
            {
                return status;
            }

            /* Write the data into the register and then update the data valid flag. */
            PIB->gpif_egress_data[threadIndex] = *buffer_p++;
            PIB->gpif_data_ctrl = (1 << (CY_U3P_GPIF_EG_DATA_VALID_POS + threadIndex));
        }
    }

    return CY_U3P_SUCCESS;
}

/*[]*/
