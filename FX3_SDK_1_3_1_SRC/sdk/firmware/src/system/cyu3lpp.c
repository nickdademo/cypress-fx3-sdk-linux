/*
## Cypress USB 3.0 Platform Source file (cyu3lpp.c)
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

#include <cyu3uart.h>
#include <cyu3i2c.h>
#include <cyu3spi.h>
#include <cyu3gpio.h>
#include <cyu3error.h>
#include <cyu3protocol.h>
#include <cyu3vic.h>
#include <cyu3mmu.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3utils.h>
#include <cyu3lpp.h>
#include <cyu3regs.h>
#include <cyfx3_api.h>

/* Macros */
#define CY_U3P_LPP_STACK_SIZE           (0x400)     /* 1K stack for PIB Thread */
#define CY_U3P_LPP_THREAD_PRIORITY      (4)         /* LPP thread priority = 4 */

/* Global or static variable */
static uint32_t glLppMask;                          /* LPP event mask */
static CyU3PEvent glLppEvt;                         /* lpp event */
static CyU3PThread glLppThread;                     /* Lpp thread */
static uint32_t glLppMask;                          /* LPP event mask */
extern CyBool_t glSdk_GpioIsOn;

uint8_t glLppActive = 0;
CyU3PLppInterruptHandler glLppInterruptHandler[5] = {0};

extern void
CyU3PUartIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));
extern void
CyU3PI2cIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));
extern void
CyU3PI2sIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));
extern void
CyU3PGpioCoreIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));
extern void
CyU3PSpiIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));

CyBool_t
CyU3PAreLppsOff (
        uint32_t retainGpioState)
{
    if (glLppActive & 0x0F)
        return CyFalse;

    if ((retainGpioState == 0) && (glLppActive & 0x10))
        return CyFalse;

    return CyTrue;
}

/* Updates the Active\DeActive state of the LPP peripheral. */
static void
CyU3UpdateLppActiveInfo (
        CyU3PLppModule_t module,
        CyBool_t enable,
        CyU3PLppInterruptHandler intrHandler)
{
    uint8_t value = 1 << module;
    CyU3PVicVector_t vicVector = (CyU3PVicVector_t)(CY_U3P_VIC_I2C_CORE_VECTOR + module);

    glLppInterruptHandler[module] = intrHandler;

    /* Enable/Disable the interrupt at the VIC level. */
    if (enable)
    {
        /* Enable the interrupt only if a handler is registered. */
        if (intrHandler != 0)
            CyU3PVicEnableInt (vicVector);
        glLppActive |= value;
    }
    else
    {
        glLppActive &= ~value;
        CyU3PVicDisableInt (vicVector);
    }
}

CyU3PReturnStatus_t
CyU3PUartSetClock (
        uint32_t baudRate)
{
    int32_t  clkdiv;
    uint32_t regVal = 0;
    uint32_t clkFreq = glSysClkFreq;

    if (baudRate == 0)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* For low baud rates, the divider must be off the
    * CY_U3P_SYS_CLK_BY_16. */
    if (baudRate < CY_U3P_UART_BAUDRATE_600)
    {
        clkFreq = (glSysClkFreq >> 4);
    }

    /* Clock approximation needs to be done. Since we have
    * a half divider, we will use the following algorithm:
    * if x is the actual divider and n is the required
    * integral divider to be used, then following conditions
    * are used to evaluate:
    * if (x - floor(x)) < 0.25 ==> n = floor(x);
    * if (((x - floor(x)) >= 0.25) &&
    *     (x - floor(x)) < 0.5) ==> n = floor(x) + half divider;
    * if (x - floor(x)) >= 0.75 ==> n = floor(x) + 1;
    *
    * For this the source frequency is multiplied 4 times
    * and two least significant bits are used as decimal places
    * for evaluation.
    *
    * 00     ==> n = floor(x);
    * 01, 10 ==> n = floor(x) + half divider;
    * 11     ==> n = floor(x) + 1;
    */
    /* UART core clock must be 16X the baud rate. */
    clkdiv = (clkFreq << 2) / (baudRate * 16);

    if ((clkdiv & 0x03) == 0)
    {
        /*  (x - floor(x)) < 0.25. */
        clkdiv >>= 2;
    }
    else if (((clkdiv & 0x03) == 1) || ((clkdiv & 0x03) == 2))
    {
        /* (((x - floor(x)) >= 0.25) && (x - floor(x)) < 0.5) */
        clkdiv >>= 2;
        regVal |= CY_U3P_GCTL_UARTCLK_HALFDIV;
    }
    else /* ((clkdiv & 0x03) == 3) */
    {
        /* (x - floor(x)) >= 0.75. */
        clkdiv = ((clkdiv >> 2)  + 1);
    }

    clkdiv--;
    if ((clkdiv < 1) || (clkdiv > CY_U3P_GCTL_UARTCLK_DIV_MASK))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* For low baud rates, use CY_U3P_SYS_CLK_BY_16. */
    if (baudRate < CY_U3P_UART_BAUDRATE_600)
    {
        regVal |= (clkdiv | (CY_U3P_SYS_CLK_BY_16 << CY_U3P_GCTL_UARTCLK_SRC_POS)
            | CY_U3P_GCTL_UARTCLK_CLK_EN);
    }
    else
    {
        regVal |= (clkdiv | (CY_U3P_SYS_CLK << CY_U3P_GCTL_UARTCLK_SRC_POS)
            | CY_U3P_GCTL_UARTCLK_CLK_EN);
    }

    /* Update the register. */
    GCTL->uart_core_clk = regVal;

    return CY_U3P_SUCCESS;
}

