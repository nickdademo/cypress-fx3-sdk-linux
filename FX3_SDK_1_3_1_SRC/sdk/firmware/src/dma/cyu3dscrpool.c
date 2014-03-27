/*
 ## Cypress USB 3.0 Platform header file (cyu3dscrpool.c)
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

/* Summary
 * This file implements the free descriptor pool that is used in firmware.
 */

#include <cyu3os.h>
#include <cyu3error.h>
#include <cyu3descriptor.h>
#include <cyu3dma.h>
#include <cyu3regs.h>

#define CyU3PBitCountToByteCount(bits) ((bits) / 8)
#define CyU3PBitCountToWordCount(bits) ((bits) / 32)

typedef struct CyU3PDmaDescList_t
{
    uint16_t   availCount;       /* Count of available descriptors.        */
    uint16_t   curDscr;          /* Position of last allocated descriptor. */
    uint32_t   bitMap[CyU3PBitCountToWordCount(CY_U3P_DMA_DSCR_COUNT)];
                                 /* Bit map representing state of each descriptor. */
    CyU3PMutex lock;             /* Lock for the descriptor pool.          */
} CyU3PDmaDscrList;

CyU3PDmaDscrList glDmaDscrList;

void
CyU3PDmaDscrListCreate (
        void)
{
    /* Mark all of descriptors as free for allocation. */
    CyU3PMemSet ((uint8_t *)glDmaDscrList.bitMap, 0x00, \
            CyU3PBitCountToByteCount(CY_U3P_DMA_DSCR_COUNT));

    /* Mark the first descriptor as used since this is used by bootloader */
    glDmaDscrList.bitMap[0]  = 0x00000001;
    glDmaDscrList.availCount = CY_U3P_DMA_DSCR_COUNT - 1;
    glDmaDscrList.curDscr    = 1;
    CyU3PMutexCreate (&(glDmaDscrList.lock), CYU3P_NO_INHERIT);

    return;
}

void
CyU3PDmaDscrListDestroy (
        void)
{
    /* Mark all of the descriptors as not available. */
    CyU3PMemSet ((uint8_t *)glDmaDscrList.bitMap, 0xFF, \
            CyU3PBitCountToByteCount(CY_U3P_DMA_DSCR_COUNT));
    glDmaDscrList.availCount = 0;
    CyU3PMutexDestroy (&(glDmaDscrList.lock));

    return;
}

CyU3PReturnStatus_t
CyU3PDmaDscrGet (
        uint16_t *index_p)
{
    uint32_t val, i, j;
    uint16_t index = 0;

    if (index_p == NULL)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    CyU3PMutexGet (&(glDmaDscrList.lock), CYU3P_WAIT_FOREVER);

    /* Search through the bit-map to find the next free descriptor index. */
    if (glDmaDscrList.availCount)
    {
        for (i = 0; i < (CyU3PBitCountToWordCount(CY_U3P_DMA_DSCR_COUNT)); i++)
        {
            val = glDmaDscrList.bitMap[i];
            if (val != 0xFFFFFFFF)
            {
                for (j = 1; j != 0; j <<= 1, index++)
                {
                    if ((val & j) == 0)
                    {
                        glDmaDscrList.bitMap[i] = (val | j);
                        glDmaDscrList.curDscr   = index;
                        glDmaDscrList.availCount--;
                        *index_p = index;
                        CyU3PMutexPut (&(glDmaDscrList.lock));
                        return CY_U3P_SUCCESS;
                    }
                }
            }
            else
            {
                index += 32;
            }
        }
    }

    CyU3PMutexPut (&(glDmaDscrList.lock));
    return CY_U3P_ERROR_FAILURE;
}

