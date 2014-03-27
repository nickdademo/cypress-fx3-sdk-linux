/*
 ## Cypress USB 3.0 Platform source file (cyu3utils.c)
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

#include <cyu3types.h>
#include <cyu3os.h>
#include <cyu3error.h>
#include <cyu3utils.h>

#ifdef CY_USE_ARMCC

#ifdef CYU3P_DEBUG
#define CY_U3P_UTILS_US_LOOP_CNT    0x1F
#else
#define CY_U3P_UTILS_US_LOOP_CNT    0x2F
#endif

/* Busy wait function used for delays of upto 250 us */
void
CyU3PBusyWait (
        uint16_t usWait)
{
    uint32_t i;

    while (usWait--)
    {
        /* An appoximation for 1us */
        for (i = 0; i < CY_U3P_UTILS_US_LOOP_CNT; i++)
        {
            i += 10;
            i -= 10;
        }
    }
}

#else

void
CyU3PBusyWait (
        uint16_t usWait)
{
    /* Each count accounts for 4 clock cycles. This means that we approximately need a count of
     * 50 for a 1 us delay. */
    register uint32_t loopCnt = usWait * 50;

    __asm__ volatile
        (
         "DelayLoop:"
         "subs %0, %0, #1\n\t"          /* 1 cycle */
         "bhi DelayLoop\n\t"            /* 3 cycles if condition match, 1 cycle when fail */
         : "+r" (loopCnt)
         :
         :
        );
}

#endif

void
CyU3PMemCopy32 (
        uint32_t *dest, 
        uint32_t *src,
        uint32_t count)
{
    /* Loop unrolling for faster operation */
    while (count >> 3)
    {
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        dest[4] = src[4];
        dest[5] = src[5];
        dest[6] = src[6];
        dest[7] = src[7];

        count -= 8;
        dest += 8;
        src += 8;
    }

    while (count--)
    {
        *dest = *src;
        dest++;
        src++;
    }
}

/* Summary
 * This function is used to compute the checksum.
 */
CyU3PReturnStatus_t
CyU3PComputeChecksum (
        uint32_t *buffer,
        uint32_t  length,
        uint32_t *chkSum)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint32_t count;

    /* Validate the parameters */
    if ((buffer == NULL) || (length == 0) || (chkSum == NULL))
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }
    else
    {
        *chkSum = 0;
        for (count = 0; count < (length / 4); count++)
        {
            *chkSum += buffer[count];
        }
    }
    return status;
}

CyU3PReturnStatus_t
CyU3PReadDeviceRegisters (
        uvint32_t *regAddr,
        uint8_t    numRegs,
        uint32_t  *dataBuf)
{
    if ((dataBuf == 0) || (numRegs == 0) || (((uint32_t)regAddr & 0xF0000000) < 0xE0000000))
        return CY_U3P_ERROR_BAD_ARGUMENT;

    while (numRegs)
    {
        *dataBuf++ = *regAddr++;
        numRegs--;
    }

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PWriteDeviceRegisters (
        uvint32_t *regAddr,
        uint8_t    numRegs,
        uint32_t  *dataBuf)
{
    if ((dataBuf == 0) || (numRegs == 0) || (((uint32_t)regAddr & 0xF0000000) < 0xE0000000))
        return CY_U3P_ERROR_BAD_ARGUMENT;

    while (numRegs)
    {
        *regAddr++ = *dataBuf++;
        numRegs--;
    }

    return CY_U3P_SUCCESS;
}

#ifdef CY_USE_ARMCC

void
__aeabi_memset (
        void   *dest,
        size_t  n,
        int     c)
{
    uint8_t *ptr = (uint8_t *)dest;
    while (n--)
    {
        *ptr++ = (uint8_t)c;
    }
}

void
__aeabi_memset4 (
        void   *dest,
        size_t  n,
        int     c)
{
    __aeabi_memset (dest, n, c);
}

void
__aeabi_memclr (
        void   *dest,
        size_t  n)
{
    __aeabi_memset (dest, n, 0);
}

void __aeabi_memclr4 (
        void   *dest,
        size_t  n)
{
    __aeabi_memset (dest, n, 0);
}

void
__aeabi_memcpy (
        void       *dest,
        const void *src,
        size_t      n)
{
    uint8_t *pd = (uint8_t *)dest;
    uint8_t *ps = (uint8_t *)src;

    if (dest < src)
    {
        while (n--)
        {
            *pd++ = *ps++;
        }
    }
    else
    {
        pd = (uint8_t *)dest + n - 1;
        ps = (uint8_t *)src + n - 1;
        while (n--)
        {
            *pd-- = *ps--;
        }
    }
}

void
__aeabi_memcpy4 (
        void       *dest,
        const void *src,
        size_t      n)
{
    __aeabi_memcpy (dest, src, n);
}

#else

void
__nop (
        void)
{
    __asm__ volatile
        (
         "mov r0, r0\n\t"
        );
}

#endif

/* [] */

