/*
 ## Cypress USB 3.0 Platform source file (cyfx3utils.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2011-2012,
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

#include "cyfx3utils.h"

#define CY_FX3_BOOT_US_LOOP_CNT    40 /* Approximation for a 1us on silicon */

/* Summary:
   A simple delay routine.
*/
void
CyFx3BootBusyWait (
        uint16_t usWait)
{
    uint32_t i;

    while (usWait--)
    {
        /* An appoximation for 1us */
        for (i = 0; i < CY_FX3_BOOT_US_LOOP_CNT; i++)
        {
            i += 10;
            i -= 10;
        }
    }
}

/* [] */
