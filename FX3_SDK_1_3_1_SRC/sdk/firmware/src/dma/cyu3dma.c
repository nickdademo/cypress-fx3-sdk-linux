/*
 ## Cypress USB 3.0 Platform source file (cyu3dma.c)
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

/* This file defines the DMA module thread and the event handlers for the
 * DMA module */

#define CY_U3P_DMA_THREAD_STACK         (0x400)
#define CY_U3P_DMA_THREAD_PRIORITY      (2)
#define CY_U3P_DMA_QUEUE_SIZE           (32)
#define CY_U3P_DMA_MSG_SIZE             (16)

#define CY_U3P_DMA_EVENT_MASK           (0xFFFFFFFF)

CyU3PThread     glDmaThread;
CyU3PEvent      glDmaEvent;
CyU3PQueue      glDmaQueue;

uint16_t        glUibSetAvlEn   = 0;    /* Bit-map showing IN endpoints for whom the AVL_EN bit has to be set. */
uint16_t        glUibInSetAvlEn = 0;    /* Bit-map showing OUT endpoints for whom the AVL_EN bit has to be set. */

/* Control information for the sockets */
CyU3PDmaSocketCtrl glDmaLPPSocketCtrl[CY_U3P_DMA_LPP_NUM_SCK] = {0};
CyU3PDmaSocketCtrl glDmaPIBSocketCtrl[CY_U3P_DMA_PIB_NUM_SCK] = {0};
CyU3PDmaSocketCtrl glDmaSIBSocketCtrl[CY_U3P_DMA_SIB_NUM_SCK] = {0};
CyU3PDmaSocketCtrl glDmaUIBSocketCtrl[CY_U3P_DMA_UIB_NUM_SCK] = {0};
CyU3PDmaSocketCtrl glDmaUIBINSocketCtrl[CY_U3P_DMA_UIBIN_NUM_SCK] = {0};

CyU3PDmaSocketCtrl *glDmaSocketCtrl[CY_U3P_NUM_IP_BLOCK_ID] = 
{
    glDmaLPPSocketCtrl,
    glDmaPIBSocketCtrl,
    glDmaSIBSocketCtrl,
    glDmaUIBSocketCtrl,
    glDmaUIBINSocketCtrl
};

typedef void (*CyU3PDmaChannelIntHandler) (uint32_t *msg);
CyU3PDmaChannelIntHandler glMultiCastIntHandler = 0;

void
CyU3PDmaSetMulticastIntHandler (
        CyU3PDmaChannelIntHandler handler_p)
{
    glMultiCastIntHandler = handler_p;
}

void
CyU3PDmaInit (
        void)
{
    CyU3PMemSet ((uint8_t *)glDmaLPPSocketCtrl,   0, (sizeof(CyU3PDmaSocketCtrl) * CY_U3P_DMA_LPP_NUM_SCK));
    CyU3PMemSet ((uint8_t *)glDmaPIBSocketCtrl,   0, (sizeof(CyU3PDmaSocketCtrl) * CY_U3P_DMA_PIB_NUM_SCK));
    CyU3PMemSet ((uint8_t *)glDmaSIBSocketCtrl,   0, (sizeof(CyU3PDmaSocketCtrl) * CY_U3P_DMA_SIB_NUM_SCK));
    CyU3PMemSet ((uint8_t *)glDmaUIBSocketCtrl,   0, (sizeof(CyU3PDmaSocketCtrl) * CY_U3P_DMA_UIB_NUM_SCK));
    CyU3PMemSet ((uint8_t *)glDmaUIBINSocketCtrl, 0, (sizeof(CyU3PDmaSocketCtrl) * CY_U3P_DMA_UIBIN_NUM_SCK));
    glDmaSocketCB = NULL;

    CyU3PDmaDscrListCreate ();

    /* Invoke the block allocator initialization */
    CyU3PDmaBufferInit ();

    glUibSetAvlEn   = 0;
    glUibInSetAvlEn = 0;

    return;
}

/* Function to set the Per-EP Mult (AVL_EN) setting for a USB endpoint. */
void
CyU3PDmaSetUsbSocketMult (
        uint8_t  ep,
        CyBool_t burstEnable)
{
    uint8_t index = ep & 0x0F;

    if ((ep & 0x7F) > 0x0F)
        return;

    if (ep & 0x80)
    {
        if (burstEnable)
            glUibSetAvlEn |= 1 << index;
        else
            glUibSetAvlEn &= ~(1 << index);

        /* If a channel has already been created, update the AVL_EN bit in the SOCK_STATUS register. */
        if (glDmaUIBSocketCtrl[index].handle != NULL)
        {
            if (burstEnable)
                UIB->sck[index].status |= CY_U3P_UIB_AVL_ENABLE;
            else
                UIB->sck[index].status &= ~CY_U3P_UIB_AVL_ENABLE;
        }
    }
    else
    {
        if (burstEnable)
            glUibInSetAvlEn |= 1 << index;
        else
            glUibInSetAvlEn &= ~(1 << index);

        /* If a channel has already been created, update the AVL_EN bit in the SOCK_STATUS register. */
        if (glDmaUIBINSocketCtrl[index].handle != NULL)
        {
            if (burstEnable)
                UIBIN->sck[index].status |= CY_U3P_UIB_AVL_ENABLE;
            else
                UIBIN->sck[index].status &= ~CY_U3P_UIB_AVL_ENABLE;
        }
    }
}

