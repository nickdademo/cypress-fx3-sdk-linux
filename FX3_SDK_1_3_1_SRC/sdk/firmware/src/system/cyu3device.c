/*
 ## Cypress USB 3.0 Platform source file (cyu3device.c)
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

/* This file defines the device specific APIs. */

#include <cyu3system.h>
#include <cyu3mmu.h>
#include <cyu3vic.h>
#include <cyu3error.h>
#include <cyu3utils.h>
#include <cyu3regs.h>
#include <cyfx3_api.h>
#include <cyu3usbpp.h>

#include <cyfx3_api.h>

#define CY_U3P_SYS_BACKUP_CLK_FREQ      (32000) /* Backup clock frequency.                  */

#pragma arm

uint32_t glSysClkFreq = 0;
uint32_t glWatchDogPeriod = 0;
uint32_t glUsbMpllDefault = 0x910410;

CyBool_t glIsICacheEnabled = CyFalse;
CyBool_t glIsDCacheEnabled = CyFalse;
CyBool_t glDmaHandleDCache = CyFalse;

#define CYU3P_WATCHDOG_MULTIPLIER       (33)
#define CYU3P_WATCHDOG_MODE             (2)

CyBool_t glSdk_UsbIsOn  = CyFalse;
CyBool_t glSdk_GpioIsOn = CyFalse;

static void
CyU3PDevCheckDevStateStruct (
        void)
{
    CyU3PUsbDescTable_t *state_p = (CyU3PUsbDescTable_t *)CYU3P_DEVSTATE_LOCATION;
    uint32_t length, checksum = 0;

    if (state_p->signature != CY_USBDEV_SIGNATURE)
        goto StructErrFound;

    /* Work-around for checksum calculation bug in 1.2.x and older SDKs. */
    if ((state_p->bootSignature == CY_USB_BOOTER_SIGNATURE) && (state_p->revision >= CY_USB_BOOTER_REV_1_3_0))
        length = state_p->length;
    else
        length = state_p->length & 0xFF;
    if ((state_p->length == 0) || (state_p->length > (0x1000 - 8)))
        goto StructErrFound;

    CyU3PComputeChecksum ((uint32_t *)CYU3P_DEVSTATE_LOCATION, length, &checksum);
    if (checksum != *((uint32_t *)(CYU3P_DEVSTATE_LOCATION + state_p->length)))
        goto StructErrFound;

    /* If the boot signature is not found, assume that the booter version is 1.1.1 */
    if (state_p->bootSignature != CY_USB_BOOTER_SIGNATURE)
        state_p->revision = CY_USB_BOOTER_REV_1_1_1;

    glSdk_UsbIsOn = CyTrue;
    if ((state_p->revision >= CY_USB_BOOTER_REV_1_2_1) && (state_p->leaveGpioOn))
    {
        /* Check if the GPIO block is on. */
        if (((GCTL->gpio_fast_clk & CY_U3P_GCTL_GPIOFCLK_CLK_EN) != 0) &&
                ((GPIO->lpp_gpio_power & CY_U3P_LPP_GPIO_ACTIVE) != 0))
        {
            glSdk_GpioIsOn = CyTrue;
        }
    }
    return;

StructErrFound:
    /* Make sure that the structure signature is corrupted at this stage. */
    state_p->signature = 0xFFFFFFFF;
}

extern uint32_t *glGpioRegBkp;
extern void
CyU3PLppRestoreState (
        uint32_t *reg_p);

