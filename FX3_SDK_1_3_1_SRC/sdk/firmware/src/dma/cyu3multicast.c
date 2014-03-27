/*
 ## Cypress USB 3.0 Platform source file (cyu3multicast.c)
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

/* This file has code to create and manage multicast DMA channels. */

#include <cyu3os.h>
#include <cyu3error.h>
#include <cyu3mmu.h>
#include <cyu3socket.h>
#include <cyu3protocol.h>
#include <cyu3dma.h>
#include <cyu3regs.h>

/* Types and functions used from other source files. */
typedef void (*CyU3PDmaChannelIntHandler) (
        uint32_t *msg);
typedef CyU3PReturnStatus_t (*CyU3PDmaMultiChannelOpern) (
        CyU3PDmaMultiChannel *chHandle);
typedef void (*CyU3PDmaMultiChannelSetup) (
        CyU3PDmaMultiChannel *chHandle, uint32_t cnt, uint16_t offset);

extern void
CyU3PDmaSetMulticastIntHandler (
        CyU3PDmaChannelIntHandler handler_p);
extern void
CyU3PDmaSetMulticastHandlers (
        CyU3PDmaMultiChannelOpern configure_p,
        CyU3PDmaMultiChannelOpern destroy_p,
        CyU3PDmaMultiChannelOpern reset_p,
        CyU3PDmaMultiChannelSetup setxfer_p);
extern void
CyU3PDmaMultiChannelHandleError (
        CyU3PDmaMultiChannel *h);
extern CyU3PReturnStatus_t
CyU3PDmaMultiChannelAcquireLock (
        CyU3PDmaMultiChannel *handle,
        uint32_t waitOption);

static void
CyU3PDmaMultiConfigureSockets_TypeMulticast (
        CyU3PDmaMultiChannel *handle)
{
    uint16_t sckCount;
    CyU3PDmaSocketConfig_t sck;

    /* Configure the producer sockets */
    handle->activeProdIndex[0] = handle->firstProdIndex[0];
    CyU3PDmaSocketDisable (handle->prodSckId[0]);
    CyU3PDmaSocketGetConfig (handle->prodSckId[0], &sck);
    sck.dscrChain = handle->firstProdIndex[0];
    sck.xferSize = 0;
    sck.xferCount = 0;
    sck.status = (CY_U3P_TRUNCATE | CY_U3P_EN_PROD_EVENTS | CY_U3P_SUSP_TRANS);
    if (CyU3PDmaIsSockAvlEnReqd (handle->prodSckId[0]))
        sck.status |= CY_U3P_UIB_AVL_ENABLE;

    sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
    sck.intrMask = (CY_U3P_TRANS_DONE | CY_U3P_ERROR | CY_U3P_PRODUCE_EVENT);
    CyU3PDmaSocketSetConfig (handle->prodSckId[0], &sck);

    /* Configure the consumer sockets */
    for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
    {
        handle->activeConsIndex[sckCount] = handle->firstConsIndex[sckCount];
        CyU3PDmaSocketDisable (handle->consSckId[sckCount]);
        CyU3PDmaSocketGetConfig (handle->consSckId[sckCount], &sck);
        sck.dscrChain = handle->firstConsIndex[sckCount];
        sck.xferSize = 0;
        sck.xferCount = 0;
        sck.status = (CY_U3P_TRUNCATE | CY_U3P_EN_CONS_EVENTS | CY_U3P_SUSP_TRANS);
        if (CyU3PDmaIsSockAvlEnReqd (handle->consSckId[sckCount]))
            sck.status |= CY_U3P_UIB_AVL_ENABLE;

        sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
        sck.intrMask = (CY_U3P_CONSUME_EVENT | CY_U3P_ERROR);

        CyU3PDmaSocketSetConfig (handle->consSckId[sckCount], &sck);

        /* Reset the discard counter. */
        handle->discardCount[sckCount] = 0;
    }
}

