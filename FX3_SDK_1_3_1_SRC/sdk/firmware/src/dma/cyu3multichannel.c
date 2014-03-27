/*
 ## Cypress USB 3.0 Platform source file (cyu3multichannel.c)
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

/* This file defines the channel based multi socket DMA model */

#include <cyu3os.h>
#include <cyu3error.h>
#include <cyu3mmu.h>
#include <cyu3socket.h>
#include <cyu3dma.h>
#include <cyu3regs.h>

#define CY_U3P_DMA_API_THREAD_PRIORITY      (2)

typedef CyU3PReturnStatus_t (*CyU3PDmaMultiChannelOpern) (
        CyU3PDmaMultiChannel *chHandle);
typedef void (*CyU3PDmaMultiChannelSetup) (
        CyU3PDmaMultiChannel *chHandle, uint32_t cnt, uint16_t offset);

CyU3PDmaMultiChannelOpern glMultiChannelConfigure = 0;
CyU3PDmaMultiChannelOpern glMultiChannelDestroy   = 0;
CyU3PDmaMultiChannelOpern glMultiChannelReset     = 0;
CyU3PDmaMultiChannelSetup glMultiChannelSetXfer   = 0;

void
CyU3PDmaSetMulticastHandlers (
        CyU3PDmaMultiChannelOpern configure_p,
        CyU3PDmaMultiChannelOpern destroy_p,
        CyU3PDmaMultiChannelOpern reset_p,
        CyU3PDmaMultiChannelSetup setxfer_p)
{
    glMultiChannelConfigure = configure_p;
    glMultiChannelDestroy   = destroy_p;
    glMultiChannelReset     = reset_p;
    glMultiChannelSetXfer   = setxfer_p;
}

CyU3PDmaMultiChannel *
CyU3PDmaMultiChannelGetHandle (
        CyU3PDmaSocketId_t sckId)
{
    uint8_t ipNum, sckNum;

    ipNum = CyU3PDmaGetIpNum (sckId);
    sckNum = CyU3PDmaGetSckNum (sckId);

    /* Verify that this is a valid socket. */
    if (!CyU3PDmaSocketIsValid (sckId))
    {
        return NULL;
    }

    return glDmaSocketCtrl[ipNum][sckNum].multiHandle;
}

CyU3PReturnStatus_t
CyU3PDmaMultiChannelAcquireLock (
        CyU3PDmaMultiChannel *handle,
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
CyU3PDmaMultiConfigureSockets_TypeManyToOne (
        CyU3PDmaMultiChannel *handle)
{
    uint16_t sckCount;
    CyU3PDmaSocketConfig_t sck;

    /* Configure the producer sockets */
    for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
    {
        handle->activeProdIndex[sckCount] = handle->firstProdIndex[sckCount];
        CyU3PDmaSocketDisable (handle->prodSckId[sckCount]);
        CyU3PDmaSocketGetConfig (handle->prodSckId[sckCount], &sck);
        sck.dscrChain = handle->firstProdIndex[sckCount];
        sck.xferSize = 0;
        sck.xferCount = 0;
        sck.status = (CY_U3P_TRUNCATE | CY_U3P_EN_PROD_EVENTS | CY_U3P_SUSP_TRANS);
        if (CyU3PDmaIsSockAvlEnReqd (handle->prodSckId[sckCount]))
            sck.status |= CY_U3P_UIB_AVL_ENABLE;

        sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
        
        if (handle->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE)
        {
            sck.intrMask = CY_U3P_ERROR;
        }
        else
        {
            sck.intrMask = (CY_U3P_ERROR | CY_U3P_PRODUCE_EVENT);
        }

        CyU3PDmaSocketSetConfig (handle->prodSckId[sckCount], &sck);
    }

    /* Configure the consumer socket */
    handle->activeConsIndex[0] = handle->firstConsIndex[0];
    CyU3PDmaSocketDisable (handle->consSckId[0]);
    CyU3PDmaSocketGetConfig (handle->consSckId[0], &sck);
    sck.dscrChain = handle->firstConsIndex[0];
    sck.xferSize = 0;
    sck.xferCount = 0;
    sck.status = (CY_U3P_TRUNCATE | CY_U3P_EN_CONS_EVENTS | CY_U3P_SUSP_TRANS);
    if (CyU3PDmaIsSockAvlEnReqd (handle->consSckId[0]))
        sck.status |= CY_U3P_UIB_AVL_ENABLE;

    sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));
    if (handle->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE)
    {
        sck.intrMask = (CY_U3P_TRANS_DONE | CY_U3P_ERROR);
    }
    else
    {
        sck.intrMask = (CY_U3P_CONSUME_EVENT | CY_U3P_TRANS_DONE | CY_U3P_ERROR);
    }

    CyU3PDmaSocketSetConfig (handle->consSckId[0], &sck);

    /* Reset the discard counter. */
    handle->discardCount[0] = 0;
}

