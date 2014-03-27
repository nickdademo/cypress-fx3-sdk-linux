/*
 ## Cypress EZ-USB FX3 Source file (cyu3gpiocomplex.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2012,
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

#include <cyu3gpio.h>
#include <cyu3error.h>
#include <cyu3utils.h>
#include <cyu3lpp.h>
#include <gpio_regs.h>

/*
 * @@GPIO
 * Summary
 * This file contains the driver code and the convenience APIs for accessing the FX3 GPIOs in complex mode.
 */

extern CyBool_t glIsGpioActive;

CyU3PReturnStatus_t
CyU3PGpioSetComplexConfig (
        uint8_t gpioId,
        CyU3PGpioComplexConfig_t *cfg_p)
{
    uint32_t value = 0;

    if (!glIsGpioActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (!CyU3PIsGpioValid(gpioId))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (cfg_p == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    /* Validate against the IO matrix only if the pin has
     * input or output stage enabled. */
    if (cfg_p->inputEn | cfg_p->driveLowEn | cfg_p->driveHighEn)
    {
        /* Check for IO matrix configuration only if
         * the pin is used as input or output. This
         * is not required if the pin is used a timer. */        
        if (!CyU3PIsGpioComplexIOConfigured(gpioId))
        {
            return CY_U3P_ERROR_NOT_CONFIGURED;
        }        
    }

    /* Validate parameters. */
    if (cfg_p->pinMode > CY_U3P_GPIO_MODE_MEASURE_ANY_ONCE)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (cfg_p->timerMode > CY_U3P_GPIO_TIMER_ANY_EDGE)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (cfg_p->intrMode > CY_U3P_GPIO_INTR_TIMER_ZERO)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Input stage. */
    if (cfg_p->inputEn)
    {
        value |= CY_U3P_LPP_GPIO_INPUT_EN;
    }
    /* Output stage. */
    if (cfg_p->driveLowEn)
    {
        value |= CY_U3P_LPP_GPIO_DRIVE_LO_EN;
    }
    if (cfg_p->driveHighEn)
    {
        value |= CY_U3P_LPP_GPIO_DRIVE_HI_EN;
    }
    if (cfg_p->outValue)
    {
        value |= CY_U3P_LPP_GPIO_OUT_VALUE;
    }

    /* Interrupt mode. */
    value |= ((cfg_p->intrMode << CY_U3P_LPP_GPIO_INTRMODE_POS) &
            CY_U3P_LPP_GPIO_INTRMODE_MASK);
    /* Clear any pending interrupt. */
    value |= CY_U3P_LPP_GPIO_INTR;

    value |= ((cfg_p->pinMode << CY_U3P_LPP_GPIO_MODE_POS) &
            CY_U3P_LPP_GPIO_MODE_MASK);

    /* Timer mode. */
    value |= ((cfg_p->timerMode << CY_U3P_LPP_GPIO_TIMER_MODE_POS) &
            CY_U3P_LPP_GPIO_TIMER_MODE_MASK);

    /* Disable the GPIO before configuration. */
    GPIO->lpp_gpio_pin[gpioId % 8].status &= ~CY_U3P_LPP_GPIO_ENABLE;

    /* Update the register configurations. */
    GPIO->lpp_gpio_pin[gpioId % 8].timer = cfg_p->timer;
    GPIO->lpp_gpio_pin[gpioId % 8].period = cfg_p->period;
    GPIO->lpp_gpio_pin[gpioId % 8].threshold = cfg_p->threshold;
    GPIO->lpp_gpio_pin[gpioId % 8].status = value;

    /* Enable the GPIO. */
    GPIO->lpp_gpio_pin[gpioId % 8].status |= CY_U3P_LPP_GPIO_ENABLE;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioComplexUpdate (
        uint8_t gpioId,
        uint32_t threshold,
        uint32_t period)
{
    uint8_t index;

    if (!glIsGpioActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (!CyU3PIsGpioValid(gpioId))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    index = gpioId % 8;
    if (!(GPIO->lpp_gpio_pin[index].status &
                CY_U3P_LPP_GPIO_ENABLE))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    GPIO->lpp_gpio_pin[index].threshold = threshold;
    GPIO->lpp_gpio_pin[index].period = period;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioComplexGetThreshold (
        uint8_t gpioId,
        uint32_t *threshold_p)
{
    uint8_t index;

    if (!glIsGpioActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (!CyU3PIsGpioValid(gpioId))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (threshold_p == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    index = gpioId % 8;
    if (!(GPIO->lpp_gpio_pin[index].status &
                CY_U3P_LPP_GPIO_ENABLE))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    *threshold_p = GPIO->lpp_gpio_pin[index].threshold;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioComplexSampleNow (
        uint8_t gpioId,
        uint32_t *value_p)
{
    uint8_t index;
    uint32_t regVal;

    if (!glIsGpioActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (!CyU3PIsGpioValid(gpioId))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (value_p == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    index = gpioId % 8;

    regVal = (GPIO->lpp_gpio_pin[index].status &
            ~CY_U3P_LPP_GPIO_INTR);
    if (!(regVal & CY_U3P_LPP_GPIO_ENABLE))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (regVal & CY_U3P_LPP_GPIO_MODE_MASK)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    /* Set the mode to sample now. Wait for the sampling to
     * complete and read the value from the threshold register. */
    GPIO->lpp_gpio_pin[index].status = (regVal |
            (CY_U3P_GPIO_MODE_SAMPLE_NOW << CY_U3P_LPP_GPIO_MODE_POS));
    while (GPIO->lpp_gpio_pin[index].status & CY_U3P_LPP_GPIO_MODE_MASK);
    *value_p = GPIO->lpp_gpio_pin[index].threshold;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioComplexPulseNow (
        uint8_t gpioId,
        uint32_t threshold)
{
    uint8_t index;
    uint32_t regVal;

    if (!glIsGpioActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (!CyU3PIsGpioValid(gpioId))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (threshold == NULL)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    index = gpioId % 8;

    regVal = (GPIO->lpp_gpio_pin[index].status &
            ~CY_U3P_LPP_GPIO_INTR);
    if (!(regVal & CY_U3P_LPP_GPIO_ENABLE))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (regVal & CY_U3P_LPP_GPIO_MODE_MASK)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }
    if ((regVal & (CY_U3P_LPP_GPIO_DRIVE_LO_EN |
                    CY_U3P_LPP_GPIO_DRIVE_HI_EN)) == 0)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    /* Update the threshold register. Change mode to pulse now. */
    GPIO->lpp_gpio_pin[index].threshold = threshold;
    GPIO->lpp_gpio_pin[index].status = (regVal |
            (CY_U3P_GPIO_MODE_PULSE_NOW << CY_U3P_LPP_GPIO_MODE_POS));

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioComplexPulse (
        uint8_t gpioId,
        uint32_t threshold)
{
    uint8_t index;
    uint32_t regVal;

    if (!glIsGpioActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (!CyU3PIsGpioValid(gpioId))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (threshold == NULL)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    index = gpioId % 8;

    regVal = (GPIO->lpp_gpio_pin[index].status &
            ~CY_U3P_LPP_GPIO_INTR);
    if (!(regVal & CY_U3P_LPP_GPIO_ENABLE))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (regVal & CY_U3P_LPP_GPIO_MODE_MASK)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }
    if ((regVal & (CY_U3P_LPP_GPIO_DRIVE_LO_EN |
                    CY_U3P_LPP_GPIO_DRIVE_HI_EN)) == 0)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    /* Update the threshold register. Change mode to pulse. */
    GPIO->lpp_gpio_pin[index].threshold = threshold;
    GPIO->lpp_gpio_pin[index].status = (regVal |
            (CY_U3P_GPIO_MODE_PULSE << CY_U3P_LPP_GPIO_MODE_POS));

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioComplexMeasureOnce (
        uint8_t gpioId,
        CyU3PGpioComplexMode_t pinMode)
{
    uint8_t index;
    uint32_t regVal;

    if (!glIsGpioActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (!CyU3PIsGpioValid(gpioId))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((pinMode != CY_U3P_GPIO_MODE_MEASURE_LOW_ONCE) &&
            (pinMode != CY_U3P_GPIO_MODE_MEASURE_HIGH_ONCE) &&
            (pinMode != CY_U3P_GPIO_MODE_MEASURE_NEG_ONCE) &&
            (pinMode != CY_U3P_GPIO_MODE_MEASURE_POS_ONCE) &&
            (pinMode != CY_U3P_GPIO_MODE_MEASURE_ANY_ONCE))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    index = gpioId % 8;

    regVal = (GPIO->lpp_gpio_pin[index].status &
            ~CY_U3P_LPP_GPIO_INTR);
    if (!(regVal & CY_U3P_LPP_GPIO_ENABLE))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (regVal & CY_U3P_LPP_GPIO_MODE_MASK)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }
    if ((regVal & CY_U3P_LPP_GPIO_INPUT_EN) == 0)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    /* Update the threshold register. Change mode to pulse now. */
    GPIO->lpp_gpio_pin[index].threshold = 0;
    GPIO->lpp_gpio_pin[index].status = (regVal |
            (pinMode << CY_U3P_LPP_GPIO_MODE_POS));

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioComplexWaitForCompletion (
        uint8_t gpioId,
        uint32_t *threshold_p,
        CyBool_t isWait)
{
    uint8_t index;
    uint32_t regVal;
    CyU3PGpioComplexMode_t pinMode;

    if (!glIsGpioActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (!CyU3PIsGpioValid(gpioId))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    index = gpioId % 8;

    regVal = (GPIO->lpp_gpio_pin[index].status &
            ~CY_U3P_LPP_GPIO_INTR);
    if (!(regVal & CY_U3P_LPP_GPIO_ENABLE))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    pinMode = (CyU3PGpioComplexMode_t)((regVal & CY_U3P_LPP_GPIO_MODE_MASK) >>
            CY_U3P_LPP_GPIO_MODE_POS);

    switch (pinMode)
    {
        case CY_U3P_GPIO_MODE_STATIC:
            break;

        case CY_U3P_GPIO_MODE_PULSE_NOW:
        case CY_U3P_GPIO_MODE_PULSE:
        case CY_U3P_GPIO_MODE_MEASURE_LOW_ONCE:
        case CY_U3P_GPIO_MODE_MEASURE_HIGH_ONCE:
        case CY_U3P_GPIO_MODE_MEASURE_NEG_ONCE:
        case CY_U3P_GPIO_MODE_MEASURE_POS_ONCE:
        case CY_U3P_GPIO_MODE_MEASURE_ANY_ONCE:
            if (isWait)
            {
                while (GPIO->lpp_gpio_pin[index].status & CY_U3P_LPP_GPIO_MODE_MASK);
            }
            else
            {
                return CY_U3P_ERROR_TIMEOUT;
            }
            break;

        default:
            return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if (threshold_p != NULL)
    {
        *threshold_p = GPIO->lpp_gpio_pin[index].threshold;
    }

    return CY_U3P_SUCCESS;
}