static CyU3PReturnStatus_t
CyU3PDmaMultiChannelConfigure_TypeMulticast (
        CyU3PDmaMultiChannel *handle)
{
    CyU3PDmaDescriptor_t prodDscr, consDscr;
    uint32_t status, dscrSync;
    uint16_t count, index, sckCount, consIndex;

    /* Mark the consumer socket as CPU. */
    dscrSync = (CY_U3P_CPU_SOCKET_CONS | CY_U3P_EN_CONS_EVENT | CY_U3P_EN_CONS_INT);
    dscrSync  |= (((handle->prodSckId[0] << CY_U3P_PROD_SCK_POS) |
                CY_U3P_EN_PROD_EVENT | CY_U3P_EN_PROD_INT));

    /* Create the descriptor chain. There shall be one producer descriptor chain
     * and validSckCount number of consumer descriptor chains.
     */
    status = CyU3PDmaDscrChainCreate (&(handle->firstProdIndex[0]),
            handle->count, handle->size, dscrSync);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Create the consumer chains. */
    for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
    {
        /* Create the consumer chain but do not allocate buffers.
         * Update the dscrSync value for the consumer chain. */
        dscrSync  = (handle->consSckId[sckCount] | CY_U3P_EN_CONS_EVENT | CY_U3P_EN_CONS_INT);
        dscrSync |= (((CY_U3P_CPU_SOCKET_PROD << CY_U3P_PROD_SCK_POS) |
                    CY_U3P_EN_PROD_EVENT | CY_U3P_EN_PROD_INT));

        CyU3PDmaDscrChainCreate (&(handle->firstConsIndex[sckCount]), handle->count, 0, dscrSync);
        handle->consDisabled[sckCount] = 0;
    }

    /* Modify the consumer chain such that the next producer descriptor
     * points to the current consumer descriptor of the next socket. This
     * is used as a link list to point to the chain of consumer descriptors
     * which point to the same buffer. The last descriptor shall point to
     * producer index. Modify the producer chain to include the producer offsets. */
    for (count = 0, index = handle->firstProdIndex[0]; count < handle->count; count++)
    {
        CyU3PDmaDscrGetConfig (index, &prodDscr);
        /* Update the consumer chain information on the producer descriptors. */
        prodDscr.chain &= CY_U3P_WR_NEXT_DSCR_MASK;
        prodDscr.chain |= handle->firstConsIndex[0];

        for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
        {
            /* Find the consumer chain for the buffer. The firstConsIndex is not modified
             * for sckCount + 1. */
            consIndex = (sckCount == handle->validSckCount - 1) ? index :
                handle->firstConsIndex[sckCount + 1];

            /* Update the buffers for the consumer chains. */
            CyU3PDmaDscrGetConfig (handle->firstConsIndex[sckCount], &consDscr);
            consDscr.buffer = prodDscr.buffer + handle->consHeader;
            /* The magic number 0xF is present to make the size multiple of 16 bytes */
            /* Since the consumer socket can actually transmit data of sizes not aligned to
             * 16 bytes unlike the producer socket, the create call does not mandate
             * the caller to create buffers of sizes to match. But the descriptor takes only
             * buffer sizes as multiples of 16 bytes. So the size needs to be calculated
             * for descriptor update. But transfer can only be done to the actual buffer size.
             * This count is only for the hardware information. */
            consDscr.size = ((handle->size + 0xF - handle->consHeader) &
                    CY_U3P_BUFFER_SIZE_MASK);
            /* Build up an index in the producer chain, so that when a produce event
             * is received, all consumer descriptors for the buffer can be located. */
            consDscr.chain &= CY_U3P_RD_NEXT_DSCR_MASK;
            consDscr.chain |= (consIndex << CY_U3P_WR_NEXT_DSCR_POS);
            CyU3PDmaDscrSetConfig (handle->firstConsIndex[sckCount], &consDscr);

            /* Use firstConsIndex variable as a temp. Last interation will
             * update the variable to the consumer chain first node. */
            handle->firstConsIndex[sckCount] = (consDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
        }

        /* Also update the buffer for producer chain with offsets. */
        prodDscr.buffer += handle->prodHeader;
        prodDscr.size = (handle->size - handle->prodHeader - handle->prodFooter);
        CyU3PDmaDscrSetConfig (index, &prodDscr);

        /* Move to the next buffer. */
        index = (prodDscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
    }

    /* Update the reference pointers. */
    handle->currentProdIndex = handle->commitProdIndex = handle->firstProdIndex[0];
    handle->currentConsIndex = handle->commitConsIndex = handle->firstConsIndex[0];

    /* Allocate an override descriptor */
    CyU3PDmaDscrGet (&(handle->overrideDscrIndex));

    /* Configure the sockets. */
    CyU3PDmaMultiConfigureSockets_TypeMulticast (handle);

    return CY_U3P_SUCCESS;
}

static CyU3PReturnStatus_t
CyU3PDmaMultiChannelDestroy_TypeMulticast (
        CyU3PDmaMultiChannel *handle)
{
    CyU3PDmaSocketConfig_t sck;
    CyU3PDmaDescriptor_t dscr;
    uint8_t sckNum, ipNum;
    uint16_t count, sckCount, index;

    /* Disable and configure the producer socket */
    CyU3PDmaSocketDisable (handle->prodSckId[0]);
    CyU3PDmaSocketGetConfig (handle->prodSckId[0], &sck);
    sck.dscrChain = CY_U3P_RD_NEXT_DSCR_MASK;
    CyU3PDmaSocketSetConfig (handle->prodSckId[0], &sck);
    sckNum = CyU3PDmaGetSckNum (handle->prodSckId[0]);
    ipNum  = CyU3PDmaGetIpNum (handle->prodSckId[0]);
    glDmaSocketCtrl[ipNum][sckNum].handle = NULL;

    /* Disable and configure the consumer socket. */
    for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
    {

        CyU3PDmaSocketDisable (handle->consSckId[sckCount]);
        CyU3PDmaSocketGetConfig (handle->consSckId[sckCount], &sck);
        sck.dscrChain = CY_U3P_RD_NEXT_DSCR_MASK;
        CyU3PDmaSocketSetConfig (handle->consSckId[sckCount], &sck);
        sckNum = CyU3PDmaGetSckNum (handle->consSckId[sckCount]);
        ipNum  = CyU3PDmaGetIpNum (handle->consSckId[sckCount]);
        glDmaSocketCtrl[ipNum][sckNum].handle = NULL;
    }

    /* Free the descriptor chain. Since the buffer offsets
     * has been added, the buffers has to be freed manually. */
    index = handle->firstProdIndex[0];
    for (count = 0; count < (handle->count * handle->validSckCount); count++)
    {
        CyU3PDmaDscrGetConfig (index, &dscr);
        CyU3PDmaBufferFree (dscr.buffer - handle->prodHeader);
        CyU3PDmaDscrPut (index);
        index = (dscr.chain >> CY_U3P_RD_NEXT_DSCR_POS);
    }

    /* Now free the consumer chains. */
    for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
    {
        CyU3PDmaDscrChainDestroy (handle->firstConsIndex[sckCount], handle->count,
                CyFalse, CyFalse);
    }

    /* Free the override descriptor */
    CyU3PDmaDscrPut (handle->overrideDscrIndex);

    return CY_U3P_SUCCESS;
}

static void
CyU3PDmaMultiChannelSetXfer_TypeMulticast (
        CyU3PDmaMultiChannel *handle,
        uint32_t count,
        uint16_t multiSckOffset)
{
    CyU3PDmaSocketConfig_t sck;
    uint16_t sckCount;

    /* Update the state information first. */
    handle->state = CY_U3P_DMA_ACTIVE;

    /* Disable the sockets. */
    for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
    {
        CyU3PDmaSocketDisable (handle->consSckId[sckCount]);
    }
    CyU3PDmaSocketDisable (handle->prodSckId[0]);

    /* Set the producer socket suspend option. */
    CyU3PDmaUpdateSocketSuspendOption (handle->prodSckId[0],
            handle->prodSusp);

    /* Configure the producer socket */
    CyU3PDmaSocketGetConfig (handle->prodSckId[0], &sck);
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
    CyU3PDmaSocketSetConfig (handle->prodSckId[0], &sck);

    /* Configure the consumer sockets */
    for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
    {
        /* No need to configure a socket that is disabled. */
        if (handle->consDisabled[sckCount] != 0)
            continue;

        CyU3PDmaSocketGetConfig (handle->consSckId[sckCount], &sck);
        /* Since the transfer size specified is for the producer socket,
         * the transfer count for the consumer sockets shall be maintained
         * as infinite. */
        sck.xferSize = 0;
        sck.xferCount = 0;
        sck.intrMask &= (~(CY_U3P_TRANS_DONE));
        sck.status &= (~CY_U3P_TRUNCATE);

        sck.status &= (~CY_U3P_GO_SUSPEND);
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
        CyU3PDmaSocketSetConfig (handle->consSckId[sckCount], &sck);
    }
}

static CyU3PReturnStatus_t
CyU3PDmaMultiChannelReset_TypeMulticast (
        CyU3PDmaMultiChannel *handle)
{
    CyU3PDmaDescriptor_t dscr;
    uint16_t dscrIndex;
    uint16_t count, sckCount;

    /* Disable and re-configure the sockets. */
    CyU3PDmaMultiConfigureSockets_TypeMulticast (handle);

    /* Cleanup the producer descriptors */
    dscrIndex = handle->firstProdIndex[0];
    count = handle->count * handle->validSckCount;
    while ((count--) != 0)
    {
        CyU3PDmaDscrGetConfig (dscrIndex, &dscr);
        dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
        CyU3PDmaDscrSetConfig (dscrIndex, &dscr);
        dscrIndex = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
    }

    /* Clean up the consumer descriptors. */
    for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
    {
        handle->bufferCount[sckCount]  = 0;
        handle->discardCount[sckCount] = 0;

        dscrIndex = handle->activeConsIndex[sckCount] =
            handle->firstConsIndex[sckCount];
        for (count = 0; count < handle->count; count++)
        {
            CyU3PDmaDscrGetConfig (dscrIndex, &dscr);
            dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
            CyU3PDmaDscrSetConfig (dscrIndex, &dscr);
            dscrIndex = (dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
        }
    }

    /* Update the reference pointers. */
    handle->currentProdIndex = handle->commitProdIndex =
        handle->activeProdIndex[0] = handle->firstProdIndex[0];
    handle->currentConsIndex = handle->commitConsIndex =
        handle->firstConsIndex[0];

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaMulticastSocketSelect (
        CyU3PDmaMultiChannel *chHandle,
        uint32_t              consMask)
{
    uint32_t status;
    uint16_t index;
    CyBool_t isChanged = CyFalse;

    status = CyU3PDmaMultiChannelAcquireLock (chHandle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
        return status;

    if (chHandle->type != CY_U3P_DMA_TYPE_MULTICAST)
        status = CY_U3P_ERROR_NOT_SUPPORTED;

    /* At least one of the consumer sockets needs to be enabled at any time. */
    consMask = (consMask & ((1 << chHandle->validSckCount) - 1));
    if (consMask == 0)
        status = CY_U3P_ERROR_BAD_ARGUMENT;

    if (chHandle->state != CY_U3P_DMA_CONFIGURED)
        status = CY_U3P_ERROR_ALREADY_STARTED;

    if (status == CY_U3P_SUCCESS)
    {
        for (index = 0; index < chHandle->validSckCount; index++)
        {
            if ((consMask & (1 << index)) == 0)
            {
                if (chHandle->consDisabled[index] == 0)
                    isChanged = CyTrue;
                chHandle->consDisabled[index] = 1;
            }
            else
            {
                if (chHandle->consDisabled[index] == 1)
                    isChanged = CyTrue;
                chHandle->consDisabled[index] = 0;
            }
        }
    }

    if (isChanged)
        CyU3PDmaMultiChannelReset_TypeMulticast (chHandle);

    CyU3PMutexPut (&(chHandle->lock));
    return status;
}

void
CyU3PDmaIntHandler_TypeMulticast (
        uint32_t *msg)
{
    uint32_t status;
    uint16_t sckId, index, sckIndex, i;
    uint16_t activeIndex;
    CyU3PDmaMultiChannel *h;
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaCBInput_t input;
    CyU3PDmaSocketConfig_t sck;

    h = (CyU3PDmaMultiChannel *)(msg[1]);
    sckId = (msg[0] >> 16);
    status = msg[2];

    /* Check for the error condition first. */
    if (status & CY_U3P_ERROR)
    {
        CyU3PDmaMultiChannelHandleError (h);
        return;
    }

    /* Acquire the lock before accessing the channel handle. */
    CyU3PMutexGet (&(h->lock), CYU3P_WAIT_FOREVER);

    if (sckId != h->prodSckId[0]) /* Consumer socket events */
    {
        activeIndex = (msg[3] & CY_U3P_DSCR_NUMBER_MASK);

        /* Identify the sckCount for the socket which triggered this
         * interrupt. */
        for (sckIndex = 0; sckIndex < h->validSckCount; sckIndex++)
        {
            if (h->consSckId[sckIndex] == sckId)
            {
                break;
            }
        }

        if (status & CY_U3P_CONSUME_EVENT)
        {
            if (h->state != CY_U3P_DMA_PROD_OVERRIDE)
            {
                CyBool_t isDone;
                /* This is a consume event generated during normal transfer.
                 * Decrement the buffer count for this particular socket.
                 * Check if the buffer count for all other sockets are less than
                 * or equal to the current socket. If so then all consumer sockets
                 * have completed transmission of the buffer. Mark the producer
                 * descriptor as free, send a consume event and then invoke the
                 * consumer callback. Also send notification event to the waiting
                 * threads and invoke the callback. This should be done for all
                 * valid buffers. */
                do
                {
                    /* Flag to track if all consumers have completed
                     * transfer of the current buffer. */
                    isDone = CyTrue;

                    /* Decrement the buffer count. */
                    h->bufferCount[sckIndex]--;

                    /* Check if the buffer has been transmitted by all sockets. */
                    for (i = 0; i < h->validSckCount; i++)
                    {
                        if (h->bufferCount[i] > h->bufferCount[sckIndex])
                        {
                            isDone = CyFalse;
                            break;
                        }
                    }

                    if (isDone)
                    {
                        /* Mark the buffer as empty in the producer descriptor. */
                        index = h->commitProdIndex;
                        CyU3PDmaDscrGetConfig (index, &dscr);
                        /* Advance pointer to next descriptor. */
                        h->commitProdIndex = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
                        dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
                        CyU3PDmaDscrSetConfig (index, &dscr);
                        CyU3PSysBarrierSync ();
                        /* Send the produce event to the producer socket. */
                        CyU3PDmaSocketSendEvent (h->prodSckId[0], h->commitProdIndex, CyFalse);

                        if ((h->cb) && (h->notification & CY_U3P_DMA_CB_CONS_EVENT))
                        {
                            h->cb (h, CY_U3P_DMA_CB_CONS_EVENT, 0);
                        }
                    }

                    /* Advance to the next consumer descriptor */
                    CyU3PDmaDscrGetConfig (h->activeConsIndex[sckIndex], &dscr);
                    h->activeConsIndex[sckIndex] = (dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);

                } while (h->activeConsIndex[sckIndex] != activeIndex);

                /* If the producer has signalled all data has been produced,
                 * then check if all buffers has been consumed. If so, then
                 * send the notification and the callback for transfer completion. */
                if (h->state == CY_U3P_DMA_IN_COMPLETION)
                {
                    /* Check for if all sockets have completed
                     * transfer. */
                    isDone = CyTrue;
                    for (i = 0; i < h->validSckCount; i++)
                    {
                        if (h->bufferCount[i] != 0)
                        {
                            isDone = CyFalse;
                            break;
                        }
                    }

                    if (isDone)
                    {
                        h->state = CY_U3P_DMA_CONFIGURED;
                        /* Send event notification to any waiting threads */
                        CyU3PEventSet (&(h->flags), CY_U3P_DMA_CB_XFER_CPLT, CYU3P_EVENT_OR);
                        CyU3PSysBarrierSync ();
                        if ((h->cb) && (h->notification & CY_U3P_DMA_CB_XFER_CPLT))
                        {
                            h->cb (h, CY_U3P_DMA_CB_XFER_CPLT, 0);
                        }
                    }
                }
            }
            else /* CY_U3P_DMA_PROD_OVERRIDE */
            {
                /* Configure the socket to normal mode */
                h->state = CY_U3P_DMA_CONFIGURED;
                /* Mask the consumer event interrupt for auto channels. */
                if (h->type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY)
                {
                    CyU3PDmaSocketGetConfig (sckId, &sck);
                    sck.intr = 0;
                    sck.intrMask &= ~CY_U3P_CONSUME_EVENT;
                    CyU3PDmaSocketSetConfig (sckId, &sck);
                }
                /* Send event notification to any waiting threads */
                CyU3PEventSet (&(h->flags), CY_U3P_DMA_CB_SEND_CPLT, CYU3P_EVENT_OR);
                /* Clear any completed notification */
                status &= (~CY_U3P_TRANS_DONE);
                CyU3PSysBarrierSync ();
                if ((h->cb) && (h->notification & CY_U3P_DMA_CB_SEND_CPLT))
                {
                    h->cb (h, CY_U3P_DMA_CB_SEND_CPLT, 0);
                }
            }
        }

        /* Check if the socket was suspended.
         * If so invoke the corresponding callback. */
        if (status & CY_U3P_SUSPEND)
        {
            CyU3PSysBarrierSync ();
            if ((h->cb) && (h->notification & CY_U3P_DMA_CB_PROD_SUSP))
            {
                h->cb (h, CY_U3P_DMA_CB_PROD_SUSP, 0);
            }
        }
    }
    else /* Producer socket events */
    {
        activeIndex = (msg[3] & CY_U3P_DSCR_NUMBER_MASK);
        if (status & CY_U3P_PRODUCE_EVENT)
        {
            if (h->state != CY_U3P_DMA_CONS_OVERRIDE)
            {
                /* This is a produce event generated during normal transfer.
                 * Loop for all valid buffers and send notification event
                 * to waiting threads and invoke the callback */
                do
                {
                    CyU3PDmaDscrGetConfig (h->activeProdIndex[0], &dscr);
                    input.buffer_p.buffer = dscr.buffer;
                    input.buffer_p.count = (dscr.size >> CY_U3P_BYTE_COUNT_POS);
                    input.buffer_p.size = (dscr.size & CY_U3P_BUFFER_SIZE_MASK);
                    input.buffer_p.status = (dscr.size & CY_U3P_DMA_BUFFER_STATUS_MASK);

                    /* Increment the buffer count for all consumer sockets. */
                    for (sckIndex = 0; sckIndex < h->validSckCount; sckIndex++)
                    {
                        if (h->consDisabled[sckIndex] == 0)
                            h->bufferCount[sckIndex]++;
                    }

                    if (h->isDmaHandleDCache)
                    {
                        /* Since the buffer is received from the H/W, make sure that
                         * the buffer is flushed before returning from this call. */
                        CyU3PSysFlushDRegion ((uint32_t *)input.buffer_p.buffer,
                                input.buffer_p.size);
                    }

                    if ((dscr.size & CY_U3P_BUFFER_OCCUPIED))
                    {
                        /* Send event notification to any waiting threads */
                        CyU3PEventSet (&(h->flags), CY_U3P_DMA_CB_PROD_EVENT, CYU3P_EVENT_OR);
                    }
                    h->activeProdIndex[0] = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
                    CyU3PSysBarrierSync ();
                    if ((h->cb) && (h->notification & CY_U3P_DMA_CB_PROD_EVENT))
                    {
                        h->cb (h, CY_U3P_DMA_CB_PROD_EVENT, &input);
                    }
                } while (h->activeProdIndex[0] != activeIndex);
            }
            else /* CY_U3P_DMA_CONS_OVERRIDE */
            {
                /* Configure the socket to the normal mode */
                CyU3PDmaDscrGetConfig (h->overrideDscrIndex, &dscr);
                input.buffer_p.buffer = dscr.buffer;
                input.buffer_p.count = (dscr.size >> CY_U3P_BYTE_COUNT_POS);
                input.buffer_p.size = (dscr.size & CY_U3P_BUFFER_SIZE_MASK);
                input.buffer_p.status = (dscr.size & CY_U3P_DMA_BUFFER_STATUS_MASK);
                h->state = CY_U3P_DMA_CONFIGURED;

                if (h->isDmaHandleDCache)
                {
                    /* Since the buffer is received from the H/W, make sure that
                     * the buffer is flushed before returning from this call. */
                    CyU3PSysFlushDRegion ((uint32_t *)input.buffer_p.buffer,
                            input.buffer_p.size);
                }

                /* Mask the producer event interrupt for auto channels. */
                if (h->type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY)
                {
                    CyU3PDmaSocketGetConfig (sckId, &sck);
                    sck.intr = 0;
                    sck.intrMask &= ~CY_U3P_PRODUCE_EVENT;
                    CyU3PDmaSocketSetConfig (sckId, &sck);
                }
                /* Send event notification to any waiting threads */
                CyU3PEventSet (&(h->flags), CY_U3P_DMA_CB_RECV_CPLT, CYU3P_EVENT_OR);
                /* Clear any completed notification */
                status &= (~CY_U3P_TRANS_DONE);
                CyU3PSysBarrierSync ();
                if ((h->cb) && (h->notification & CY_U3P_DMA_CB_RECV_CPLT))
                {
                    h->cb (h, CY_U3P_DMA_CB_RECV_CPLT, &input);
                }
            }
        }
        if (status & CY_U3P_TRANS_DONE)
        {
            h->state = CY_U3P_DMA_IN_COMPLETION;
            CyU3PSysBarrierSync ();
        }
    }

    /* Release the lock. */
    CyU3PMutexPut (&(h->lock));
}

/* Register multicast DMA creation and management code with the channel handler. */
void
CyU3PDmaEnableMulticast (
        void)
{
    /* Register the handlers for multicast DMA channels with the DMA module. */
    CyU3PDmaSetMulticastIntHandler (CyU3PDmaIntHandler_TypeMulticast);
    CyU3PDmaSetMulticastHandlers (
            CyU3PDmaMultiChannelConfigure_TypeMulticast,
            CyU3PDmaMultiChannelDestroy_TypeMulticast,
            CyU3PDmaMultiChannelReset_TypeMulticast,
            CyU3PDmaMultiChannelSetXfer_TypeMulticast);

}

/*[]*/

