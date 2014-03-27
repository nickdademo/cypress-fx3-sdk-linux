/*
 ## Cypress USB 3.0 Platform source file (cyu3multichannelutils.c)
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

/* This file contains some of the less frequently used multichannel APIs. */

#include <cyu3os.h>
#include <cyu3error.h>
#include <cyu3mmu.h>
#include <cyu3socket.h>
#include <cyu3dma.h>
#include <cyu3regs.h>

#define CY_U3P_DMA_API_THREAD_PRIORITY      (2)

extern CyU3PReturnStatus_t
CyU3PDmaMultiChannelAcquireLock (
        CyU3PDmaMultiChannel *handle,
        uint32_t waitOption);

CyU3PReturnStatus_t
CyU3PDmaMultiChannelUpdateMode (
        CyU3PDmaMultiChannel *handle,
        CyU3PDmaMode_t dmaMode)
{
    uint32_t status;

    /* Check parameter validity. */
    if (dmaMode >= CY_U3P_DMA_NUM_MODES)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
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
    CyU3PMutexPut (&handle->lock);

    return status;
}

CyU3PReturnStatus_t
CyU3PDmaMultiChannelSetWrapUp (
        CyU3PDmaMultiChannel *handle,
        uint16_t multiSckOffset)
{
    uint32_t status;

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
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
    if ((handle->type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY) ||
            (handle->type == CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY) ||
            (handle->type == CY_U3P_DMA_TYPE_MULTICAST))
    {
        /* Make sure that it is the valid argument. */
        if (multiSckOffset != 0)
        {
            status = CY_U3P_ERROR_BAD_ARGUMENT;
        }
    }
    else /* CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE or CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE */
    {
        /* Make sure that the offset is valid. */
        if (multiSckOffset >= handle->validSckCount)
        {
            status = CY_U3P_ERROR_BAD_ARGUMENT;
        }
    }

    if (status == CY_U3P_SUCCESS)
    {
        /* Set the wrapup bit. */
        CyU3PDmaSocketSetWrapUp (handle->prodSckId[multiSckOffset]);
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return status;
}

CyU3PReturnStatus_t
CyU3PDmaMultiChannelSetSuspend (
        CyU3PDmaMultiChannel *handle,
        CyU3PDmaSckSuspType_t prodSusp,
        CyU3PDmaSckSuspType_t consSusp)
{
    uint32_t status;

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
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
    if (((handle->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE) ||
                (handle->type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE))
            && (prodSusp != CY_U3P_DMA_SCK_SUSP_NONE))
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (((handle->type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY) ||
                (handle->type == CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY) ||
                (handle->type == CY_U3P_DMA_TYPE_MULTICAST))
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
        if (handle->prodSusp != prodSusp)
        {
            handle->prodSusp = prodSusp;
            CyU3PDmaUpdateSocketSuspendOption (handle->prodSckId[0], prodSusp);
        }

        /* Update consumer socket option. */
        if (handle->consSusp != consSusp)
        {
            handle->consSusp = consSusp;
            CyU3PDmaUpdateSocketSuspendOption (handle->consSckId[0], consSusp);
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
CyU3PDmaMultiChannelResume (
        CyU3PDmaMultiChannel *handle,
        CyBool_t        isProdResume,
        CyBool_t        isConsResume)
{
    uint32_t status;

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
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
    if (((handle->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE) ||
                (handle->type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE) ||
                (handle->type == CY_U3P_DMA_TYPE_MULTICAST))
            && (isProdResume == CyTrue))
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (((handle->type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY) ||
                (handle->type == CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY))
            && (isConsResume == CyTrue))
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
        CyU3PDmaUpdateSocketResume (handle->prodSckId[0],
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
        CyU3PDmaUpdateSocketResume (handle->consSckId[0],
                handle->consSusp);
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));
 
    return status;
}


CyU3PReturnStatus_t
CyU3PDmaMultiChannelAbort (
        CyU3PDmaMultiChannel *handle)
{
    CyU3PThread *thread_p;
    uint32_t priority;
    uint32_t status;
    uint16_t sckCount;

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    if ((handle->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE) ||
            (handle->type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE))
    {
        /* Disable all valid producer sockets. */
        for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
        {
            CyU3PDmaSocketDisable (handle->prodSckId[sckCount]);
        }
        /* Disable the consumer socket. */
        CyU3PDmaSocketDisable (handle->consSckId[0]);
    }
    else /* ONE_TO_MANY or MULTICAST */
    {
        /* Disable all valid consumer sockets. */
        for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
        {
            CyU3PDmaSocketDisable (handle->consSckId[sckCount]);
        }
        /* Disable the producer socket. */
        CyU3PDmaSocketDisable (handle->prodSckId[0]);
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
            (~(CY_U3P_DMA_CB_XFER_CPLT | CY_U3P_DMA_CB_SEND_CPLT | CY_U3P_DMA_CB_RECV_CPLT)),
            CYU3P_EVENT_AND);
    /* Set the aborted flag. */
    CyU3PEventSet (&(handle->flags), CY_U3P_DMA_CB_ABORTED, CYU3P_EVENT_OR);

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

/* This function uses override descriptor and loads a buffer to be sent out
 * through a consumer socket. It does not wait for completion.
 * On completion the dma callback function will be invoked as a notification.
 */
CyU3PReturnStatus_t
CyU3PDmaMultiChannelSetupSendBuffer (
        CyU3PDmaMultiChannel *handle,
        CyU3PDmaBuffer_t *buffer_p,
        uint16_t multiSckOffset)
{
    uint16_t size;
    uint16_t consSckId;
    uint32_t status;
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaSocketConfig_t sck;

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

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Should have the correct offsets. */
    if ((handle->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE) ||
            (handle->type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE))
    {
        if (multiSckOffset != 0)
        {
            status = CY_U3P_ERROR_BAD_ARGUMENT;
        }
    }
    else /* CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY or CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY or MULTICAST */
    {
        if (handle->validSckCount <= multiSckOffset)
        {
            status = CY_U3P_ERROR_BAD_ARGUMENT;
        }
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

    if ((handle->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE) ||
            (handle->type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE))
    {
        consSckId = handle->consSckId[0];
    }
    else /* CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY or CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY or MULTICAST */
    {
        consSckId = handle->consSckId[multiSckOffset];
    }

    /* Suspend the consumer socket */
    CyU3PDmaSocketDisable (consSckId);

    /* Set the suspend option for the consumer socket to none. */
    CyU3PDmaUpdateSocketSuspendOption (consSckId,
            CY_U3P_DMA_SCK_SUSP_NONE);

    /* Clear the event flags before queuing a request */
    CyU3PEventSet (&(handle->flags),  (~(CY_U3P_DMA_CB_XFER_CPLT |
                    CY_U3P_DMA_CB_SEND_CPLT | CY_U3P_DMA_CB_RECV_CPLT |
                    CY_U3P_DMA_CB_ABORTED)), CYU3P_EVENT_AND);
   
    /* Update the state information first. */
    handle->state = CY_U3P_DMA_PROD_OVERRIDE;

    /* Configure the consumer socket */
    CyU3PDmaSocketGetConfig (consSckId, &sck);
    handle->currentConsIndex = sck.dscrChain & CY_U3P_DSCR_NUMBER_MASK;
    /* Configure the override descriptor */
    CyU3PDmaDscrGetConfig (handle->overrideDscrIndex, &dscr);
    dscr.buffer = buffer_p->buffer;
    dscr.sync   = (consSckId) | CY_U3P_EN_CONS_EVENT |
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
    if (CyU3PDmaIsSockAvlEnReqd (consSckId) == CyFalse)
        sck.status &= ~CY_U3P_AVL_ENABLE;

    /* Use the DMA_BYTE_MODE since the data size to be transmitted is known */
    sck.status &= (~CY_U3P_UNIT);
    sck.xferSize = buffer_p->count;
    sck.xferCount = 0;
    sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
    sck.intrMask |= CY_U3P_CONSUME_EVENT;
    sck.intrMask &= (~CY_U3P_TRANS_DONE);
    sck.dscrChain = handle->overrideDscrIndex;
    CyU3PDmaSocketSetConfig (consSckId, &sck);

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return CY_U3P_SUCCESS;
}

/* This function uses override descriptor and loads a buffer to receive data
 * through a producer socket. It does not wait for completion.
 * On completion the dma callback function will be invoked as a notification.
 */
CyU3PReturnStatus_t
CyU3PDmaMultiChannelSetupRecvBuffer (
        CyU3PDmaMultiChannel *handle,
        CyU3PDmaBuffer_t *buffer_p,
        uint16_t multiSckOffset)
{
    uint16_t prodSckId;
    uint32_t status;
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaSocketConfig_t sck;

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

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Should have the correct offsets. */
    if ((handle->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE) ||
            (handle->type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE))
    {
        if (handle->validSckCount <= multiSckOffset)
        {
            status = CY_U3P_ERROR_BAD_ARGUMENT;
        }
    }
    else /* CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY or CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY or MULTICAST */
    {
        if (multiSckOffset != 0)
        {
            status = CY_U3P_ERROR_BAD_ARGUMENT;
        }
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

    if ((handle->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE) ||
            (handle->type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE))
    {
        prodSckId = handle->prodSckId[multiSckOffset];
    }
    else /* CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY or CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY or MULTICAST */
    {
        prodSckId = handle->prodSckId[0];
    }

    /* Suspend the producer socket */
    CyU3PDmaSocketDisable (prodSckId);

    /* Set the suspend option for the producer socket to none. */
    CyU3PDmaUpdateSocketSuspendOption (prodSckId,
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
    CyU3PDmaSocketGetConfig (prodSckId, &sck);
    handle->currentProdIndex = sck.dscrChain & CY_U3P_DSCR_NUMBER_MASK;
    /* Configure the override descriptor */
    CyU3PDmaDscrGetConfig (handle->overrideDscrIndex, &dscr);
    dscr.buffer = buffer_p->buffer;
    dscr.sync   = ((prodSckId << CY_U3P_PROD_SCK_POS) |
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
    CyU3PDmaSocketSetConfig (prodSckId, &sck);

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaMultiChannelWaitForRecvBuffer (
        CyU3PDmaMultiChannel *handle,
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

    status = CyU3PDmaMultiChannelAcquireLock (handle, waitOption);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
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
            status = CyU3PDmaMultiChannelAcquireLock (handle, waitOption);
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
CyU3PDmaMultiChannelGetStatus (
        CyU3PDmaMultiChannel *handle,
        CyU3PDmaState_t *state,
        uint32_t *prodXferCount,
        uint32_t *consXferCount,
        uint8_t   sckIndex)
{
    uint8_t index;
    uint32_t status, flags, chState;
    CyU3PDmaSocketConfig_t sck;
    CyU3PDmaDescriptor_t dscr;

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    if (sckIndex >= handle->validSckCount)
    {
        CyU3PMutexPut (&handle->lock);
        return CY_U3P_ERROR_BAD_ARGUMENT;
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
        if ((handle->type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY) ||
                (handle->type == CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY) ||
                (handle->type == CY_U3P_DMA_TYPE_MULTICAST))
        {
            index = 0;
        }
        else /* CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE and CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE */
        {
            index = sckIndex;
        }

        CyU3PDmaSocketGetConfig (handle->prodSckId[index], &sck);
        *prodXferCount = sck.xferCount;

        /* Since the MODE_BUFFER is used in case of producer override,
         * the actual count has to be received from the override descriptor. */
        if (chState == CY_U3P_DMA_RECV_COMPLETED)
        {
            CyU3PDmaDscrGetConfig (handle->overrideDscrIndex, &dscr);
            *prodXferCount = (dscr.size >> CY_U3P_BYTE_COUNT_POS);
        }
    }

    if (consXferCount != NULL)
    {
        if ((handle->type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY) ||
                (handle->type == CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY) ||
                (handle->type == CY_U3P_DMA_TYPE_MULTICAST))
        {
            index = sckIndex;
        }
        else /* CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE and CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE */
        {
            index = 0;
        }

        CyU3PDmaSocketGetConfig (handle->consSckId[index], &sck);
        *consXferCount = sck.xferCount;
    }

    CyU3PMutexPut (&(handle->lock));
    return status;
}

CyU3PReturnStatus_t
CyU3PDmaMultiChannelCacheControl (
        CyU3PDmaMultiChannel *handle,
        CyBool_t isDmaHandleDCache)
{
    uint32_t status;

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
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

