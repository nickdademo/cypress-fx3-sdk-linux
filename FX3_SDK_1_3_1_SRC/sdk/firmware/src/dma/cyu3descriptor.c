/*
 ## Cypress USB 3.0 Platform source file (cyu3descriptor.c)
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

/* This file defines the descriptor modification functions */

#include <cyu3descriptor.h>
#include <cyu3mmu.h>
#include <cyu3error.h>
#include <cyu3regs.h>

/* The descriptors located at memory address 0x40000000. */
CyU3PDmaDescriptor_t *glDmaDescriptor = (CyU3PDmaDescriptor_t *)(CY_U3P_DMA_DSCR0_LOCATION);

CyU3PReturnStatus_t
CyU3PDmaDscrSetConfig (
        uint16_t dscrIndex,
        CyU3PDmaDescriptor_t *dscr_p)
{
    if ((dscrIndex >= CY_U3P_DMA_DSCR_COUNT) || (dscrIndex == 0))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (dscr_p == 0)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    glDmaDescriptor[dscrIndex] = *dscr_p;

    /* Make sure that the data is written to the memory */
    CyU3PSysClearDRegion ((uint32_t *)&(glDmaDescriptor[dscrIndex]), sizeof(CyU3PDmaDescriptor_t));
    CyU3PSysBarrierSync ();

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaDscrGetConfig (
        uint16_t dscrIndex,
        CyU3PDmaDescriptor_t *dscr_p)
{
    if ((dscrIndex >= CY_U3P_DMA_DSCR_COUNT) || (dscrIndex == 0))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (dscr_p == 0)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Make sure that the data is read from the memory and not the cache */
    CyU3PSysClearDRegion ((uint32_t *)&(glDmaDescriptor[dscrIndex]), sizeof(CyU3PDmaDescriptor_t));
    CyU3PSysBarrierSync ();

    *dscr_p = glDmaDescriptor[dscrIndex];

    return CY_U3P_SUCCESS;
}

/*[]*/
