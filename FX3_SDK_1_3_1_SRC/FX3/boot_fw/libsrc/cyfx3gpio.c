/*
 ## Cypress USB 3.0 Platform source file (cyu3i2c.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2011,
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

#include <cyfx3gpio.h>
#include <cyfx3device.h>
#include <cyfx3iocfg.h>
#include <cyfx3error.h>
#include <cyfx3utils.h>
#include <gctl_regs.h>
#include <gpio_regs.h>
#include <lpp_regs.h>
#include <cyfx3bootloader.h>

/*
 * @@GPIO
 * Summary
 * This file contains the GPIO functionalities.
 */

/*Summary
  Clock divider values for sampling simple GPIOs.

  Description
  The simple GPIOs are always sampled at a frequency
  lower than the fast clock. This divider value can
  be selected for power optimizations.

See Also
  * CyFx3BootGpioSimpleClkDiv_t
  * CyFx3BootGpioInit
  */
typedef enum CyFx3BootGpioSimpleClkDiv_t
{
    CY_FX3_BOOT_GPIO_SIMPLE_DIV_BY_2 = 0,    /* Fast clock by 2. */
    CY_FX3_BOOT_GPIO_SIMPLE_DIV_BY_4,        /* Fast clock by 4. */
    CY_FX3_BOOT_GPIO_SIMPLE_DIV_BY_16,       /* Fast clock by 16. */
    CY_FX3_BOOT_GPIO_SIMPLE_DIV_BY_64,       /* Fast clock by 64. */
    CY_FX3_BOOT_GPIO_SIMPLE_NUM_DIV          /* Number of divider enumerations. */
} CyFx3BootGpioSimpleClkDiv_t;

/*Summary
  Clock configuration information for the GPIO block.

  Description
  The clock for the GPIO block can be configured to required frequency.
  The default values can be

  fastClkDiv = 2
  slowClkDiv = 0
  simpleDiv = CY_FX3_BOOT_GPIO_SIMPLE_DIV_BY_2
  clkSrc = CY_FX3_BOOT_SYS_CLK

  These default values must be used if only simple GPIO is required.

  See Also
  * CyFx3BootGpioSimpleClkDiv_t
  * CyFx3BootGpioInit
  */
typedef struct CyFx3BootGpioClock_t
{
    uint8_t fastClkDiv;         /* Divider value for the GPIO fast clock.
                                   This is used for all complex GPIO sampling
                                   unless otherwise specified. The min value
                                   is 2 and max value is 16. */
    uint8_t slowClkDiv;         /* Divider value for the GPIO slow clock.
                                   The clock is based out of fast clock.
                                   The min value is 2 and max value is 64.
                                   If zero is used, then slow clock is not
                                   used and is disabled. */
    CyBool_t halfDiv;           /* This allows the fast clock to be divided
                                   by a non integral value of 0.5. This can be
                                   done only if slow clock is disabled. */
    CyFx3BootGpioSimpleClkDiv_t simpleDiv; /* Divider value from fast clock for sampling
                                   simple GPIOs. */
    CyFx3BootSysClockSrc_t clkSrc;    /* The clock source to be used for this peripheral. */
} CyFx3BootGpioClock_t;

extern CyFx3BootIoMatrixConfig_t glBootIoCfg;
extern uint8_t  glLppBlockOn;
extern CyBool_t glBootGpioIsOn;

/* 
 * This function initializes the GPIO module. 
 */
void
CyFx3BootGpioInit (
        void)
{
    uint32_t value;
    CyFx3BootGpioClock_t gpioClk;

    /* If the GPIO block has been left ON by design in the SDK firmware, don't touch the block at this stage. */
    if ((glBootGpioIsOn) && ((GCTL->gpio_fast_clk & CY_U3P_GCTL_GPIOFCLK_CLK_EN) != 0) &&
            ((GPIO->lpp_gpio_power & CY_U3P_LPP_GPIO_ACTIVE) != 0))
    {
        glLppBlockOn |= 0x08;
        return;
    }

    gpioClk.fastClkDiv = 2;
    gpioClk.slowClkDiv = 32;
    gpioClk.halfDiv    = CyFalse;
    gpioClk.simpleDiv  = CY_FX3_BOOT_GPIO_SIMPLE_DIV_BY_16;
    gpioClk.clkSrc     = CY_FX3_BOOT_SYS_CLK_BY_2;

    /* Set the GPIO fast clock. */
    value =  ((gpioClk.fastClkDiv - 1) |
            ((uint32_t)gpioClk.clkSrc << CY_U3P_GCTL_GPIOFCLK_SRC_POS) |
            ((uint32_t)gpioClk.simpleDiv << CY_U3P_GCTL_GPIOFCLK_SIMPLE_POS) |
            CY_U3P_GCTL_GPIOFCLK_CLK_EN);

    if (gpioClk.halfDiv)
    {
        value |= CY_U3P_GCTL_GPIOFCLK_HALFDIV;
    }

    GCTL->gpio_fast_clk = value;

    if (gpioClk.slowClkDiv != 0)
    {
        CyFx3BootBusyWait (1);
        GCTL->gpio_slow_clk = (gpioClk.slowClkDiv - 1) |
            CY_U3P_GCTL_GPIOFCLK_CLK_EN;
    }

    if (!glLppBlockOn)
    {
        /* Reset and enable the LPP block */
        LPP->power &= ~CY_U3P_LPP_RESETN;
        LPP->power |= CY_U3P_LPP_RESETN;
        while (!(LPP->power & CY_U3P_LPP_ACTIVE));
    }

    /* Indicate that the GPIO block is turned on. */
    glLppBlockOn |= 0x08;

    /* Hold the GPIO block in reset. */
    GPIO->lpp_gpio_power &= ~CY_U3P_LPP_GPIO_RESETN;
    /* Enable the GPIO block */
    GPIO->lpp_gpio_power |= CY_U3P_LPP_GPIO_RESETN;

    while (!(GPIO->lpp_gpio_power & CY_U3P_LPP_GPIO_ACTIVE));
}