static CyU3PReturnStatus_t
CyU3PDmaMultiChannelConfigure_TypeManyToOne (
        CyU3PDmaMultiChannel *handle)
{
    CyU3PDmaDescriptor_t prodDscr, consDscr;
    uint32_t status, dscrSync;
    uint16_t count, index, sckCount;

    /* Mark the producer socket as CPU. This shall be updated after the producer chain is created. */
    dscrSync = (handle->consSckId[0] | CY_U3P_EN_CONS_EVENT | CY_U3P_EN_CONS_INT);
    dscrSync  |= (((CY_U3P_CPU_SOCKET_PROD << CY_U3P_PROD_SCK_POS) |
                    CY_U3P_EN_PROD_EVENT | CY_U3P_EN_PROD_INT));

    /* Create the descriptor chain. There is only one chain for AUTO mode.
     * Consumer chain will have all the buffers sequentially linked
     * and the producers shall take the interleaved buffers. So the write chain
     * shall be split into as many producers are present and there shall be
     * a single read chain.
     * For manual mode of operation, there shall be one consumer descriptor chain
     * and validSckCount number of producer descriptor chains.
     */
    status = CyU3PDmaDscrChainCreate (&(handle->firstConsIndex[0]),
            handle->count * handle->validSckCount, handle->size, dscrSync);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    if (handle->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE)
    {
        /* Modify the write chains for interleaved operation. In the first iteration,
         * update the pointers. In the next set of iterations modify the producer chain.
         * Run the loop for handle->count + 1 times to update all nodes in producer chain.
         * The firstProdIndex variable is used as a scratch variable to traverse the producer chain. */
        for (count = 0, index = handle->firstConsIndex[0]; count <= handle->count; count++)
        {
            for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
            {
                /* Get the consumer descriptor. */
                CyU3PDmaDscrGetConfig (index, &consDscr);
                if (count == 0)
                {
                    /* Update the producer chain for pointers. */
                    handle->firstProdIndex[sckCount] = index;
                }
                else
                {
                    /* Modify the producer chains to interleaved operation. */
                    CyU3PDmaDscrGetConfig (handle->firstProdIndex[sckCount], &prodDscr);
                    prodDscr.sync &= (~(CY_U3P_PROD_IP_MASK | CY_U3P_PROD_SCK_MASK));
                    prodDscr.sync |= (((handle->prodSckId[sckCount] << CY_U3P_PROD_SCK_POS) |
                                CY_U3P_EN_PROD_EVENT | CY_U3P_EN_PROD_INT));
                    prodDscr.chain &= CY_U3P_RD_NEXT_DSCR_MASK;
                    prodDscr.chain |= (index << CY_U3P_WR_NEXT_DSCR_POS);
                    CyU3PDmaDscrSetConfig (handle->firstProdIndex[sckCount], &prodDscr);
                    /* Last interation will update the variable to the producer chain first node. */
                    handle->firstProdIndex[sckCount] = index;
                }
                /* Move to the next buffer. */
                index = (consDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
            }
        }
    }
    else /* CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE */
    {
        /* Modify the write chains for interleaved operation. In the first iteration,
         * create the producer chain. Update the producer and cosumer buffer for offsets.
         * Run the loop for handle->count times to update all nodes in producer chain.
         * The firstProdIndex variable is used as a scratch variable to traverse the producer chain. */
        for (count = 0, index = handle->firstConsIndex[0]; count < handle->count; count++)
        {
            for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
            {
                if (count == 0)
                {
                    /* Create the producer chain but do not allocate buffers. Update the dscrSync value 
                     * for the manual mode producer chain. */
                    dscrSync  = (CY_U3P_CPU_SOCKET_CONS | CY_U3P_EN_CONS_EVENT | CY_U3P_EN_CONS_INT);
                    dscrSync |= (((handle->prodSckId[sckCount] << CY_U3P_PROD_SCK_POS) |
                                CY_U3P_EN_PROD_EVENT | CY_U3P_EN_PROD_INT));

                    CyU3PDmaDscrChainCreate (&(handle->firstProdIndex[sckCount]), handle->count, 0, dscrSync);
                }

                /* Update the buffers for the producer chains. */
                CyU3PDmaDscrGetConfig (handle->firstProdIndex[sckCount], &prodDscr);
                CyU3PDmaDscrGetConfig (index, &consDscr);
                prodDscr.buffer = consDscr.buffer + handle->prodHeader;
                prodDscr.size = (consDscr.size - handle->prodHeader - handle->prodFooter);
                CyU3PDmaDscrSetConfig (handle->firstProdIndex[sckCount], &prodDscr);

                /* Also update the buffer for consumer chain with consHeader offset. */
                consDscr.buffer += handle->consHeader;
                /* The magic number 0xF is present to make the size multiple of 16 bytes */
                /* Since the consumer socket can actually transmit data of sizes not aligned to
                 * 16 bytes unlike the producer socket, the create call does not mandate
                 * the caller to create buffers of sizes to match. But the descriptor takes only
                 * buffer sizes as multiples of 16 bytes. So the size needs to be calculated
                 * for descriptor update. But transfer can only be done to the actual buffer size.
                 * This count is only for the hardware information. */
                consDscr.size = ((handle->size + 0xF - handle->consHeader) &
                        CY_U3P_BUFFER_SIZE_MASK);
                /* Build up an index in the consumer chain, so that when a consume event
                 * is received, the producer descriptor can be located. The producer descriptor
                 * for the buffer shall be stored in the write chain portion of the consumer descriptor. */
                consDscr.chain &= CY_U3P_RD_NEXT_DSCR_MASK;
                consDscr.chain |= (handle->firstProdIndex[sckCount] << CY_U3P_WR_NEXT_DSCR_POS);
                CyU3PDmaDscrSetConfig (index, &consDscr);

                /* Move to the next buffer. */
                index = (consDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
                /* Last interation will update the variable to the producer chain first node. */
                handle->firstProdIndex[sckCount] = (prodDscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
            }
        }
    }

    /* Configure the sockets. */
    CyU3PDmaMultiConfigureSockets_TypeManyToOne (handle);

    /* Update the reference pointers. */
    handle->currentProdIndex = handle->commitProdIndex = handle->firstProdIndex[0];
    handle->currentConsIndex = handle->commitConsIndex = handle->firstConsIndex[0];

    /* Allocate an override descriptor */
    CyU3PDmaDscrGet (&(handle->overrideDscrIndex));

    return CY_U3P_SUCCESS;
}

static void
CyU3PDmaMultiConfigureSockets_TypeOneToMany (
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

    if (handle->type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY)
    {
        sck.intrMask = (CY_U3P_TRANS_DONE | CY_U3P_ERROR);
    }
    else /* CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY */
    {
        sck.intrMask = (CY_U3P_TRANS_DONE | CY_U3P_ERROR | 
                CY_U3P_PRODUCE_EVENT);
    }
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
        if (handle->type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY)
        {
            sck.intrMask = (CY_U3P_ERROR);
        }
        else /* CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY */
        {
            sck.intrMask = (CY_U3P_CONSUME_EVENT | CY_U3P_ERROR);
        }

        CyU3PDmaSocketSetConfig (handle->consSckId[sckCount], &sck);

        /* Reset the discard counter. */
        handle->discardCount[sckCount] = 0;
    }
}

static CyU3PReturnStatus_t
CyU3PDmaMultiChannelConfigure_TypeOneToMany (
        CyU3PDmaMultiChannel *handle)
{
    CyU3PDmaDescriptor_t prodDscr, consDscr;
    uint32_t status, dscrSync;
    uint16_t count, index, sckCount;

    /* Mark the consumer socket as CPU. This shall be updated after the consumer chain is created. */
    dscrSync = (CY_U3P_CPU_SOCKET_CONS | CY_U3P_EN_CONS_EVENT | CY_U3P_EN_CONS_INT);
    dscrSync  |= (((handle->prodSckId[0] << CY_U3P_PROD_SCK_POS) |
                    CY_U3P_EN_PROD_EVENT | CY_U3P_EN_PROD_INT));

    /* Create the descriptor chain. There is only one chain for AUTO mode.
     * Producer chain will have all the buffers sequentially linked
     * and the consumers shall take the interleaved buffers. So the read chain
     * shall be split into as many consumers are present and there shall be
     * a single write chain.
     * For manual mode of operation, there shall be one producer descriptor chain
     * and validSckCount number of consumer descriptor chains.
     */
    status = CyU3PDmaDscrChainCreate (&(handle->firstProdIndex[0]),
            handle->count * handle->validSckCount, handle->size, dscrSync);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    if (handle->type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY)
    {
        /* Modify the read chains for interleaved operation. In the first iteration,
         * update the pointers. In the next set of iterations modify the consumer chain.
         * Run the loop for handle->count + 1 times to update all nodes in consumer chain.
         * The firstConsIndex variable is used as a scratch variable to traverse the chain. */
        for (count = 0, index = handle->firstProdIndex[0]; count <= handle->count; count++)
        {
            for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
            {
                /* Get the producer descriptor. */
                CyU3PDmaDscrGetConfig (index, &prodDscr);
                if (count == 0)
                {
                    /* Update the consumer chain for pointers. */
                    handle->firstConsIndex[sckCount] = index;
                }
                else
                {
                    /* Modify the consumer chains to interleaved operation. */
                    CyU3PDmaDscrGetConfig (handle->firstConsIndex[sckCount], &consDscr);
                    consDscr.sync &= (~(CY_U3P_CONS_IP_MASK | CY_U3P_CONS_SCK_MASK));
                    consDscr.sync |= (((handle->consSckId[sckCount] << CY_U3P_CONS_SCK_POS) |
                                CY_U3P_EN_CONS_EVENT | CY_U3P_EN_CONS_INT));
                    consDscr.chain &= CY_U3P_WR_NEXT_DSCR_MASK;
                    consDscr.chain |= (index << CY_U3P_RD_NEXT_DSCR_POS);
                    CyU3PDmaDscrSetConfig (handle->firstConsIndex[sckCount], &consDscr);
                    /* Last interation will update the variable to the consumer chain first node. */
                    handle->firstConsIndex[sckCount] = index;
                }
                /* Move to the next buffer. */
                index = (prodDscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
            }
        }
    }
    else /* CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY */
    {
        /* Modify the read chains for interleaved operation. In the first iteration,
         * create the consumer chain. Update the producer and cosumer buffer for offsets.
         * Run the loop for handle->count times to update all nodes in consumer chain.
         * The firstConsIndex variable is used as a scratch variable to traverse the chain. */
        for (count = 0, index = handle->firstProdIndex[0]; count < handle->count; count++)
        {
            for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
            {
                if (count == 0)
                {
                    /* Create the consumer chain but do not allocate buffers. Update the dscrSync value 
                     * for the manual mode consumer chain. */
                    dscrSync  = (handle->consSckId[sckCount] | CY_U3P_EN_CONS_EVENT | CY_U3P_EN_CONS_INT);
                    dscrSync |= (((CY_U3P_CPU_SOCKET_PROD << CY_U3P_PROD_SCK_POS) |
                                CY_U3P_EN_PROD_EVENT | CY_U3P_EN_PROD_INT));

                    CyU3PDmaDscrChainCreate (&(handle->firstConsIndex[sckCount]), handle->count, 0, dscrSync);
                }

                /* Update the buffers for the consumer chains. */
                CyU3PDmaDscrGetConfig (handle->firstConsIndex[sckCount], &consDscr);
                CyU3PDmaDscrGetConfig (index, &prodDscr);
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
                /* Build up an index in the consumer chain, so that when a consume event
                 * is received, the producer descriptor can be located. The producer descriptor
                 * for the buffer shall be stored in the write chain portion of the consumer descriptor. */
                consDscr.chain &= CY_U3P_RD_NEXT_DSCR_MASK;
                consDscr.chain |= (index << CY_U3P_WR_NEXT_DSCR_POS);
                CyU3PDmaDscrSetConfig (handle->firstConsIndex[sckCount], &consDscr);

                /* Also update the buffer for producer chain with offsets. */
                prodDscr.buffer += handle->prodHeader;
                prodDscr.size = (handle->size - handle->prodHeader - handle->prodFooter);
                /* Update the consumer chain information on the producer descriptors. */
                prodDscr.chain &= CY_U3P_WR_NEXT_DSCR_MASK;
                prodDscr.chain |= handle->firstConsIndex[sckCount];
                CyU3PDmaDscrSetConfig (index, &prodDscr);

                /* Move to the next buffer. */
                index = (prodDscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
                /* Last interation will update the variable to the consumer chain first node. */
                handle->firstConsIndex[sckCount] = (consDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
            }
        }
    }

    /* Update the reference pointers. */
    handle->currentProdIndex = handle->commitProdIndex = handle->firstProdIndex[0];
    handle->currentConsIndex = handle->commitConsIndex = handle->firstConsIndex[0];

    /* Allocate an override descriptor */
    CyU3PDmaDscrGet (&(handle->overrideDscrIndex));

    /* Configure the sockets. */
    CyU3PDmaMultiConfigureSockets_TypeOneToMany (handle);

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaMultiChannelCreate (
        CyU3PDmaMultiChannel *handle,
        CyU3PDmaMultiType_t type,
        CyU3PDmaMultiChannelConfig_t *config)
{
    uint8_t sckNum, ipNum;
    uint8_t index, tmp;
    uint32_t status;

    /* Do basic error checking before initializing channel */
    if (config == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if (handle == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if (config->count == 0)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((config->validSckCount < CY_U3P_DMA_MIN_MULTI_SCK_COUNT) ||
            (config->validSckCount > CY_U3P_DMA_MAX_MULTI_SCK_COUNT))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY) ||
            (type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE))
    {
        if (CyU3PDmaDscrGetFreeCount () <=
                (config->count * config->validSckCount))
        {
            return CY_U3P_ERROR_MEMORY_ERROR;
        }
    }
    else if ((type == CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY) ||
            (type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE))
    {
        /* Two times the number of buffers for manual channels. */
        if (CyU3PDmaDscrGetFreeCount () <=
                ((config->count * config->validSckCount) << 1))
        {
            return CY_U3P_ERROR_MEMORY_ERROR;
        }
    }
    else /* CY_U3P_DMA_TYPE_MULTICAST */
    {
        /* Three times the number of buffers. */
        if (CyU3PDmaDscrGetFreeCount () <=
                (config->count * (config->validSckCount + 1)))
        {
            return CY_U3P_ERROR_MEMORY_ERROR;
        }
    }

    if ((config->size == 0) || (config->size > CY_U3P_DMA_MAX_BUFFER_SIZE))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE) ||
            (type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY))
    {
        if (config->size & (~CY_U3P_BUFFER_SIZE_MASK))
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
    if ((type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE) ||
            (type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE))
    {
        if (config->prodAvailCount != 0)
        {
            if ((config->prodAvailCount >= config->count) ||
                    (config->count > CY_U3P_DMA_MAX_AVAIL_COUNT))
            {
                return CY_U3P_ERROR_BAD_ARGUMENT;
            }
        }

        /* Check producer sockets */
        for (index = 0; index < config->validSckCount; index++)
        {
            ipNum = CyU3PDmaGetIpNum (config->prodSckId[index]);
            sckNum = CyU3PDmaGetSckNum (config->prodSckId[index]);

            /* Verify that this is a valid socket. */
            if (!CyU3PDmaSocketIsValidProducer (config->prodSckId[index]))
            {
                return CY_U3P_ERROR_BAD_ARGUMENT;
            }
            if (glDmaSocketCtrl[ipNum][sckNum].multiHandle != NULL)
            {
                return CY_U3P_ERROR_BAD_ARGUMENT;
            }
            /* Verify that the sockets are not the same. */
            for (tmp = (index + 1); tmp < config->validSckCount; tmp++)
            {
                if (config->prodSckId[index] == config->prodSckId[tmp])
                {
                    return CY_U3P_ERROR_BAD_ARGUMENT;
                }
            }
        }

        /* Check consumer socket */
        ipNum = CyU3PDmaGetIpNum (config->consSckId[0]);
        sckNum = CyU3PDmaGetSckNum (config->consSckId[0]);
        /* Verify that this is a valid socket. */
        if (!CyU3PDmaSocketIsValidConsumer (config->consSckId[0]))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
        if (glDmaSocketCtrl[ipNum][sckNum].multiHandle != NULL)
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }

        /* Update socket control structures so that the sockets 
         * are locked. */
        glDmaSocketCtrl[ipNum][sckNum].multiHandle = handle;
        for (index = 0; index < config->validSckCount; index++)
        {
            ipNum = CyU3PDmaGetIpNum (config->prodSckId[index]);
            sckNum = CyU3PDmaGetSckNum (config->prodSckId[index]);
            glDmaSocketCtrl[ipNum][sckNum].multiHandle = handle;
        }
    }
    else /* ONE_TO_MANY or MULTICAST */
    {
        /* Avail count feature is not supported on a one to many
         * channel. */
        if (config->prodAvailCount != 0)
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
        /* Check consumer sockets */
        for (index = 0; index < config->validSckCount; index++)
        {
            ipNum = CyU3PDmaGetIpNum (config->consSckId[index]);
            sckNum = CyU3PDmaGetSckNum (config->consSckId[index]);
            /* Verify that this is a valid socket. */
            if (!CyU3PDmaSocketIsValidConsumer (config->consSckId[index]))
            {
                return CY_U3P_ERROR_BAD_ARGUMENT;
            }
            if (glDmaSocketCtrl[ipNum][sckNum].multiHandle != NULL)
            {
                return CY_U3P_ERROR_BAD_ARGUMENT;
            }
            /* Verify that the sockets are not the same. */
            for (tmp = (index + 1); tmp < config->validSckCount; tmp++)
            {
                if (config->consSckId[index] == config->consSckId[tmp])
                {
                    return CY_U3P_ERROR_BAD_ARGUMENT;
                }
            }
        }

        /* Check producer socket */
        ipNum = CyU3PDmaGetIpNum (config->prodSckId[0]);
        sckNum = CyU3PDmaGetSckNum (config->prodSckId[0]);
        /* Verify that this is a valid socket. */
        if (!CyU3PDmaSocketIsValidProducer (config->prodSckId[0]))
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }
        if (glDmaSocketCtrl[ipNum][sckNum].multiHandle != NULL)
        {
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }

        /* Update socket control structures so that the sockets 
         * are locked. */
        glDmaSocketCtrl[ipNum][sckNum].multiHandle = handle;
        for (index = 0; index < config->validSckCount; index++)
        {
            ipNum = CyU3PDmaGetIpNum (config->consSckId[index]);
            sckNum = CyU3PDmaGetSckNum (config->consSckId[index]);
            glDmaSocketCtrl[ipNum][sckNum].multiHandle = handle;
        }
    }

    /* Load and initialize the configuration information */
    handle->type = type;
    handle->size = config->size;
    handle->count = config->count;
    handle->prodAvailCount = config->prodAvailCount;
    handle->validSckCount = config->validSckCount;

    /* Copy out all socket information. */
    for (index = 0; index < CY_U3P_DMA_MAX_MULTI_SCK_COUNT; index++)
    {
        handle->prodSckId[index] = config->prodSckId[index];
        handle->consSckId[index] = config->consSckId[index];
	handle->bufferCount[index] = 0;
	handle->discardCount[index] = 0;
    }

    handle->prodHeader = config->prodHeader;
    handle->prodFooter = config->prodFooter;
    handle->consHeader = config->consHeader;
    handle->dmaMode    = config->dmaMode;
    handle->notification = config->notification;
    CyU3PMutexCreate (&(handle->lock), CYU3P_NO_INHERIT);
    CyU3PEventCreate (&(handle->flags));
    handle->cb = config->cb;
    handle->isDmaHandleDCache = glDmaHandleDCache;

    switch (type)
    {
        case CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE:
        case CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE:
            status = CyU3PDmaMultiChannelConfigure_TypeManyToOne (handle);
            break;
        case CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY:
        case CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY:
            status = CyU3PDmaMultiChannelConfigure_TypeOneToMany (handle);
            break;
        case CY_U3P_DMA_TYPE_MULTICAST:
            if (glMultiChannelConfigure == 0)
                status = CY_U3P_ERROR_INVALID_SEQUENCE;
            else
                status = glMultiChannelConfigure (handle);
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
        /* Update socket control structures. */
        if ((type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE) ||
                (type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE))
        {
            ipNum = CyU3PDmaGetIpNum (config->consSckId[0]);
            sckNum = CyU3PDmaGetSckNum (config->consSckId[0]);
            glDmaSocketCtrl[ipNum][sckNum].multiHandle = NULL;
            for (index = 0; index < config->validSckCount; index++)
            {
                ipNum = CyU3PDmaGetIpNum (config->prodSckId[index]);
                sckNum = CyU3PDmaGetSckNum (config->prodSckId[index]);
                glDmaSocketCtrl[ipNum][sckNum].multiHandle = NULL;
            }
        }
        else /* ONE_TO_MANY or MULTICAST */
        {
            ipNum = CyU3PDmaGetIpNum (config->prodSckId[0]);
            sckNum = CyU3PDmaGetSckNum (config->prodSckId[0]);
            glDmaSocketCtrl[ipNum][sckNum].multiHandle = NULL;
            for (index = 0; index < config->validSckCount; index++)
            {
                ipNum = CyU3PDmaGetIpNum (config->consSckId[index]);
                sckNum = CyU3PDmaGetSckNum (config->consSckId[index]);
                glDmaSocketCtrl[ipNum][sckNum].multiHandle = NULL;
            }
        }  
    }

    return status;
}

static void
CyU3PDmaMultiChannelDestroy_TypeManyToOne (
        CyU3PDmaMultiChannel *handle)
{
    CyU3PDmaSocketConfig_t sck;
    CyU3PDmaDescriptor_t dscr;
    uint8_t sckNum, ipNum;
    uint16_t count, sckCount, index;

    /* Disable and configure the producer sockets */
    for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
    {
        CyU3PDmaSocketDisable (handle->prodSckId[sckCount]);
        CyU3PDmaSocketGetConfig (handle->prodSckId[sckCount], &sck);
        sck.dscrChain = CY_U3P_RD_NEXT_DSCR_MASK;
        CyU3PDmaSocketSetConfig (handle->prodSckId[sckCount], &sck);
        sckNum = CyU3PDmaGetSckNum (handle->prodSckId[sckCount]);
        ipNum  = CyU3PDmaGetIpNum (handle->prodSckId[sckCount]);
        glDmaSocketCtrl[ipNum][sckNum].handle = NULL;
    }

    /* Disable and configure the consumer socket. */
    CyU3PDmaSocketDisable (handle->consSckId[0]);
    CyU3PDmaSocketGetConfig (handle->consSckId[0], &sck);
    sck.dscrChain = CY_U3P_RD_NEXT_DSCR_MASK;
    CyU3PDmaSocketSetConfig (handle->consSckId[0], &sck);
    sckNum = CyU3PDmaGetSckNum (handle->consSckId[0]);
    ipNum  = CyU3PDmaGetIpNum (handle->consSckId[0]);
    glDmaSocketCtrl[ipNum][sckNum].handle = NULL;

    /* Free the descriptor chain */
    if (handle->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE)
    {
        CyU3PDmaDscrChainDestroy (handle->firstConsIndex[0],
                (handle->count * handle->validSckCount), CyFalse, CyTrue);
    }
    else /* CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE */
    {
        /* Since the buffer offsets has been added,
         * the buffers has to be freed manually. */
        index = handle->firstConsIndex[0];
        for (count = 0; count < (handle->count * handle->validSckCount); count++)
        {
            CyU3PDmaDscrGetConfig (index, &dscr);
            CyU3PDmaBufferFree (dscr.buffer - handle->consHeader);
            CyU3PDmaDscrPut (index);
            index = (dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
        }

        /* Now free the producer chains. */
        for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
        {
            CyU3PDmaDscrChainDestroy (handle->firstProdIndex[sckCount], handle->count,
                    CyTrue, CyFalse);
        }
    }

    /* Free the override descriptor */
    CyU3PDmaDscrPut (handle->overrideDscrIndex);

    return;
}

static void
CyU3PDmaMultiChannelDestroy_TypeOneToMany (
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

    /* Free the descriptor chain */
    if (handle->type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY)
    {
        CyU3PDmaDscrChainDestroy (handle->firstProdIndex[0],
                (handle->count * handle->validSckCount), CyTrue, CyTrue);
    }
    else /* CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY */
    {
        /* Since the buffer offsets has been added,
         * the buffers has to be freed manually. */
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
    }

    /* Free the override descriptor */
    CyU3PDmaDscrPut (handle->overrideDscrIndex);

    return;
}

CyU3PReturnStatus_t
CyU3PDmaMultiChannelDestroy (
        CyU3PDmaMultiChannel *handle)
{
    uint32_t status;

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    switch (handle->type)
    {
        case CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE:
        case CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE:
            CyU3PDmaMultiChannelDestroy_TypeManyToOne (handle);
            break;
        case CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY:
        case CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY:
            CyU3PDmaMultiChannelDestroy_TypeOneToMany (handle);
            break;
        case CY_U3P_DMA_TYPE_MULTICAST:
            if (glMultiChannelDestroy == 0)
                return CY_U3P_ERROR_INVALID_SEQUENCE;
            else
                glMultiChannelDestroy (handle);
            break;
        default:
            /* This is not a valid channel. Just release the
             * mutex and retunrn. */
            CyU3PMutexPut (&handle->lock);
            return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Destroy the locks */
    handle->state = CY_U3P_DMA_NOT_CONFIGURED;
    CyU3PMutexDestroy (&(handle->lock));
    CyU3PEventDestroy (&(handle->flags));

    return CY_U3P_SUCCESS;
}

static void
CyU3PDmaMultiChannelSetXfer_TypeManyToOne (
        CyU3PDmaMultiChannel *handle,
        uint32_t count,
        uint16_t multiSckOffset)
{
    CyU3PDmaSocketConfig_t sck;
    CyU3PDmaDescriptor_t dscr;
    uint16_t sckCount, index;

    /* Update the status information first. */
    handle->state = CY_U3P_DMA_ACTIVE;

    /* Disable the sockets */
    for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
    {
        CyU3PDmaSocketDisable (handle->prodSckId[sckCount]);
    }
    CyU3PDmaSocketDisable (handle->consSckId[0]);

    /* Set the consumer socket suspend option. */
    CyU3PDmaUpdateSocketSuspendOption (handle->consSckId[0],
            handle->consSusp);

    /* Update the producer reference pointers. */
    handle->currentProdIndex = handle->commitProdIndex = 
        handle->firstProdIndex[multiSckOffset];

    /* Configure the producer sockets */
    for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
    {
        CyU3PDmaSocketGetConfig (handle->prodSckId[sckCount], &sck);
        
        /* Since the transfer size specified is for the consumer socket,
         * the transfer count for the producer sockets shall be maintained
         * as infinite. */
        sck.xferSize = 0;
        sck.xferCount = 0;
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
        if (handle->prodAvailCount != 0)
        {
            /* Update number of free buffers and the min avail count. */
            sck.status |= (CY_U3P_AVL_ENABLE | (handle->count) | 
                    (handle->prodAvailCount << CY_U3P_AVL_MIN_POS));
        }
        sck.intr = (~(CY_U3P_SCK_INTR_DEFAULT));

        /* Update the socket descriptor chain to the beginning. */
        if (sckCount >= multiSckOffset)
        {
            sck.dscrChain = handle->firstProdIndex[sckCount];
        }
        else
        {
            CyU3PDmaDscrGetConfig (handle->firstProdIndex[sckCount], &dscr);
            sck.dscrChain = dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS;
        }

        handle->activeProdIndex[sckCount] = sck.dscrChain;
        CyU3PDmaSocketSetConfig (handle->prodSckId[sckCount], &sck);
    }

    /* Configure the consumer socket */
    CyU3PDmaSocketGetConfig (handle->consSckId[0], &sck);
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

    /* Update the socket descriptor chain to the offset specified. */
    index = handle->firstConsIndex[0];
    for (sckCount = 0; sckCount < multiSckOffset; sckCount++)
    {
        CyU3PDmaDscrGetConfig (index, &dscr);
        index = dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK;
    }
    sck.dscrChain = index;

    /* Update the consumer reference pointers. */
    handle->currentConsIndex = handle->commitConsIndex = 
        handle->activeConsIndex[0] = index;

    /* Configure the consumer socket. */
    CyU3PDmaSocketSetConfig (handle->consSckId[0], &sck);

}

static void
CyU3PDmaMultiChannelSetXfer_TypeOneToMany (
        CyU3PDmaMultiChannel *handle,
        uint32_t count,
        uint16_t multiSckOffset)
{
    CyU3PDmaSocketConfig_t sck;
    CyU3PDmaDescriptor_t dscr;
    uint16_t sckCount, index;

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
    /* Update the socket descriptor chain to the offset specified. */
    index = handle->firstProdIndex[0];
    for (sckCount = 0; sckCount < multiSckOffset; sckCount++)
    {
        CyU3PDmaDscrGetConfig (index, &dscr);
        index = dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS;
    }
    sck.dscrChain = index;
    /* Update the producer reference pointers. */
    handle->currentProdIndex = handle->commitProdIndex =
        handle->activeProdIndex[0] = index;

    CyU3PDmaSocketSetConfig (handle->prodSckId[0], &sck);

    /* Update the consumer reference pointers. */
    handle->currentConsIndex = handle->commitConsIndex = 
        handle->firstConsIndex[multiSckOffset];

    /* Configure the consumer socket */
    for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
    {
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

        /* Update the socket descriptor chain to the beginning. */
        if (sckCount >= multiSckOffset)
        {
            sck.dscrChain = handle->firstConsIndex[sckCount];
        }
        else
        {
            CyU3PDmaDscrGetConfig (handle->firstConsIndex[sckCount], &dscr);
            sck.dscrChain = dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK;
        }

        handle->activeConsIndex[sckCount] = sck.dscrChain;
        CyU3PDmaSocketSetConfig (handle->consSckId[sckCount], &sck);
    }
}

CyU3PReturnStatus_t
CyU3PDmaMultiChannelSetXfer (
        CyU3PDmaMultiChannel *handle,
        uint32_t count,
        uint16_t multiSckOffset)
{
    uint32_t status;

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    if (handle->state != CY_U3P_DMA_CONFIGURED)
    {
        status = CY_U3P_ERROR_ALREADY_STARTED;
    }
    if (multiSckOffset >= handle->validSckCount)
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((handle->type == CY_U3P_DMA_TYPE_MULTICAST) &&
            (multiSckOffset != 0))
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
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
            case CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE:
            case CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE:
                CyU3PDmaMultiChannelSetXfer_TypeManyToOne (handle, count,
                        multiSckOffset);
                break;
            case CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY:
            case CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY:
                CyU3PDmaMultiChannelSetXfer_TypeOneToMany (handle, count,
                        multiSckOffset);
                break;
            case CY_U3P_DMA_TYPE_MULTICAST:
                if (glMultiChannelSetXfer == 0)
                    return CY_U3P_ERROR_INVALID_SEQUENCE;
                else
                    glMultiChannelSetXfer (handle, count, multiSckOffset);
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

static void
CyU3PDmaMultiChannelReset_TypeManyToOne (
        CyU3PDmaMultiChannel *handle)
{
    CyU3PDmaDescriptor_t dscr;
    uint16_t dscrIndex;
    uint16_t count, sckCount;

    /* Disable and reconfigure the sockets. */
    CyU3PDmaMultiConfigureSockets_TypeManyToOne (handle);

    /* Cleanup the consumer descriptors */
    dscrIndex = handle->firstConsIndex[0];
    count = handle->count * handle->validSckCount;
    while ((count--) != 0)
    {
        CyU3PDmaDscrGetConfig (dscrIndex, &dscr);
        dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
        CyU3PDmaDscrSetConfig (dscrIndex, &dscr);
        dscrIndex = (dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
    }

    if (handle->type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE)
    {
        /* Clean up the producer descriptors. */
        for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
        {
            dscrIndex = handle->activeProdIndex[sckCount] =
                handle->firstProdIndex[sckCount];
            for (count = 0; count < handle->count; count++)
            {
                CyU3PDmaDscrGetConfig (dscrIndex, &dscr);
                dscr.size &= CY_U3P_BUFFER_SIZE_MASK;
                CyU3PDmaDscrSetConfig (dscrIndex, &dscr);
                dscrIndex = (dscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
            }
        }
    }

    /* Update the reference pointers. */
    handle->currentProdIndex = handle->commitProdIndex =
        handle->firstProdIndex[0];
    handle->currentConsIndex = handle->commitConsIndex =
        handle->activeConsIndex[0] = handle->firstConsIndex[0];
}

static void
CyU3PDmaMultiChannelReset_TypeOneToMany (
        CyU3PDmaMultiChannel *handle)
{
    CyU3PDmaDescriptor_t dscr;
    uint16_t dscrIndex;
    uint16_t count, sckCount;

    /* Disable and re-configure the sockets. */
    CyU3PDmaMultiConfigureSockets_TypeOneToMany (handle);

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

    if (handle->type == CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY)
    {
        /* Clean up the consumer descriptors. */
        for (sckCount = 0; sckCount < handle->validSckCount; sckCount++)
        {
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
    }

    /* Update the reference pointers. */
    handle->currentProdIndex = handle->commitProdIndex =
        handle->activeProdIndex[0] = handle->firstProdIndex[0];
    handle->currentConsIndex = handle->commitConsIndex =
        handle->firstConsIndex[0];
}

CyU3PReturnStatus_t
CyU3PDmaMultiChannelReset (
        CyU3PDmaMultiChannel *handle)
{
    CyU3PThread *thread_p;
    uint32_t priority;
    uint32_t status;

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    switch (handle->type)
    {
        case CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE:
        case CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE:
            CyU3PDmaMultiChannelReset_TypeManyToOne (handle);
            break;
        case CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY:
        case CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY:
            CyU3PDmaMultiChannelReset_TypeOneToMany (handle);
            break;
        case CY_U3P_DMA_TYPE_MULTICAST:
            if (glMultiChannelReset == 0)
                return CY_U3P_ERROR_INVALID_SEQUENCE;
            else
                glMultiChannelReset (handle);
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
    CyU3PEventSet (&(handle->flags), (~(CY_U3P_DMA_CB_XFER_CPLT | CY_U3P_DMA_CB_SEND_CPLT |
                    CY_U3P_DMA_CB_RECV_CPLT | CY_U3P_DMA_CB_ERROR)), CYU3P_EVENT_AND);
    /* Set the aborted flag. */
    CyU3PEventSet (&(handle->flags), CY_U3P_DMA_CB_ABORTED, CYU3P_EVENT_OR);

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
CyU3PDmaMultiChannelGetBuffer (
        CyU3PDmaMultiChannel *handle,
        CyU3PDmaBuffer_t *buffer_p,
        uint32_t waitOption)
{
    uint32_t status, flags;
    CyU3PDmaDescriptor_t dscr;
    CyU3PDmaSocketConfig_t sck;

    /* Check the parameter validity first. */
    if (buffer_p == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    status = CyU3PDmaMultiChannelAcquireLock (handle, waitOption);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* This API can be called only for manual channels. Also the
     * channel must be in active state. */
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

    if (handle->type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY)
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if (status != CY_U3P_SUCCESS)
    {
        /* Release the lock and return. */
        CyU3PMutexPut (&(handle->lock));
        return status;
    }

    if (handle->type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE)
    {
        CyU3PDmaSocketGetConfig (handle->consSckId[0], &sck);
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

        /* Release the mutex. */
        CyU3PMutexPut (&(handle->lock));

        return status;
    }

    /* MANUAL_ONE_TO_MANY, MANUAL_MANY_TO_ONE and MULTICAST. */
    /* Clear previous events before checking the buffer status. */
    CyU3PEventSet (&(handle->flags), (~CY_U3P_DMA_CB_PROD_EVENT), CYU3P_EVENT_AND);

    CyU3PDmaDscrGetConfig (handle->currentProdIndex, &dscr);
    /* Check if the buffer is occupied. If not then wait until there is
     * a produce event. */
    if (!(dscr.size & CY_U3P_BUFFER_OCCUPIED))
    {
        /* Before waiting, release the lock. */
        CyU3PMutexPut (&(handle->lock));
        status = CyU3PEventGet (&(handle->flags), (CY_U3P_DMA_CB_PROD_EVENT |
                    CY_U3P_DMA_CB_ABORTED | CY_U3P_DMA_CB_ERROR),
                CYU3P_EVENT_OR, &flags, waitOption);
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
        CyU3PDmaDscrGetConfig (handle->currentProdIndex, &dscr);
    }
    else if (handle->isDmaHandleDCache)
    {
        /* Ensure that the DMA thread had a chance to run. */
        CyU3PThreadRelinquish ();
    }

    /* Load the buffer address and byte count information regarding the buffer.
     * The information is provided as the producer sees it and does not include 
     * the prodHeader and prodFooter. */
    buffer_p->buffer = ((uint8_t *)dscr.buffer);
    buffer_p->count  = (dscr.size >> CY_U3P_BYTE_COUNT_POS);
    buffer_p->size   = (dscr.size & CY_U3P_BUFFER_SIZE_MASK);
    /* Read the buffer status from the descriptor. */
    buffer_p->status = (dscr.size & CY_U3P_DMA_BUFFER_STATUS_MASK);

    /* Release the mutex. */
    CyU3PMutexPut (&(handle->lock));

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaMultiChannelCommitBuffer (
        CyU3PDmaMultiChannel *handle,
        uint16_t count,
        uint16_t bufStatus)
{
    uint8_t type;
    uint16_t sckId;
    uint16_t index, sckIndex;
    uint32_t status;
    CyU3PDmaSocketConfig_t sck;
    CyU3PDmaDescriptor_t prodDscr, consDscr;

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    type = handle->type;
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

    if (type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY)
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if ((type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE) &&
            (status == CY_U3P_SUCCESS))
    {
        CyU3PDmaSocketGetConfig (handle->consSckId[0], &sck);

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

        /* Release the lock and return. */
        CyU3PMutexPut (&(handle->lock));
        return status;
    }

    /* MANUAL_MANY_TO_ONE, MANUAL_ONE_TO_MANY and MULTICAST. */

    if (status == CY_U3P_SUCCESS)
    {
        CyU3PDmaDscrGetConfig (handle->currentProdIndex, &prodDscr);
        CyU3PDmaDscrGetConfig (handle->currentConsIndex, &consDscr);
        if (!(prodDscr.size & CY_U3P_BUFFER_OCCUPIED))
        {
            status = CY_U3P_ERROR_INVALID_SEQUENCE;
        }
        /* We are checking only the first consumer socket for
         * multicast channels. This should be good enough for
         * general error checking. In case full check has to be
         * made, then all descriptors have to be read and verified. */
        if (consDscr.size & CY_U3P_BUFFER_OCCUPIED)
        {
            status = CY_U3P_ERROR_INVALID_SEQUENCE;
        }
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
            CyU3PSysCleanDRegion ((uint32_t *)consDscr.buffer, consDscr.size &
                    CY_U3P_BUFFER_SIZE_MASK);
        }

    if (type == CY_U3P_DMA_TYPE_MULTICAST)
    {
        /* In case of multicast channel, there are multiple
         * consumer sockets and all consumer descriptors for 
         * the current buffer has to be updated. */
        index = handle->currentConsIndex;
        for (sckIndex = 0; sckIndex < handle->validSckCount; sckIndex++)
        {
            CyU3PDmaDscrGetConfig (index, &consDscr);

            /* Commit the data on enabled consumers only. If the consumer is not enabled, we can treat the
             * buffer as completed on the socket. */
            if (handle->consDisabled[sckIndex] == 0)
            {
            consDscr.size &= CY_U3P_BUFFER_SIZE_MASK;
            consDscr.size |= (count << CY_U3P_BYTE_COUNT_POS);
            /* Update the status bits. */
            if (bufStatus == 0)
            {
                /* The buffer status of the producer descriptor is copied. */
                consDscr.size |= (prodDscr.size & CY_U3P_DMA_BUFFER_STATUS_MASK);
            }
            else
            {
                /* The buffer status is set as provided. */
                consDscr.size |= ((bufStatus & CY_U3P_DMA_BUFFER_STATUS_WRITE_MASK) 
                        | CY_U3P_DMA_BUFFER_OCCUPIED);
            }

            CyU3PDmaDscrSetConfig (index, &consDscr);
            /* Identify the socket for sending the event. */
            sckId = consDscr.sync & (CY_U3P_CONS_IP_MASK | CY_U3P_CONS_SCK_MASK);
            CyU3PDmaSocketSendEvent (sckId, index, CyTrue);
            }

            /* Advance index to next consumer descriptor for the buffer.
             * This information is stored in the write descriptor region. */
            index = (consDscr.chain  >> CY_U3P_WR_NEXT_DSCR_POS);
        }

        /* Move the pointers to the next buffer in the chain. The producer
         * descriptor holds information about this. */
        handle->currentProdIndex = (prodDscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
        /* The next consumer index is stored in the next producer descriptor. */
        CyU3PDmaDscrGetConfig (handle->currentProdIndex, &prodDscr);
        handle->currentConsIndex = prodDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK;
    }
    else /* MANUAL_ONE_TO_MANY and MANUAL_MANY_TO_ONE. */
    {
        /* In case of MANUAL channel, the consumer descriptor chain is
         * different. Configure the consumer descriptor chain to commit
         * the modified buffer */
        consDscr.size &= CY_U3P_BUFFER_SIZE_MASK;
        consDscr.size |= (count << CY_U3P_BYTE_COUNT_POS);
        /* Update the status bits. */
        if (bufStatus == 0)
        {
            /* The buffer status of the producer descriptor is copied. */
            consDscr.size |= (prodDscr.size & CY_U3P_DMA_BUFFER_STATUS_MASK);
        }
        else
        {
            /* The buffer status is set as provided. */
            consDscr.size |= ((bufStatus & CY_U3P_DMA_BUFFER_STATUS_WRITE_MASK) 
                    | CY_U3P_DMA_BUFFER_OCCUPIED);
        }

        CyU3PDmaDscrSetConfig (handle->currentConsIndex, &consDscr);
        /* Identify the socket for sending the event. */
        sckId = consDscr.sync & (CY_U3P_CONS_IP_MASK | CY_U3P_CONS_SCK_MASK);
        CyU3PDmaSocketSendEvent (sckId, handle->currentConsIndex, CyTrue);

        /* Move the pointers to the next buffer in the chain. The cosumer descriptor holds
         * information about both the chains. Extract next node from the descriptor. */
        if (type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE)
        {
            handle->currentConsIndex = consDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK;
            /* The next producer index is stored in the next consumer descriptor. */
            CyU3PDmaDscrGetConfig (handle->currentConsIndex, &consDscr);
            handle->currentProdIndex = consDscr.chain >> CY_U3P_WR_NEXT_DSCR_POS;
        }
        else /* CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY */
        {
            handle->currentProdIndex = (prodDscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
            /* The next consumer index is stored in the next producer descriptor. */
            CyU3PDmaDscrGetConfig (handle->currentProdIndex, &prodDscr);
            handle->currentConsIndex = prodDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK;
        }
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDmaMultiChannelDiscardBuffer (
        CyU3PDmaMultiChannel *handle)
{
    uint8_t type;
    uint16_t i, sckId;
    uint32_t status;
    CyU3PDmaSocketConfig_t sck;
    CyU3PDmaDescriptor_t prodDscr, consDscr;

    status = CyU3PDmaMultiChannelAcquireLock (handle, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    type = handle->type;
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

    if ((type == CY_U3P_DMA_TYPE_AUTO_ONE_TO_MANY) ||
            (type == CY_U3P_DMA_TYPE_MULTICAST))
    {
        status = CY_U3P_ERROR_NOT_SUPPORTED;
    }

    if (status != CY_U3P_SUCCESS)
    {
        /* Release the lock and return. */
        CyU3PMutexPut (&(handle->lock));
        return status;
    }

    if (type == CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE)
    {
        CyU3PDmaSocketGetConfig (handle->consSckId[0], &sck);

        if (sck.status & CY_U3P_SUSPENDED)
        {
            i = sck.dscrChain & CY_U3P_DSCR_NUMBER_MASK;
            CyU3PDmaDscrGetConfig (i, &consDscr);
            if (consDscr.size & CY_U3P_DMA_BUFFER_OCCUPIED)
            {
                /* First move the consumer socket to the next
                 * descriptor in chain. */
                sck.dscrChain &= ~(CY_U3P_DSCR_NUMBER_MASK);
                sck.dscrChain |= (consDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
                sck.intr = 0;
                CyU3PDmaSocketSetConfig (handle->consSckId[0], &sck);

                /* Clear the current descriptor and send event
                 * to the producer. */
                consDscr.size &= CY_U3P_BUFFER_SIZE_MASK;
                sckId = (consDscr.sync & (CY_U3P_CONS_IP_MASK |
                            CY_U3P_PROD_SCK_MASK));
                CyU3PDmaDscrSetConfig (i, &consDscr);
                CyU3PDmaSocketSendEvent (sckId, i, CyFalse);
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

        /* Release the lock and return. */
        CyU3PMutexPut (&(handle->lock));
        return status;
    }

    /* Configure the consumer descriptor chain to set the discard marker. */
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

        if (type == CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE)
        {
            /* If the discard count is zero, then enable the stall
             * interrupt for the consumer socket. */
            if (handle->discardCount[0] == 0)
            {
                CyU3PDmaSocketGetConfig (handle->consSckId[0], &sck);
                sck.intrMask |= CY_U3P_STALL;
                /* Take care not to clear the interrupts. */
                sck.intr     = 0;
                CyU3PDmaSocketSetConfig (handle->consSckId[0], &sck);
            }
            /* Increment the discard buffer count. */
            handle->discardCount[0]++;

            handle->currentConsIndex = consDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK;
            /* The next producer index is stored in the next consumer descriptor. */
            CyU3PDmaDscrGetConfig (handle->currentConsIndex, &consDscr);
            handle->currentProdIndex = consDscr.chain >> CY_U3P_WR_NEXT_DSCR_POS;
        }
        else /* CY_U3P_DMA_TYPE_MANUAL_ONE_TO_MANY */
        {
            /* Identify the sckCount for the consSckId so that the
             * corresponding discardCount can be updated. */
            sckId = consDscr.sync & (CY_U3P_CONS_IP_MASK |
                    CY_U3P_CONS_SCK_MASK);
            for (i = 0; i < handle->validSckCount; i++)
            {
                if (handle->consSckId[i] == sckId)
                {
                    break;
                }
            }

            /* If the discard count is zero, then enable the stall
             * interrupt for the consumer socket. */
            if (handle->discardCount[i] == 0)
            {
                CyU3PDmaSocketGetConfig (sckId, &sck);
                sck.intrMask |= CY_U3P_STALL;
                /* Take care not to clear the interrupts. */
                sck.intr     = 0;
                CyU3PDmaSocketSetConfig (sckId, &sck);
            }
            /* Increment the discard buffer count. */
            handle->discardCount[i]++;

            handle->currentProdIndex = (prodDscr.chain >> CY_U3P_WR_NEXT_DSCR_POS);
            /* The next consumer index is stored in the next producer descriptor. */
            CyU3PDmaDscrGetConfig (handle->currentProdIndex, &prodDscr);
            handle->currentConsIndex = prodDscr.chain & CY_U3P_RD_NEXT_DSCR_MASK;
        }
    }

    /* Release the lock. */
    CyU3PMutexPut (&(handle->lock));

    return status;
}

CyU3PReturnStatus_t
CyU3PDmaMultiChannelWaitForCompletion (
        CyU3PDmaMultiChannel *handle,
        uint32_t waitOption)
{
    uint32_t state;
    uint32_t status;
    uint32_t flags, mask = 0;

    status = CyU3PDmaMultiChannelAcquireLock (handle, waitOption);
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

/* [] */