CyU3PReturnStatus_t
CyU3PDeviceInit (
        CyU3PSysClockConfig_t *clkCfg)
{
    uint32_t t1;
    CyBool_t useDivider = CyFalse;
    CyBool_t watchdogEn = CyFalse;
    uint32_t t2, backupDiv = 0;
    CyBool_t setSysClkDiv = CyFalse;

    if (clkCfg != NULL)
    {
        if ((clkCfg->cpuClkDiv < 2) || (clkCfg->cpuClkDiv > 16))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
        if ((clkCfg->dmaClkDiv < 2) || (clkCfg->dmaClkDiv > 16))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
        if ((clkCfg->mmioClkDiv < 2) || (clkCfg->mmioClkDiv > 16))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
        if ((clkCfg->mmioClkDiv < clkCfg->dmaClkDiv) ||
                (clkCfg->mmioClkDiv % clkCfg->dmaClkDiv != 0))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
        if (clkCfg->clkSrc >= CY_U3P_NUM_CLK_SRC)
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }

        t1 = ((clkCfg->cpuClkDiv - 1) | ((clkCfg->dmaClkDiv - 1) << CY_U3P_GCTL_CPUCLK_DMA_DIV_POS) |
                ((clkCfg->mmioClkDiv - 1) << CY_U3P_GCTL_CPUCLK_MMIO_DIV_POS) |
                (clkCfg->clkSrc << CY_U3P_GCTL_CPUCLK_SRC_POS));

        setSysClkDiv = clkCfg->setSysClk400;
    }
    else
    {
        /* Set CPU, DMA and MMIO clock to divide to 2, and CLK_SRC to SYS_CLK. */
        t1 = (1 | (1 << CY_U3P_GCTL_CPUCLK_DMA_DIV_POS) |
                (1 << CY_U3P_GCTL_CPUCLK_MMIO_DIV_POS) |
                (CY_U3P_SYS_CLK << CY_U3P_GCTL_CPUCLK_SRC_POS));
    }

    /* Update the CPU clock register and read back to flush. */
    GCTL->cpu_clk_cfg = t1;
    t1 = GCTL->cpu_clk_cfg;

    /* Wait for sometime for the clock to stabilize. */
    CyU3PBusyWait (10);

    /* Identify the SYS_CLK. FSLC[1:0] gives the frequency
     * and FSLC[2] gives the source. */
    t1 = ((GCTL->pll_cfg & CY_U3P_FSLC_MASK) >> CY_U3P_FSLC_POS);
    switch (t1 & 0x03)
    {
    /* FSLC * FBDIV / REFDIV / OUTDIV */
    case 0: /* 19.2MHz * 21 / 1  / 1. */
        backupDiv    = 19200000;
        if (setSysClkDiv)
            glSysClkFreq = 403200000;
        else
            glSysClkFreq = 384000000;
        break;

    case 1: /* 26.0MHz * 16 / 1  / 1. */
        backupDiv        = 26000000;
        glSysClkFreq     = 416000000;
        glUsbMpllDefault = 0x910700;
        break;

    case 2: /* 38.4MHz * 21 / 2  / 1. */
        backupDiv        = 38400000;
        glUsbMpllDefault = 0x910608;
        if (setSysClkDiv)
            glSysClkFreq = 403200000;
        else
            glSysClkFreq = 384000000;
        break;

    default: /* 52.0MHz * 16 / 2  / 1. */
        backupDiv        = 52000000;
        glSysClkFreq     = 416000000;
        glUsbMpllDefault = 0x910580;
        break;
    }

    /* If the SYSCLK has already been switched to 403.2 MHz, leave it at that setting. */
    if ((GCTL->pll_cfg & CY_U3P_FBDIV_MASK) == 21)
        glSysClkFreq = 403200000;

    /* Update the SYSCLK setting if the reference clock source is a 19.2 MHz crystal. */
    if ((glSysClkFreq == 403200000) && ((GCTL->pll_cfg & CY_U3P_FBDIV_MASK) != 21))
    {
        GCTL->pll_cfg = GCTL->pll_cfg & ~CY_U3P_FBDIV_MASK | 21;
        CyU3PBusyWait (100);
        while ((GCTL->pll_cfg & CY_U3P_PLL_LOCK) == 0);
        CyU3PBusyWait (100);
    }

    t2 = GCTLAON->watchdog_cs;
    CyU3PBusyWait (10);

    if ((t2 & CY_U3P_GCTL_MODE0_MASK) == CYU3P_WATCHDOG_MODE)
    {
        watchdogEn = CyTrue;
        GCTLAON->watchdog_cs = t2 | CY_U3P_GCTL_MODE0_MASK;
        CyU3PBusyWait (10);
    }

    /* If the boot-loader has already setup the backup clock, then do not disable this.
       The bit cannot be cleared safely. Otherwise, check for user specification. */
    if ((t2 & CY_U3P_GCTL_BACKUP_CLK) || ((clkCfg != NULL) && (clkCfg->useStandbyClk == CyFalse)))
    {
        useDivider = CyTrue;
    }

    if (!useDivider)
    {
        /* Check whether 32 KHz clock input is available by turning on the watchdog timer for 200 us.
           If there is a clock input, the timer is expected to move by 5 or 6 in this duration.
         */

        /* Enable WDT0 free running mode. */
        GCTLAON->watchdog_cs = ((t2 & ~CY_U3P_GCTL_MODE0_MASK) | (1 << CY_U3P_GCTL_MODE0_POS));
        CyU3PBusyWait (100);

        t1 = GCTLAON->watchdog_timer0;
        CyU3PBusyWait (200);

        if (t1 == GCTLAON->watchdog_timer0)
        {
            /* Timer has not moved. No clock input is available. Use the backup divider. */
            useDivider = CyTrue;
        }

        /* Restore the WDT settings. */
        CyU3PBusyWait (100);
        GCTLAON->watchdog_cs = t2;
        CyU3PBusyWait (100);
    }

    /* Do not update the divider if it is already been updated by the bootloader. */
    if ((useDivider) && (!(GCTLAON->watchdog_cs & CY_U3P_GCTL_BACKUP_CLK)))
    {
        /* Apply 0.5 approximation. If the actual divider is x and 
         * the integral divider is n, then
         * (x - floor(x)) < 0.5 ==> n = floor(x);
         * (x - floor(x)) >= 0.5 ==> n = floor(x) + 1;
         *
         * For this source frequency is multiplied by 4,
         * and the last two bits are used for deciding this.
         * if last 2 bits evaluate to 00 or 01 then it is floor
         * and if the last two bits evaluate to 10 or 11 then
         * it is floor + 1. Here the backupDiv represents divider - 1.
         */
        backupDiv <<= 2;
        backupDiv /= CY_U3P_SYS_BACKUP_CLK_FREQ;
        t1 = backupDiv & 0x03;
        backupDiv >>= 2;
        if (t1 < 0x10)
        {
            backupDiv--;
        }

        /* If the divider has gone beyond valid value, then set to the maximum valid divider. */
        backupDiv <<= CY_U3P_GCTL_BACKUP_DIVIDER_POS;
        if (backupDiv > CY_U3P_GCTL_BACKUP_DIVIDER_MASK)
        {
            backupDiv = CY_U3P_GCTL_BACKUP_DIVIDER_MASK;
        }

        CyU3PBusyWait (100);
        t1 = GCTLAON->watchdog_cs;
        t1 = (t1 & ~CY_U3P_GCTL_BACKUP_DIVIDER_MASK) | backupDiv;
        CyU3PBusyWait (100);

        GCTLAON->watchdog_cs = t1;
        CyU3PBusyWait (100);

        GCTLAON->watchdog_cs |= CY_U3P_GCTL_BACKUP_CLK;
        CyU3PBusyWait (100);
    }

    /* Clear the WARM_BOOT flag. */
    GCTLAON->control &= ~CY_U3P_GCTL_WARM_BOOT;
    CyU3PBusyWait (100);

    CyU3PVicInit ();

    /* Enable interrupts globally at this point. */
