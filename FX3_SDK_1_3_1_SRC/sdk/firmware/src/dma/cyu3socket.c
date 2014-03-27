/*
 ## Cypress USB 3.0 Platform source file (cyu3socket.c)
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
#include <cyu3regs.h>
#include <cyu3sibpp.h>

/* This file defines all the socket related operations */

/* Global socket callback handler. */
CyU3PDmaSocketCallback_t glDmaSocketCB = NULL;

/* Struture defining all the sockets in the device */
CyU3PDmaSocket_t *glDmaSocket [CY_U3P_NUM_IP_BLOCK_ID] = 
{
    (CyU3PDmaSocket_t *)CY_U3P_LPP_SCK_DSCR_ADDRESS(0),
    (CyU3PDmaSocket_t *)CY_U3P_PIB_SCK_DSCR_ADDRESS(0),
    (CyU3PDmaSocket_t *)CY_U3P_SIB_SCK_DSCR_ADDRESS(0),
    (CyU3PDmaSocket_t *)CY_U3P_UIB_SCK_DSCR_ADDRESS(0),
    (CyU3PDmaSocket_t *)CY_U3P_UIBIN_SCK_DSCR_ADDRESS(0)
};

/* A constant array having the maximum sockets for each
 * IP block. */
const uint8_t glValidDmaSckCnt[CY_U3P_NUM_IP_BLOCK_ID] =
{
    CY_U3P_DMA_LPP_NUM_SCK,
    CY_U3P_DMA_PIB_NUM_SCK,
    CY_U3P_DMA_SIB_NUM_SCK,
    CY_U3P_DMA_UIB_NUM_SCK,
    CY_U3P_DMA_UIBIN_NUM_SCK,
};

CyBool_t
CyU3PDmaSocketIsValid (
        uint16_t sckId)
{
    uint8_t ip = CyU3PDmaGetIpNum (sckId);
    uint8_t sck = CyU3PDmaGetSckNum (sckId);
    uvint32_t *power = NULL;

    switch (ip)
    {
        case CY_U3P_LPP_IP_BLOCK_ID:
            power = (uvint32_t *)CY_U3P_LPP_POWER_ADDRESS;
            break;

        case CY_U3P_PIB_IP_BLOCK_ID:
            power = (uvint32_t *)CY_U3P_PIB_POWER_ADDRESS;
            break;

        case CY_U3P_SIB_IP_BLOCK_ID:
            power = (uvint32_t *)CY_U3P_SIB_POWER_ADDRESS;
            break;

        case CY_U3P_UIB_IP_BLOCK_ID:
        case CY_U3P_UIBIN_IP_BLOCK_ID:
            power = (uvint32_t *)CY_U3P_UIB_POWER_ADDRESS;
            break;

        default:
            return CyFalse;
    }

    if ((*power & CY_U3P_PIB_ACTIVE) == 0)
    {
        return CyFalse;
    }

    if (sck < glValidDmaSckCnt[ip])
    {
        return CyTrue;
    }

    return CyFalse;
}

CyBool_t
CyU3PDmaSocketIsValidProducer (
        uint16_t sckId)
{
    uint8_t ip = CyU3PDmaGetIpNum (sckId);
    uint8_t sck = CyU3PDmaGetSckNum (sckId);
    uint8_t min, max;
    uvint32_t *power = NULL;

    switch (ip)
    {
        case CY_U3P_LPP_IP_BLOCK_ID:
            min = CY_U3P_DMA_LPP_MIN_PROD_SCK;
            max = CY_U3P_DMA_LPP_MAX_PROD_SCK;
            power = (uvint32_t *)CY_U3P_LPP_POWER_ADDRESS;
            break;

        case CY_U3P_PIB_IP_BLOCK_ID:
            min = CY_U3P_DMA_PIB_MIN_PROD_SCK;
            max = CY_U3P_DMA_PIB_MAX_PROD_SCK;
            power = (uvint32_t *)CY_U3P_PIB_POWER_ADDRESS;
            break;

        case CY_U3P_SIB_IP_BLOCK_ID:
            min = CY_U3P_DMA_SIB_MIN_PROD_SCK;
            max = CY_U3P_DMA_SIB_MAX_PROD_SCK;
            power = (uvint32_t *)CY_U3P_SIB_POWER_ADDRESS;
            break;

        case CY_U3P_UIBIN_IP_BLOCK_ID:
            min = CY_U3P_DMA_UIBIN_MIN_PROD_SCK;
            max = CY_U3P_DMA_UIBIN_MAX_PROD_SCK;
            power = (uvint32_t *)CY_U3P_UIB_POWER_ADDRESS;
            break;

        default:
            return CyFalse;
    }

    if ((*power & CY_U3P_PIB_ACTIVE) == 0)
    {
        return CyFalse;
    }

    if ((sck >= min) && (sck <= max))
    {
        return CyTrue;
    }

    return CyFalse;
}

