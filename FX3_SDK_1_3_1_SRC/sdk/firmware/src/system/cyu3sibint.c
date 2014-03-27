/*
## Cypress USB 3.0 Platform source file (cyu3sibint.c)
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

#include <cyu3regs.h>
#include <cyu3sibpp.h>
#include <cyu3vic.h>

/* Summary:
   This file defines SIB0 and SIB1 Interrupt Handlers.
 */

/* Since the interrupts cannot be handled completely in the interrupt
* context, the SIB core interrupt is disabled, and the event flag is set
* for the corresponding port so that the SIB thread can handle the
* interrupt and re-enable the core interrupt.
*/

extern void
CyU3PSib0IntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));
extern void
CyU3PSib1IntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));

/* SIB0 Interrupt handler */
void
CyU3PSib0IntHandler (
                     void)
{
    /* Disable core interrupt and set the interrupt event for Port 0. */
    CyU3PVicDisableInt (CY_U3P_VIC_SIB0_CORE_VECTOR);
    CyU3PEventSet (&glSibEvent, CY_U3P_SIB_EVT_PORT_0, CYU3P_EVENT_OR);
}

/* SIB1 Interrupt handler */
void
CyU3PSib1IntHandler (
                     void)
{
    /* Disable core interrupt and set the interrupt event for Port 1. */
    CyU3PVicDisableInt (CY_U3P_VIC_SIB1_CORE_VECTOR);
    CyU3PEventSet (&glSibEvent, CY_U3P_SIB_EVT_PORT_1, CYU3P_EVENT_OR);
}

/*[]*/