#ifdef CY_USE_ARMCC
    __enable_irq ();
#else
    __asm__ volatile
        (
         "stmdb sp!, {r0}\n\t"                  /* Save R0 value on the stack. */
         "mrs   r0, CPSR\n\t"                   /* Read current CPSR value. */
         "bic   r0, r0, #0xC0\n\t"              /* Enable IRQ and FIQ. */
         "msr   CPSR_c, r0\n\t"                 /* Write back to disable interrupts. */
         "ldmia sp!, {r0}\n\t"                  /* Restore R0 register content. */
        );
#endif

    /* If the boot firmware had enabled the watchdog timer, enable it again with a duration of 1 second. */
    if (watchdogEn)
    {
        CyU3PSysWatchDogConfigure (CyTrue, 1000);
    }

    if (glGpioRegBkp)
        CyU3PLppRestoreState (glGpioRegBkp);

    /* Check for device status information stored by the boot firmware. */
    CyU3PDevCheckDevStateStruct ();

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDeviceGetSysClkFreq (
        uint32_t *freq)
{
    if (freq == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if (glSysClkFreq == 0)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    *freq = glSysClkFreq;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDeviceCacheControl (
        CyBool_t isICacheEnable,
        CyBool_t isDCacheEnable,
        CyBool_t isDmaHandleDCache)
{
    if ((isDmaHandleDCache) && (!isDCacheEnable))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    CyU3PSysBarrierSync ();
    CyU3PSysClearDCache ();
    CyU3PSysFlushICache ();
    CyU3PSysDisableICache ();
    CyU3PSysDisableDCache ();

    if (isICacheEnable)
    {
        CyU3PSysEnableICache ();
        CyU3PSysBarrierSync ();
        glIsICacheEnabled = isICacheEnable;
    }

    glDmaHandleDCache = CyFalse;
    if (isDCacheEnable)
    {
        CyU3PSysEnableDCache ();
        CyU3PSysBarrierSync ();
        glDmaHandleDCache = isDmaHandleDCache;
        glIsDCacheEnabled = isDCacheEnable;
    }

    CyU3PSysBarrierSync ();

    return CY_U3P_SUCCESS;
}

/* Reset the FX3 device. */
void
CyU3PDeviceReset (
        CyBool_t isWarmReset)
{
    uint32_t ctrl;

    ctrl = GCTLAON->control;
    CyU3PBusyWait (100);

    if (isWarmReset)
    {
        /* Copy the firmware entry address into location 0x40000000. */
        *((uint32_t *)0x40000000) = (uint32_t)(&CyU3PFirmwareEntry);

        /* Make sure the D-Cache is cleaned and disabled. */
        CyU3PSysClearDCache ();
        CyU3PSysDisableDCache ();

        ctrl |= CY_U3P_GCTL_WARM_BOOT;
        VIC->int_clear = 0xFFFFFFFF;
	CyU3PFreeHeaps ();
    }

    ctrl &= ~CY_U3P_GCTL_HARD_RESET_N;
    GCTLAON->control = ctrl;

    for (;;)
        CyU3PBusyWait (1);
}

void
CyU3PSysSetupMMU (
        void)
{
    CyU3PSysBarrierSync ();
    CyU3PSysClearDCache ();
    CyU3PSysFlushICache ();
    CyU3PSysDisableCacheMMU ();
    CyU3PSysInitTCMs ();

    CyU3PInitPageTable ();
    CyU3PSysEnableMMU ();
    CyU3PSysLoadTLB ();

    CyU3PSysBarrierSync ();
}

void
CyU3PSysWatchDogConfigure (
        CyBool_t enable,
        uint32_t period)
{
    uint32_t wdcs;
   
    wdcs = GCTLAON->watchdog_cs;
    CyU3PBusyWait (100);

    if (enable)
    {
        /* If watchdog0 is already enabled, disable and then re-enable with appropriate frequency. */
        if ((wdcs & CY_U3P_GCTL_MODE0_MASK) != CY_U3P_GCTL_MODE0_MASK)
        {
            GCTLAON->watchdog_cs = wdcs | CY_U3P_GCTL_MODE0_MASK;
            CyU3PBusyWait (100);
        }

        glWatchDogPeriod = (period * CYU3P_WATCHDOG_MULTIPLIER);
        if (glWatchDogPeriod < period)
            glWatchDogPeriod = 0xFFFFFFFF;

        GCTLAON->watchdog_timer0 = glWatchDogPeriod;
        CyU3PBusyWait (100);

        /* Configure watchdog0 as required. */
        wdcs  = (wdcs & ~(CY_U3P_GCTL_MODE0_MASK | CY_U3P_GCTL_BITS0_MASK));
        wdcs |= (CY_U3P_GCTL_BITS0_MASK | CYU3P_WATCHDOG_MODE);

        GCTLAON->watchdog_cs = wdcs;
        CyU3PBusyWait (100);

        /* Ensure that watchdog1 configuration is not screwed up. */
        if ((wdcs & CY_U3P_GCTL_MODE1_MASK) == CY_U3P_GCTL_MODE1_MASK)
        {
            CyU3POsTimerInit (glOsTimerInterval);
        }
    }
    else
    {
        /* Disable watchdog0 if currently enabled. */
        if ((wdcs & CY_U3P_GCTL_MODE0_MASK) != CY_U3P_GCTL_MODE0_MASK)
        {
            GCTLAON->watchdog_cs |= CY_U3P_GCTL_MODE0_MASK;
            CyU3PBusyWait (100);
        }
        glWatchDogPeriod = 0;
    }
}

void
CyU3PSysWatchDogClear (
        void)
{
    if (glWatchDogPeriod)
    {
        if (glWatchDogPeriod & 0x01)
            glWatchDogPeriod--;
        else
            glWatchDogPeriod++;
        GCTLAON->watchdog_timer0 = glWatchDogPeriod;
        CyU3PBusyWait (100);
    }
}

CyU3PPartNumber_t
CyU3PDeviceGetPartNumber (
        void)
{
    return (CyFx3DevIdentifyPart ());
}