CyBool_t
CyU3PDmaSocketIsValidConsumer (
        uint16_t sckId)
{
    uint8_t ip = CyU3PDmaGetIpNum (sckId);
    uint8_t sck = CyU3PDmaGetSckNum (sckId);
    uint8_t min, max;
    uvint32_t *power = NULL;

    switch (ip)
    {
        case CY_U3P_LPP_IP_BLOCK_ID:
            min = CY_U3P_DMA_LPP_MIN_CONS_SCK;
            max = CY_U3P_DMA_LPP_MAX_CONS_SCK;
            power = (uvint32_t *)CY_U3P_LPP_POWER_ADDRESS;
            break;

        case CY_U3P_PIB_IP_BLOCK_ID:
            min = CY_U3P_DMA_PIB_MIN_CONS_SCK;
            max = CY_U3P_DMA_PIB_MAX_CONS_SCK;
            power = (uvint32_t *)CY_U3P_PIB_POWER_ADDRESS;
            break;

        case CY_U3P_SIB_IP_BLOCK_ID:
            min = CY_U3P_DMA_SIB_MIN_CONS_SCK;
            max = CY_U3P_DMA_SIB_MAX_CONS_SCK;
            power = (uvint32_t *)CY_U3P_SIB_POWER_ADDRESS;
            break;

        case CY_U3P_UIB_IP_BLOCK_ID:
            min = CY_U3P_DMA_UIB_MIN_CONS_SCK;
            max = CY_U3P_DMA_UIB_MAX_CONS_SCK;
            power = (uvint32_t *)CY_U3P_UIB_POWER_ADDRESS;
            break;

        default:
            return CyFalse;
    }

    if ((*power & CY_U3P_PIB_ACTIVE) == 0)
    {
        return CyFalse;
    }

    if ((sck >= min) && (sck <= max))
    {
        return CyTrue;
    }

    return CyFalse;
}

CyU3PReturnStatus_t
CyU3PDmaSocketSetConfig (
        uint16_t sckId,
        CyU3PDmaSocketConfig_t *sck_p)
{
    /* Identify the socket and load the configuration */
    uint8_t ip = CyU3PDmaGetIpNum (sckId);
    uint8_t sck = CyU3PDmaGetSckNum (sckId);

    if (sck_p == NULL)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    glDmaSocket[ip][sck].dscrChain = sck_p->dscrChain;
    glDmaSocket[ip][sck].xferSize  = sck_p->xferSize;
    glDmaSocket[ip][sck].xferCount = sck_p->xferCount;
    glDmaSocket[ip][sck].intr      = sck_p->intr;
    glDmaSocket[ip][sck].intrMask  = sck_p->intrMask;
    glDmaSocket[ip][sck].status    = sck_p->status;

    CyU3PSysBarrierSync ();

    return CY_U3P_SUCCESS;
}