/* This Function stop the clock for the UART */
CyU3PReturnStatus_t
CyU3PUartStopClock(
                   void)
{
    GCTL->uart_core_clk &= (~CY_U3P_GCTL_UARTCLK_CLK_EN);
    return CY_U3P_SUCCESS;
}

/* UART Interrupt handler */
void
CyU3PUartIntHandler (
                     void)
{
    CyU3PVicDisableInt (CY_U3P_VIC_UART_CORE_VECTOR);
    CyU3PLppEventSend (CY_U3P_LPP_EVENT_UART_INTR);
}

/*
* Set the frequency of the I2c depending on the bitRate.
*/
CyU3PReturnStatus_t
CyU3PI2cSetClock (
                  uint32_t bitRate)
{
    int32_t  clkdiv;
    uint32_t temp = 0;

    if (bitRate == 0)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Clock approximation needs to be done. Since we have
    * a half divider, we will use the following algorithm:
    * if x is the actual divider and n is the required
    * integral divider to be used, then following conditions
    * are used to evaluate:
    * if (x - floor(x)) < 0.25 ==> n = floor(x);
    * if (((x - floor(x)) >= 0.25) &&
    *     (x - floor(x)) < 0.5) ==> n = floor(x) + half divider;
    * if (x - floor(x)) >= 0.75 ==> n = floor(x) + 1;
    *
    * For this the source frequency is multiplied 4 times
    * and two least significant bits are used as decimal places
    * for evaluation.
    *
    * 00     ==> n = floor(x);
    * 01, 10 ==> n = floor(x) + half divider;
    * 11     ==> n = floor(x) + 1;
    */
    /* I2C core clock needs to be 10X the bus clock. */
    /* The I2C clock is set to SYS clock/16.
    * Hence (SYS clock / 16) * 4 = SYS clock / 4 */
    clkdiv = (glSysClkFreq >> 2) / (bitRate * 10);

    if ((clkdiv & 0x03) == 0)
    {
        /*  (x - floor(x)) < 0.25. */
        clkdiv >>= 2;
    }
    else if (((clkdiv & 0x03) == 1) || ((clkdiv & 0x03) == 2))
    {
        /* (((x - floor(x)) >= 0.25) && (x - floor(x)) < 0.5) */
        clkdiv >>= 2;
        temp |= CY_U3P_GCTL_I2CCLK_HALFDIV;
    }
    else /* ((clkdiv & 0x03) == 3) */
    {
        /* (x - floor(x)) >= 0.75. */
        clkdiv = ((clkdiv >> 2)  + 1);
    }

    clkdiv--;
    if ((clkdiv < 1) || (clkdiv > CY_U3P_GCTL_I2CCLK_DIV_MASK))
    {
        /* The max clock rate allowed
        * is half the source clock. and min
        * clock allowed is 2 ^ 10 divider. */
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    temp |= (clkdiv | (CY_U3P_SYS_CLK_BY_16 << CY_U3P_GCTL_I2CCLK_SRC_POS) |
        CY_U3P_GCTL_I2CCLK_CLK_EN);

    GCTL->i2c_core_clk = temp;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PI2cStopClock(
                  void)
{
    /* Disable the I2c clock */
    GCTL->i2c_core_clk &= (~CY_U3P_GCTL_I2CCLK_CLK_EN);
    return CY_U3P_SUCCESS;
}

/*
* This function sets the Drive Strength for the I2C lines.
*/
CyU3PReturnStatus_t
CyU3PSetI2cDriveStrength (
                          CyU3PDriveStrengthState_t i2cDriveStrength)
{

    /* Check if the I2C is initiaized */
    if (!((glLppActive>>CY_U3P_LPP_I2C)&1))
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (i2cDriveStrength > CY_U3P_DS_FULL_STRENGTH)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Reset the current I2C Drive strength */
    GCTL->ds &= ~(CY_U3P_I2CDS_MASK);

    /* Set the requested Drive strength */
    GCTL->ds |= (((uint32_t)i2cDriveStrength << CY_U3P_I2CDS_POS) & CY_U3P_I2CDS_MASK);

    return CY_U3P_SUCCESS;

}

void
CyU3PI2cIntHandler (
                    void)
{
    CyU3PVicDisableInt (CY_U3P_VIC_I2C_CORE_VECTOR);
    CyU3PLppEventSend (CY_U3P_LPP_EVENT_I2C_INTR);
}

/*
* Internal function which sets the frequency
* depending on the sampling rate
*/
CyU3PReturnStatus_t
CyU3PI2sSetClock (
                  uint32_t clkRate)
{
    int32_t  clkdiv;
    uint32_t temp = 0;

    if (clkRate == 0)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Clock approximation needs to be done. Since we have
    * a half divider, we will use the following algorithm:
    * if x is the actual divider and n is the required
    * integral divider to be used, then following conditions
    * are used to evaluate:
    * if (x - floor(x)) < 0.25 ==> n = floor(x);
    * if (((x - floor(x)) >= 0.25) &&
    *     (x - floor(x)) < 0.5) ==> n = floor(x) + half divider;
    * if (x - floor(x)) >= 0.75 ==> n = floor(x) + 1;
    *
    * For this the source frequency is multiplied 4 times
    * and two least significant bits are used as decimal places
    * for evaluation.
    *
    * 00     ==> n = floor(x);
    * 01, 10 ==> n = floor(x) + half divider;
    * 11     ==> n = floor(x) + 1;
    */
    clkdiv = (glSysClkFreq << 2) / clkRate;

    if ((clkdiv & 0x03) == 0)
    {
        /*  (x - floor(x)) < 0.25. */
        clkdiv >>= 2;
    }
    else if (((clkdiv & 0x03) == 1) || ((clkdiv & 0x03) == 2))
    {
        /* (((x - floor(x)) >= 0.25) && (x - floor(x)) < 0.5) */
        clkdiv >>= 2;
        temp |= CY_U3P_GCTL_I2SCLK_HALFDIV;
    }
    else /* ((clkdiv & 0x03) == 3) */
    {
        /* (x - floor(x)) >= 0.75. */
        clkdiv = ((clkdiv >> 2)  + 1);
    }

    clkdiv--;
    /* The clock divider has to be always greater than or equal to 1. */
    if ((clkdiv < 1) || (clkdiv > CY_U3P_GCTL_I2SCLK_DIV_MASK))
    {
        /* Max clock is divide by 2 and min clock
        * is divide by 2 ^ 15. */
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    temp |= (clkdiv | (CY_U3P_SYS_CLK << CY_U3P_GCTL_I2SCLK_SRC_POS)
        | CY_U3P_GCTL_I2SCLK_CLK_EN);

    GCTL->i2s_core_clk = temp;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PI2sStopClock(
                  void)
{
    /* Disable the I2s clock */
    GCTL->i2s_core_clk &= (~CY_U3P_GCTL_I2SCLK_CLK_EN);
    return CY_U3P_SUCCESS;
}


/* I2S Interrupt handler */
void
CyU3PI2sIntHandler (
                    void)
{
    CyU3PVicDisableInt (CY_U3P_VIC_I2S_CORE_VECTOR);
    CyU3PLppEventSend (CY_U3P_LPP_EVENT_I2S_INTR);
}

CyU3PReturnStatus_t
CyU3PGpioSetClock(
                  CyU3PGpioClock_t *clk_p)
{
    uint32_t value=0;
    /* Validate the input parameters. */
    if ((clk_p->fastClkDiv < 2) || (clk_p->fastClkDiv > 16))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (clk_p->slowClkDiv == 0)
    {
        if (clk_p->halfDiv)
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
    }
    else
    {
        if ((clk_p->slowClkDiv < 2) || (clk_p->slowClkDiv > 64))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
    }
    if (clk_p->clkSrc >= CY_U3P_NUM_CLK_SRC)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (clk_p->simpleDiv >= CY_U3P_GPIO_SIMPLE_NUM_DIV)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Set the GPIO fast clock. */
    value =  ((clk_p->fastClkDiv - 1) |
        ((uint32_t)clk_p->clkSrc << CY_U3P_GCTL_GPIOFCLK_SRC_POS) |
        ((uint32_t)clk_p->simpleDiv << CY_U3P_GCTL_GPIOFCLK_SIMPLE_POS) |
        CY_U3P_GCTL_GPIOFCLK_CLK_EN);
    if (clk_p->halfDiv)
    {
        value |= CY_U3P_GCTL_GPIOFCLK_HALFDIV;
    }
    GCTL->gpio_fast_clk = value;

    if (clk_p->slowClkDiv != 0)
    {
        CyU3PBusyWait (1);
        GCTL->gpio_slow_clk = (clk_p->slowClkDiv - 1) |
            CY_U3P_GCTL_GPIOFCLK_CLK_EN;
    }
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PGpioStopClock(
                   void)
{
    /* Disable the clocks. */
    GCTL->gpio_slow_clk = 0;
    CyU3PBusyWait (1);
    GCTL->gpio_fast_clk = 0;
    return CY_U3P_SUCCESS;
}

/*
* This function sets the drive strength for all the GPIOs.
*/
CyU3PReturnStatus_t
CyU3PSetGpioDriveStrength (
                           CyU3PDriveStrengthState_t gpioDriveStrength /* GPIO Drive strength */
                           )
{
    uint32_t regVal;

    if (!((glLppActive >> CY_U3P_LPP_GPIO)&1))
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (gpioDriveStrength > CY_U3P_DS_FULL_STRENGTH)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Reset the current GPIO Drive strength */
    regVal = GCTL->ds & ~(CY_U3P_PDS_G_MASK | CY_U3P_S0DS_G_MASK | CY_U3P_S1DS_G_MASK |
        CY_U3P_S1LDS_G_MASK | CY_U3P_I2CDS_G_MASK);

    /* Set the requested Drive strength */
    regVal |= (((uint32_t)gpioDriveStrength << CY_U3P_PDS_G_POS) & CY_U3P_PDS_G_MASK) |
        (((uint32_t)gpioDriveStrength << CY_U3P_S0DS_G_POS) & CY_U3P_S0DS_G_MASK) |
        (((uint32_t)gpioDriveStrength << CY_U3P_S1DS_G_POS) & CY_U3P_S1DS_G_MASK) |
        (((uint32_t)gpioDriveStrength << CY_U3P_S1LDS_G_POS) & CY_U3P_S1LDS_G_MASK) |
        (((uint32_t)gpioDriveStrength << CY_U3P_I2CDS_G_POS) & CY_U3P_I2CDS_G_MASK);
    GCTL->ds = regVal;

    return CY_U3P_SUCCESS;
}

#define CY_U3P_GPIO_COUNT (61)

CyU3PReturnStatus_t
CyU3PGpioSetIoMode (
                    uint8_t gpioId,
                    CyU3PGpioIoMode_t ioMode)
{
    uint32_t mask;
    uvint32_t *wpuPtr, *wpdPtr;

    /* Check for parameter validity. */
    if (gpioId >= CY_U3P_GPIO_COUNT)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (gpioId < 32)
    {
        mask = (1 << gpioId);
        wpuPtr = &GCTL->wpu_cfg0;
        wpdPtr = &GCTL->wpd_cfg0;
    }
    else
    {
        mask = (1 << (gpioId - 32));
        wpuPtr = &GCTL->wpu_cfg1;
        wpdPtr = &GCTL->wpd_cfg1;
    }

    switch (ioMode)
    {
    case CY_U3P_GPIO_IO_MODE_NONE:
        *wpuPtr &= ~mask;
        *wpdPtr &= ~mask;
        break;

    case CY_U3P_GPIO_IO_MODE_WPU:
        *wpdPtr &= ~mask;
        *wpuPtr |=  mask;
        break;

    case CY_U3P_GPIO_IO_MODE_WPD:
        *wpuPtr &= ~mask;
        *wpdPtr |=  mask;
        break;

    default:
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    return CY_U3P_SUCCESS;
}

void
CyU3PGpioCoreIntHandler (
                         void)
{
    /* The GPIO interrupt handler is to be called directly from the ISR. */
    if (glLppInterruptHandler[CY_U3P_LPP_GPIO] != NULL)
    {
        glLppInterruptHandler[CY_U3P_LPP_GPIO] ();
    }
    else
        CyU3PVicDisableInt (CY_U3P_VIC_GPIO_CORE_VECTOR);
}

/*
* Internal function which sets the frequency depending on the clock
*/
CyU3PReturnStatus_t
CyU3PSpiSetClock (
                  uint32_t clock)
{
    int32_t  clkdiv;
    uint32_t temp = 0;

    /* Calculate the clk division value. */
    if (clock == 0)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Clock approximation needs to be done. Since we have
    * a half divider, we will use the following algorithm:
    * if x is the actual divider and n is the required
    * integral divider to be used, then following conditions
    * are used to evaluate:
    * if (x - floor(x)) < 0.25 ==> n = floor(x);
    * if (((x - floor(x)) >= 0.25) &&
    *     (x - floor(x)) < 0.5) ==> n = floor(x) + half divider;
    * if (x - floor(x)) >= 0.75 ==> n = floor(x) + 1;
    *
    * For this the source frequency is multiplied 4 times
    * and two least significant bits are used as decimal places
    * for evaluation.
    *
    * 00     ==> n = floor(x);
    * 01, 10 ==> n = floor(x) + half divider;
    * 11     ==> n = floor(x) + 1;
    */
    /* The SPI core clock should be twice as fast as the interface clock. */
    clkdiv = (glSysClkFreq << 2) / (clock * 2);

    if ((clkdiv & 0x03) == 0)
    {
        /*  (x - floor(x)) < 0.25. */
        clkdiv >>= 2;
    }
    else if (((clkdiv & 0x03) == 1) || ((clkdiv & 0x03) == 2))
    {
        /* (((x - floor(x)) >= 0.25) && (x - floor(x)) < 0.5) */
        clkdiv >>= 2;
        temp |= CY_U3P_GCTL_SPICLK_HALFDIV;
    }
    else /* ((clkdiv & 0x03) == 3) */
    {
        /* (x - floor(x)) >= 0.75. */
        clkdiv = ((clkdiv >> 2)  + 1);
    }

    clkdiv--;
    if ((clkdiv < 1) || (clkdiv > CY_U3P_GCTL_SPICLK_DIV_MASK))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    temp |= (clkdiv | (CY_U3P_SYS_CLK << CY_U3P_GCTL_SPICLK_SRC_POS)
        | CY_U3P_GCTL_SPICLK_CLK_EN);

    GCTL->spi_core_clk = temp;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PSpiStopClock(
                  void)
{
    /* Disable the spi clock */
    GCTL->spi_core_clk &= (~CY_U3P_GCTL_SPICLK_CLK_EN);
    return CY_U3P_SUCCESS;
}

void
CyU3PSpiIntHandler (
                    void)
{
    CyU3PVicDisableInt (CY_U3P_VIC_SPI_CORE_VECTOR);
    CyU3PLppEventSend (CY_U3P_LPP_EVENT_SPI_INTR);
}

static void
CyU3PLppSocketInit (void)
{
    uint8_t i;

    /* Make sure all LPP sockets are disabled and interrupts cleared. */
    for (i = 0; i < CY_U3P_DMA_LPP_NUM_SCK; i++)
    {
        LPP->sck[i].status = CY_U3P_LPP_SCK_STATUS_DEFAULT;
        LPP->sck[i].intr = ~CY_U3P_LPP_SCK_INTR_DEFAULT;
        LPP->sck[i].intr_mask = CY_U3P_LPP_SCK_INTR_MASK_DEFAULT;
    }
}

void
CyU3PLppRestoreState (
        uint32_t *reg_p)
{
    uint32_t i;

    glLppActive = 0;
    GCTL->gpio_fast_clk = reg_p[0];
    CyU3PBusyWait (1);
    GCTL->gpio_slow_clk = reg_p[1];
    CyU3PBusyWait (1);
    GCTL->gpio_simple0  = reg_p[2];
    CyU3PBusyWait (1);
    GCTL->gpio_simple1  = reg_p[3];
    CyU3PBusyWait (1);
    GCTL->iomatrix      = reg_p[4];
    CyU3PBusyWait (1);

    CyU3PLppInit (CY_U3P_LPP_GPIO, (CyU3PLppInterruptHandler)reg_p[5]);
    CyU3PBusyWait (1);

    GPIO->lpp_gpio_power |= CY_U3P_LPP_GPIO_RESETN;
    CyU3PBusyWait (10);
    while (!(GPIO->lpp_gpio_power & CY_U3P_LPP_GPIO_ACTIVE));

    /* Now restore all GPIO state register values. */
    for (i = 0; i < 61; i++)
    {
        GPIO->lpp_gpio_simple[i] = reg_p[i + 6];
        CyU3PBusyWait (1);
    }

    /* Un-freeze the GPIOs. */
    GCTLAON->control &= ~CY_U3P_GCTL_FREEZE_IO;
}

CyBool_t
CyU3PLppGpioBlockIsOn (
        void)
{
    return glSdk_GpioIsOn;
}

CyU3PReturnStatus_t
CyU3PLppInit (
        CyU3PLppModule_t         lppModule,
        CyU3PLppInterruptHandler intrHandler)
{
    CyBool_t clkIsOn = CyFalse;

    /* Check if the block has already been started. */
    if (glLppActive & (uint8_t)(1 << lppModule))
        return CY_U3P_ERROR_ALREADY_STARTED;

    if ((lppModule == CY_U3P_LPP_I2S) && (!CyFx3DevIsI2sSupported()))
        return CY_U3P_ERROR_NOT_SUPPORTED;

    /* Ensure that the clock for this block has been turned on. */
    switch (lppModule)
    {
        case CY_U3P_LPP_I2C:
            if ((GCTL->i2c_core_clk & CY_U3P_GCTL_I2CCLK_CLK_EN) != 0)
                clkIsOn = CyTrue;
            break;
        case CY_U3P_LPP_I2S:
            if ((GCTL->i2s_core_clk & CY_U3P_GCTL_I2SCLK_CLK_EN) != 0)
                clkIsOn = CyTrue;
            break;
        case CY_U3P_LPP_SPI:
            if ((GCTL->spi_core_clk & CY_U3P_GCTL_SPICLK_CLK_EN) != 0)
                clkIsOn = CyTrue;
            break;
        case CY_U3P_LPP_UART:
            if ((GCTL->uart_core_clk & CY_U3P_GCTL_UARTCLK_CLK_EN) != 0)
                clkIsOn = CyTrue;
            break;
        case CY_U3P_LPP_GPIO:
            if ((GCTL->gpio_fast_clk & CY_U3P_GCTL_GPIOFCLK_CLK_EN) != 0)
                clkIsOn = CyTrue;
            break;
    }

    if (!clkIsOn)
        return CY_U3P_ERROR_INVALID_SEQUENCE;

    /* Identify if the LPP block has been initialized. */
    if (!glLppActive)
    {
        if (!glSdk_GpioIsOn)
        {
            /* Reset and enable the LPP block */
            LPP->power &= ~CY_U3P_LPP_RESETN;
            CyU3PBusyWait (10);
            LPP->power |= CY_U3P_LPP_RESETN;
            while (!(LPP->power & CY_U3P_LPP_ACTIVE));
        }

        /* Initialize all sockets. */
        CyU3PLppSocketInit ();

        /* Enable the LPP DMA interrupt. */
        CyU3PVicEnableInt (CY_U3P_VIC_LPP_DMA_VECTOR);
    }

    CyU3UpdateLppActiveInfo (lppModule, CyTrue, intrHandler);
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PLppDeInit (
        CyU3PLppModule_t lppModule)
{
    if ((glLppActive & (uint8_t)(1 << lppModule)) == 0)
        return CY_U3P_ERROR_NOT_STARTED;

    CyU3UpdateLppActiveInfo (lppModule, CyFalse, NULL);

    if (lppModule == CY_U3P_LPP_GPIO)
        glSdk_GpioIsOn = CyFalse;

    if ((glLppActive == 0) && (CyU3PLppGpioBlockIsOn () == CyFalse))
    {
        /* Disable the LPP DMA interrupt */
        CyU3PVicDisableInt (CY_U3P_VIC_LPP_DMA_VECTOR);
        CyU3PBusyWait (10);
        LPP->power &= ~CY_U3P_LPP_RESETN;
    }

    return CY_U3P_SUCCESS;
}

/*
* Lpp Thread
* This processes the requests from other firmware and mailbox.
*/
void
CyU3PLppThreadEntry (uint32_t threadInput)
{
    /* Local variables */
    uint32_t flag, status;

    /* Send the completion event to the System thread */
    CyU3PSysModuleInitCompleteEvt (CY_U3P_LPP_MODULE_ID);

    /* Wait for some event to happen */
    glLppMask = (CY_U3P_LPP_EVENT_I2C_INTR | CY_U3P_LPP_EVENT_I2S_INTR
        | CY_U3P_LPP_EVENT_SPI_INTR | CY_U3P_LPP_EVENT_UART_INTR);

    for (;;)
    {
        status = CyU3PEventGet (&glLppEvt, glLppMask, CYU3P_EVENT_OR_CLEAR, &flag,
            CYU3P_WAIT_FOREVER);
        /* And with the mask so as not to get spurious event */
        flag &= glLppMask;

        if (status != CY_U3P_SUCCESS)
        {
            continue;
        }

        if (flag & CY_U3P_LPP_EVENT_I2C_INTR)
        {
            if(glLppInterruptHandler[CY_U3P_LPP_I2C]  != NULL)
            {
                glLppInterruptHandler[CY_U3P_LPP_I2C]();
            }
            CyU3PVicEnableInt (CY_U3P_VIC_I2C_CORE_VECTOR);

        }
        if (flag & CY_U3P_LPP_EVENT_I2S_INTR)
        {
            if(glLppInterruptHandler[CY_U3P_LPP_I2S] != NULL)
            {
                glLppInterruptHandler[CY_U3P_LPP_I2S]();
            }
            CyU3PVicEnableInt (CY_U3P_VIC_I2S_CORE_VECTOR);

        }
        if (flag & CY_U3P_LPP_EVENT_SPI_INTR)
        {
            if(glLppInterruptHandler[CY_U3P_LPP_SPI] != NULL)
            {
                glLppInterruptHandler[CY_U3P_LPP_SPI]();
            }
            CyU3PVicEnableInt (CY_U3P_VIC_SPI_CORE_VECTOR);

        }

        if (flag & CY_U3P_LPP_EVENT_UART_INTR)
        {
            if(glLppInterruptHandler[CY_U3P_LPP_UART] != NULL)
            {
                glLppInterruptHandler[CY_U3P_LPP_UART]();
            }
            CyU3PVicEnableInt (CY_U3P_VIC_UART_CORE_VECTOR);
        }
    }
}

/*
* This function contains the instructions required to be called from CyU3PApplicationDefine
* function for the Lpp module to be working. e.g memory allocation for Lpp thread etc.
*/
void
CyU3PLppApplicationDefine (
                           void)
{
    uint8_t *pointer;

    /* Start the serial driver thread, and create an event group for it to wait to on. */
    pointer = CyU3PMemAlloc (CY_U3P_LPP_STACK_SIZE);
    CyU3PThreadCreate (&glLppThread, "05_LPP_THREAD", CyU3PLppThreadEntry, 0, pointer,
        CY_U3P_LPP_STACK_SIZE, CY_U3P_LPP_THREAD_PRIORITY, CY_U3P_LPP_THREAD_PRIORITY,
        CYU3P_NO_TIME_SLICE, CYU3P_AUTO_START);

    CyU3PEventCreate (&glLppEvt);
}

CyU3PReturnStatus_t
CyU3PLppEventSend (
                   uint32_t eventMask)
{
    uint32_t status;
    status = CyU3PEventSet (&glLppEvt, eventMask, CYU3P_EVENT_OR);
    return status;
}

/*[]*/
