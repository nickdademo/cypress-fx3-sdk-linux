/*
 ## Cypress EZ-USB FX3 Source file (cyu3gpio.c)
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
 * This file contains the driver code and the convenience APIs for accessing the FX3 GPIOs.
 */
CyBool_t glIsGpioActive = CyFalse;              /* Whether the GPIO block has been powered up. */
static CyU3PGpioIntrCb_t glGpioIrq = NULL;      /* Callback for GPIO interrupt notifications. */

/* Register for the Call back function for GPIO Interrupt */
void 
CyU3PRegisterGpioCallBack (
                           CyU3PGpioIntrCb_t gpioIntrCb)
{
    glGpioIrq = gpioIntrCb;
}

/* GPIO Interrupt handler */
void
CyU3PGpioInt_Handler (
                      void)
{
    uint16_t index;
    uint8_t i, j;
    uint8_t gpio = 0;

    /* Check for the complex GPIO interrupt */
    for (i = 0, index = 1; index <= CY_U3P_LPP_GPIO_INTR7; i++)
    {
        if (GPIO->lpp_gpio_pin_intr & index)
        {
            /* Clear the interrupt. */
            GPIO->lpp_gpio_pin[i].status |= CY_U3P_LPP_GPIO_INTR;
            /* If a callback has been registered, identify
             * the correct GPIO. */
            if (glGpioIrq != NULL)
            {
                gpio = i;
                for (j = i; j < 32; j += 8)
                {
                    if (CyU3PIsGpioComplexIOConfigured(j))                        
                    {
                        gpio = j;
                        break;
                    }
                    else if (CyU3PIsGpioComplexIOConfigured(32+j))
                    {
                        gpio = j + 32;
                        break;
                    }
                }

                /* Invoke the callback. */
                glGpioIrq (gpio);
            }
        }
        index = index << 1;
    }

    /* Check for the simple GPIO interrupt */
    if ((GPIO->lpp_gpio_intr0) != 0)
    {
        for (i = 0; i < 32; i++)
        {
            if (GPIO->lpp_gpio_intr0 & (1 << i))
            {
                /* Clear the interrupt */
                GPIO->lpp_gpio_simple[i] |= CY_U3P_LPP_GPIO_INTR;
                if (glGpioIrq != NULL)
                {
                    glGpioIrq (i);
                }
            }
        }
    }
    if ((GPIO->lpp_gpio_intr1) != 0)
    {
        for (i = 0; i < 32; i++)
        {
            if (GPIO->lpp_gpio_intr1 & (1 << i))
            {
                /* Clear the interrupt */
                GPIO->lpp_gpio_simple[i + 32] |= CY_U3P_LPP_GPIO_INTR;
                if (glGpioIrq != NULL)
                {
                    glGpioIrq (i + 32);
                }
            }
        }
    }
}

/* 
 * This function initializes the GPIO module. 
 */
CyU3PReturnStatus_t
CyU3PGpioInit (
               CyU3PGpioClock_t *clk_p,
               CyU3PGpioIntrCb_t irq)
{    
    CyU3PReturnStatus_t status;
    if (glIsGpioActive)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }
    if (clk_p == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    /* Store the interrupt handler function pointer. */
    CyU3PRegisterGpioCallBack(irq);

    /* If the boot firmware has left the GPIO block ON, do not reset it. */
    if ((CyU3PLppGpioBlockIsOn () == CyFalse) || ((GPIO->lpp_gpio_power & CY_U3P_LPP_GPIO_ACTIVE) == 0))
    {
        status = CyU3PGpioSetClock (clk_p);
        if (status != CY_U3P_SUCCESS)
            return status;

        /* Register the fact that GPIO has been started with the FX3 API library. */
        status = CyU3PLppInit (CY_U3P_LPP_GPIO, CyU3PGpioInt_Handler);
        if (status != CY_U3P_SUCCESS)
            return status;

        /* Power the GPIO block ON, and wait for it to be active. */
        GPIO->lpp_gpio_power &= ~CY_U3P_LPP_GPIO_RESETN;
        CyU3PBusyWait (10);
        GPIO->lpp_gpio_power |= CY_U3P_LPP_GPIO_RESETN;
        while (!(GPIO->lpp_gpio_power & CY_U3P_LPP_GPIO_ACTIVE));
    }
    else
    {
        /* GPIO block is already ON. Just register the block with the FX3 API library. */
        status = CyU3PLppInit (CY_U3P_LPP_GPIO, CyU3PGpioInt_Handler);
        if (status != CY_U3P_SUCCESS)
            return status;
    }

    /* Update the status flag. */
    glIsGpioActive = CyTrue;
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioDeInit (
                 void)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    if (!glIsGpioActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Reset the reset bit */
    GPIO->lpp_gpio_power &= ~(CY_U3P_LPP_GPIO_RESETN);
    CyU3PBusyWait (10);

    /* Mark the block as disabled. */
    glIsGpioActive = CyFalse;

    /* Identify if the LPP block has to be disabled. */
    status = CyU3PLppDeInit (CY_U3P_LPP_GPIO);

    CyU3PGpioStopClock();
    
    return status;
}

CyU3PReturnStatus_t
CyU3PGpioSetSimpleConfig (
                          uint8_t gpioId,
                          CyU3PGpioSimpleConfig_t *cfg_p)
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
    if (cfg_p->intrMode > CY_U3P_GPIO_INTR_HIGH_LEVEL)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Validate against the IO matrix. */
    if (!CyU3PIsGpioSimpleIOConfigured(gpioId))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
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

    /* Enable the GPIO. */
    value |= CY_U3P_LPP_GPIO_ENABLE;

    /* Write and read back from the register. */
    GPIO->lpp_gpio_simple[gpioId] = value;
    value = GPIO->lpp_gpio_simple[gpioId];

    return CY_U3P_SUCCESS;
}