static CyU3PReturnStatus_t
CyU3PDmaSocketUpdateStatus (
        uint16_t sckId,
        CyU3PDmaSocketConfig_t *sck_p)
{
    /* Identify the socket and load the configuration */
    uint8_t ip = CyU3PDmaGetIpNum (sckId);
    uint8_t sck = CyU3PDmaGetSckNum (sckId);

    if (sck_p == NULL)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Update only bits that can be safely changed. */
    glDmaSocket[ip][sck].intr      = sck_p->intr;
    glDmaSocket[ip][sck].intrMask  = sck_p->intrMask;
    glDmaSocket[ip][sck].status    = sck_p->status;

    CyU3PSysBarrierSync ();

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaSocketGetConfig (
        uint16_t sckId,
        CyU3PDmaSocketConfig_t *sck_p)
{
    /* Identify the socket and read the configuration */
    uint8_t ip = CyU3PDmaGetIpNum (sckId);
    uint8_t sck = CyU3PDmaGetSckNum (sckId);

    if (sck_p == NULL)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    sck_p->dscrChain = glDmaSocket[ip][sck].dscrChain;
    sck_p->xferSize = glDmaSocket[ip][sck].xferSize;
    sck_p->xferCount = glDmaSocket[ip][sck].xferCount;
    sck_p->status = glDmaSocket[ip][sck].status;
    sck_p->intr = glDmaSocket[ip][sck].intr;
    sck_p->intrMask = glDmaSocket[ip][sck].intrMask;

    return CY_U3P_SUCCESS;
}

void
CyU3PDmaSocketSetWrapUp (
        uint16_t sckId)
{
    uint8_t ip = CyU3PDmaGetIpNum (sckId);
    uint8_t sck = CyU3PDmaGetSckNum (sckId);
    uint32_t value  = glDmaSocket[ip][sck].status;

    /* Do nothing if the socket is disabled. */
    if ((value & CY_U3P_ENABLED) == 0)
        return;

    value |= (CY_U3P_WRAPUP);
    glDmaSocket[ip][sck].status = value;
}

void
CyU3PDmaSocketDisable (
        uint16_t sckId)
{
    uint8_t ip = CyU3PDmaGetIpNum (sckId);
    uint8_t sck = CyU3PDmaGetSckNum (sckId);
    uint32_t value  = glDmaSocket[ip][sck].status;

    /* Do nothing if the socket is already disabled. */
    if ((value & CY_U3P_ENABLED) == 0)
        return;

    value &= ~(CY_U3P_GO_ENABLE | CY_U3P_WRAPUP);
    glDmaSocket[ip][sck].status = value;
    while (glDmaSocket[ip][sck].status & (CY_U3P_ENABLED));
}

void
CyU3PDmaSocketEnable (
        uint16_t sckId)
{
    uint8_t ip = CyU3PDmaGetIpNum (sckId);
    uint8_t sck = CyU3PDmaGetSckNum (sckId);

    glDmaSocket[ip][sck].status |= CY_U3P_GO_ENABLE;
    while (!(glDmaSocket[ip][sck].status & CY_U3P_ENABLED));
}

void
CyU3PDmaSocketResume (
        uint16_t sckId)
{
    uint8_t ip = CyU3PDmaGetIpNum (sckId);
    uint8_t sck = CyU3PDmaGetSckNum (sckId);

    glDmaSocket[ip][sck].status &= (~CY_U3P_GO_SUSPEND);
    glDmaSocket[ip][sck].intr = (CY_U3P_SUSPEND | CY_U3P_PARTIAL_BUF);
    CyU3PSysBarrierSync ();
}

void
CyU3PDmaUpdateSocketSuspendOption (
        uint16_t sckId,
        uint16_t suspendOption)
{
    CyBool_t isSuspended;
    CyU3PDmaSocketConfig_t sck;

    CyU3PDmaSocketGetConfig (sckId, &sck);

    /* Idenitify if the socket is currently suspended. 
     * If the socket is suspended, then socket state
     * should not change until resume is called. */
    isSuspended = (sck.status & CY_U3P_SUSPENDED) ? CyTrue : CyFalse;
    if (isSuspended == CyTrue)
    {
        if ((sck.status & CY_U3P_GO_SUSPEND) == 0)
        {
            sck.status |= CY_U3P_GO_SUSPEND;
            sck.intr = 0;
            CyU3PDmaSocketUpdateStatus (sckId, &sck);
            /* Read back the socket configuration. */
            CyU3PDmaSocketGetConfig (sckId, &sck);
        }

        sck.intr = 0;
    }
    else
    {
        /* Before setting the suspend condition for the first time
         * clear all interrupts. */
        sck.status &= ~CY_U3P_GO_SUSPEND;
        sck.intr = (CY_U3P_SUSPEND | CY_U3P_PARTIAL_BUF);
    }

    /* By default clear all suspend options. */
    sck.status &= ~(CY_U3P_SUSP_EOP | CY_U3P_SUSP_PARTIAL);
    sck.intrMask &= ~(CY_U3P_SUSPEND | CY_U3P_PARTIAL_BUF);

    /* Set suspend options. Set the interrupt masks only if
     * the sockets are active. */
    switch (suspendOption)
    {
        case CY_U3P_DMA_SCK_SUSP_NONE:
            /* Do nothing. This is the default condition and
             * is taken care of. */
            break;

        case CY_U3P_DMA_SCK_SUSP_EOP:
            sck.status |= CY_U3P_SUSP_EOP;
            if (isSuspended == CyFalse)
            {
                sck.intrMask |= CY_U3P_SUSPEND;
            }
            break;

        case CY_U3P_DMA_SCK_SUSP_CUR_BUF:
            if (isSuspended == CyFalse)
            {
                sck.status |= CY_U3P_GO_SUSPEND;
                sck.intrMask |= CY_U3P_SUSPEND;
            }
            break;

        case CY_U3P_DMA_SCK_SUSP_CONS_PARTIAL_BUF:
            sck.status |= CY_U3P_SUSP_PARTIAL;
            if (isSuspended ==  CyFalse)
            {
                sck.intrMask |= CY_U3P_PARTIAL_BUF;
            }
            break;

        default:
            /* Do nothing as this check
             * has already been done. */
            break;
        }

        /* Update the socket. */
        CyU3PDmaSocketUpdateStatus (sckId, &sck);
}

void
CyU3PDmaUpdateSocketResume (
        uint16_t sckId,
        uint16_t suspendOption)
{
    CyU3PDmaSocketConfig_t sck;

    CyU3PDmaSocketGetConfig (sckId, &sck);

    if ((sck.status & CY_U3P_SUSPENDED) == 0)
    {
        /* Socket is not in suspended state.
         * Just return.*/
        return;
    }

    /* Clear all suspend options. */
    sck.status &= ~CY_U3P_GO_SUSPEND;
    sck.intr = (CY_U3P_SUSPEND | CY_U3P_PARTIAL_BUF);
    sck.intrMask &= ~(CY_U3P_SUSPEND | CY_U3P_PARTIAL_BUF);

    /* Set interrupt masks. */
    switch (suspendOption)
    {
        case CY_U3P_DMA_SCK_SUSP_NONE:
            /* Do nothing. This is the default condition and
             * is taken care of. */
            break;

        case CY_U3P_DMA_SCK_SUSP_EOP:
            sck.intrMask |= CY_U3P_SUSPEND;
            break;

        case CY_U3P_DMA_SCK_SUSP_CUR_BUF:
            sck.intrMask |= CY_U3P_SUSPEND;
            break;

        case CY_U3P_DMA_SCK_SUSP_CONS_PARTIAL_BUF:
            sck.intrMask |= CY_U3P_PARTIAL_BUF;
            break;

        default:
            /* Do nothing as this check
             * has already been done. */
            break;
    }

    /* Update the socket. */
    CyU3PDmaSocketUpdateStatus (sckId, &sck);
}

void
CyU3PDmaSocketSendEvent (
        uint16_t sckId,
        uint16_t dscrIndex,
        CyBool_t isOccupied)
{
    uint8_t ip = CyU3PDmaGetIpNum (sckId);
    uint8_t sck = CyU3PDmaGetSckNum (sckId);
    uint32_t type;

    type = dscrIndex;
    if (isOccupied == CyTrue) 
    {
        type |= CY_U3P_EVENT_EVENT_TYPE;
    }

    glDmaSocket[ip][sck].sckEvent = type;
    CyU3PSysBarrierSync ();
}

void
CyU3PDmaSocketRegisterCallback (
        CyU3PDmaSocketCallback_t cb)
{
    glDmaSocketCB = cb;
}

/* [] */