CyU3PReturnStatus_t
CyU3PDmaDscrPut (
        uint16_t index)
{
    uint32_t val, i;

    if ((index >= CY_U3P_DMA_DSCR_COUNT) || (index == 0))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    CyU3PMutexGet (&(glDmaDscrList.lock), CYU3P_WAIT_FOREVER);
    /* Mark the descriptor index free, and increment the count of
     * free descriptors. */
    i = CyU3PBitCountToWordCount(index);
    val  = glDmaDscrList.bitMap[i];
    val &= ~(1 << (index % 32));
    glDmaDscrList.bitMap[i] = val;

    glDmaDscrList.availCount++;
    CyU3PMutexPut (&(glDmaDscrList.lock));
    return CY_U3P_SUCCESS;
}

uint16_t
CyU3PDmaDscrGetFreeCount (
        void)
{
    return glDmaDscrList.availCount;
}

CyU3PReturnStatus_t
CyU3PDmaDscrChainCreate (
        uint16_t *dscrIndex_p,
        uint16_t count,
        uint16_t bufferSize,
        uint32_t dscrSync)
{
    uint16_t curIndex, nextIndex, i;
    uint32_t status = CY_U3P_SUCCESS;
    CyU3PDmaDescriptor_t dscr;

    if (CyU3PDmaDscrGetFreeCount () < count)
    {
        return CY_U3P_ERROR_MEMORY_ERROR;
    }
    if (dscrIndex_p == NULL)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (count == 0)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Create the chain */
    i = count;
    CyU3PDmaDscrGet (dscrIndex_p);
    curIndex = *dscrIndex_p;

    /* Allocate and update the required set of descriptors */
    while ((i--) != 0)
    {
        if (i != 0)
        {
            CyU3PDmaDscrGet (&nextIndex);
        }
        else
        {
            /* Create a loop back. */
            nextIndex = *dscrIndex_p;
        }

        if ((bufferSize != 0) && (status == CY_U3P_SUCCESS))
        {
            dscr.buffer = CyU3PDmaBufferAlloc (bufferSize);
            dscr.size   = (bufferSize & CY_U3P_BUFFER_SIZE_MASK);
            if (dscr.buffer == NULL)
            {
                status = CY_U3P_ERROR_MEMORY_ERROR;
            }
            if ((dscr.buffer < CY_U3P_DMA_BUFFER_AREA_BASE) ||
                    ((dscr.buffer + bufferSize) >= CY_U3P_DMA_BUFFER_AREA_LIMIT))
            {
                status = CY_U3P_ERROR_MEMORY_ERROR;
            }
        }
        else
        {
            dscr.buffer = NULL;
            dscr.size   = 0;
        }

        dscr.sync   = dscrSync;
        /* Update both the consumer and producer chains with the same descriptor. */
        dscr.chain = nextIndex | (nextIndex << CY_U3P_WR_NEXT_DSCR_POS);
        CyU3PDmaDscrSetConfig (curIndex, &dscr);
        curIndex = nextIndex;
    }

    /* If there was a memory allocation error, clean up all the allocated 
     * memory and free the descriptor chain. */
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDmaDscrChainDestroy (*dscrIndex_p, count, CyTrue, CyTrue);
    }

    return status;
}

void
CyU3PDmaDscrChainDestroy (
        uint16_t dscrIndex,
        uint16_t count,
        CyBool_t isProdChain,
        CyBool_t freeBuffer)
{
    uint16_t nextIndex;
    CyU3PDmaDescriptor_t dscr;

    while ((count--) != 0)
    {
        CyU3PDmaDscrGetConfig (dscrIndex, &dscr);
        if (freeBuffer && (dscr.buffer != NULL))
        {
            CyU3PDmaBufferFree (dscr.buffer);
        }
        if (isProdChain)
        {
            nextIndex = ((dscr.chain & CY_U3P_WR_NEXT_DSCR_MASK) >> CY_U3P_WR_NEXT_DSCR_POS);
        }
        else
        {
            nextIndex = (dscr.chain & CY_U3P_RD_NEXT_DSCR_MASK);
        }
        CyU3PDmaDscrPut (dscrIndex);
        dscrIndex = nextIndex;
    }

    return;
}

/* [] */