CyBool_t
CyU3PDmaIsSockAvlEnReqd (
        uint16_t sckId)
{
    if (((CyU3PDmaGetIpNum (sckId) == CY_U3P_UIB_IP_BLOCK_ID) && (glUibSetAvlEn & (1 << CyU3PDmaGetSckNum (sckId)))) ||
            ((CyU3PDmaGetIpNum (sckId) == CY_U3P_UIBIN_IP_BLOCK_ID) &&
             (glUibInSetAvlEn & (1 << CyU3PDmaGetSckNum (sckId)))))
        return CyTrue;
    return CyFalse;
}

void
CyU3PDmaDeInit (
        void)
{
    uint8_t i;

    /* TODO: Loop and destroy all channels. */

    CyU3PDmaBufferDeInit ();
    CyU3PDmaDscrListDestroy ();

    /* Clear all the socket control structures */
    for (i = 0; i < CY_U3P_DMA_LPP_NUM_SCK; i++)
    {
        glDmaLPPSocketCtrl[i].handle = NULL;
    }
    for (i = 0; i < CY_U3P_DMA_PIB_NUM_SCK; i++)
    {
        glDmaPIBSocketCtrl[i].handle = NULL;
    }
    for (i = 0; i < CY_U3P_DMA_SIB_NUM_SCK; i++)
    {
        glDmaSIBSocketCtrl[i].handle = NULL;
    }
    for (i = 0; i < CY_U3P_DMA_UIB_NUM_SCK; i++)
    {
        glDmaUIBSocketCtrl[i].handle = NULL;
    }
    for (i = 0; i < CY_U3P_DMA_UIBIN_NUM_SCK; i++)
    {
        glDmaUIBINSocketCtrl[i].handle = NULL;
    }

    return;
}

static void
CyU3PDmaChannelHandleError (
        CyU3PDmaChannel *h)
{
    /* Acquire the lock. */
    CyU3PMutexGet (&(h->lock), CYU3P_WAIT_FOREVER);

    /* Disable the producer socket. */
    if (h->type != CY_U3P_DMA_TYPE_MANUAL_OUT)
    {
        CyU3PDmaSocketDisable (h->prodSckId);
    }
    /* Disable the consumer socket. */
    if (h->type != CY_U3P_DMA_TYPE_MANUAL_IN)
    {
        CyU3PDmaSocketDisable (h->consSckId);
    }

    /* Clear the event flags */
    CyU3PEventSet (&(h->flags), 
            (~(CY_U3P_DMA_CB_XFER_CPLT | CY_U3P_DMA_CB_SEND_CPLT |
               CY_U3P_DMA_CB_RECV_CPLT)),
            CYU3P_EVENT_AND);

    /* Set the error flag. */
    CyU3PEventSet (&(h->flags), CY_U3P_DMA_CB_ERROR, CYU3P_EVENT_OR);
    /* Change the channel state to error. */
    h->state = CY_U3P_DMA_ERROR;

    /* Invoke the callback if notification is registered. */
    if ((h->cb) && (h->notification & CY_U3P_DMA_CB_ERROR))
    {
        h->cb (h, CY_U3P_DMA_CB_ERROR, 0);
    }

    /* Release the lock. */
    CyU3PMutexPut (&(h->lock));
}

