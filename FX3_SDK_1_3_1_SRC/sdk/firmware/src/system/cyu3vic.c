/*
 ## Cypress USB 3.0 Platform source file (cyu3vic.c)
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

#include <cyu3gpio.h>
#include <cyu3os.h>
#include <cyu3system.h>
#include <cyu3vic.h>
#include <cyu3utils.h>
#include <cyu3regs.h>

#pragma arm

/* This file defines the VIC specific initialization and handling. */
#define CY_U3P_WDT1_DISABLE_MASK                (CY_U3P_GCTL_MODE1_MASK)
#define CY_U3P_WDT1_MODE_INTR_MASK              (1 << CY_U3P_GCTL_MODE1_POS)

/* Assuming timer clock frequency as 32KHz */
#define CY_U3P_OS_TIMER_TICK_OFFSET     (7)
#define CY_U3P_OS_TIMER_TICK_COUNT      (32)

uint16_t glOsTimerInterval = 1;
static uint32_t glOsTimerTickCount = CY_U3P_OS_TIMER_TICK_COUNT;

void
CyU3PVicInit (
        void)
{
    register uint32_t ctrlReg;

    /* Relocate the exception vectors to 0x00000000 */
#ifdef CY_USE_ARMCC
    __asm { MRC P15, 0, ctrlReg, c1, c0, 0; }
    ctrlReg &= 0xFFFFDFFF;
    __asm { MCR P15, 0, ctrlReg, c1, c0, 0; }
#else
    register uint32_t tmp;

    __asm__ volatile
        (
         "MRC   p15, 0, %0, c1, c0, 0\n\t"
         "MOV   %1, #0xFFFFDFFF\n\t"
         "AND   %0, %0, %1\n\t"
         "MCR   p15, 0, %0, c1, c0, 0\n\t"
         : "+r" (ctrlReg), "+r" (tmp)
         :
         : "memory", "cc"
        );
#endif

    /* Disable all interrupts in VIC */
    VIC->int_clear = 0xFFFFFFFF;

    /* Read back clear register to flush the write buffer */
    ctrlReg = VIC->int_clear;
    VIC->int_select = 0;
    CyU3PVicClearInt ();
    CyU3PVicSetupIntVectors ();
}

void
CyU3PVicEnableInt (
        uint32_t vectorNum)
{
    VIC->int_enable = (1 << (vectorNum));
}

uint32_t
CyU3PVicDisableAllInterrupts (
        void)
{
    uint32_t en = VIC->int_enable;

    VIC->int_clear = en;
    return en;
}

void
CyU3PVicEnableInterrupts (
        uint32_t mask)
{
    VIC->int_enable = mask;
}

void
CyU3PVicDisableInt (
        uint32_t vectorNum)
{
    VIC->int_clear = (1 << (vectorNum));
}

void
CyU3PVicClearInt (
        void)
{
    VIC->address = 0;
}

uint32_t
CyU3PVicIntGetStatus (
        void)
{
    return VIC->raw_status;
}

uint32_t
CyU3PVicIRQGetStatus (
        void)
{
    return VIC->irq_status;
}

uint32_t
CyU3PVicFIQGetStatus (
        void)
{
    return VIC->fiq_status;
}

void
CyU3PVicIntSetPriority (
        uint32_t vectorNum,
        uint32_t priority)
{
    VIC->vect_priority[(vectorNum)] = ((priority) & CY_U3P_VIC_INT_PRI_MASK);
}

uint32_t
CyU3PVicIntGetPriority (
        uint32_t vectorNum)
{
    return (VIC->vect_priority[(vectorNum)] & CY_U3P_VIC_INT_PRI_MASK);
}

void
CyU3PVicIntSetPriorityMask (
        uint32_t priorityMask)
{
    VIC->priority_mask = ((priorityMask) & CY_U3P_PRIO_MASK_MASK);
}

uint32_t
CyU3PVicIntGetPriorityMask (
        void)
{
    return (VIC->priority_mask & CY_U3P_PRIO_MASK_MASK);
}

void
CyU3POsTimerInit (
        uint16_t intervalMs)
{
    uint8_t tmp;
    uint32_t wdtcs;

    /* Using WDT1 for OS scheduling */

    /* Disable the interrupt. */
    CyU3PVicDisableInt (CY_U3P_VIC_WDT_VECTOR);

    /* Disable timer. */
    wdtcs = GCTLAON->watchdog_cs;
    CyU3PBusyWait (100);
    GCTLAON->watchdog_cs = wdtcs | CY_U3P_WDT1_DISABLE_MASK;
    CyU3PBusyWait (100);

    wdtcs = GCTLAON->watchdog_cs;

    /* If the timer interval is zero or greater than
     * 1s, the default value of 1ms will be loaded. */
    if ((intervalMs == 0) || (intervalMs > 1000))
    {
        glOsTimerInterval = 1;
        glOsTimerTickCount = CY_U3P_OS_TIMER_TICK_COUNT;
    }
    else
    {
        glOsTimerInterval = intervalMs;
        glOsTimerTickCount = intervalMs * CY_U3P_OS_TIMER_TICK_COUNT;
    }

    /* Adjust the time taken for disabling and enabling the
     * timer as a part of interrupt handling. */
    glOsTimerTickCount -= CY_U3P_OS_TIMER_TICK_OFFSET;

    /* Calculate the significant bits for the timer. */
    tmp = 0;
    while ((1 << tmp) <= glOsTimerTickCount)
    {
        tmp++;
    }

    /* Load the timer count. */
    GCTLAON->watchdog_timer1 = glOsTimerTickCount;
    CyU3PBusyWait (100);

    /* Enable the WDT1 interrupt */
    CyU3PVicEnableInt (CY_U3P_VIC_WDT_VECTOR);

    /* Start the timer after clearing any pending interrupts
     * and setting the significant bits to corresponding value. */
    wdtcs = GCTLAON->watchdog_cs & (~CY_U3P_GCTL_MODE1_MASK);
    wdtcs |= (CY_U3P_GCTL_INTR1 | (tmp << CY_U3P_GCTL_BITS1_POS));
    wdtcs |= CY_U3P_WDT1_MODE_INTR_MASK;
    CyU3PBusyWait (100);

    GCTLAON->watchdog_cs = wdtcs;
}

extern void
CyU3PWDTIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));

/* WDT interrupt handler */
void
CyU3PWDTIntHandler (
        void)
{
    uint32_t wdtcs;

    /* Disable the timer. */
    GCTLAON->watchdog_cs |= CY_U3P_WDT1_DISABLE_MASK;
    wdtcs = GCTLAON->watchdog_cs;

    /* Reload the counter. */
    GCTLAON->watchdog_timer1 = glOsTimerTickCount;

    /* Restart the timer */
    wdtcs = GCTLAON->watchdog_cs & (~CY_U3P_GCTL_MODE1_MASK);
    wdtcs |= (CY_U3P_GCTL_INTR1 | CY_U3P_WDT1_MODE_INTR_MASK);
    GCTLAON->watchdog_cs = wdtcs;

    /* Invoke the OS scheduler. */
    CyU3POsTimerHandler ();
}

/* [] */