/* 
 * This function Disables the GPIO.
 */
CyU3PReturnStatus_t
CyU3PGpioDisable (
                  uint8_t gpioId)
{
    if (!glIsGpioActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (!CyU3PIsGpioValid(gpioId))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    GPIO->lpp_gpio_simple[gpioId] = 0;
    GPIO->lpp_gpio_pin[gpioId % 8].status = 0;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioGetValue (
                   uint8_t  gpioId,
                   CyBool_t *value_p)
{
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

    if (CyU3PIsGpioSimpleIOConfigured(gpioId))
    {
        regVal = GPIO->lpp_gpio_simple[gpioId];
    }
    else if (CyU3PIsGpioComplexIOConfigured(gpioId))
    {
        regVal = GPIO->lpp_gpio_pin[gpioId % 8].status;
    }
    else
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    
    if (!(regVal & CY_U3P_LPP_GPIO_ENABLE))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    /* Check if the pin is configured as input. If input is enabled,
    * then read that value. Otherwise return the out value. */
    if (regVal & CY_U3P_LPP_GPIO_INPUT_EN)
    {
        *value_p = (CyBool_t)((regVal & CY_U3P_LPP_GPIO_IN_VALUE) >> 1);
    }
    else
    {
        *value_p = (CyBool_t)(regVal & CY_U3P_LPP_GPIO_OUT_VALUE);
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioSimpleGetValue (
                         uint8_t  gpioId,
                         CyBool_t *value_p)
{
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

    regVal = GPIO->lpp_gpio_simple[gpioId];

    /* Check if the pin is configured as input. If input is enabled,
    * then read that value. Otherwise return the out value. */
    if (regVal & CY_U3P_LPP_GPIO_INPUT_EN)
    {
        *value_p = (CyBool_t)((regVal & CY_U3P_LPP_GPIO_IN_VALUE) >> 1);
    }
    else
    {
        *value_p = (CyBool_t)(regVal & CY_U3P_LPP_GPIO_OUT_VALUE);
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioSetValue (
                   uint8_t  gpioId,
                   CyBool_t value)
{
    uint32_t regVal;
    uvint32_t *regPtr;

    if (!glIsGpioActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (!CyU3PIsGpioValid(gpioId))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (CyU3PIsGpioSimpleIOConfigured(gpioId))
    {
        regPtr = &GPIO->lpp_gpio_simple[gpioId];
    }
    else if (CyU3PIsGpioComplexIOConfigured(gpioId))
    {
        regPtr = &GPIO->lpp_gpio_pin[gpioId % 8].status;
    }
    else
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
       

    regVal = (*regPtr & ~CY_U3P_LPP_GPIO_INTR);
    if (!(regVal & CY_U3P_LPP_GPIO_ENABLE))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    if (value)
    {
        regVal |= CY_U3P_LPP_GPIO_OUT_VALUE;
    }
    else
    {
        regVal &= ~CY_U3P_LPP_GPIO_OUT_VALUE;
    }

    *regPtr = regVal;
    regVal = *regPtr;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioSimpleSetValue (
                         uint8_t  gpioId,
                         CyBool_t value)
{
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

    regVal = (GPIO->lpp_gpio_simple[gpioId] &
        ~(CY_U3P_LPP_GPIO_INTR | CY_U3P_LPP_GPIO_OUT_VALUE));
    if (value)
    {
        regVal |= CY_U3P_LPP_GPIO_OUT_VALUE;
    }

    GPIO->lpp_gpio_simple[gpioId] = regVal;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioGetIOValues (
                      uint32_t *gpioVal0_p,
                      uint32_t *gpioVal1_p)
{
    if (!glIsGpioActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if (gpioVal0_p)
    {
        *gpioVal0_p = GPIO->lpp_gpio_invalue0;
    }
    if (gpioVal1_p)
    {
        *gpioVal1_p = GPIO->lpp_gpio_invalue1;
    }

    return CY_U3P_SUCCESS;
}


/* [] */