void
CyFx3BootGpioDeInit (
        void)
{
    /* Reset the reset bit */
    GPIO->lpp_gpio_power &= ~(CY_U3P_LPP_GPIO_RESETN);

    if (glLppBlockOn == 0x08)
    {
        /* Reset and disable the LPP block */
        LPP->power &= ~CY_U3P_LPP_RESETN;
        while (LPP->power & CY_U3P_LPP_ACTIVE);
        glLppBlockOn = 0;
    }

    /* Disable the clocks. */
    GCTL->gpio_slow_clk = 0;
    CyFx3BootBusyWait (1);
    GCTL->gpio_fast_clk = 0;

    glLppBlockOn &= ~(0x08);
}

/* Configure a simple GPIO */
CyFx3BootErrorCode_t
CyFx3BootGpioSetSimpleConfig (
       uint8_t gpioId,
       CyFx3BootGpioSimpleConfig_t *cfg_p)
{
    uint32_t value = 0;

    if (!cfg_p)
    {
        return CY_FX3_BOOT_ERROR_NULL_POINTER;
    }

    /* Validate against the IO matrix. */
    if (gpioId < 32)
    {
        if (((glBootIoCfg.gpioSimpleEn[0] & (1 << gpioId)) == 0))
        {
            return CY_FX3_BOOT_ERROR_NOT_CONFIGURED;
        }
    }
    else
    {
        if (((glBootIoCfg.gpioSimpleEn[1] & (1 << (gpioId - 32))) == 0))
        {
            return CY_FX3_BOOT_ERROR_NOT_CONFIGURED;
        }
    }

    if (cfg_p->inputEn)
    {
        /* Input. */
        value |= CY_U3P_LPP_GPIO_INPUT_EN;
        value |= ((cfg_p->intrMode << CY_U3P_LPP_GPIO_INTRMODE_POS) &
            CY_U3P_LPP_GPIO_INTRMODE_MASK);
        value |= CY_U3P_LPP_GPIO_INTR;
    }
    else
    {
        /* Output. */
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
    }

    value |= CY_U3P_LPP_GPIO_ENABLE;

    /* Write and read back from the register. */
    GPIO->lpp_gpio_simple[gpioId] = value;
    value = GPIO->lpp_gpio_simple[gpioId];

    return CY_FX3_BOOT_SUCCESS;
}

/* 
 * This function Disables the GPIO.
 */
void
CyFx3BootGpioDisable (
        uint8_t gpioId)
{
    GPIO->lpp_gpio_simple[gpioId] = 0;
    GPIO->lpp_gpio_pin[gpioId % 8].status = 0;
}

CyFx3BootErrorCode_t
CyFx3BootGpioGetValue (
        uint8_t  gpioId,
        CyBool_t *value_p)
{
    uint32_t regVal;

    if (!value_p)
    {
        return CY_FX3_BOOT_ERROR_NULL_POINTER;
    }

    if (gpioId < 32)
    {
        if (glBootIoCfg.gpioSimpleEn[0] & (1 << gpioId))
        {
            regVal = GPIO->lpp_gpio_simple[gpioId];
        }
        else
        {
            return CY_FX3_BOOT_ERROR_NOT_CONFIGURED;
        }
    }
    else
    {
        if (glBootIoCfg.gpioSimpleEn[1] & (1 << (gpioId - 32)))
        {
            regVal = GPIO->lpp_gpio_simple[gpioId];
        }
        else
        {
            return CY_FX3_BOOT_ERROR_NOT_CONFIGURED;
        }
    }
    if (!(regVal & CY_U3P_LPP_GPIO_ENABLE))
    {
        return CY_FX3_BOOT_ERROR_NOT_CONFIGURED;
    }

    if (regVal & CY_U3P_LPP_GPIO_INPUT_EN)
    {
        *value_p = (CyBool_t)((regVal & CY_U3P_LPP_GPIO_IN_VALUE) >> 1);
    }
    else
    {
        *value_p = (CyBool_t)(regVal & CY_U3P_LPP_GPIO_OUT_VALUE);
    }

    return CY_FX3_BOOT_SUCCESS;
}

CyFx3BootErrorCode_t
CyFx3BootGpioSetValue (
        uint8_t  gpioId,
        CyBool_t value)
{
    uint32_t regVal;
    uvint32_t *regPtr;

    if (gpioId < 32)
    {
        if (glBootIoCfg.gpioSimpleEn[0] & (1 << gpioId))
        {
            regPtr = &GPIO->lpp_gpio_simple[gpioId];
        }
        else
        {
            return CY_FX3_BOOT_ERROR_NOT_CONFIGURED;
        }
    }
    else
    {
        if (glBootIoCfg.gpioSimpleEn[1] & (1 << (gpioId - 32)))
        {
            regPtr = &GPIO->lpp_gpio_simple[gpioId];
        }
        else
        {
            return CY_FX3_BOOT_ERROR_NOT_CONFIGURED;
        }
    }

    regVal = (*regPtr & ~CY_U3P_LPP_GPIO_INTR);
    if (!(regVal & CY_U3P_LPP_GPIO_ENABLE))
    {
        return CY_FX3_BOOT_ERROR_NOT_CONFIGURED;
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

    return CY_FX3_BOOT_SUCCESS;
}

/* [] */

