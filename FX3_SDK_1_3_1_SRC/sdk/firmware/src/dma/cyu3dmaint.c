/*
 ## Cypress USB 3.0 Platform source file (cyu3dmaint.c)
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

#include <cyu3os.h>
#include <cyu3error.h>
#include <cyu3mmu.h>
#include <cyu3socket.h>
#include <cyu3dma.h>
#include <cyu3protocol.h>
#include <cyu3regs.h>
#include <cyu3sibpp.h>

/* This file defines all the DMA related interrupt handlers */

extern void
CyU3PLppDmaIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));
extern void
CyU3PPportDmaIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));
extern void
CyU3PSibDmaIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));
extern void
CyU3PUsbDmaIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));

/* DMA interrupt handler. Since the interrupt flags must be cleared for
 * subsequent interrupts to appear, the interrupt handler must not wait. 
 * The actual handling of the DMA interrupt shall be done in the thread
 * context */
static __inline void
CyU3PDmaIntHandler (
        uint8_t ipNum,
        uint8_t sckNum)
{
    CyU3PDmaChannel *h;
    uint16_t sckId;
    uint32_t tmp;
    uint32_t status;
    uint32_t msg[4];

    sckId = CyU3PDmaGetSckId(ipNum, sckNum);
    status = (glDmaSocket[ipNum][sckNum].intr & glDmaSocket[ipNum][sckNum].intrMask);

    /* Identify the channel handler */
    h = glDmaSocketCtrl[ipNum][sckNum].handle;
    if (h != 0)
    {
        /* As the DMA engine is fast, there is a change of missing
         * interrupts. Clear the interrupts, identify the active socket.
         * If the interrupt is again set, then repeat the process. */
        do
        {
            status |= (glDmaSocket[ipNum][sckNum].intr & glDmaSocket[ipNum][sckNum].intrMask);
            /* Identify if any of the interrupts are part of suspend
             * operation. If so clear the interrupt mask and not the
             * interrupt bit. */
            tmp = status & (CY_U3P_SUSPEND | CY_U3P_PARTIAL_BUF);
            glDmaSocket[ipNum][sckNum].intrMask &= ~tmp;
            /* Clear the rest of the interrupts. */
            glDmaSocket[ipNum][sckNum].intr = (status & ~tmp);
            msg[3] = glDmaSocket[ipNum][sckNum].dscrChain & CY_U3P_DSCR_NUMBER_MASK;
            tmp = (glDmaSocket[ipNum][sckNum].intr & glDmaSocket[ipNum][sckNum].intrMask);
        } while (tmp);

        msg[0] = CY_U3P_MSG_RQT;
        msg[0] |= CY_U3P_INT_MODULE_ID << CY_U3P_MSG_SRC_ID_POS;
        msg[0] |= (sckId << 16);
        msg[1] = (uint32_t)h;
        msg[2] = status;

        /* Send notification and interrupt flags to the DMA thread */
        if (CyU3PDmaMsgSend (msg, CYU3P_NO_WAIT, CyFalse) != CY_U3P_SUCCESS)
        {
            /* TODO: Use assert. */
        }
    }
    else
    {
        /* This is a socket without any channel primitive.
         * Invoke the socket interrupt callback if registered. */
        if (glDmaSocketCB != NULL)
        {
            glDmaSocketCB (sckId, status);
        }
        else
        {
            /* This is a spurious interrupt. 
             * TODO: Use assert. */
            glDmaSocket[ipNum][sckNum].intr = status;
        }
    }
}

void
CyU3PLppDmaIntHandler (
        void)
{
    uint8_t i;
    uint32_t regVal = CY_U3P_LPP_SCK_INTR0;

    /* Identify sockets having interrupt notifications */
    for(i = 0; i < CY_U3P_DMA_LPP_NUM_SCK; i++)
    {
        if (regVal & (1 << i))
        {
            CyU3PDmaIntHandler (CY_U3P_LPP_IP_BLOCK_ID, i);
        }
    }
}

void
CyU3PPportDmaIntHandler (
        void)
{
    uint8_t i;
    uint32_t regVal = CY_U3P_PIB_SCK_INTR0;
 
    /* Identify sockets having interrupt notifications */
    if (regVal & 0x0000FFFF)
    {
        for(i = 0; i < 15; i++)
        {
            if (regVal & (1 << i))
            {
                CyU3PDmaIntHandler (CY_U3P_PIB_IP_BLOCK_ID, i);
            }
        }
    }
    if (regVal & 0xFFFF0000)
    {
        for(i = 16; i < CY_U3P_DMA_PIB_NUM_SCK; i++)
        {
            if (regVal & (1 << i))
            {
                CyU3PDmaIntHandler (CY_U3P_PIB_IP_BLOCK_ID, i);
            }
        }
    }
}

void
CyU3PSibDmaIntHandler (
        void)
{
    uint8_t i;
    uint32_t regVal = CY_U3P_SIB_SCK_INTR0;
 
    /* Identify sockets having interrupt notifications */
    for(i = 0; i < CY_U3P_DMA_SIB_NUM_SCK; i++)
    {
        if (regVal & (1 << i))
        {
            CyU3PDmaIntHandler (CY_U3P_SIB_IP_BLOCK_ID, i);
        }
    }
}

void
CyU3PUsbDmaIntHandler (
        void)
{
    uint8_t i;
    uint32_t regVal = CY_U3P_UIB_SCK_INTR0;
 
    /* Identify egress sockets having interrupt notifications */
    if (regVal)
    {
        for(i = 0; i < CY_U3P_DMA_UIB_NUM_SCK; i++)
        {
            if (regVal & (1 << i))
            {
                CyU3PDmaIntHandler (CY_U3P_UIB_IP_BLOCK_ID, i);
            }
        }
    }

    regVal = CY_U3P_UIBIN_SCK_INTR0;
    /* Identify ingress sockets having interrupt notifications */
    if (regVal)
    {
        for(i = 0; i < CY_U3P_DMA_UIBIN_NUM_SCK; i++)
        {
            if (regVal & (1 << i))
            {
                CyU3PDmaIntHandler (CY_U3P_UIBIN_IP_BLOCK_ID, i);
            }
        }
    }
}

/* [] */

