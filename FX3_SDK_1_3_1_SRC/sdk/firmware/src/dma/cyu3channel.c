/*
 ## Cypress USB 3.0 Platform source file (cyu3channel.c)
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

/* This file defines the channel based DMA model */

#include <cyu3os.h>
#include <cyu3error.h>
#include <cyu3mmu.h>
#include <cyu3socket.h>
#include <cyu3dma.h>
#include <cyu3regs.h>
#include <cyu3utils.h>

#define CY_U3P_DMA_API_THREAD_PRIORITY      (2)

CyU3PDmaChannel *
CyU3PDmaChannelGetHandle (
        CyU3PDmaSocketId_t sckId)
{
    uint8_t ipNum, sckNum;

    /* Verify that this is a valid socket. */
    if (!CyU3PDmaSocketIsValid (sckId))
    {
        return NULL;
    }

    ipNum = CyU3PDmaGetIpNum (sckId);
    sckNum = CyU3PDmaGetSckNum (sckId);

    return glDmaSocketCtrl[ipNum][sckNum].singleHandle;
}

static CyU3PReturnStatus_t
CyU3PDmaChannelAcquireLock (
        CyU3PDmaChannel *handle,
        uint32_t waitOption)
{
    if (handle == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if (handle->state == CY_U3P_DMA_NOT_CONFIGURED)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (CyU3PMutexGet (&(handle->lock), waitOption) != CY_U3P_SUCCESS)
    {
        return CY_U3P_ERROR_MUTEX_FAILURE;
    }

    return CY_U3P_SUCCESS;
}

static void
CyU3PDmaConfigureSockets_TypeAuto (
        CyU3PDmaChannel *handle)
{
    CyU3PDmaSocketConfig_t sck;

    /* Disable sockets before configuring */
    CyU3PDmaSocketDisable (handle->prodSckId);
    CyU3PDmaSocketDisable (handle->consSckId);

    /* Configure the producer socket */
    CyU3PDmaSocketGetConfig (handle->prodSckId, &sck);
    sck.dscrChain = handle->firstProdIndex;
    sck.xferSize = 0;
    sck.xferCount = 0;
    sck.status = (CY_U3P_TRUNCATE | CY_U3P_EN_PROD_EVENTS | CY_U3P_SUSP_TRANS);
    if (CyU3PDmaIsSockAvlEnReqd (handle->prodSckId))
        sck.status |= CY_U3P_UIB_AVL_ENABLE;

    sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
    sck.intrMask = (handle->type == CY_U3P_DMA_TYPE_AUTO_SIGNAL) ? \
                        (CY_U3P_ERROR | CY_U3P_PRODUCE_EVENT) : (CY_U3P_ERROR);
    CyU3PDmaSocketSetConfig (handle->prodSckId, &sck);

    /* Configure the consumer socket */
    CyU3PDmaSocketGetConfig (handle->consSckId, &sck);
    sck.dscrChain = handle->firstProdIndex;
    sck.xferSize = 0;
    sck.xferCount = 0;
    sck.status = (CY_U3P_TRUNCATE | CY_U3P_EN_CONS_EVENTS | CY_U3P_SUSP_TRANS);
    if (CyU3PDmaIsSockAvlEnReqd (handle->consSckId))
        sck.status |= CY_U3P_UIB_AVL_ENABLE;

    sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
    sck.intrMask = (CY_U3P_TRANS_DONE | CY_U3P_ERROR);
    CyU3PDmaSocketSetConfig (handle->consSckId, &sck);

    /* Reset the discard count to zero. */
    handle->discardCount = 0;
}

static CyU3PReturnStatus_t
CyU3PDmaChannelConfigure_TypeAuto (
        CyU3PDmaChannel *handle)
{
    uint32_t status, dscrSync;
    
    /* Allocate and update the required set of descriptors */
    dscrSync   = (handle->consSckId | CY_U3P_EN_CONS_EVENT);
    dscrSync  |= ((handle->prodSckId << CY_U3P_PROD_SCK_POS) |
            CY_U3P_EN_PROD_EVENT);
    if (handle->type == CY_U3P_DMA_TYPE_AUTO_SIGNAL)
    {
        /* Enable produce events. */
        dscrSync |= CY_U3P_EN_PROD_INT;
    }
    status = CyU3PDmaDscrChainCreate (&(handle->firstProdIndex),
            handle->count, handle->size, dscrSync);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Update the descriptor pointers. */
    handle->activeProdIndex = handle->commitProdIndex =
        handle->currentProdIndex = handle->firstProdIndex;
    handle->activeConsIndex = handle->commitConsIndex =
        handle->currentConsIndex = handle->firstConsIndex;

    /* Allocate a override descriptor */
    CyU3PDmaDscrGet (&(handle->overrideDscrIndex));

    /* Configure the sockets. */
    CyU3PDmaConfigureSockets_TypeAuto (handle);

    return CY_U3P_SUCCESS;
}

static void
CyU3PDmaConfigureSockets_TypeManual (
        CyU3PDmaChannel *handle)
{
    CyU3PDmaSocketConfig_t sck;

    /* Configure the producer socket */
    if (handle->type != CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        CyU3PDmaSocketDisable (handle->prodSckId);
        CyU3PDmaSocketGetConfig (handle->prodSckId, &sck);
        sck.dscrChain = handle->firstProdIndex;
        sck.xferSize = 0;
        sck.xferCount = 0;
        sck.status = (CY_U3P_TRUNCATE | CY_U3P_EN_PROD_EVENTS | CY_U3P_SUSP_TRANS);
        if (CyU3PDmaIsSockAvlEnReqd (handle->prodSckId))
            sck.status |= CY_U3P_UIB_AVL_ENABLE;

        sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
        sck.intrMask = (CY_U3P_ERROR | CY_U3P_PRODUCE_EVENT);
        if (handle->type == CY_U3P_DMA_TYPE_MANUAL_IN)
        {
            sck.intrMask |= CY_U3P_TRANS_DONE;
        }
        CyU3PDmaSocketSetConfig (handle->prodSckId, &sck);
    }

    /* Configure the consumer socket */
    if (handle->type != CY_U3P_DMA_TYPE_MANUAL_IN)
    {
        CyU3PDmaSocketDisable (handle->consSckId);
        CyU3PDmaSocketGetConfig (handle->consSckId, &sck);
        sck.dscrChain = handle->firstConsIndex;
        sck.xferSize = 0;
        sck.xferCount = 0;
        sck.status = (CY_U3P_TRUNCATE | CY_U3P_EN_CONS_EVENTS | CY_U3P_SUSP_TRANS);
        if (CyU3PDmaIsSockAvlEnReqd (handle->consSckId))
            sck.status |= CY_U3P_UIB_AVL_ENABLE;

        sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
        sck.intrMask = (CY_U3P_CONSUME_EVENT | CY_U3P_TRANS_DONE |
                CY_U3P_ERROR);

        CyU3PDmaSocketSetConfig (handle->consSckId, &sck);
    }

    /* Reset the discard count to zero. */
    handle->discardCount = 0;
}

static CyU3PReturnStatus_t
CyU3PDmaChannelConfigure_TypeManual (
        CyU3PDmaChannel *handle)
{
    CyU3PDmaDescriptor_t dscr, consDscr;
    uint32_t status, dscrSync;
    uint16_t *dscrIndex_p = &(handle->firstProdIndex);
    uint16_t count, index;

    /* This is the descriptor sync value for both MANUAL and MANUAL_IN */
    dscrSync   = (CY_U3P_CPU_SOCKET_CONS | CY_U3P_EN_CONS_EVENT |
            CY_U3P_EN_CONS_INT);
    dscrSync  |= (((handle->prodSckId << CY_U3P_PROD_SCK_POS) |
                CY_U3P_EN_PROD_EVENT | CY_U3P_EN_PROD_INT));

    if (handle->type == CY_U3P_DMA_TYPE_MANUAL_IN)
    {
        handle->consSckId = CY_U3P_CPU_SOCKET_CONS;
    }
    else if (handle->type == CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        handle->prodSckId = CY_U3P_CPU_SOCKET_PROD;
        dscrIndex_p = &(handle->firstConsIndex);
        dscrSync    = (handle->consSckId | CY_U3P_EN_CONS_EVENT |
                CY_U3P_EN_CONS_INT);
        dscrSync   |= (((CY_U3P_CPU_SOCKET_PROD << CY_U3P_PROD_SCK_POS) |
                    CY_U3P_EN_PROD_EVENT | CY_U3P_EN_PROD_INT));
    }

    /* Create the primary descriptor chain. */
    if (handle->count != 0)
    {
        status = CyU3PDmaDscrChainCreate (dscrIndex_p, handle->count,
                handle->size, dscrSync);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        if (handle->type == CY_U3P_DMA_TYPE_MANUAL)
        {
            /* Create the consumer descriptor chain but do not allocate memory. */
            dscrSync   = (handle->consSckId | CY_U3P_EN_CONS_EVENT |
                        CY_U3P_EN_CONS_INT);
            dscrSync  |= (((CY_U3P_CPU_SOCKET_PROD << CY_U3P_PROD_SCK_POS) |
                        CY_U3P_EN_PROD_EVENT | CY_U3P_EN_PROD_INT));
            status = CyU3PDmaDscrChainCreate (&(handle->firstConsIndex), handle->count,
                    0, dscrSync);
            if (status != CY_U3P_SUCCESS)
            {
                /* Free the producer chain. */
                CyU3PDmaDscrChainDestroy (*dscrIndex_p, handle->count, CyTrue, CyTrue);
                return status;
            }
        }

        handle->activeProdIndex = handle->commitProdIndex =
            handle->currentProdIndex = handle->firstProdIndex;
        handle->activeConsIndex = handle->commitConsIndex =
            handle->currentConsIndex = handle->firstConsIndex;

        /* Modify the descriptor chain(s) for channel specific parameters. */
        for (count = 0, index = *dscrIndex_p; count < handle->count; count++)
        {
            CyU3PDmaDscrGetConfig (index, &dscr);
            if (handle->type == CY_U3P_DMA_TYPE_MANUAL_OUT)
            {
                dscr.buffer += handle->consHeader;
                /* The magic number 0xF is present to make the size multiple of 16 bytes */
                /* Since the consumer socket can actually transmit data of sizes not aligned to
                 * 16 bytes unlike the producer socket, the create call does not mandate
                 * the caller to create buffers of sizes to match. But the descriptor takes only
                 * buffer sizes as multiples of 16 bytes. So the size needs to be calculated
                 * for descriptor update. But transfer can only be done to the actual buffer size.
                 * This count is only for the hardware information. */
                dscr.size = ((handle->size + 0xF - handle->consHeader) &
                        CY_U3P_BUFFER_SIZE_MASK);
            }
            else /* CY_U3P_DMA_TYPE_MANUAL_IN, CY_U3P_DMA_TYPE_MANUAL */
            {
                if (handle->type == CY_U3P_DMA_TYPE_MANUAL)
                {
                    /* In case of MANUAL mode, the consumer chain has to be 
                     * updated with the buffer information as well. */
                    CyU3PDmaDscrGetConfig (handle->currentConsIndex, &consDscr);
                    consDscr.buffer = dscr.buffer + handle->consHeader;
                    /* Same as in MANUAL_OUT. */
                    consDscr.size = ((handle->size + 0xF - handle->consHeader) &
                            CY_U3P_BUFFER_SIZE_MASK);
                    CyU3PDmaDscrSetConfig (handle->currentConsIndex, &consDscr);
                    /* Move currentConsIndex to the next consumer descriptor. */
                    handle->currentConsIndex = (consDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
                }

                dscr.buffer += handle->prodHeader;
                dscr.size   = ((handle->size - handle->prodHeader - handle->prodFooter) &
                        CY_U3P_BUFFER_SIZE_MASK);
            }

            CyU3PDmaDscrSetConfig (index, &dscr);
            index = (dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
        }
    }
    else /* Only used for the override modes. */
    {
        handle->activeProdIndex = handle->commitProdIndex =
            handle->currentProdIndex = handle->firstProdIndex = 0xFFFF;
        handle->activeConsIndex = handle->commitConsIndex =
            handle->currentConsIndex = handle->firstConsIndex = 0xFFFF;
    }

    /* Allocate an override descriptor */
    CyU3PDmaDscrGet (&(handle->overrideDscrIndex));
    /* Configure the sockets. */
    CyU3PDmaConfigureSockets_TypeManual (handle);

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaChannelCreate (
        CyU3PDmaChannel *handle,
        CyU3PDmaType_t type,
        CyU3PDmaChannelConfig_t *config)
{
    uint32_t status;
    uint8_t sckNum, ipNum;

    /* Parameter error checking */
    if (config == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if (handle == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if ((CyU3PDmaDscrGetFreeCount () <= (config->count + 1)) ||
            ((type == CY_U3P_DMA_TYPE_MANUAL) && 
             (CyU3PDmaDscrGetFreeCount () <= ((config->count << 2) + 1))))
    {
        return CY_U3P_ERROR_MEMORY_ERROR;
    }
    if ((config->size == 0) || (config->size > CY_U3P_DMA_MAX_BUFFER_SIZE))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if ((type == CY_U3P_DMA_TYPE_AUTO) ||
            (type == CY_U3P_DMA_TYPE_AUTO_SIGNAL))
    {
        if (config->size & (~CY_U3P_BUFFER_SIZE_MASK))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
        if (config->count == 0)
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
    }
    else
    {
        if ((config->size - config->prodHeader - config->prodFooter) &
                (~CY_U3P_BUFFER_SIZE_MASK))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
    }
    if (config->dmaMode >= CY_U3P_DMA_NUM_MODES)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Verfiy that the sockets are not already associated 
     * with a channel. 
     */
    if (type != CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        /* Producer socket */
        ipNum = CyU3PDmaGetIpNum (config->prodSckId);
        sckNum = CyU3PDmaGetSckNum (config->prodSckId);
        /* Verify that this is a valid socket. */
        if (!CyU3PDmaSocketIsValidProducer (config->prodSckId))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
        if (glDmaSocketCtrl[ipNum][sckNum].singleHandle != NULL)
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
    }
    else /* CY_U3P_DMA_TYPE_MANUAL_OUT */
    {
        /* Verify that the producer socket is CPU. */
        if (config->prodSckId != CY_U3P_CPU_SOCKET_PROD)
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
    }
    if (type != CY_U3P_DMA_TYPE_MANUAL_IN)
    {
        /* Consumer socket */
        ipNum = CyU3PDmaGetIpNum (config->consSckId);
        sckNum = CyU3PDmaGetSckNum (config->consSckId);
        /* Verify that this is a valid socket. */
        if (!CyU3PDmaSocketIsValidConsumer (config->consSckId))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
        if (glDmaSocketCtrl[ipNum][sckNum].singleHandle != NULL)
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
    }
    else /* CY_U3P_DMA_TYPE_MANUAL_IN */
    {
        /* Verify that the consumer socket is CPU. */
        if (config->consSckId != CY_U3P_CPU_SOCKET_CONS)
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
    }
    if (config->prodAvailCount != 0)
    {
        if ((config->prodAvailCount >= config->count) ||
                (config->count > CY_U3P_DMA_MAX_AVAIL_COUNT))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
    }

    /* Lock out other channel create calls to the same socket(s). */
    if (type != CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        /* Producer socket */
        ipNum = CyU3PDmaGetIpNum (config->prodSckId);
        sckNum = CyU3PDmaGetSckNum (config->prodSckId);
        glDmaSocketCtrl[ipNum][sckNum].singleHandle = handle;
    }
    if (type != CY_U3P_DMA_TYPE_MANUAL_IN)
    {
        /* Consumer socket */
        ipNum = CyU3PDmaGetIpNum (config->consSckId);
        sckNum = CyU3PDmaGetSckNum (config->consSckId);
        glDmaSocketCtrl[ipNum][sckNum].singleHandle = handle;
    }

    /* Load and initialize the configuration information */
    handle->type  = type;
    handle->size  = config->size;
    handle->count = config->count;
    handle->prodAvailCount = config->prodAvailCount;
    handle->prodSckId  = config->prodSckId;
    handle->consSckId  = config->consSckId;
    handle->prodHeader = config->prodHeader;
    handle->prodFooter = config->prodFooter;
    handle->consHeader = config->consHeader;
    handle->dmaMode    = config->dmaMode;
    handle->notification = config->notification;
    handle->prodSusp = CY_U3P_DMA_SCK_SUSP_NONE;
    handle->consSusp = CY_U3P_DMA_SCK_SUSP_NONE;
    CyU3PMutexCreate (&(handle->lock), CYU3P_NO_INHERIT);
    CyU3PEventCreate (&(handle->flags));
    handle->cb = config->cb;
    handle->isDmaHandleDCache = glDmaHandleDCache;

    switch (type)
    {
        case CY_U3P_DMA_TYPE_AUTO:
        case CY_U3P_DMA_TYPE_AUTO_SIGNAL:
            status = CyU3PDmaChannelConfigure_TypeAuto (handle);
            break;
        case CY_U3P_DMA_TYPE_MANUAL_IN:
        case CY_U3P_DMA_TYPE_MANUAL_OUT:
        case CY_U3P_DMA_TYPE_MANUAL:
            status = CyU3PDmaChannelConfigure_TypeManual (handle);
            break;
        default:
            status = CY_U3P_ERROR_BAD_ARGUMENT;
            break;
    }

    if (status == CY_U3P_SUCCESS)
    {
        handle->state = CY_U3P_DMA_CONFIGURED;
    }
    else
    {
        CyU3PMutexDestroy (&(handle->lock));
        CyU3PEventDestroy (&(handle->flags));
        if (type != CY_U3P_DMA_TYPE_MANUAL_OUT)
        {
            /* Producer socket */
            ipNum = CyU3PDmaGetIpNum (config->prodSckId);
            sckNum = CyU3PDmaGetSckNum (config->prodSckId);
            glDmaSocketCtrl[ipNum][sckNum].singleHandle = NULL;
        }
        if (type != CY_U3P_DMA_TYPE_MANUAL_IN)
        {
            /* Consumer socket */
            ipNum = CyU3PDmaGetIpNum (config->consSckId);
            sckNum = CyU3PDmaGetSckNum (config->consSckId);
            glDmaSocketCtrl[ipNum][sckNum].singleHandle = NULL;
        }
        handle->state = CY_U3P_DMA_NOT_CONFIGURED;
    }

    return status;
}

static CyU3PReturnStatus_t
CyU3PDmaChannelDestroy_TypeAuto (
        CyU3PDmaChannel *handle)
{
    CyU3PDmaSocketConfig_t sck;
    uint8_t sckNum, ipNum;

    /* Disable the sockets */
    CyU3PDmaSocketDisable (handle->prodSckId);
    CyU3PDmaSocketDisable (handle->consSckId);

    /* Configure the producer socket to point to invalid descriptor */
    CyU3PDmaSocketGetConfig (handle->prodSckId, &sck);
    sck.dscrChain = CY_U3P_RD_NEXT_DSCR_MASK;
    CyU3PDmaSocketSetConfig (handle->prodSckId, &sck);
    sckNum = CyU3PDmaGetSckNum (handle->prodSckId);
    ipNum  = CyU3PDmaGetIpNum (handle->prodSckId);
    glDmaSocketCtrl[ipNum][sckNum].singleHandle = NULL;

    /* Configure the consumer socket to point to invalid descriptor */
    CyU3PDmaSocketGetConfig (handle->consSckId, &sck);
    sck.dscrChain = CY_U3P_RD_NEXT_DSCR_MASK;
    CyU3PDmaSocketSetConfig (handle->consSckId, &sck);
    sckNum = CyU3PDmaGetSckNum (handle->consSckId);
    ipNum  = CyU3PDmaGetIpNum (handle->consSckId);
    glDmaSocketCtrl[ipNum][sckNum].singleHandle = NULL;

    /* Free the descriptor chain */
    CyU3PDmaDscrChainDestroy (handle->firstProdIndex, handle->count,
            CyTrue, CyTrue);

    /* Free the override descriptor */
    CyU3PDmaDscrPut (handle->overrideDscrIndex);

    return CY_U3P_SUCCESS;
}

static CyU3PReturnStatus_t
CyU3PDmaChannelDestroy_TypeManual (
        CyU3PDmaChannel *handle)
{
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaSocketConfig_t sck;
    uint16_t index, count;
    uint8_t sckNum, ipNum;

    /* Disable the producer socket */
    if (handle->type != CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        CyU3PDmaSocketDisable (handle->prodSckId);
        CyU3PDmaSocketGetConfig (handle->prodSckId, &sck);
        sck.dscrChain = CY_U3P_RD_NEXT_DSCR_MASK;
        CyU3PDmaSocketSetConfig (handle->prodSckId, &sck);
        sckNum = CyU3PDmaGetSckNum (handle->prodSckId);
        ipNum  = CyU3PDmaGetIpNum (handle->prodSckId);
        glDmaSocketCtrl[ipNum][sckNum].singleHandle = NULL;
    }

    /* Disable the consumer socket */
    if (handle->type != CY_U3P_DMA_TYPE_MANUAL_IN)
    {
        CyU3PDmaSocketDisable (handle->consSckId);
        CyU3PDmaSocketGetConfig (handle->consSckId, &sck);
        sck.dscrChain = CY_U3P_RD_NEXT_DSCR_MASK;
        CyU3PDmaSocketSetConfig (handle->consSckId, &sck);
        sckNum = CyU3PDmaGetSckNum (handle->consSckId);
        ipNum  = CyU3PDmaGetIpNum (handle->consSckId);
        glDmaSocketCtrl[ipNum][sckNum].singleHandle = NULL;
    }

    /* Free the primary descriptor chain and free the buffers. */
    if (handle->count != 0)
    {
        if (handle->type == CY_U3P_DMA_TYPE_MANUAL_OUT)
        {
            index = handle->firstConsIndex;
            for (count = 0; count < handle->count; count++)
            {
                CyU3PDmaDscrGetConfig (index, &dscr);
                CyU3PDmaBufferFree (dscr.buffer - handle->consHeader);
                CyU3PDmaDscrPut (index);
                index = (dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
            }
        }
        else /* MANUAL and MANUAL_IN */
        {
            index = handle->firstProdIndex;
            for (count = 0; count < handle->count; count++)
            {
                CyU3PDmaDscrGetConfig (index, &dscr);
                CyU3PDmaBufferFree (dscr.buffer - handle->prodHeader);
                CyU3PDmaDscrPut (index);
                index = (dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
            }
        }

        if (handle->type == CY_U3P_DMA_TYPE_MANUAL)
        {
            /* Free the consumer descriptor chain */
            CyU3PDmaDscrChainDestroy (handle->firstConsIndex,
                    handle->count, CyFalse, CyFalse);

        }
    }

    /* Free the override descriptor */
    CyU3PDmaDscrPut (handle->overrideDscrIndex);

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaChannelDestroy (
        CyU3PDmaChannel *handle)
{
    uint32_t status;

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    switch (handle->type)
    {
        case CY_U3P_DMA_TYPE_AUTO:
        case CY_U3P_DMA_TYPE_AUTO_SIGNAL:
            CyU3PDmaChannelDestroy_TypeAuto (handle);
            break;
        case CY_U3P_DMA_TYPE_MANUAL_IN:
        case CY_U3P_DMA_TYPE_MANUAL_OUT:
        case CY_U3P_DMA_TYPE_MANUAL:
            CyU3PDmaChannelDestroy_TypeManual (handle);
            break;
        default:
            /* This is not a valid channel. Just release the
             * mutex and retunrn. */
            CyU3PMutexPut (&(handle->lock));
            return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Destroy the locks */
    handle->state = CY_U3P_DMA_NOT_CONFIGURED;
    CyU3PMutexDestroy (&(handle->lock));
    CyU3PEventDestroy (&(handle->flags));

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaChannelUpdateMode (
        CyU3PDmaChannel *handle,
        CyU3PDmaMode_t dmaMode)
{
    uint32_t status;

    /* Check parameter validity. */
    if (dmaMode >= CY_U3P_DMA_NUM_MODES)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    if (handle->state == CY_U3P_DMA_CONFIGURED)
    {
        /* Update the channel mode. */
        handle->dmaMode = dmaMode;
    }
    else
    {
        /* Channel mode can be updated only when it is in
         * configured mode. */
        status = CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return status;
}

static void
CyU3PDmaChannelSetXfer_TypeAuto (
        CyU3PDmaChannel *handle,
        uint32_t count)
{
    CyU3PDmaSocketConfig_t sck;

    /* Suspend the sockets */
    CyU3PDmaSocketDisable (handle->prodSckId);
    CyU3PDmaSocketDisable (handle->consSckId);

    /* Update the state information first. */
    handle->state = CY_U3P_DMA_ACTIVE;

    /* Set the producer socket suspend option. */
    CyU3PDmaUpdateSocketSuspendOption (handle->prodSckId,
            handle->prodSusp);

    /* Set the consumer socket suspend option. */
    CyU3PDmaUpdateSocketSuspendOption (handle->consSckId,
            handle->consSusp);

    /* Configure the producer socket */
    CyU3PDmaSocketGetConfig (handle->prodSckId, &sck);
    sck.xferSize = count;
    sck.xferCount = 0;
    if(count == 0)
    {
        /* Turn off the CY_U3P_TRUNCATE bit if the
         * transfer is infinite.
         */
        sck.status &= (~CY_U3P_TRUNCATE);
    }
    else
    {
        sck.status |= CY_U3P_TRUNCATE;
    }
    sck.status &= ~(CY_U3P_WRAPUP);
    sck.status |= CY_U3P_GO_ENABLE;
    if (handle->dmaMode == CY_U3P_DMA_MODE_BUFFER)
    {
        sck.status |= CY_U3P_UNIT;
    }
    else
    {
        sck.status &= (~CY_U3P_UNIT);
    }
    if (handle->prodAvailCount != 0)
    {
        /* Update number of free buffers and the min avail count. */
        sck.status |= (CY_U3P_AVL_ENABLE | (handle->count) | 
                (handle->prodAvailCount << CY_U3P_AVL_MIN_POS));
    }
    sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
    CyU3PDmaSocketSetConfig (handle->prodSckId, &sck);

    /* Configure the consumer socket */
    CyU3PDmaSocketGetConfig (handle->consSckId, &sck);
    sck.xferSize = count;
    sck.xferCount = 0;
    if(count == 0)
    {
        /* If the transfer length is infinite, 
         * turn off the done interrupt */
        sck.intrMask &= (~(CY_U3P_TRANS_DONE));
        sck.status &= (~CY_U3P_TRUNCATE);
    }
    else
    {
        sck.intrMask |= CY_U3P_TRANS_DONE;
        sck.status |= CY_U3P_TRUNCATE;
    }
    sck.status &= ~(CY_U3P_WRAPUP);
    sck.status |= CY_U3P_GO_ENABLE;
    if (handle->dmaMode == CY_U3P_DMA_MODE_BUFFER)
    {
        sck.status |= CY_U3P_UNIT;
    }
    else
    {
        sck.status &= (~CY_U3P_UNIT);
    }
    CyU3PDmaSocketSetConfig (handle->consSckId, &sck);

}

static void
CyU3PDmaChannelSetXfer_TypeManual (
        CyU3PDmaChannel *handle,
        uint32_t count)
{
    CyU3PDmaSocketConfig_t sck;

    /* Update the state information. */
    handle->state = CY_U3P_DMA_ACTIVE;

    /* Configure the producer socket */
    if (handle->type != CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        CyU3PDmaSocketDisable (handle->prodSckId);

        /* Set the producer socket suspend option. */
        CyU3PDmaUpdateSocketSuspendOption (handle->prodSckId,
                handle->prodSusp);

        CyU3PDmaSocketGetConfig (handle->prodSckId, &sck);
        sck.xferSize = count;
        sck.xferCount = 0;
        sck.status &= ~(CY_U3P_WRAPUP);
        sck.status |= CY_U3P_GO_ENABLE;
        if (handle->dmaMode == CY_U3P_DMA_MODE_BUFFER)
        {
            sck.status |= CY_U3P_UNIT;
        }
        else
        {
            sck.status &= (~CY_U3P_UNIT);
        }
        if (handle->prodAvailCount != 0)
        {
            /* Update number of free buffers and the min avail count. */
            sck.status |= (CY_U3P_AVL_ENABLE | (handle->count) | 
                    (handle->prodAvailCount << CY_U3P_AVL_MIN_POS));
        }
        sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
        if((count == 0) || (handle->type == CY_U3P_DMA_TYPE_MANUAL))
        {
            /* If the transfer length is infinite or manual mode, 
             * turn off the done interrupt for producer */
            sck.xferSize = 0;
            sck.status &= (~CY_U3P_TRUNCATE);
            sck.intrMask &= (~(CY_U3P_TRANS_DONE));
        }
        else
        {
            sck.status |= CY_U3P_TRUNCATE;
            sck.intrMask |= CY_U3P_TRANS_DONE;
        }
        CyU3PDmaSocketSetConfig (handle->prodSckId, &sck);
    }

    /* Configure the consumer socket */
    if (handle->type != CY_U3P_DMA_TYPE_MANUAL_IN)
    {
        CyU3PDmaSocketDisable (handle->consSckId);

        /* Set the consumer socket suspend option. */
        CyU3PDmaUpdateSocketSuspendOption (handle->consSckId,
                handle->consSusp);

        CyU3PDmaSocketGetConfig (handle->consSckId, &sck);
        sck.xferSize = count;
        sck.xferCount = 0;
        sck.status &= ~(CY_U3P_WRAPUP);
        sck.status |= CY_U3P_GO_ENABLE;
        if (handle->dmaMode == CY_U3P_DMA_MODE_BUFFER)
        {
            sck.status |= CY_U3P_UNIT;
        }
        else
        {
            sck.status &= (~CY_U3P_UNIT);
        }
        sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
        if (count == 0)
        {
            /* If the transfer length is infinite, 
             * turn off the done interrupt */
            sck.status &= (~CY_U3P_TRUNCATE);
            sck.intrMask &= (~(CY_U3P_TRANS_DONE));
        }
        else
        {
            sck.status |= CY_U3P_TRUNCATE;
            sck.intrMask |= CY_U3P_TRANS_DONE;
        }

        CyU3PDmaSocketSetConfig (handle->consSckId, &sck);
    }
}

CyU3PReturnStatus_t
CyU3PDmaChannelSetXfer (
        CyU3PDmaChannel *handle,
        uint32_t count)
{
    uint32_t status;

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* This API can be called only if the buffer count is 
     * non-zero and the channel is in configured state. */
    if (handle->state != CY_U3P_DMA_CONFIGURED)
    {
        status = CY_U3P_ERROR_ALREADY_STARTED;
    }
    if (handle->count == 0)
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if (status == CY_U3P_SUCCESS)
    {
        /* Clear the event flags before queuing a request */
        CyU3PEventSet (&(handle->flags), (~(CY_U3P_DMA_CB_XFER_CPLT |
                        CY_U3P_DMA_CB_SEND_CPLT | CY_U3P_DMA_CB_RECV_CPLT | 
                        CY_U3P_DMA_CB_ABORTED)), CYU3P_EVENT_AND);

        /* Store the transfer size for future reference */
        handle->xferSize = count;

        switch (handle->type)
        {
            case CY_U3P_DMA_TYPE_AUTO:
            case CY_U3P_DMA_TYPE_AUTO_SIGNAL:
                CyU3PDmaChannelSetXfer_TypeAuto (handle, count);
                break;
            case CY_U3P_DMA_TYPE_MANUAL_IN:
            case CY_U3P_DMA_TYPE_MANUAL_OUT:
            case CY_U3P_DMA_TYPE_MANUAL:
                CyU3PDmaChannelSetXfer_TypeManual (handle, count);
                break;
            default:
                /* Code will not reach here. */
                break;
        }
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return status;
}

CyU3PReturnStatus_t
CyU3PDmaChannelSetWrapUp (
        CyU3PDmaChannel *handle)
{
    uint32_t status;

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* This API can be called only if the channel is
     * in active mode or in consumer override mode. */
    if ((handle->state != CY_U3P_DMA_CONS_OVERRIDE) &&
            (handle->state != CY_U3P_DMA_ACTIVE))
    {
        status = CY_U3P_ERROR_INVALID_SEQUENCE;
    }
    if (handle->type == CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if (status == CY_U3P_SUCCESS)
    {
        /* Set the wrapup bit. */
        CyU3PDmaSocketSetWrapUp (handle->prodSckId);
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return status;
}

CyU3PReturnStatus_t
CyU3PDmaChannelSetSuspend (
        CyU3PDmaChannel *handle,
        CyU3PDmaSckSuspType_t prodSusp,
        CyU3PDmaSckSuspType_t consSusp)
{
    uint32_t status;

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* This API can be called only if the channel is
     * in configured mode or in active mode. */
    if ((handle->state != CY_U3P_DMA_CONFIGURED) &&
            (handle->state != CY_U3P_DMA_ACTIVE))
    {
        status = CY_U3P_ERROR_INVALID_SEQUENCE;
    }
    if ((handle->type == CY_U3P_DMA_TYPE_MANUAL_OUT)
            && (prodSusp != CY_U3P_DMA_SCK_SUSP_NONE))
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((handle->type == CY_U3P_DMA_TYPE_MANUAL_IN)
            && (consSusp != CY_U3P_DMA_SCK_SUSP_NONE))
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (prodSusp >= CY_U3P_DMA_SCK_SUSP_CONS_PARTIAL_BUF)
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (consSusp > CY_U3P_DMA_SCK_SUSP_CONS_PARTIAL_BUF)
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (status != CY_U3P_SUCCESS)
    {
        /* Release the lock. */
        CyU3PMutexPut (&(handle->lock));
        return status;
    }

    /* If the channel is active, then update the 
     * socket directly. */
    if (handle->state == CY_U3P_DMA_ACTIVE)
    {
        /* Update producer socket option. */
        if ((handle->type != CY_U3P_DMA_TYPE_MANUAL_OUT) &&
                (handle->prodSusp != prodSusp))
        {
            handle->prodSusp = prodSusp;
            CyU3PDmaUpdateSocketSuspendOption (handle->prodSckId, prodSusp);
        }

        /* Update consumer socket option. */
        if ((handle->type != CY_U3P_DMA_TYPE_MANUAL_IN) &&
                (handle->consSusp != consSusp))
        {
            handle->consSusp = consSusp;
            CyU3PDmaUpdateSocketSuspendOption (handle->consSckId, consSusp);
        }
    }
    else /* CY_U3P_DMA_CONFIGURED */
    {
        handle->prodSusp = prodSusp;
        handle->consSusp = consSusp;
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaChannelResume (
        CyU3PDmaChannel *handle,
        CyBool_t        isProdResume,
        CyBool_t        isConsResume)
{
    uint32_t status;

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* This API can be called only if the channel is
     * in active state. */
    if (handle->state != CY_U3P_DMA_ACTIVE)
    {
        status = CY_U3P_ERROR_NOT_STARTED;
    }
    if ((handle->type == CY_U3P_DMA_TYPE_MANUAL_OUT) &&
            (isProdResume == CyTrue))
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((handle->type == CY_U3P_DMA_TYPE_MANUAL_IN) &&
            (isConsResume == CyTrue))
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (status != CY_U3P_SUCCESS)
    {
        /* Release the lock. */
        CyU3PMutexPut (&(handle->lock));

        return status;
    }

    if (isProdResume == CyTrue)
    {
        /* Resume the producer socket. If the socket was
         * asked to be suspended after the current buffer,
         * suspend state has to be reverted to SUSP_NONE. */
        if (handle->prodSusp == CY_U3P_DMA_SCK_SUSP_CUR_BUF)
        {
            handle->prodSusp = CY_U3P_DMA_SCK_SUSP_NONE;
        }
        /* Clear all interrupts and setup the corresponding
         * interrupt masks. */
        CyU3PDmaUpdateSocketResume (handle->prodSckId,
                handle->prodSusp);
    }

    if (isConsResume == CyTrue)
    {
        /* Resume the consumer socket. If the socket was
         * asked to be suspended after the current buffer,
         * suspend state has to be reverted to SUSP_NONE. */
        if (handle->prodSusp == CY_U3P_DMA_SCK_SUSP_CUR_BUF)
        {
            handle->prodSusp = CY_U3P_DMA_SCK_SUSP_NONE;
        }
        /* Clear all interrupts and setup the corresponding
         * interrupt masks. */
        CyU3PDmaUpdateSocketResume (handle->consSckId,
                handle->consSusp);
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));
 
    return status;
}

static void
CyU3PDmaChannelReset_TypeAuto (
        CyU3PDmaChannel *handle)
{
    CyU3PDmaDescriptor_t dscr;
    uint16_t dscrIndex;
    uint16_t count = handle->count;

    dscrIndex = handle->firstProdIndex;

    /* Disable and re-configure the sockets. */
    CyU3PDmaConfigureSockets_TypeAuto (handle);

    /* Cleanup the descriptors */
    while ((count--) != 0)
    {
        CyU3PDmaDscrGetConfig (dscrIndex, &dscr);
        dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
        CyU3PDmaDscrSetConfig (dscrIndex, &dscr);
        dscrIndex = (dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
    }
}

static void
CyU3PDmaChannelReset_TypeManual (
        CyU3PDmaChannel *handle)
{
    CyU3PDmaDescriptor_t dscr;
    uint16_t dscrIndex;
    uint16_t count = handle->count;

    /* Disable and re-configure the sockets. */
    CyU3PDmaConfigureSockets_TypeManual (handle);

    /* Reconfigure the producer descriptor chain */
    if (handle->type != CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        dscrIndex = handle->firstProdIndex;
        while ((count--) != 0)
        {
            CyU3PDmaDscrGetConfig (dscrIndex, &dscr);
            dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
            CyU3PDmaDscrSetConfig (dscrIndex, &dscr);
            dscrIndex = (dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
        }
        handle->activeProdIndex = handle->commitProdIndex = 
            handle->currentProdIndex = handle->firstProdIndex;
    }

    /* Reconfigure the consumer descriptor chain */
    if (handle->type != CY_U3P_DMA_TYPE_MANUAL_IN)
    {
        dscrIndex = handle->firstConsIndex;
        count = handle->count;
        while ((count--) != 0)
        {
            CyU3PDmaDscrGetConfig (dscrIndex, &dscr);
            dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
            CyU3PDmaDscrSetConfig (dscrIndex, &dscr);
            dscrIndex = (dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
        }
        handle->activeConsIndex = handle->commitConsIndex = 
            handle->currentConsIndex = handle->firstConsIndex;
    }
}

CyU3PReturnStatus_t
CyU3PDmaChannelReset (
        CyU3PDmaChannel *handle)
{
    CyU3PThread *thread_p;
    uint32_t priority;
    uint32_t status;

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    switch (handle->type)
    {
        case CY_U3P_DMA_TYPE_AUTO:
        case CY_U3P_DMA_TYPE_AUTO_SIGNAL:
            CyU3PDmaChannelReset_TypeAuto (handle);
            break;
        case CY_U3P_DMA_TYPE_MANUAL_IN:
        case CY_U3P_DMA_TYPE_MANUAL_OUT:
        case CY_U3P_DMA_TYPE_MANUAL:
            CyU3PDmaChannelReset_TypeManual (handle);
            break;
        default:
            /* Code will not reach here. */
            break;
    }

    /* Temporarily set the priority of this thread to that of DMA thread
     * to prevent any pre-emption. Setup the flags and invoke the callback. */
    thread_p = CyU3PThreadIdentify ();
    if (thread_p != NULL)
    {
        CyU3PThreadPriorityChange (thread_p, CY_U3P_DMA_API_THREAD_PRIORITY,
                &priority);
    }

    /* Clear the event flags */
    CyU3PEventSet (&(handle->flags), 
            (~(CY_U3P_DMA_CB_XFER_CPLT | CY_U3P_DMA_CB_SEND_CPLT |
               CY_U3P_DMA_ERROR | CY_U3P_DMA_CB_RECV_CPLT)),
            CYU3P_EVENT_AND);
    /* Set the aborted flag. */
    CyU3PEventSet (&(handle->flags), CY_U3P_DMA_CB_ABORTED, CYU3P_EVENT_OR);
    /* Change the channel state to configured. */
    handle->state = CY_U3P_DMA_CONFIGURED;

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    /* Invoke the callback if notification is registered. */
    if ((handle->cb) && (handle->notification & CY_U3P_DMA_CB_ABORTED))
    {
        handle->cb (handle, CY_U3P_DMA_CB_ABORTED, 0);
    }

    /* Restore the thread priority to its original value. */
    if (thread_p != NULL)
    {
        CyU3PThreadPriorityChange (thread_p, priority, &priority);
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaChannelAbort (
        CyU3PDmaChannel *handle)
{
    CyU3PThread *thread_p;
    uint32_t priority;
    uint32_t status;

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Disable the producer socket. */
    if (handle->type != CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        CyU3PDmaSocketDisable (handle->prodSckId);
    }
    /* Disable the consumer socket. */
    if (handle->type != CY_U3P_DMA_TYPE_MANUAL_IN)
    {
        CyU3PDmaSocketDisable (handle->consSckId);
    }

    /* Temporarily set the priority of this thread to that of DMA thread
     * to prevent any pre-emption. Setup the flags and invoke the callback. */
    thread_p = CyU3PThreadIdentify ();
    if (thread_p != NULL)
    {
        CyU3PThreadPriorityChange (thread_p, CY_U3P_DMA_API_THREAD_PRIORITY,
                &priority);
    }

    /* Clear the event flags */
    CyU3PEventSet (&(handle->flags), 
            (~(CY_U3P_DMA_CB_XFER_CPLT | CY_U3P_DMA_CB_SEND_CPLT |
               CY_U3P_DMA_CB_RECV_CPLT)),
            CYU3P_EVENT_AND);

    /* Set the aborted flag. */
    CyU3PEventSet (&(handle->flags), CY_U3P_DMA_CB_ABORTED, CYU3P_EVENT_OR);
    /* Change the channel state to aborted. */
    handle->state = CY_U3P_DMA_ABORTED;

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    /* Invoke the callback if notification is registered. */
    if ((handle->cb) && (handle->notification & CY_U3P_DMA_CB_ABORTED))
    {
        handle->cb (handle, CY_U3P_DMA_CB_ABORTED, 0);
    }

    /* Restore the thread priority to its original value. */
    if (thread_p != NULL)
    {
        CyU3PThreadPriorityChange (thread_p, priority, &priority);
    }

    return CY_U3P_SUCCESS;
}

/* This function uses override descriptor and loads a buffer
 * to be sent out through a consumer socket. It does not wait
 * for completion. On completion the dma callback function 
 * will be invoked as a notification.
 */
CyU3PReturnStatus_t
CyU3PDmaChannelSetupSendBuffer (
        CyU3PDmaChannel *handle,
        CyU3PDmaBuffer_t *buffer_p)
{
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaSocketConfig_t sck;
    uint16_t size;
    uint32_t status;

    /* Check for parameter validity first. */
    if (buffer_p == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if ((buffer_p->buffer < CY_U3P_DMA_BUFFER_AREA_BASE) ||
            ((buffer_p->buffer + buffer_p->count) >= CY_U3P_DMA_BUFFER_AREA_LIMIT))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (buffer_p->count > CY_U3P_DMA_MAX_BUFFER_SIZE)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* The API call is not supported for MANUAL_IN channel as it does
     * not have a consumer socket. It can be invoked only in the
     * configured state. */
    if (handle->type == CY_U3P_DMA_TYPE_MANUAL_IN)
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
    }
    if (handle->state != CY_U3P_DMA_CONFIGURED)
    {
        status = CY_U3P_ERROR_ALREADY_STARTED;
    }

    if (status != CY_U3P_SUCCESS)
    {
        /* Release the lock and return. */
        CyU3PMutexPut (&(handle->lock));
        return status;
    }

    if (handle->isDmaHandleDCache)
    {
        /* Since the buffer is received from the CPU, make sure that
         * the buffer is cleaned before sending it to DMA adapter. */
        if (buffer_p->count)
        {
            CyU3PSysCleanDRegion ((uint32_t *)buffer_p->buffer, buffer_p->count);
        }
    }

    /* Suspend the consumer socket */
    CyU3PDmaSocketDisable (handle->consSckId);

    /* Set the suspend option for the consumer socket to none. */
    CyU3PDmaUpdateSocketSuspendOption (handle->consSckId,
            CY_U3P_DMA_SCK_SUSP_NONE);

    /* Clear the event flags before queuing a request */
    CyU3PEventSet (&(handle->flags),  (~(CY_U3P_DMA_CB_XFER_CPLT |
                    CY_U3P_DMA_CB_SEND_CPLT | CY_U3P_DMA_CB_RECV_CPLT |
                    CY_U3P_DMA_CB_ABORTED)), CYU3P_EVENT_AND);
   
    /* Update the state information first. */
    handle->state = CY_U3P_DMA_PROD_OVERRIDE;

    /* Configure the consumer socket */
    CyU3PDmaSocketGetConfig (handle->consSckId, &sck);
    handle->currentConsIndex = sck.dscrChain & CY_U3P_DSCR_NUMBER_MASK;
    /* Configure the override descriptor */
    CyU3PDmaDscrGetConfig (handle->overrideDscrIndex, &dscr);
    dscr.buffer = buffer_p->buffer;
    dscr.sync   = (handle->consSckId) | CY_U3P_EN_CONS_EVENT |
        CY_U3P_EN_CONS_INT;
    dscr.sync  |= (CY_U3P_CPU_SOCKET_PROD << CY_U3P_PROD_SCK_POS);
    dscr.chain  = (handle->currentConsIndex | CY_U3P_WR_NEXT_DSCR_MASK);
    size = ((buffer_p->count + 0xF) & CY_U3P_BUFFER_SIZE_MASK);
    dscr.size   = ((buffer_p->count << CY_U3P_BYTE_COUNT_POS) | 
            size | CY_U3P_BUFFER_OCCUPIED);
    /* Update the buffer status. */
    dscr.size |= (buffer_p->status & CY_U3P_DMA_BUFFER_STATUS_WRITE_MASK);
    CyU3PDmaDscrSetConfig (handle->overrideDscrIndex, &dscr);
    sck.status &= (~(CY_U3P_GO_SUSPEND | CY_U3P_WRAPUP));
    sck.status |= CY_U3P_GO_ENABLE;
    sck.status |= CY_U3P_TRUNCATE;
    /* Disable the avail count check. */
    sck.status &= ~CY_U3P_AVL_ENABLE;
    /* Use the DMA_BYTE_MODE since the data size to be transmitted is known */
    sck.status &= (~CY_U3P_UNIT);
    sck.xferSize = buffer_p->count;
    sck.xferCount = 0;
    sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
    sck.intrMask |= CY_U3P_CONSUME_EVENT;
    sck.intrMask &= (~CY_U3P_TRANS_DONE);
    sck.dscrChain = handle->overrideDscrIndex;
    CyU3PDmaSocketSetConfig (handle->consSckId, &sck);

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return CY_U3P_SUCCESS;
}

/* This function uses override descriptor and loads a buffer to
 * receive data through a producer socket. It does not wait for
 * completion. On completion the dma callback function will be 
 * invoked as a notification.
 */
CyU3PReturnStatus_t
CyU3PDmaChannelSetupRecvBuffer (
        CyU3PDmaChannel *handle,
        CyU3PDmaBuffer_t *buffer_p)
{
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaSocketConfig_t sck;
    uint32_t status;

    /* Check the parameter validity first. */
    if (buffer_p == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if ((buffer_p->buffer < CY_U3P_DMA_BUFFER_AREA_BASE) ||
            ((buffer_p->buffer + buffer_p->size) >= CY_U3P_DMA_BUFFER_AREA_LIMIT))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (buffer_p->size > CY_U3P_DMA_MAX_BUFFER_SIZE)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((buffer_p->size == 0) || (buffer_p->size & (~CY_U3P_BUFFER_SIZE_MASK)))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* This API is not supported for MANUAL_OUT channel as it
     * does not have a producer socket. The channel should be
     * in the configured state. */
    if (handle->type == CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
    }
    if (handle->state != CY_U3P_DMA_CONFIGURED)
    {
        status = CY_U3P_ERROR_ALREADY_STARTED;
    }

    if (status != CY_U3P_SUCCESS)
    {
        /* Release the lock and return. */
        CyU3PMutexPut (&(handle->lock));
        return status;
    }

    /* Suspend the producer socket */
    CyU3PDmaSocketDisable (handle->prodSckId);

    /* Set the suspend option for the producer socket to none. */
    CyU3PDmaUpdateSocketSuspendOption (handle->prodSckId,
            CY_U3P_DMA_SCK_SUSP_NONE);

    /* Clear the event flag before queuing a request */
    CyU3PEventSet (&(handle->flags),  (~(CY_U3P_DMA_CB_XFER_CPLT |
                    CY_U3P_DMA_CB_SEND_CPLT |
                    CY_U3P_DMA_CB_RECV_CPLT | CY_U3P_DMA_CB_ABORTED)),
            CYU3P_EVENT_AND);

    /* Update the state information first. */
    handle->state = CY_U3P_DMA_CONS_OVERRIDE;

    if (handle->isDmaHandleDCache)
    {
        /* Since the buffer is received from the H/W, make sure that
         * the buffer is flushed before queueing the request. */
        CyU3PSysFlushDRegion ((uint32_t *)buffer_p->buffer, buffer_p->size);
    }

    /* Configure the producer socket */
    CyU3PDmaSocketGetConfig (handle->prodSckId, &sck);
    handle->currentProdIndex = sck.dscrChain & CY_U3P_DSCR_NUMBER_MASK;
    /* Configure the override descriptor */
    CyU3PDmaDscrGetConfig (handle->overrideDscrIndex, &dscr);
    dscr.buffer = buffer_p->buffer;
    dscr.sync   = (((handle->prodSckId) << CY_U3P_PROD_SCK_POS) |
            (CY_U3P_EN_PROD_INT | CY_U3P_EN_PROD_EVENT));
    dscr.sync  |= (CY_U3P_CPU_SOCKET_CONS);
    dscr.chain  = ((handle->currentProdIndex << CY_U3P_WR_NEXT_DSCR_POS) |
            CY_U3P_RD_NEXT_DSCR_MASK);
    dscr.size = (buffer_p->size & CY_U3P_BUFFER_SIZE_MASK);
    CyU3PDmaDscrSetConfig (handle->overrideDscrIndex, &dscr);
    sck.status &= (~(CY_U3P_GO_SUSPEND | CY_U3P_WRAPUP));
    sck.status |= CY_U3P_GO_ENABLE;
    /* Use DMA_BUFFER_MODE since the actual byte count
     * to be received is not known */
    sck.status |= CY_U3P_UNIT;
    sck.status |= CY_U3P_TRUNCATE;
    sck.xferSize = 1;
    sck.xferCount = 0;
    sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
    sck.intrMask |= CY_U3P_PRODUCE_EVENT;
    sck.intrMask &= (~CY_U3P_TRANS_DONE);
    sck.dscrChain = handle->overrideDscrIndex;
    CyU3PDmaSocketSetConfig (handle->prodSckId, &sck);

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaChannelWaitForRecvBuffer (
        CyU3PDmaChannel *handle,
        CyU3PDmaBuffer_t *buffer_p,
        uint32_t waitOption)
{
    uint32_t state;
    uint32_t status;
    uint32_t flags;
    CyU3PDmaDescriptor_t dscr;

    /* Check for parameter validity first. */
    if (buffer_p == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    status = CyU3PDmaChannelAcquireLock (handle, waitOption);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* This API is not supported for MANUAL_OUT channel as it
     * does not have a producer socket. This call will succeed
     * only if there was a previous SetupRecvBuffer call. */
    if (handle->type == CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
    }

    state = handle->state;

    if (status == CY_U3P_SUCCESS)
    {
        if (state == CY_U3P_DMA_CONS_OVERRIDE)
        {
            /* Release the lock and Wait for the consumer 
             * socket override transaction to complete */
            CyU3PMutexPut (&(handle->lock));
            status = CyU3PEventGet (&(handle->flags), (CY_U3P_DMA_CB_RECV_CPLT |
                        CY_U3P_DMA_CB_ABORTED | CY_U3P_DMA_CB_ERROR), CYU3P_EVENT_OR,
                    &flags, waitOption);
            if (status != CY_U3P_SUCCESS)
            {
                return CY_U3P_ERROR_TIMEOUT;
            }
            if (flags & CY_U3P_DMA_CB_ERROR)
            {
                return CY_U3P_ERROR_DMA_FAILURE;
            }
            if (flags & CY_U3P_DMA_CB_ABORTED)
            {
                return CY_U3P_ERROR_ABORTED;
            }
            /* Acquire the lock again. */
            status = CyU3PDmaChannelAcquireLock (handle, waitOption);
            if (status != CY_U3P_SUCCESS)
            {
                return status;
            }
        }
        else if (state == CY_U3P_DMA_CONFIGURED)
        {
            /* See if the transfer is already completed */
            status = CyU3PEventGet (&(handle->flags), (CY_U3P_DMA_CB_RECV_CPLT |
                        CY_U3P_DMA_CB_ABORTED), CYU3P_EVENT_OR, &flags, CYU3P_NO_WAIT);
            if (status != CY_U3P_SUCCESS)
            {
                status = CY_U3P_ERROR_INVALID_SEQUENCE;
            }
            else if (flags & CY_U3P_DMA_CB_ABORTED)
            {
                status = CY_U3P_ERROR_NOT_STARTED;
            }
            else
            {
                /* Nothing to do here. */
            }
        }
        else 
        {
            status = CY_U3P_ERROR_INVALID_SEQUENCE;
        }
    }

    /* If we reach here with an error, then we have to
     * release the lock and return. */
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PMutexPut (&(handle->lock));
        return status;
    }

    /* Get the transfer status. */
    CyU3PDmaDscrGetConfig (handle->overrideDscrIndex, &dscr);
    buffer_p->buffer = dscr.buffer;
    buffer_p->count = (dscr.size >> CY_U3P_BYTE_COUNT_POS);
    buffer_p->size = (dscr.size & CY_U3P_BUFFER_SIZE_MASK);
    buffer_p->status = (dscr.size & CY_U3P_DMA_BUFFER_STATUS_MASK);

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaChannelGetBuffer (
        CyU3PDmaChannel *handle,
        CyU3PDmaBuffer_t *buffer_p,
        uint32_t waitOption)
{
    uint8_t type;
    uint32_t status, flags;
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaSocketConfig_t sck;

    /* Check the parameter validity first. */
    if (buffer_p == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    status = CyU3PDmaChannelAcquireLock (handle, waitOption);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* This API can be called only for manual channels. Also the
     * channel must be in active state. */
    if ((handle->state != CY_U3P_DMA_ACTIVE) &&
            (handle->state != CY_U3P_DMA_IN_COMPLETION))
    {
        status = CY_U3P_ERROR_NOT_STARTED;
    }

#ifndef CYU3P_DISABLE_ERROR_CHECK
    if (handle->state == CY_U3P_DMA_ERROR)
    {
        status = CY_U3P_ERROR_DMA_FAILURE;
    }
    if (handle->state == CY_U3P_DMA_ABORTED)
    {
        status = CY_U3P_ERROR_ABORTED;
    }
#endif /* CYU3P_DISABLE_ERROR_CHECK */

    if (status != CY_U3P_SUCCESS)
    {
        /* Release the lock and return. */
        CyU3PMutexPut (&(handle->lock));
        return status;
    }

    type = handle->type;

    if ((type == CY_U3P_DMA_TYPE_MANUAL) || (type == CY_U3P_DMA_TYPE_MANUAL_IN))
    {
        /* Clear any previous events before waiting for a buffer. */
        CyU3PEventSet (&(handle->flags), (~CY_U3P_DMA_CB_PROD_EVENT), CYU3P_EVENT_AND);
        CyU3PDmaDscrGetConfig (handle->currentProdIndex, &dscr);
        /* In case of MANUAL_IN channel this function returns an occupied buffer.
         * If the buffer is empty, wait for it to be occupied */
        if (!(dscr.size & CY_U3P_BUFFER_OCCUPIED))
        {
            /* Before waiting, release the lock. */
            CyU3PMutexPut (&(handle->lock));
            status = CyU3PEventGet (&(handle->flags), (CY_U3P_DMA_CB_PROD_EVENT |
                        CY_U3P_DMA_CB_ABORTED | CY_U3P_DMA_CB_ERROR), CYU3P_EVENT_OR,
                    &flags, waitOption);
            if (status != CY_U3P_SUCCESS)
            {
                return CY_U3P_ERROR_TIMEOUT;
            }
            if (flags & CY_U3P_DMA_CB_ERROR)
            {
                return CY_U3P_ERROR_DMA_FAILURE;
            }
            if (flags & CY_U3P_DMA_CB_ABORTED)
            {
                return CY_U3P_ERROR_ABORTED;
            }
            /* Acquire the lock again. */
            status = CyU3PDmaChannelAcquireLock (handle, waitOption);
            if (status != CY_U3P_SUCCESS)
            {
                return status;
            }

            CyU3PDmaDscrGetConfig (handle->currentProdIndex, &dscr);
        }
        else if (handle->isDmaHandleDCache)
        {
            /* Ensure that the DMA thread had a chance to run. */
            CyU3PThreadRelinquish ();
        }
        else
        {
            /* Nothing to do. */
        }

        /* Load the buffer address and byte count information regarding the buffer
         * The buffer is as produced by the producer and so will not include the
         * prodHeader and prodFooter. */
        buffer_p->buffer = (uint8_t *)dscr.buffer;
        buffer_p->count  = (dscr.size >> CY_U3P_BYTE_COUNT_POS);
        buffer_p->size   = (dscr.size & CY_U3P_BUFFER_SIZE_MASK);
        buffer_p->status = (dscr.size & CY_U3P_DMA_BUFFER_STATUS_MASK);
    }
    else if (type == CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        /* Clear any previous events before waiting for a buffer. */
        CyU3PEventSet (&(handle->flags), (~CY_U3P_DMA_CB_CONS_EVENT), CYU3P_EVENT_AND);
        CyU3PDmaDscrGetConfig (handle->currentConsIndex, &dscr);
        /* In case of MANUAL_OUT channel this function returns an empty buffer.
         * If the buffer is not empty wait for it to become empty */
        if (dscr.size & CY_U3P_BUFFER_OCCUPIED)
        {
            /* Before waiting, release the lock. */
            CyU3PMutexPut (&(handle->lock));
            status = CyU3PEventGet (&(handle->flags), (CY_U3P_DMA_CB_CONS_EVENT |
                        CY_U3P_DMA_CB_ABORTED | CY_U3P_DMA_CB_ERROR), CYU3P_EVENT_OR,
                    &flags, waitOption);
            if (status != CY_U3P_SUCCESS)
            {
                return CY_U3P_ERROR_TIMEOUT;
            }
            if (flags & CY_U3P_DMA_CB_ERROR)
            {
                return CY_U3P_ERROR_DMA_FAILURE;
            }
            if (flags & CY_U3P_DMA_CB_ABORTED)
            {
                return CY_U3P_ERROR_ABORTED;
            }
            /* Acquire the lock again. */
            status = CyU3PDmaChannelAcquireLock (handle, waitOption);
            if (status != CY_U3P_SUCCESS)
            {
                return status;
            }

            CyU3PDmaDscrGetConfig (handle->currentConsIndex, &dscr);
        }

        /* In this case, the buffer is empty. The count is the consumer header,
         * the size will be the size of the total buffer allocated. */
        buffer_p->buffer = ((uint8_t *)dscr.buffer - handle->consHeader);
        buffer_p->count = handle->consHeader;
        buffer_p->size = handle->size;
        /* In this case the buffer is free and ready to be filled.
         * So there are no markers to be read. */
        buffer_p->status = 0;
    }
    else /* CY_U3P_DMA_TYPE_AUTO and CY_U3P_DMA_TYPE_AUTO_SIGNAL */
    {
        CyU3PDmaSocketGetConfig (handle->consSckId, &sck);
        CyU3PDmaDscrGetConfig ((sck.dscrChain &
                    CY_U3P_DSCR_NUMBER_MASK), &dscr);

        if ((sck.status & CY_U3P_SUSPENDED) &&
                (dscr.size & CY_U3P_DMA_BUFFER_OCCUPIED))
        {
            /* Load buffer address and byte count and status
             * information regarding the buffer. */
            buffer_p->buffer = (uint8_t *)dscr.buffer;
            buffer_p->count  = (dscr.size >> CY_U3P_BYTE_COUNT_POS);
            buffer_p->size   = (dscr.size & CY_U3P_BUFFER_SIZE_MASK);
            buffer_p->status = (dscr.size & CY_U3P_DMA_BUFFER_STATUS_MASK);

            if (handle->isDmaHandleDCache)
            {
                /* Since the buffer is received from the H/W, make sure that
                 * the buffer is flushed before returning from this call. */
                CyU3PSysFlushDRegion ((uint32_t *)buffer_p->buffer, buffer_p->size);
            }
        }
        else
        {
            status = CY_U3P_ERROR_INVALID_SEQUENCE;
        }
    }

    /* Release the mutex. */
    CyU3PMutexPut (&(handle->lock));

    return status;
}

CyU3PReturnStatus_t
CyU3PDmaChannelSendData (
        CyU3PDmaChannel *handle,
        uint8_t         *buffer,
        uint16_t         count)
{
    uint32_t status;
    CyU3PDmaDescriptor_t consDscr;
    CyU3PDmaSocketConfig_t sck;

    /* Check for parameter validity first. */
    if (buffer == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if ((buffer < CY_U3P_DMA_BUFFER_AREA_BASE) || ((buffer + count) >= CY_U3P_DMA_BUFFER_AREA_LIMIT))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (count > CY_U3P_DMA_MAX_BUFFER_SIZE)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    if (handle->type != CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
    }

    /* Socket should be in enabled state. */
    CyU3PDmaSocketGetConfig (handle->consSckId, &sck);
    if ((sck.status & CY_U3P_ENABLED) == 0)
    {
        status = CY_U3P_ERROR_NOT_STARTED;
    }

    if (status != CY_U3P_SUCCESS)
    {
        /* Release the lock and return. */
        CyU3PMutexPut (&(handle->lock));
        return status;
    }

#ifdef CYU3P_DMA_HANDLE_DCACHE
    if (handle->isDmaHandleDCache)
    {
        if (count)
        {
            CyU3PSysCleanDRegion ((uint32_t *)buffer, count);
        }
    }
#endif

    /* Clear the event flags before queuing a request */
    CyU3PEventSet (&(handle->flags),  (~(CY_U3P_DMA_CB_XFER_CPLT |
                    CY_U3P_DMA_CB_SEND_CPLT | CY_U3P_DMA_CB_RECV_CPLT |
                    CY_U3P_DMA_CB_ABORTED)), CYU3P_EVENT_AND);

    /* Configure the consumer socket */
    handle->currentConsIndex = sck.dscrChain & CY_U3P_DSCR_NUMBER_MASK;

    /* Configure the descriptor */
    CyU3PDmaDscrGetConfig (handle->currentConsIndex, &consDscr);
    if (consDscr.size & CY_U3P_BUFFER_OCCUPIED)
    {
        status = CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    if (status == CY_U3P_SUCCESS)
    {
        handle->state = CY_U3P_DMA_PROD_OVERRIDE;

        /* Make sure that the consume event interrupt is turned on. */
        sck.intrMask |= CY_U3P_CONSUME_EVENT;
        sck.intrMask &= ~CY_U3P_TRANS_DONE;
        CyU3PDmaSocketSetConfig (handle->consSckId, &sck);

        /* Update the DMA descriptor. */
        consDscr.buffer = buffer;
        consDscr.size   = (count << CY_U3P_BYTE_COUNT_POS) | CY_U3P_BUFFER_OCCUPIED | CY_U3P_UIB_EOP |
            ((count + 15) & 0xFFF0);
        consDscr.sync   = consDscr.sync | CY_U3P_EN_CONS_EVENT | CY_U3P_EN_CONS_INT;
        CyU3PDmaDscrSetConfig (handle->currentConsIndex, &consDscr);

        /* Send an event to the socket to notify it that data is ready. */
        CyU3PDmaSocketSendEvent (handle->consSckId, handle->currentConsIndex, CyTrue);
        handle->currentConsIndex = (consDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));
    return status;
}

CyU3PReturnStatus_t
CyU3PDmaChannelCommitBuffer (
        CyU3PDmaChannel *handle,
        uint16_t count,
        uint16_t bufStatus)
{
    uint8_t type;
    uint16_t index;
    uint32_t status;
    CyU3PDmaSocketConfig_t sck;
    CyU3PDmaDescriptor_t prodDscr, consDscr;

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* The API can be called only for MANUAL and MANUAL_OUT
     * channels. Also the channel must be in active state. */
    if (count > (handle->size - handle->consHeader))
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (handle->state != CY_U3P_DMA_ACTIVE)
    {
        status = CY_U3P_ERROR_NOT_STARTED;
    }

#ifndef CYU3P_DISABLE_ERROR_CHECK
    if (handle->state == CY_U3P_DMA_ERROR)
    {
        status = CY_U3P_ERROR_DMA_FAILURE;
    }
    if (handle->state == CY_U3P_DMA_ABORTED)
    {
        status = CY_U3P_ERROR_ABORTED;
    }
#endif /* CYU3P_DISABLE_ERROR_CHECK */

    if (handle->type == CY_U3P_DMA_TYPE_MANUAL_IN)
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if (status != CY_U3P_SUCCESS)
    {
        /* Release the lock and return. */
        CyU3PMutexPut (&(handle->lock));
        return status;
    }

    type = handle->type;
    if (type == CY_U3P_DMA_TYPE_MANUAL)
    {
        /* In case of MANUAL channel, the consumer descriptor
         * chain is different. Configure the consumer descriptor
         * chain to commit the modified buffer */
        CyU3PDmaDscrGetConfig (handle->currentProdIndex, &prodDscr);
        CyU3PDmaDscrGetConfig (handle->currentConsIndex, &consDscr);
        if (!(prodDscr.size & CY_U3P_BUFFER_OCCUPIED))
        {
            status = CY_U3P_ERROR_INVALID_SEQUENCE;
        }
        if (consDscr.size & CY_U3P_BUFFER_OCCUPIED)
        {
            status = CY_U3P_ERROR_INVALID_SEQUENCE;
        }

        if (status == CY_U3P_SUCCESS)
        {
            consDscr.size &= CY_U3P_BUFFER_SIZE_MASK;
            consDscr.size |= (count << CY_U3P_BYTE_COUNT_POS);
            /* Update the status bits. */
            if (bufStatus == 0)
            {
                /* The buffer status of the producer 
                 * descriptor is copied. */
                consDscr.size |= (prodDscr.size & 
                        CY_U3P_DMA_BUFFER_STATUS_MASK);
            }
            else
            {
                /* The buffer status is set as provided. */
                consDscr.size |= ((bufStatus & CY_U3P_DMA_BUFFER_STATUS_WRITE_MASK) |
                        CY_U3P_DMA_BUFFER_OCCUPIED);
            }

            if (handle->isDmaHandleDCache)
            {
                /* Since the buffer is received from the CPU, make sure that
                 * the buffer is cleaned before sending it to DMA adapter. */
                CyU3PSysCleanDRegion ((uint32_t *)consDscr.buffer, consDscr.size &
                        CY_U3P_BUFFER_SIZE_MASK);
            }

            CyU3PDmaDscrSetConfig (handle->currentConsIndex,
                    &consDscr);
            CyU3PDmaSocketSendEvent (handle->consSckId, 
                    handle->currentConsIndex, CyTrue);
            /* Move the pointers to the next buffer in the chain */
            handle->currentProdIndex = (prodDscr.chain & 
                    CY_U3P_RD_NEXT_DSCR_MASK);
            handle->currentConsIndex = (consDscr.chain &
                    CY_U3P_RD_NEXT_DSCR_MASK);
        }
    }
    else if (type == CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        /* Commit the modified buffer to the consumer socket */
        CyU3PDmaDscrGetConfig (handle->currentConsIndex, &consDscr);
        if (consDscr.size & CY_U3P_BUFFER_OCCUPIED)
        {
            status = CY_U3P_ERROR_INVALID_SEQUENCE;
        }

        if (status == CY_U3P_SUCCESS)
        {
            consDscr.size &= (~CY_U3P_BYTE_COUNT_MASK);
            consDscr.size |= ((count << CY_U3P_BYTE_COUNT_POS) |
                    CY_U3P_BUFFER_OCCUPIED);

            /* Update the status bits. */
            consDscr.size |= (bufStatus & CY_U3P_DMA_BUFFER_STATUS_WRITE_MASK);
            if (handle->isDmaHandleDCache)
            {
                /* Since the buffer is received from the CPU, make sure that
                 * the buffer is cleaned before sending it to DMA adapter. */
                CyU3PSysCleanDRegion ((uint32_t *)consDscr.buffer, consDscr.size &
                        CY_U3P_BUFFER_SIZE_MASK);
            }

            CyU3PDmaDscrSetConfig (handle->currentConsIndex, &consDscr);
            CyU3PDmaSocketSendEvent (handle->consSckId,
                    handle->currentConsIndex, CyTrue);
            handle->currentConsIndex = (consDscr.chain &
                CY_U3P_RD_NEXT_DSCR_MASK);
        }
    }
    else /* CY_U3P_DMA_TYPE_AUTO and CY_U3P_DMA_TYPE_AUTO_SIGNAL */
    {
        CyU3PDmaSocketGetConfig (handle->consSckId, &sck);

        if (sck.status & CY_U3P_SUSPENDED)
        {
            index = sck.dscrChain & CY_U3P_DSCR_NUMBER_MASK;
            CyU3PDmaDscrGetConfig (index, &consDscr);
            /* Update consumer descriptor with given count and status. */
            if (consDscr.size & CY_U3P_DMA_BUFFER_OCCUPIED)
            {
                consDscr.size &= ~(CY_U3P_BYTE_COUNT_MASK);
                consDscr.size |= (count << CY_U3P_BYTE_COUNT_POS);
                if (bufStatus != 0)
                {
                    /* The buffer status is set as provided. */
                    consDscr.size &= ~(CY_U3P_DMA_BUFFER_STATUS_MASK);
                    consDscr.size |= ((bufStatus | CY_U3P_DMA_BUFFER_OCCUPIED)
                            & CY_U3P_DMA_BUFFER_STATUS_WRITE_MASK);
                }

                if (handle->isDmaHandleDCache)
                {
                    /* Since the buffer is received from the CPU, make sure that
                     * the buffer is cleaned before sending it to DMA adapter. */
                    CyU3PSysCleanDRegion ((uint32_t *)consDscr.buffer, consDscr.size &
                            CY_U3P_BUFFER_SIZE_MASK);
                }

                CyU3PDmaDscrSetConfig (index, &consDscr);
            }
            else
            {
                status = CY_U3P_ERROR_INVALID_SEQUENCE;
            }
        }
        else
        {
            status = CY_U3P_ERROR_INVALID_SEQUENCE;
        }
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return status;
}

CyU3PReturnStatus_t
CyU3PDmaChannelDiscardBuffer (
        CyU3PDmaChannel *handle)
{
    uint8_t type;
    uint16_t index;
    uint32_t status;
    uint32_t priority;
    CyU3PThread *thread_p;
    CyU3PDmaSocketConfig_t sck;
    CyU3PDmaDescriptor_t prodDscr, consDscr;

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    type = handle->type;
    /* This API is supported only for MANUAL and MANUAL_OUT
     * channels. Also the channel must be in active state. */
    if ((handle->state != CY_U3P_DMA_ACTIVE) &&
            (handle->state != CY_U3P_DMA_IN_COMPLETION))
    {
        status = CY_U3P_ERROR_NOT_STARTED;
    }
#ifndef CYU3P_DISABLE_ERROR_CHECK
    if (handle->state == CY_U3P_DMA_ERROR)
    {
        status = CY_U3P_ERROR_DMA_FAILURE;
    }
    if (handle->state == CY_U3P_DMA_ABORTED)
    {
        status = CY_U3P_ERROR_ABORTED;
    }
#endif /* CYU3P_DISABLE_ERROR_CHECK */
    if (handle->type == CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if (status != CY_U3P_SUCCESS)
    {
        /* Release the lock and return. */
        CyU3PMutexPut (&(handle->lock));
        return status;
    }

    if (type == CY_U3P_DMA_TYPE_MANUAL)
    {
        /* In case of MANUAL channel, the consumer descriptor
         * chain is different. Configure the consumer descriptor
         * chain to set the discard marker. */
        CyU3PDmaDscrGetConfig (handle->currentProdIndex, &prodDscr);
        CyU3PDmaDscrGetConfig (handle->currentConsIndex, &consDscr);
        if (!(prodDscr.size & CY_U3P_BUFFER_OCCUPIED))
        {
            status = CY_U3P_ERROR_INVALID_SEQUENCE;
        }
        if (consDscr.size & CY_U3P_BUFFER_OCCUPIED)
        {
            status = CY_U3P_ERROR_INVALID_SEQUENCE;
        }

        if (status == CY_U3P_SUCCESS)
        {
            /* Set the discard marker. */
            consDscr.size &= CY_U3P_BUFFER_SIZE_MASK;
            consDscr.size |= (CY_U3P_MARKER);
            CyU3PDmaDscrSetConfig (handle->currentConsIndex,
                    &consDscr);

            /* If the discard count is zero, then enable the stall
             * interrupt for the consumer socket. */
            if (handle->discardCount == 0)
            {
                CyU3PDmaSocketGetConfig (handle->consSckId, &sck);
                sck.intrMask |= CY_U3P_STALL;
                /* Take care not to clear the interrupts. */
                sck.intr     = 0;
                CyU3PDmaSocketSetConfig (handle->consSckId, &sck);
            }
            /* Increment the discard buffer count. */
            handle->discardCount++;

            /* Move the pointers to the next buffer in the chain. */
            handle->currentProdIndex = (prodDscr.chain & 
                    CY_U3P_RD_NEXT_DSCR_MASK);
            handle->currentConsIndex = (consDscr.chain &
                    CY_U3P_RD_NEXT_DSCR_MASK);
        }
    }
    else if (type == CY_U3P_DMA_TYPE_MANUAL_IN)
    {
        /* Identify the received buffer from the producer socket */
        CyU3PDmaDscrGetConfig (handle->currentProdIndex, &prodDscr);
        if (!(prodDscr.size & CY_U3P_BUFFER_OCCUPIED))
        {
            /* Release the lock and return. */
            CyU3PMutexPut (&(handle->lock));
            return CY_U3P_ERROR_INVALID_SEQUENCE;
        }

        prodDscr.size &= CY_U3P_BUFFER_SIZE_MASK;
        CyU3PDmaDscrSetConfig (handle->currentProdIndex, &prodDscr);
        CyU3PDmaSocketSendEvent (handle->prodSckId, handle->currentProdIndex, CyFalse);

        if (handle->isDmaHandleDCache)
        {
            /* Make sure that this buffer is removed from the D-Cache at this point. */
            CyU3PSysFlushDRegion ((uint32_t *)prodDscr.buffer, prodDscr.size & CY_U3P_BUFFER_SIZE_MASK);
        }

        /* Move the pointer to the next buffer in the chain */
        handle->currentProdIndex = prodDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK;
        if ((handle->state == CY_U3P_DMA_IN_COMPLETION) &&
                (handle->currentProdIndex == handle->activeProdIndex))
        {
            /* This is the last packet of the transfer. Temporarily set the
             * priority of this thread to that of DMA thread to prevent any pre-emption.
             * Then mark the transfer complete and invoke the callback. */
            thread_p = CyU3PThreadIdentify ();
            if (thread_p != NULL)
            {
                CyU3PThreadPriorityChange (thread_p, CY_U3P_DMA_API_THREAD_PRIORITY,
                        &priority);
            }
            handle->state = CY_U3P_DMA_CONFIGURED;
            /* Send event notification to any waiting threads */
            CyU3PEventSet (&(handle->flags), CY_U3P_DMA_CB_XFER_CPLT, CYU3P_EVENT_OR);
            /* Invoke the completed callback from here. */
            if ((handle->cb) && (handle->notification & CY_U3P_DMA_CB_XFER_CPLT))
            {
                handle->cb (handle, CY_U3P_DMA_CB_XFER_CPLT, 0);
            }
            /* Restore the thread priority to its original value. */
            if (thread_p != NULL)
            {
                CyU3PThreadPriorityChange (thread_p, priority, &priority);
            }
        }
    }
    else /* CY_U3P_DMA_TYPE_AUTO and CY_U3P_DMA_TYPE_AUTO_SIGNAL */
    {
        CyU3PDmaSocketGetConfig (handle->consSckId, &sck);

        if (sck.status & CY_U3P_SUSPENDED)
        {
            index = sck.dscrChain & CY_U3P_DSCR_NUMBER_MASK;
            CyU3PDmaDscrGetConfig (index, &consDscr);
            if (consDscr.size & CY_U3P_DMA_BUFFER_OCCUPIED)
            {
                /* First move the consumer socket to the next
                 * descriptor in chain. */
                sck.dscrChain &= ~(CY_U3P_DSCR_NUMBER_MASK);
                sck.dscrChain |= (consDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
                sck.intr = 0;
                CyU3PDmaSocketSetConfig (handle->consSckId, &sck);

                /* Clear the current descriptor and send event
                 * to the producer. */
                consDscr.size &= CY_U3P_BUFFER_SIZE_MASK;
                CyU3PDmaDscrSetConfig (index, &consDscr);
                CyU3PDmaSocketSendEvent (handle->prodSckId, index, CyFalse);
            }
            else
            {
                status = CY_U3P_ERROR_INVALID_SEQUENCE;
            }
        }
        else
        {
            status = CY_U3P_ERROR_INVALID_SEQUENCE;
        }
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return status;
}

CyU3PReturnStatus_t
CyU3PDmaChannelWaitForCompletion (
        CyU3PDmaChannel *handle,
        uint32_t waitOption)
{
    uint32_t state;
    uint32_t status;
    uint32_t flags, mask = 0;

    status = CyU3PDmaChannelAcquireLock (handle, waitOption);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    state = handle->state;

    /* See if the transfer is already completed */
    if (state == CY_U3P_DMA_CONFIGURED)
    {
        status = CyU3PEventGet (&(handle->flags), 
                (CY_U3P_DMA_CB_XFER_CPLT | CY_U3P_DMA_CB_SEND_CPLT |
                 CY_U3P_DMA_CB_RECV_CPLT),
                CYU3P_EVENT_OR, &flags, CYU3P_NO_WAIT);
        if (status != CY_U3P_SUCCESS)
        {
            status = CY_U3P_ERROR_NOT_STARTED;
        }
    }
    else if (state == CY_U3P_DMA_ACTIVE)
    {
        /* The API is not supported for infinite transfers */
        if (handle->xferSize == 0)
        {
            status = CY_U3P_ERROR_NOT_SUPPORTED;
        }
        /* Wait for the normal transaction to complete */
        mask = (CY_U3P_DMA_CB_XFER_CPLT | CY_U3P_DMA_CB_ABORTED | CY_U3P_DMA_CB_ERROR);
    }
    else if (state == CY_U3P_DMA_PROD_OVERRIDE)
    {
        /* Wait for the producer socket override transaction to complete */
        mask = (CY_U3P_DMA_CB_SEND_CPLT | CY_U3P_DMA_CB_ABORTED | CY_U3P_DMA_CB_ERROR);
    }
    else if (state == CY_U3P_DMA_CONS_OVERRIDE)
    {
        /* Wait for the consumer socket override transaction to complete */
        mask = (CY_U3P_DMA_CB_RECV_CPLT | CY_U3P_DMA_CB_ABORTED | CY_U3P_DMA_CB_ERROR);
    }
    else if (state == CY_U3P_DMA_ERROR)
    {
        status = CY_U3P_ERROR_DMA_FAILURE;
    }
    else
    {
        status = CY_U3P_ERROR_NOT_STARTED;
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    if ((status == CY_U3P_SUCCESS) && (mask != 0))
    {
        /* Release the lock before waiting. */
        status = CyU3PEventGet (&(handle->flags), mask, CYU3P_EVENT_OR,
                &flags, waitOption);
        if (status != CY_U3P_SUCCESS)
        {
            status = CY_U3P_ERROR_TIMEOUT;
        }
        else if (flags & CY_U3P_DMA_CB_ERROR)
        {
            status = CY_U3P_ERROR_DMA_FAILURE;
        }
        else if (flags & CY_U3P_DMA_CB_ABORTED)
        {
            status = CY_U3P_ERROR_ABORTED;
        }
    }

    return status;
}

CyU3PReturnStatus_t
CyU3PDmaChannelGetStatus (
        CyU3PDmaChannel *handle,
        CyU3PDmaState_t *state,
        uint32_t *prodXferCount,
        uint32_t *consXferCount)
{
    uint32_t status, flags, chState;
    CyU3PDmaSocketConfig_t sck;
    CyU3PDmaDescriptor_t dscr;

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Identify the channel state. */
    chState = handle->state;
    if (chState == CY_U3P_DMA_CONFIGURED)
    {
        /* Check if this is after a transfer completion. */
        status = CyU3PEventGet (&(handle->flags), 
                (CY_U3P_DMA_CB_XFER_CPLT | CY_U3P_DMA_CB_SEND_CPLT |
                 CY_U3P_DMA_CB_RECV_CPLT | CY_U3P_DMA_CB_ABORTED),
                CYU3P_EVENT_OR, &flags, CYU3P_NO_WAIT);

        if (status == CY_U3P_SUCCESS)
        {
            if (flags & CY_U3P_DMA_CB_XFER_CPLT)
            {
                chState = CY_U3P_DMA_XFER_COMPLETED;
            }
            else if (flags & CY_U3P_DMA_CB_SEND_CPLT)
            {
                chState = CY_U3P_DMA_SEND_COMPLETED;
            }
            else if (flags & CY_U3P_DMA_CB_RECV_CPLT)
            {
                chState = CY_U3P_DMA_RECV_COMPLETED;
            }
        }
    }

    if (state != NULL)
    {
        *state = (CyU3PDmaState_t)chState;
    }
    if (prodXferCount != NULL)
    {
        *prodXferCount = 0;
        if (handle->type != CY_U3P_DMA_TYPE_MANUAL_OUT)
        {
            CyU3PDmaSocketGetConfig (handle->prodSckId, &sck);
            *prodXferCount = sck.xferCount;

            /* Since the MODE_BUFFER is used in case of producer override,
             * the actual count has to be received from the override descriptor. */
            if (chState == CY_U3P_DMA_RECV_COMPLETED)
            {
                CyU3PDmaDscrGetConfig (handle->overrideDscrIndex, &dscr);
                *prodXferCount = (dscr.size >> CY_U3P_BYTE_COUNT_POS);
            }
        }
    }

    if (consXferCount != NULL)
    {
        *consXferCount = 0;
        if (handle->type != CY_U3P_DMA_TYPE_MANUAL_IN)
        {
            CyU3PDmaSocketGetConfig (handle->consSckId, &sck);
            *consXferCount = sck.xferCount;
        }
    }

    CyU3PMutexPut (&(handle->lock));
    return status;
}

CyU3PReturnStatus_t
CyU3PDmaChannelCacheControl (
        CyU3PDmaChannel *handle,
        CyBool_t isDmaHandleDCache)
{
    uint32_t status;

    status = CyU3PDmaChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    if (handle->state == CY_U3P_DMA_CONFIGURED)
    {
        /* Update the cache control feature. */
        handle->isDmaHandleDCache = isDmaHandleDCache;
    }
    else
    {
        /* Channel mode can be updated only when it is in
         * configured mode. */
        status = CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return status;
}

/* [] */