static void
CyU3PDmaIntHandler_TypeAuto (
        uint32_t *msg)
{
    uint8_t ipNum, sckNum;
    uint16_t activeIndex;
    uint32_t status;
    CyU3PDmaChannel *h;
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaCBInput_t input;

    h = (CyU3PDmaChannel *)(msg[1]);

    status = msg[2];

    /* Check for the error condition first. */
    if (status & CY_U3P_ERROR)
    {
        CyU3PDmaChannelHandleError (h);
        return;
    }

    /* Acquire lock before accessing channel structure. */
    CyU3PMutexGet (&(h->lock), CYU3P_WAIT_FOREVER);

    if ((msg[0] >> 16) == h->consSckId) /* Consumer socket events */
    {
        ipNum = CyU3PDmaGetIpNum (h->consSckId);
        sckNum = CyU3PDmaGetSckNum (h->consSckId);
        activeIndex = (msg[3] & CY_U3P_DSCR_NUMBER_MASK);

        if (status & CY_U3P_TRANS_DONE)
        {
            /* Enter here only if the channel is not CY_U3P_DMA_PROD_OVERRIDE */
            h->state = CY_U3P_DMA_CONFIGURED;
            /* Send event notification to any waiting threads */
            CyU3PEventSet (&(h->flags), CY_U3P_DMA_CB_XFER_CPLT, CYU3P_EVENT_OR);
            CyU3PSysBarrierSync ();
            if ((h->cb) && (h->notification & CY_U3P_DMA_CB_XFER_CPLT))
            {
                h->cb (h, CY_U3P_DMA_CB_XFER_CPLT, 0);
            }
        }
        else if (status & CY_U3P_CONSUME_EVENT)
        {
            /* TODO: Combine the if conditions if no other checks need to be done. */
            if (h->state == CY_U3P_DMA_PROD_OVERRIDE)
            {
                /* Reconfigure socket to normal mode */
                h->state = CY_U3P_DMA_CONFIGURED;
                glDmaSocket[ipNum][sckNum].intrMask &= (~CY_U3P_CONSUME_EVENT);
                /* Send event notification to any waiting threads */
                CyU3PEventSet (&(h->flags), CY_U3P_DMA_CB_SEND_CPLT, CYU3P_EVENT_OR);
                CyU3PSysBarrierSync ();
                if ((h->cb) && (h->notification & CY_U3P_DMA_CB_SEND_CPLT))
                {
                    h->cb (h, CY_U3P_DMA_CB_SEND_CPLT, 0);
                }
            }
        }
        else
        {
            /* Nothing to be done here. */
        }

        /* Check if the socket was suspended.
         * If so invoke the corresponding callback. */
        if (status & (CY_U3P_SUSPEND | CY_U3P_PARTIAL_BUF))
        {
            CyU3PSysBarrierSync ();
            if ((h->cb) && (h->notification & CY_U3P_DMA_CB_CONS_SUSP))
            {
                h->cb (h, CY_U3P_DMA_CB_CONS_SUSP, 0);
            }
        }
    }
    else /* Producer socket events */
    {
        ipNum = CyU3PDmaGetIpNum (h->prodSckId);
        sckNum = CyU3PDmaGetSckNum (h->prodSckId);
        activeIndex = (msg[3] & CY_U3P_DSCR_NUMBER_MASK);

        if (status & CY_U3P_PRODUCE_EVENT)
        {
            if (h->state != CY_U3P_DMA_CONS_OVERRIDE) /* This is AUTO_SIGNAL mode */
            {
                /* Loop for all valid buffers and get buffer information from
                 * descriptors. Invoke the callback if required. */
                do
                {
                    CyU3PDmaDscrGetConfig (h->currentProdIndex, &dscr);
                    input.buffer_p.buffer = dscr.buffer;
                    input.buffer_p.count = (dscr.size >> CY_U3P_BYTE_COUNT_POS);
                    input.buffer_p.size = (dscr.size & CY_U3P_BUFFER_SIZE_MASK);
                    input.buffer_p.status = (dscr.size & CY_U3P_DMA_BUFFER_STATUS_MASK);
                    /* Move the pointer to the next buffer */
                    h->currentProdIndex = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
                    CyU3PSysBarrierSync ();

                    if (h->isDmaHandleDCache)
                    {
                        /* Since the buffer is received from the H/W, make sure that
                         * the buffer is flushed before returning from this call. */
                        CyU3PSysFlushDRegion ((uint32_t *)input.buffer_p.buffer,
                                input.buffer_p.size);
                    }

                    if ((h->cb) && (h->notification & CY_U3P_DMA_CB_PROD_EVENT))
                    {
                        h->cb (h, CY_U3P_DMA_CB_PROD_EVENT, &input);
                    }
                } while (h->currentProdIndex != activeIndex);

                /* Update the active descriptor information. */
                h->activeProdIndex = activeIndex;
            }
            else /* This is CY_U3P_DMA_CONS_OVERRIDE */
            {
                /* Reconfigure socket to normal mode */
                CyU3PDmaDscrGetConfig (h->overrideDscrIndex, &dscr);
                input.buffer_p.buffer = dscr.buffer;
                input.buffer_p.count = (dscr.size >> CY_U3P_BYTE_COUNT_POS);
                input.buffer_p.size = (dscr.size & CY_U3P_BUFFER_SIZE_MASK);
                input.buffer_p.status = (dscr.size & CY_U3P_DMA_BUFFER_STATUS_MASK);
                h->state = CY_U3P_DMA_CONFIGURED;
                glDmaSocket[ipNum][sckNum].intrMask &= (~CY_U3P_PRODUCE_EVENT);

                if (h->isDmaHandleDCache)
                {
                    /* Since the buffer is received from the H/W, make sure that
                     * the buffer is flushed before returning from this call. */
                    CyU3PSysFlushDRegion ((uint32_t *)input.buffer_p.buffer,
                            input.buffer_p.size);
                }

                /* Send event notification to any waiting threads */
                CyU3PEventSet (&(h->flags), CY_U3P_DMA_CB_RECV_CPLT, CYU3P_EVENT_OR);
                CyU3PSysBarrierSync ();
                if ((h->cb) && (h->notification & CY_U3P_DMA_CB_RECV_CPLT))
                {
                    h->cb (h, CY_U3P_DMA_CB_RECV_CPLT, &input);
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

    /* Release the lock. */
    CyU3PMutexPut (&(h->lock));
}

static void
CyU3PDmaIntHandler_TypeManual (
        uint32_t *msg)
{
    CyU3PDmaChannel *h;
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaSocketConfig_t sck;
    CyU3PDmaCBInput_t input;
    uint32_t status;
    uint32_t activeIndex;
    
    h = (CyU3PDmaChannel *)(msg[1]);
    status = msg[2];

    /* Check for the error condition first. */
    if (status & CY_U3P_ERROR)
    {
        CyU3PDmaChannelHandleError (h);
        return;
    }

    /* Acquire lock before accessing channel structure. */
    CyU3PMutexGet (&(h->lock), CYU3P_WAIT_FOREVER);

    if ((msg[0] >> 16) == h->consSckId) /* Consumer socket events */
    {
        /* Check if this is a valid request. */
        activeIndex = (msg[3] & CY_U3P_DSCR_NUMBER_MASK);

        if (status & CY_U3P_CONSUME_EVENT)
        {
            if (h->state == CY_U3P_DMA_PROD_OVERRIDE)
            {
                /* Configure the socket to normal mode */
                h->state = CY_U3P_DMA_CONFIGURED;
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
            else if (h->type == CY_U3P_DMA_TYPE_MANUAL)
            {
                /* Move pointer to next buffer, update the producer descriptor chain
                 * send notification event to the waiting threads and invoke the callback */
                do
                {
                    CyU3PDmaDscrGetConfig (h->commitProdIndex, &dscr);
                    dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
                    CyU3PDmaDscrSetConfig (h->commitProdIndex, &dscr);
                    CyU3PDmaSocketSendEvent (h->prodSckId, h->commitProdIndex, CyFalse);
                    h->commitProdIndex = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
                    CyU3PDmaDscrGetConfig (h->activeConsIndex, &dscr);
                    h->activeConsIndex = dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK;
                    CyU3PSysBarrierSync ();
                    if ((h->cb) && (h->notification & CY_U3P_DMA_CB_CONS_EVENT))
                    {
                        h->cb (h, CY_U3P_DMA_CB_CONS_EVENT, 0);
                    }
                } while (h->activeConsIndex != activeIndex);
            }
            else /* CY_U3P_DMA_TYPE_MANUAL_OUT */
            {
                /* Check for all valid buffers and invoke the callback */
                do
                {
                    CyU3PDmaDscrGetConfig (h->currentConsIndex, &dscr);
                    if (!(dscr.size & CY_U3P_BUFFER_OCCUPIED))
                    {
                        /* Send event notification to any waiting threads */
                        CyU3PEventSet (&(h->flags), CY_U3P_DMA_CB_CONS_EVENT, CYU3P_EVENT_OR);
                    }
                    CyU3PDmaDscrGetConfig (h->activeConsIndex, &dscr);
                    h->activeConsIndex = dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK;
                    CyU3PSysBarrierSync ();
                    if ((h->cb) && (h->notification & CY_U3P_DMA_CB_CONS_EVENT))
                    {
                        h->cb (h, CY_U3P_DMA_CB_CONS_EVENT, 0);
                    }
                } while (h->activeConsIndex != activeIndex);
            }
        }
        if (status & CY_U3P_STALL)
        {
            /* This interrupt is enabled only for MANUAL channel
             * and only for the consumer socket. This is used to 
             * discard a buffer. */
            CyU3PDmaDscrGetConfig (h->activeConsIndex, &dscr);

            if (dscr.size & CY_U3P_MARKER)
            {
                /* Disable to socket before updating. */
                CyU3PDmaSocketDisable (h->consSckId);

                /* Clear the active descriptor. */
                dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
                CyU3PDmaDscrSetConfig (h->activeConsIndex, &dscr);

                /* Move the active descriptor to the next in chain. */
                h->activeConsIndex = dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK;

                /* Update the current descriptor and re-enable the socket. */
                CyU3PDmaSocketGetConfig (h->consSckId, &sck);
                sck.status &= (~(CY_U3P_GO_SUSPEND | CY_U3P_WRAPUP));
                sck.status |= CY_U3P_GO_ENABLE;
                /* Make sure no interrupts are cleared. */
                sck.intr = 0;

                /* Decrement the discard buffer count. */
                h->discardCount--;

                if (h->discardCount == 0)
                {
                    /* Clear the stall interrupt and
                     * disable the stall interrupt. */
                    sck.intr      =  CY_U3P_STALL;
                    sck.intrMask &= ~CY_U3P_STALL;
                }
                sck.dscrChain = h->activeConsIndex;
                CyU3PDmaSocketSetConfig (h->consSckId, &sck);

                /* Update and send event to the producer socket. */
                CyU3PDmaDscrGetConfig (h->commitProdIndex, &dscr);
                dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
                CyU3PDmaDscrSetConfig (h->commitProdIndex, &dscr);

                /* Since this producer buffer is being discarded, ensure that it is removed from the D-Cache. */
                if (h->isDmaHandleDCache)
                {
                    CyU3PSysFlushDRegion ((uint32_t *)dscr.buffer, dscr.size & CY_U3P_BUFFER_SIZE_MASK);
                }

                h->commitProdIndex = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
                CyU3PDmaSocketSendEvent (h->prodSckId, h->commitProdIndex, CyFalse);
                CyU3PSysBarrierSync ();
            }
        }
        if (status & CY_U3P_TRANS_DONE)
        {
            if (h->type == CY_U3P_DMA_TYPE_MANUAL)
            {
                /* Any future produce event might go unhandled
                 * as these packets cannot be committed. 
                 * So disabling the producer socket. */
                CyU3PDmaSocketDisable (h->prodSckId);
            }

            h->state = CY_U3P_DMA_CONFIGURED;
            /* Send event notification to any waiting threads */
            CyU3PEventSet (&(h->flags), CY_U3P_DMA_CB_XFER_CPLT, CYU3P_EVENT_OR);
            CyU3PSysBarrierSync ();
            if ((h->cb) && (h->notification & CY_U3P_DMA_CB_XFER_CPLT))
            {
                h->cb (h, CY_U3P_DMA_CB_XFER_CPLT, 0);
            }
        }

        /* Check if the socket was suspended.
         * If so invoke the corresponding callback. */
        if (status & (CY_U3P_SUSPEND | CY_U3P_PARTIAL_BUF))
        {
            CyU3PSysBarrierSync ();
            if ((h->cb) && (h->notification & CY_U3P_DMA_CB_CONS_SUSP))
            {
                h->cb (h, CY_U3P_DMA_CB_CONS_SUSP, 0);
            }
        }
    }
    else /* Producer socket events */
    {
        activeIndex = (msg[3] & CY_U3P_DSCR_NUMBER_MASK);

        if (status & CY_U3P_PRODUCE_EVENT)
        {
            /* CY_U3P_DMA_TYPE_MANUAL_IN, CY_U3P_DMA_TYPE_MANUAL */
            if (h->state != CY_U3P_DMA_CONS_OVERRIDE)
            {
                /* Loop for all valid buffers and get buffer information from
                 * descriptors. Invoke the callback and send notification to
                 * waiting threads if required. */
                do 
                {
                    CyU3PDmaDscrGetConfig (h->activeProdIndex, &dscr);
                    input.buffer_p.buffer = dscr.buffer;
                    input.buffer_p.count = (dscr.size >> CY_U3P_BYTE_COUNT_POS);
                    input.buffer_p.size = (dscr.size & CY_U3P_BUFFER_SIZE_MASK);
                    input.buffer_p.status = (dscr.size & CY_U3P_DMA_BUFFER_STATUS_MASK);

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

                    h->activeProdIndex = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
                    CyU3PSysBarrierSync ();
                    if ((h->cb) && (h->notification & CY_U3P_DMA_CB_PROD_EVENT))
                    {
                        h->cb (h, CY_U3P_DMA_CB_PROD_EVENT, &input);
                    }
                } while (h->activeProdIndex != activeIndex);
            }
            else /* CY_U3P_DMA_CONS_OVERRIDE mode */
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

    /* Release the lock. */
    CyU3PMutexPut (&(h->lock));
}

void
CyU3PDmaMultiChannelHandleError (
        CyU3PDmaMultiChannel *h)
{
    uint16_t sckCount;

    /* Acquire the lock. */
    CyU3PMutexGet (&(h->lock), CYU3P_WAIT_FOREVER);

    if ((h->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE) ||
            (h->type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE))
    {
        /* Disable all valid producer sockets. */
        for (sckCount = 0; sckCount < h->validSckCount; sckCount++)
        {
            CyU3PDmaSocketDisable (h->prodSckId[sckCount]);
        }
        /* Disable the consumer socket. */
        CyU3PDmaSocketDisable (h->consSckId[0]);
    }
    else
    {
        /* Disable all valid consumer sockets. */
        for (sckCount = 0; sckCount < h->validSckCount; sckCount++)
        {
            CyU3PDmaSocketDisable (h->consSckId[sckCount]);
        }
        /* Disable the producer socket. */
        CyU3PDmaSocketDisable (h->prodSckId[0]);
    }

    /* Clear the event flags */
    CyU3PEventSet (&(h->flags), 
            (~(CY_U3P_DMA_CB_XFER_CPLT | CY_U3P_DMA_CB_SEND_CPLT | CY_U3P_DMA_CB_RECV_CPLT)),
            CYU3P_EVENT_AND);
    /* Set the error flag. */
    CyU3PEventSet (&(h->flags), CY_U3P_DMA_CB_ERROR, CYU3P_EVENT_OR);

    h->state = CY_U3P_DMA_ERROR;

    /* Invoke the callback if notification is registered. */
    if ((h->cb) && (h->notification & CY_U3P_DMA_CB_ERROR))
    {
        h->cb (h, CY_U3P_DMA_CB_ERROR, 0);
    }

    /* Release the lock. */
    CyU3PMutexPut (&(h->lock));
}

static void
CyU3PDmaIntHandler_TypeManyToOne (
        uint32_t *msg)
{
    CyU3PDmaMultiChannel *h;
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaCBInput_t input;
    CyU3PDmaSocketConfig_t sck;
    uint32_t status;
    uint16_t activeIndex, i, sckId;

    h = (CyU3PDmaMultiChannel *)(msg[1]);
    status = msg[2];

    /* Check for the error condition first. */
    if (status & CY_U3P_ERROR)
    {
        CyU3PDmaMultiChannelHandleError (h);
        return;
    }

    /* Acquire the lock before accessing the channel handle. */
    CyU3PMutexGet (&(h->lock), CYU3P_WAIT_FOREVER);

    sckId = (msg[0] >> 16);

    if (sckId == h->consSckId[0]) /* Consumer socket events */
    {
        activeIndex = (msg[3] & CY_U3P_DSCR_NUMBER_MASK);

        if (status & CY_U3P_CONSUME_EVENT)
        {
            if (h->state != CY_U3P_DMA_PROD_OVERRIDE)
            {
                /* This is a consume event generated during normal transfer.
                 * This event happens only for MANUAL mode. */
                /* Since the producer descriptor information is stored in the write chain
                 * of the consumer descriptor, extract the producer descriptor index and
                 * mark the buffer empty. Then send notification event to the waiting
                 * threads and invoke the callback. Also loop for all valid buffers. */

                do
                {
                    CyU3PDmaDscrGetConfig (h->commitConsIndex, &dscr);
                    h->commitProdIndex = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
                    /* Move the commitConsIndex to the next descriptor. */
                    h->commitConsIndex = (dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
                    /* Get the producer descriptor and mark buffer to be empty. */
                    CyU3PDmaDscrGetConfig (h->commitProdIndex, &dscr);
                    dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
                    CyU3PDmaDscrSetConfig (h->commitProdIndex, &dscr);

                    /* Find the producer socket to send the event. */
                    dscr.sync &= (CY_U3P_PROD_IP_MASK | CY_U3P_PROD_SCK_MASK);
                    dscr.sync >>= CY_U3P_PROD_SCK_POS;
                    CyU3PDmaSocketSendEvent (dscr.sync, h->commitProdIndex, CyFalse);

                    CyU3PSysBarrierSync ();
                    if ((h->cb) && (h->notification & CY_U3P_DMA_CB_CONS_EVENT))
                    {
                        h->cb (h, CY_U3P_DMA_CB_CONS_EVENT, 0);
                    }
                } while (h->commitConsIndex != activeIndex);
            }
            else /* CY_U3P_DMA_PROD_OVERRIDE */
            {
                /* Configure the socket to normal mode */
                h->state = CY_U3P_DMA_CONFIGURED;
                /* Mask the consumer event interrupt for auto channels. */
                if (h->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE)
                {
                    CyU3PDmaSocketGetConfig (h->consSckId[0], &sck);
                    sck.intr = 0;
                    sck.intrMask &= ~CY_U3P_CONSUME_EVENT;
                    CyU3PDmaSocketSetConfig (h->consSckId[0], &sck);
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
        if (status & CY_U3P_STALL)
        {
            /* This interrupt is enabled only for MANUAL channel
             * and only for the consumer socket. This is used to 
             * discard a buffer. */
            CyU3PDmaDscrGetConfig (h->commitConsIndex, &dscr);
            h->commitProdIndex = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);

            if (dscr.size & CY_U3P_MARKER)
            {
                /* Disable to socket before updating. */
                CyU3PDmaSocketDisable (h->consSckId[0]);

                /* Clear the active descriptor. */
                dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
                CyU3PDmaDscrSetConfig (h->commitConsIndex, &dscr);

                /* Move the commit consumer descriptor to the next in chain. */
                h->commitConsIndex = dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK;

                /* Update the current descriptor and re-enable the socket. */
                CyU3PDmaSocketGetConfig (sckId, &sck);
                sck.status &= (~(CY_U3P_GO_SUSPEND | CY_U3P_WRAPUP));
                sck.status |= CY_U3P_GO_ENABLE;
                /* Make sure no interrupts are cleared. */
                sck.intr = 0;

                /* Decrement the discard buffer count. */
                h->discardCount[0]--;

                if (h->discardCount[0] == 0)
                {
                    /* Clear the stall interrupt and
                     * disable the stall interrupt. */
                    sck.intr      =  CY_U3P_STALL;
                    sck.intrMask &= ~CY_U3P_STALL;
                }
                sck.dscrChain = h->commitConsIndex;
                CyU3PDmaSocketSetConfig (sckId, &sck);

                /* Update and send event to the producer socket. */
                CyU3PDmaDscrGetConfig (h->commitProdIndex, &dscr);
                dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
                CyU3PDmaDscrSetConfig (h->commitProdIndex, &dscr);

                /* Since this producer buffer is being discarded, ensure that it is removed from the D-Cache. */
                if (h->isDmaHandleDCache)
                {
                    CyU3PSysFlushDRegion ((uint32_t *)dscr.buffer, dscr.size & CY_U3P_BUFFER_SIZE_MASK);
                }

                /* Find the producer socket to send the event. */
                dscr.sync &= (CY_U3P_PROD_IP_MASK | CY_U3P_PROD_SCK_MASK);
                dscr.sync >>= CY_U3P_PROD_SCK_POS;
                CyU3PDmaSocketSendEvent (dscr.sync, h->commitProdIndex, CyFalse);
                CyU3PSysBarrierSync ();
            }
        }
        if (status & CY_U3P_TRANS_DONE)
        {
            /* Any future produce event might go unhandled
             * as these packets cannot be committed. 
             * So disabling the producer sockets. */
            for (i = 0; i < h->validSckCount; i++)
            {
                CyU3PDmaSocketDisable (h->prodSckId[i]);
            }

            h->state = CY_U3P_DMA_CONFIGURED;
            /* Send event notification to any waiting threads */
            CyU3PEventSet (&(h->flags), CY_U3P_DMA_CB_XFER_CPLT, CYU3P_EVENT_OR);
            CyU3PSysBarrierSync ();
            if ((h->cb) && (h->notification & CY_U3P_DMA_CB_XFER_CPLT))
            {
                h->cb (h, CY_U3P_DMA_CB_XFER_CPLT, 0);
            }
        }
        /* Check if the socket was suspended.
         * If so invoke the corresponding callback. */
        if (status & (CY_U3P_SUSPEND | CY_U3P_PARTIAL_BUF))
        {
            CyU3PSysBarrierSync ();
            if ((h->cb) && (h->notification & CY_U3P_DMA_CB_CONS_SUSP))
            {
                h->cb (h, CY_U3P_DMA_CB_CONS_SUSP, 0);
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
                /* This is a consume event generated during normal transfer.
                 * This event happens only for MANUAL mode. */
                /* Identify the socket. */
                for (i = 0; i < h->validSckCount; i++)
                {
                    if (h->prodSckId[i] == sckId)
                    {
                        break;
                    }
                }
                /* Loop for all valid buffers and send notification event 
                 * to waiting threads and invoke the callback */
                do
                {
                    CyU3PDmaDscrGetConfig (h->activeProdIndex[i], &dscr);
                    input.buffer_p.buffer = dscr.buffer;
                    input.buffer_p.count = (dscr.size >> CY_U3P_BYTE_COUNT_POS);
                    input.buffer_p.size = (dscr.size & CY_U3P_BUFFER_SIZE_MASK);
                    input.buffer_p.status = (dscr.size & CY_U3P_DMA_BUFFER_STATUS_MASK);

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
                    h->activeProdIndex[i] = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
                    CyU3PSysBarrierSync ();
                    if ((h->cb) && (h->notification & CY_U3P_DMA_CB_PROD_EVENT))
                    {
                        h->cb (h, CY_U3P_DMA_CB_PROD_EVENT, &input);
                    }
                } while (h->activeProdIndex[i] != activeIndex);
            }
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
            if (h->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE)
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

    /* Release the lock. */
    CyU3PMutexPut (&(h->lock));
}

static void
CyU3PDmaIntHandler_TypeOneToMany (
        uint32_t *msg)
{
    uint32_t status;
    uint16_t sckId, index, i;
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
        for (i = 0; i < h->validSckCount; i++)
        {
            if (h->consSckId[i] == sckId)
            {
                break;
            }
        }

        if (status & CY_U3P_CONSUME_EVENT)
        {
            if (h->state != CY_U3P_DMA_PROD_OVERRIDE)
            {
                /* This is a consume event generated during normal transfer.
                 * This event happens only for MANUAL mode. */
                /* Use the activeIndex for the socket to identify the producer descriptor
                 * to be freed. Since the producer descriptor information is stored in
                 * the write chain of the consumer descriptor, extract the producer 
                 * descriptor index and mark the buffer empty. Then send notification 
                 * event to the waiting threads and invoke the callback. This should be
                 * done for all valid buffers. */
                do
                {
                    CyU3PDmaDscrGetConfig (h->activeConsIndex[i], &dscr);
                    /* Advance to the next consumer descriptor */
                    h->activeConsIndex[i] = (dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
                    /* Identify the producer descriptor. */
                    index = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
                    CyU3PDmaDscrGetConfig (index, &dscr);
                    dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
                    CyU3PDmaDscrSetConfig (index, &dscr);
                    /* Send the produce event to the producer socket. */
                    CyU3PDmaSocketSendEvent (h->prodSckId[0], index, CyFalse);

                    CyU3PSysBarrierSync ();
                    if ((h->cb) && (h->notification & CY_U3P_DMA_CB_CONS_EVENT))
                    {
                        h->cb (h, CY_U3P_DMA_CB_CONS_EVENT, 0);
                    }
                } while (h->activeConsIndex[i] != activeIndex);

                /* If the producer has signalled all data has been produced,
                 * then check if all buffers has been consumed. If so, then
                 * send the notification and the callback for transfer completion. */
                if (h->state == CY_U3P_DMA_IN_COMPLETION)
                {
                    i = h->count * h->validSckCount;
                    index = h->firstProdIndex[0];
                    while (i != 0)
                    {
                        CyU3PDmaDscrGetConfig (index, &dscr);
                        if (dscr.size & CY_U3P_BUFFER_OCCUPIED)
                        {
                            /* This is the next buffer in the chain to be freed. */
                            break;
                        }
                        /* Advance to the next buffer. */
                        i--;
                        index = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
                    }

                    if (i == 0)
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
        if (status & CY_U3P_STALL)
        {
            /* This interrupt is enabled only for MANUAL channel
             * and only for the consumer socket. This is used to 
             * discard a buffer. */
            CyU3PDmaDscrGetConfig (h->activeConsIndex[i], &dscr);

            if (dscr.size & CY_U3P_MARKER)
            {
                /* Disable to socket before updating. */
                CyU3PDmaSocketDisable (h->consSckId[i]);

                /* Clear the active descriptor. */
                dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
                CyU3PDmaDscrSetConfig (h->activeConsIndex[i], &dscr);

                /* Move the commit consumer descriptor to the next in chain. */
                h->activeConsIndex[i] = dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK;

                /* Update the current descriptor and re-enable the socket. */
                CyU3PDmaSocketGetConfig (h->consSckId[i], &sck);
                sck.status &= (~(CY_U3P_GO_SUSPEND | CY_U3P_WRAPUP));
                sck.status |= CY_U3P_GO_ENABLE;
                /* Make sure no interrupts are cleared. */
                sck.intr = 0;

                /* Decrement the discard buffer count. */
                h->discardCount[i]--;

                if (h->discardCount[i] == 0)
                {
                    /* Clear the stall interrupt and
                     * disable the stall interrupt. */
                    sck.intr      =  CY_U3P_STALL;
                    sck.intrMask &= ~CY_U3P_STALL;
                }
                sck.dscrChain = h->activeConsIndex[i];
                CyU3PDmaSocketSetConfig (h->consSckId[i], &sck);

                /* Identify the producer descriptor. */
                index = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
                /* Update and send event to the producer socket. */
                CyU3PDmaDscrGetConfig (index, &dscr);
                dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
                CyU3PDmaDscrSetConfig (index, &dscr);

                /* Since this producer buffer is being discarded, ensure that it is removed from the D-Cache. */
                if (h->isDmaHandleDCache)
                {
                    CyU3PSysFlushDRegion ((uint32_t *)dscr.buffer, dscr.size & CY_U3P_BUFFER_SIZE_MASK);
                }

                /* Send the produce event to the producer socket. */
                CyU3PDmaSocketSendEvent (h->prodSckId[0], index, CyFalse);
                CyU3PSysBarrierSync ();
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
                /* This is a consume event generated during normal transfer.
                 * This event happens only for MANUAL mode. */
                /* Loop for all valid buffers and send notification event
                 * to waiting threads and invoke the callback */
                do
                {
                    CyU3PDmaDscrGetConfig (h->activeProdIndex[0], &dscr);
                    input.buffer_p.buffer = dscr.buffer;
                    input.buffer_p.count = (dscr.size >> CY_U3P_BYTE_COUNT_POS);
                    input.buffer_p.size = (dscr.size & CY_U3P_BUFFER_SIZE_MASK);
                    input.buffer_p.status = (dscr.size & CY_U3P_DMA_BUFFER_STATUS_MASK);

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
            if (h->type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY)
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
            else
            {
                h->state = CY_U3P_DMA_IN_COMPLETION;
                CyU3PSysBarrierSync ();
            }
        }
    }

    /* Release the lock. */
    CyU3PMutexPut (&(h->lock));
}

void
CyU3PDmaThreadEntry (
        uint32_t input)
{
    uint8_t task;
    uint32_t status;
    uint32_t msg[CY_U3P_DMA_MSG_SIZE];
    CyU3PDmaChannel *h;

    while (1)
    {
        status = CyU3PQueueReceive (&glDmaQueue, msg, CYU3P_WAIT_FOREVER);
        if (status != CY_U3P_SUCCESS)
        {
            continue;
        }

        /* This is the only task supported. */
        task = msg[0] & CY_U3P_MSG_TASK_ID_MASK;
        if (task != CY_U3P_TASK_DMA_INT)
        {
            continue;
        }

        h = (CyU3PDmaChannel *)(msg[1]);
        switch (h->type)
        {
            case CY_U3P_DMA_TYPE_AUTO:
            case CY_U3P_DMA_TYPE_AUTO_SIGNAL:
                CyU3PDmaIntHandler_TypeAuto (msg);
                break;
            case CY_U3P_DMA_TYPE_MANUAL_IN:
            case CY_U3P_DMA_TYPE_MANUAL_OUT:
            case CY_U3P_DMA_TYPE_MANUAL:
                CyU3PDmaIntHandler_TypeManual (msg);
                break;
            case CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE:
            case CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE:
                CyU3PDmaIntHandler_TypeManyToOne (msg);
                break;
            case CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY:
            case CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY:
                CyU3PDmaIntHandler_TypeOneToMany (msg);
                break;
            case CY_U3P_DMA_TYPE_MULTICAST:
                if (glMultiCastIntHandler)
                    glMultiCastIntHandler (msg);
                break;
            default:
                break;
        }

        CyU3PThreadRelinquish ();
    }
}

void
CyU3PDmaApplicationDefine (
        void)
{
    uint32_t *ptr;

    CyU3PDmaInit ();

    CyU3PEventCreate (&glDmaEvent);
    ptr = CyU3PMemAlloc (CY_U3P_DMA_QUEUE_SIZE * CY_U3P_DMA_MSG_SIZE);
    CyU3PQueueCreate (&glDmaQueue, CY_U3P_DMA_MSG_SIZE / 4, ptr, 
            CY_U3P_DMA_QUEUE_SIZE * CY_U3P_DMA_MSG_SIZE);
    ptr = CyU3PMemAlloc (CY_U3P_DMA_THREAD_STACK);
    CyU3PThreadCreate (&glDmaThread, "01_DMA_THREAD", CyU3PDmaThreadEntry, 0, ptr,
            CY_U3P_DMA_THREAD_STACK, CY_U3P_DMA_THREAD_PRIORITY, CY_U3P_DMA_THREAD_PRIORITY, 
            CYU3P_NO_TIME_SLICE, CYU3P_AUTO_START);
}

CyU3PReturnStatus_t
CyU3PDmaMsgSend (
        uint32_t *msg,
        uint32_t wait_option,
        CyBool_t priority)
{
    uint32_t status;

    if (priority == CyTrue)
    {
        status = CyU3PQueuePrioritySend (&glDmaQueue, msg, wait_option);
    }
    else
    {
        status = CyU3PQueueSend (&glDmaQueue, msg, wait_option);
    }

    return status;
}

/* [] */

