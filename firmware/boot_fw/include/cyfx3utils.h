/*
 ## Cypress USB 3.0 Platform header file (cyfx3utils.h)
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

#ifndef __CYFX3UTILS_H__
#define __CYFX3UTILS_H__

#include <cyu3types.h>
#include <cyu3externcstart.h>

/** \file cyfx3utils.h
    \brief This file provides some generic utility functions for the use of the
    FX3 boot library.
 */

/** \brief Delay subroutine.

    **Description**\n
    This function provides delays in multiple of microseconds. The delay is provided
    by a busy execution loop, which means that the CPU cannot perform any other
    operations while this code is being executed.
*/
extern void
CyFx3BootBusyWait (
        uint16_t usWait         /**< Delay duration in micro-seconds. */
        );

#include <cyu3externcend.h>

#endif /* __CYFX3UTILS_H__ */

