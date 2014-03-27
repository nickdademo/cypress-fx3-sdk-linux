/*
 ## Cypress USB 3.0 Platform Source file (cyu3mmu.c)
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

#include <cyu3mmu.h>
#include <cyu3regs.h>
#include <cyfx3_api.h>

#pragma arm

/* Function to disable the I-cache.
 */
void
CyU3PSysDisableICache (
        void)
{
    /*
     * Read CP15 register C1
     * Clear the I-cache enable bit (Bit 12)
     * Write value back to CP15 register C1
     */
    uint32_t val;

#ifdef CY_USE_ARMCC
    __asm
    {
        MRC p15, 0, val, c1, c0, 0;
        BIC val, val, #0x1000;
        MCR p15, 0, val, c1, c0, 0;
    }
#else
    __asm__
        (
         "MRC p15, 0, %0, c1, c0, 0\n\t"
         "BIC %0, %0, #0x1000\n\t"
         "MCR p15, 0, %0, c1, c0, 0\n\t"
         : "+r" (val)
         :
         : "memory"
        );
#endif
}

/* Function to disable the D-cache.
 */
void
CyU3PSysDisableDCache (
        void)
{
    /*
     * Read CP15 register C1
     * Clear the D-cache enable bit (Bit 2)
     * Write value back to CP15 register C1
     */
    uint32_t val;

#ifdef CY_USE_ARMCC
    __asm
    {
        MRC p15, 0, val, c1, c0, 0;
        BIC val, val, #0x0004;
        MCR p15, 0, val, c1, c0, 0;
    }
#else
    __asm__
        (
         "MRC p15, 0, %0, c1, c0, 0\n\t"
         "BIC %0, %0, #0x04\n\t"
         "MCR p15, 0, %0, c1, c0, 0\n\t"
         : "+r" (val)
         :
         : "memory"
        );
#endif
}

/* Function to disable the MMU.
 */
void
CyU3PSysDisableMMU (
        void)
{
    /*
     * Read CP15 register C1
     * Clear the MMU enable bit (Bit 0)
     * Write value back to CP15 register C1
     */
    uint32_t val;

#ifdef CY_USE_ARMCC
    __asm
    {
        MRC p15, 0, val, c1, c0, 0;
        BIC val, val, #0x0001;
        MCR p15, 0, val, c1, c0, 0;
    }
#else
    __asm__
        (
         "MRC p15, 0, %0, c1, c0, 0\n\t"
         "BIC %0, %0, #0x01\n\t"
         "MCR p15, 0, %0, c1, c0, 0\n\t"
         : "+r" (val)
         :
         : "memory"
        );
#endif
}

void
CyU3PSysDisableCacheMMU (
        void)
{
    /*
     * Read CP15 register C1
     * Clear the cache and MMU enable bits (Bits 12, 2, and 0)
     * Write value back to CP15:C1 register.
     */
    uint32_t val;
    uint32_t v1;

#ifdef CY_USE_ARMCC
    __asm
    {
        MRC p15, 0, val, c1, c0, 0;
        BIC val, val, #0x1005;
        MCR p15, 0, val, c1, c0, 0;
    }

    __asm
    {
        MRC p15, 0, val, c15, c0, 0
        MOV v1, #0xFFFEFFFF
        AND val, val, v1
        MCR p15, 0, val, c15, c0, 0
    }
#else
    __asm__
        (
         "MRC p15, 0, %0, c1, c0, 0\n\t"
         "BIC %0, %0, #0x1000\n\t"
         "BIC %0, %0, #0x05\n\t"
         "MCR p15, 0, %0, c1, c0, 0\n\t"
         : "+r" (val)
         :
         : "memory"
        );

    __asm__
        (
         "MRC p15, 0, %0, c15, c0, 0\n\t"
         "MOV %1, #0xFFFEFFFF\n\t"
         "AND %0, %0, %1\n\t"
         "MCR p15, 0, %0, c15, c0, 0\n\t"
         : "+r" (val), "+r" (v1)
         :
         : "memory"
        );
#endif
}

/* Function to enable the I-Cache.
 * Sets caches up for random replacement.
 */
void
CyU3PSysEnableICache (
        void)
{
    uint32_t val;

#ifdef CY_USE_ARMCC
    __asm
    {
        MRC p15, 0, val, c1, c0, 0;
        ORR val, val, #0x1000;
        BIC val, val, #0x4000;
        MCR p15, 0, val, c1, c0, 0;
    }
#else
    __asm__
        (
         "MRC p15, 0, %0, c1, c0, 0\n\t"
         "ORR %0, %0, #0x1000\n\t"
         "BIC %0, %0, #0x4000\n\t"
         "MCR p15, 0, %0, c1, c0, 0\n\t"
         : "+r" (val)
         :
         : "memory"
        );
#endif
}

/* Function to enable the D-Cache.
 */
void
CyU3PSysEnableDCache (
        void)
{
    uint32_t val;

#ifdef CY_USE_ARMCC
    __asm
    {
        MRC p15, 0, val, c1, c0, 0;
        ORR val, val, #0x0004;
        MCR p15, 0, val, c1, c0, 0;
    }
#else
    __asm__
        (
         "MRC p15, 0, %0, c1, c0, 0\n\t"
         "ORR %0, %0, #0x04\n\t"
         "MCR p15, 0, %0, c1, c0, 0\n\t"
         : "+r" (val)
         :
         : "memory"
        );
#endif
}

/* Function to enable the MMU.
 */
void
CyU3PSysEnableMMU (
        void)
{
    uint32_t val;

#ifdef CY_USE_ARMCC
    __asm
    {
        MRC p15, 0, val, c1, c0, 0;
        ORR val, val, #0x0001;
        MCR p15, 0, val, c1, c0, 0;
    }
#else
    __asm__
        (
         "MRC p15, 0, %0, c1, c0, 0\n\t"
         "ORR %0, %0, #0x01\n\t"
         "MCR p15, 0, %0, c1, c0, 0\n\t"
         : "+r" (val)
         :
         : "memory"
        );
#endif
}

void
CyU3PSysEnableCacheMMU (
        void)
{
    uint32_t val;

#ifdef CY_USE_ARMCC
    __asm
    {
        MRC p15, 0, val, c1, c0, 0;
        ORR val, val, #0x1005;
        BIC val, val, #0x4000;
        MCR p15, 0, val, c1, c0, 0;
    }
#else
    __asm__
        (
         "MRC p15, 0, %0, c1, c0, 0\n\t"
         "ORR %0, %0, #0x1000\n\t"
         "ORR %0, %0, #0x05\n\t"
         "BIC %0, %0, #0x4000\n\t"
         "MCR p15, 0, %0, c1, c0, 0\n\t"
         : "+r" (val)
         :
         : "memory"
        );
#endif
}

void
CyU3PSysFlushCaches (
        void)
{
    uint32_t val = 0;

#ifdef CY_USE_ARMCC
    __asm
    {
        MCR p15, 0, val, c7, c7, 0;
    }
#else
    __asm__
        (
         "MCR p15, 0, %0, c7, c7, 0\n\t"
         : "+r" (val)
         :
         : "memory"
        );
#endif
}

void
CyU3PSysFlushICache (
        void)
{
    uint32_t val = 0;

#ifdef CY_USE_ARMCC
    __asm
    {
        MCR p15, 0, val, c7, c5, 0;
    }
#else
    __asm__
        (
         "MCR p15, 0, %0, c7, c5, 0\n\t"
         : "+r" (val)
         :
         : "memory"
        );
#endif
}

/* Function to flush the D-cache.
 */
void
CyU3PSysFlushDCache (
        void)
{
    uint32_t val = 0;

#ifdef CY_USE_ARMCC
    __asm
    {
        MCR p15, 0, val, c7, c6, 0;
    }
#else
    __asm__
        (
         "MCR p15, 0, %0, c7, c6, 0\n\t"
         : "+r" (val)
         :
         : "memory"
        );
#endif
}

#ifdef CY_USE_ARMCC

/* Function to clean the D-cache.
 * Uses the test/clean command to do this.
 */
__asm void
CyU3PSysCleanDCache (
        void)
{
testclean
    MRC p15, 0, pc, c7, c10, 3
    BNE testclean
    BX lr
}

/* Function to clean and invalidate the D-Cache.
 * Uses the test/clean command to do this.
 */
__asm void
CyU3PSysClearDCache (
        void)
{
testcleanflush
    MRC p15, 0, pc, c7, c14, 3
    BNE testcleanflush
    BX  lr
}

/* Function to remove a specific region of memory from the I-cache.
 */
__asm void
CyU3PSysFlushIRegion (
        uint32_t *addr,
        uint32_t  len)
{
addr RN 0
len  RN 1
tmp  RN 2

    CMP   len, #0                                       /* If len is 0, return straight away. */
    BEQ   exitLabel1

    ADD   len, addr, len                                /* End address. */
    BIC   addr, addr, #((1 << CYU3P_CACHE_LINE_SZ) - 1) /* Cache line corresponding to start address. */
    MOV   tmp, #((1 << CYU3P_CACHE_LINE_SZ) - 1)        /* Temp mask for cache line. */
    TST   len, tmp                                      /* Check if the end address spills over a cache line. */
    SUB   len, len, addr                                /* Restore len. Does not affect flags set by the TST instrn. */
    MOV   len, len, lsr #CYU3P_CACHE_LINE_SZ            /* Reduce to number of cache lines. */
    ADDNE len, len, #1                                  /* Add 1 to number of cache lines if there is a residue. */

flushIline
    MCR p15, 0, addr, c7, c5, 1
    ADD addr, addr, #(1 << CYU3P_CACHE_LINE_SZ)
    SUBS len, len, #1
    BNE flushIline

exitLabel1
    BX lr
}

/* Function to clean and flush a specific region of data memory from the D-cache.
 */
__asm void
CyU3PSysClearDRegion (
        uint32_t *addr,
        uint32_t  len)
{
addr RN 0
len  RN 1
tmp  RN 2

    CMP   len, #0                                       /* If len is 0, return straight away. */
    BEQ   exitLabel2

    ADD   len, addr, len                                /* End address. */
    BIC   addr, addr, #((1 << CYU3P_CACHE_LINE_SZ) - 1) /* Cache line corresponding to start address. */
    MOV   tmp, #((1 << CYU3P_CACHE_LINE_SZ) - 1)        /* Temp mask for cache line. */
    TST   len, tmp                                      /* Check if the end address spills over a cache line. */
    SUB   len, len, addr                                /* Restore len. Does not affect flags set by the TST instrn. */
    MOV   len, len, lsr #CYU3P_CACHE_LINE_SZ            /* Reduce to number of cache lines. */
    ADDNE len, len, #1                                  /* Add 1 to number of cache lines if there is a residue. */

clearDline
    MCR p15, 0, addr, c7, c14, 1
    ADD addr, addr, #(1 << CYU3P_CACHE_LINE_SZ)
    SUBS len, len, #1
    BNE clearDline

exitLabel2
    BX lr
}

/* Function to clean a specific region of data memory from the D-cache.
 */
__asm void
CyU3PSysCleanDRegion (
        uint32_t *addr,
        uint32_t  len)
{
addr RN 0
len  RN 1
tmp  RN 2

    CMP   len, #0                                       /* If len is 0, return straight away. */
    BEQ   exitLabel3

    ADD   len, addr, len                                /* End address. */
    BIC   addr, addr, #((1 << CYU3P_CACHE_LINE_SZ) - 1) /* Cache line corresponding to start address. */
    MOV   tmp, #((1 << CYU3P_CACHE_LINE_SZ) - 1)        /* Temp mask for cache line. */
    TST   len, tmp                                      /* Check if the end address spills over a cache line. */
    SUB   len, len, addr                                /* Restore len. Does not affect flags set by the TST instrn. */
    MOV   len, len, lsr #CYU3P_CACHE_LINE_SZ            /* Reduce to number of cache lines. */
    ADDNE len, len, #1                                  /* Add 1 to number of cache lines if there is a residue. */

cleanDline
    MCR p15, 0, addr, c7, c10, 1
    ADD addr, addr, #(1 << CYU3P_CACHE_LINE_SZ)         /* Next cache line address. */
    SUBS len, len, #1                                   /* Check if we have finished the region specified. */
    BNE cleanDline

exitLabel3
    BX lr
}

/* Function to clean a specific region of data memory from the D-cache.
 */
__asm void
CyU3PSysFlushDRegion (
        uint32_t *addr,
        uint32_t  len)
{
addr RN 0
len  RN 1
tmp  RN 2

    CMP   len, #0                                       /* If len is 0, return straight away. */
    BEQ   exitLabel4

    ADD   len, addr, len                                /* End address. */
    BIC   addr, addr, #((1 << CYU3P_CACHE_LINE_SZ) - 1) /* Cache line corresponding to start address. */
    MOV   tmp, #((1 << CYU3P_CACHE_LINE_SZ) - 1)        /* Temp mask for cache line. */
    TST   len, tmp                                      /* Check if the end address spills over a cache line. */
    SUB   len, len, addr                                /* Restore len. Does not affect flags set by the TST instrn. */
    MOV   len, len, lsr #CYU3P_CACHE_LINE_SZ            /* Reduce to number of cache lines. */
    ADDNE len, len, #1                                  /* Add 1 to number of cache lines if there is a residue. */

flushDline
    MCR p15, 0, addr, c7, c6, 1
    ADD addr, addr, #(1 << CYU3P_CACHE_LINE_SZ)         /* Next cache line address. */
    SUBS len, len, #1                                   /* Check if we have finished the region specified. */
    BNE flushDline

exitLabel4
    BX lr
}

/* Function to lock a block of code into the I-Cache.
 * Please ensure that the I-Cache has been flushed before calling this function.
 */
__asm uint32_t
CyU3PSysCacheIRegion (
        uint32_t *addr,
        uint32_t  len)
{
addr RN 0
len  RN 1
tmp  RN 2
tmp1 RN 3
val  RN 12

    MOV   tmp1, #0
    CMP   len, #0                                       /* If len is 0, return straight away. */
    BEQ   statusReturn

    ADD   len, addr, len                                /* End address. */
    BIC   addr, addr, #((1 << CYU3P_CACHE_LINE_SZ) - 1) /* Cache line corresponding to start address. */
    MOV   tmp, #((1 << CYU3P_CACHE_LINE_SZ) - 1)        /* Temp mask for cache line. */
    TST   len, tmp                                      /* Check if the end address spills over a cache line. */
    SUB   len, len, addr                                /* Restore len. Does not affect flags set by the TST instrn. */
    MOV   len, len, lsr #CYU3P_CACHE_LINE_SZ            /* Reduce to number of cache lines. */
    ADDNE len, len, #1                                  /* Add 1 to number of cache lines if there is a residue. */
    CMP   len, #(1 << CYU3P_CACHE_WAY_SZ) - 1           /* Check if the region can fit in one way. */
    BHI   statusReturn

    MRC   p15, 0, val, c9, c0, 1                        /* Read I-Cache lock status. */
    AND   tmp, val, #0x0F                               /* Mask off bits other than lock status. */

    MOV   tmp1, #1
    TST   val, tmp1                                     /* Test way #0. */
    MOVNE tmp1, tmp1, lsl #1
    TSTNE val, tmp1                                     /* Test way #1. */
    MOVNE tmp1, tmp1, lsl #1
    TSTNE val, tmp1                                     /* Test way #2. */
    MOVNE tmp1, tmp1, lsl #1
    BNE   statusReturn                                  /* Way #3 is reserved for normal cache function. */

    CMP   len, #0                                       /* Check for data to lock into the cache. */
    BEQ   statusReturn

    MVN   tmp1, tmp1                                    /* Complement tmp1 for value with L set to 0 for target way. */
    AND   tmp1, tmp1, #0x0F                             /* Mask off non-L bits. */
    BIC   val, val, #0x0F                               /* Mask off L bits from register value. */
    ADD   val, val, tmp1                                /* Set the desired values to the L bits. */

    MCR   p15, 0, val, c0, c0, 1                        /* Update the L bits in cp15:c9 */

loadICacheLine
    MCR   p15, 0, addr, c7, c13, 1                      /* Load cache line into the I-cache. */
    ADD   addr, addr, #(1 << CYU3P_CACHE_LINE_SZ)
    SUBS  len, len, #1
    BNE   loadICacheLine

    MVN   tmp1, val                                     /* Complement current value of cp15:c9 */
    AND   tmp1, tmp1, #0x0F                             /* Mask off non-L bits. */
    ORR   tmp, tmp, tmp1                                /* Or with original L bits saved above. */
    BIC   val, val, #0x0F
    AND   val, val, tmp

    MCR   p15, 0, val, c9, c0, 1                        /* Update the lock status bits on cp15:c9 */

statusReturn
    MOV   r0, tmp1                                      /* Move status into r0. */
    BX    lr
}

/* Function to lock a block of data into the D-Cache.
 * Please ensure that the D-Cache has been cleaned and flushed before calling this function.
 */
__asm uint32_t
CyU3PSysCacheDRegion (
        uint32_t *addr,
        uint32_t  len)
{
addr RN 0
len  RN 1
tmp  RN 2
tmp1 RN 3
val  RN 12

    MOV   tmp1, #0
    CMP   len, #0                                       /* If len is 0, return straight away. */
    BEQ   dStatusReturn

    ADD   len, addr, len                                /* End address. */
    BIC   addr, addr, #((1 << CYU3P_CACHE_LINE_SZ) - 1) /* Cache line corresponding to start address. */
    MOV   tmp, #((1 << CYU3P_CACHE_LINE_SZ) - 1)        /* Temp mask for cache line. */
    TST   len, tmp                                      /* Check if the end address spills over a cache line. */
    SUB   len, len, addr                                /* Restore len. Does not affect flags set by the TST instrn. */
    MOV   len, len, lsr #CYU3P_CACHE_LINE_SZ            /* Reduce to number of cache lines. */
    ADDNE len, len, #1                                  /* Add 1 to number of cache lines if there is a residue. */
    CMP   len, #(1 << CYU3P_CACHE_WAY_SZ) - 1           /* Check if the region can fit in one way. */
    BHI   dStatusReturn

    MRC   p15, 0, val, c9, c0, 0                        /* Read I-Cache lock status. */
    AND   tmp, val, #0x0F                               /* Mask off bits other than lock status. */

    MOV   tmp1, #1
    TST   val, tmp1                                     /* Test way #0. */
    MOVNE tmp1, tmp1, lsl #1
    TSTNE val, tmp1                                     /* Test way #1. */
    MOVNE tmp1, tmp1, lsl #1
    TSTNE val, tmp1                                     /* Test way #2. */
    MOVNE tmp1, tmp1, lsl #1
    BNE   dStatusReturn                                 /* Way #3 is reserved for normal cache function. */

    CMP   len, #0                                       /* Check for data to lock into the cache. */
    BEQ   dStatusReturn

    MVN   tmp1, tmp1                                    /* Complement tmp1 for value with L set to 0 for target way. */
    AND   tmp1, tmp1, #0x0F                             /* Mask off non-L bits. */
    BIC   val, val, #0x0F                               /* Mask off L bits from register value. */
    ADD   val, val, tmp1                                /* Set the desired values to the L bits. */

    MCR   p15, 0, val, c0, c0, 0                        /* Update the L bits in cp15:c9 */

loadDCacheLine
    LDR   tmp1, [addr], #(1 << CYU3P_CACHE_LINE_SZ)     /* Load a word from the target cache line to trigger cache
                                                           loading. */
    SUBS  len, len, #1
    BNE   loadDCacheLine

    MVN   tmp1, val                                     /* Complement current value of cp15:c9 */
    AND   tmp1, tmp1, #0x0F                             /* Mask off non-L bits. */
    ORR   tmp, tmp, tmp1                                /* Or with original L bits saved above. */
    BIC   val, val, #0x0F
    AND   val, val, tmp

    MCR   p15, 0, val, c9, c0, 0                        /* Update the lock status bits on cp15:c9 */

dStatusReturn
    MOV   r0, tmp1                                      /* Move status into r0. */
    BX    lr
}

#else

/* Function to clean the D-cache.
 * Uses the test/clean command to do this.
 */
void
CyU3PSysCleanDCache (
        void)
{
    __asm__
        (
         "testclean:"
         "MRC p15, 0, pc, c7, c10, 3\n\t"
         "BNE testclean\n\t"
         :
         :
         :
        );
}

/* Function to clean and invalidate the D-Cache.
 * Uses the test/clean command to do this.
 */
void
CyU3PSysClearDCache (
        void)
{
    __asm__
        (
         "testcleanflush:"
         "MRC p15, 0, pc, c7, c14, 3\n\t"
         "BNE testcleanflush\n\t"
         :
         :
         :
        );
}

/* Function to remove a specific region of memory from the I-cache.
 */
void
CyU3PSysFlushIRegion (
        uint32_t *addr,
        uint32_t  len)
{
    uint32_t tmp;

    __asm__
        (
         "CMP   %1, #0\n\t"
         "BEQ   exitLabel1\n\t"                         /* If len is 0, return straight away. */

         "ADD   %1, %0, %1\n\t"                         /* Calculate the end address. */
         "BIC   %0, %0, #0x1F\n\t"                      /* Align the start address to a cache line. */
         "MOV   %2, #0x1F\n\t"                          /* Temp mask for cache line. */
         "TST   %1, %2\n\t"                             /* Check if the end address spills over a cache line. */
         "SUB   %1, %1, %0\n\t"                         /* Restore len. Does not affect flags set by the TST instrn. */
         "MOV   %1, %1, lsr #5\n\t"                     /* Reduce to number of cache lines. */
         "ADDNE %1, %1, #1\n\t"                         /* Add 1 to number of cache lines if there is a residue. */

         "flushIline:"
         "MCR p15, 0, %0, c7, c5, 1\n\t"                /* Flush one line from the I-Cache. */
         "ADD %0, %0, #0x20\n\t"                        /* Jump to the next cache line. */
         "SUBS %1, %1, #1\n\t"                          /* Check if we are done. */
         "BNE flushIline\n\t"                           /* Repeat until done. */

         "exitLabel1:"
         "MOV r0, r0\n\t"

         : "+r" (addr), "+r" (len), "+r" (tmp)
         :
         :
        );
}

/* Function to clean and flush a specific region of data memory from the D-cache.
 */
void
CyU3PSysClearDRegion (
        uint32_t *addr,
        uint32_t  len)
{
    uint32_t tmp;

    __asm__
        (
         "CMP   %1, #0\n\t"
         "BEQ   exitLabel2\n\t"                         /* If len is 0, return straight away. */

         "ADD   %1, %0, %1\n\t"                         /* Calculate the end address. */
         "BIC   %0, %0, #0x1F\n\t"                      /* Align the start address to a cache line. */
         "MOV   %2, #0x1F\n\t"                          /* Temp mask for cache line. */
         "TST   %1, %2\n\t"                             /* Check if the end address spills over a cache line. */
         "SUB   %1, %1, %0\n\t"                         /* Restore len. Does not affect flags set by the TST instrn. */
         "MOV   %1, %1, lsr #5\n\t"                     /* Reduce to number of cache lines. */
         "ADDNE %1, %1, #1\n\t"                         /* Add 1 to number of cache lines if there is a residue. */

         "clearDline:"
         "MCR p15, 0, %0, c7, c14, 1\n\t"               /* Clean and flush one line from the D-Cache. */
         "ADD %0, %0, #0x20\n\t"                        /* Jump to the next cache line. */
         "SUBS %1, %1, #1\n\t"                          /* Check if we are done. */
         "BNE clearDline\n\t"                           /* Repeat until done. */

         "exitLabel2:"
         "MOV r0, r0\n\t"

         : "+r" (addr), "+r" (len), "+r" (tmp)
         :
         :
        );
}

/* Function to clean a specific region of data memory from the D-cache.
 */
void
CyU3PSysCleanDRegion (
        uint32_t *addr,
        uint32_t  len)
{
    uint32_t tmp;

    __asm__
        (
         "CMP   %1, #0\n\t"
         "BEQ   exitLabel3\n\t"                         /* If len is 0, return straight away. */

         "ADD   %1, %0, %1\n\t"                         /* Calculate the end address. */
         "BIC   %0, %0, #0x1F\n\t"                      /* Align the start address to a cache line. */
         "MOV   %2, #0x1F\n\t"                          /* Temp mask for cache line. */
         "TST   %1, %2\n\t"                             /* Check if the end address spills over a cache line. */
         "SUB   %1, %1, %0\n\t"                         /* Restore len. Does not affect flags set by the TST instrn. */
         "MOV   %1, %1, lsr #5\n\t"                     /* Reduce to number of cache lines. */
         "ADDNE %1, %1, #1\n\t"                         /* Add 1 to number of cache lines if there is a residue. */

         "cleanDline:"
         "MCR p15, 0, %0, c7, c10, 1\n\t"               /* Clean and flush one line from the D-Cache. */
         "ADD %0, %0, #0x20\n\t"                        /* Jump to the next cache line. */
         "SUBS %1, %1, #1\n\t"                          /* Check if we are done. */
         "BNE cleanDline\n\t"                           /* Repeat until done. */

         "exitLabel3:"
         "MOV r0, r0\n\t"

         : "+r" (addr), "+r" (len), "+r" (tmp)
         :
         :
        );
}

/* Function to clean a specific region of data memory from the D-cache.
 */
void
CyU3PSysFlushDRegion (
        uint32_t *addr,
        uint32_t  len)
{
    uint32_t tmp;

    __asm__
        (
         "CMP   %1, #0\n\t"
         "BEQ   exitLabel4\n\t"                         /* If len is 0, return straight away. */

         "ADD   %1, %0, %1\n\t"                         /* Calculate the end address. */
         "BIC   %0, %0, #0x1F\n\t"                      /* Align the start address to a cache line. */
         "MOV   %2, #0x1F\n\t"                          /* Temp mask for cache line. */
         "TST   %1, %2\n\t"                             /* Check if the end address spills over a cache line. */
         "SUB   %1, %1, %0\n\t"                         /* Restore len. Does not affect flags set by the TST instrn. */
         "MOV   %1, %1, lsr #5\n\t"                     /* Reduce to number of cache lines. */
         "ADDNE %1, %1, #1\n\t"                         /* Add 1 to number of cache lines if there is a residue. */

         "flushDline:"
         "MCR p15, 0, %0, c7, c6, 1\n\t"                /* Clean and flush one line from the D-Cache. */
         "ADD %0, %0, #0x20\n\t"                        /* Jump to the next cache line. */
         "SUBS %1, %1, #1\n\t"                          /* Check if we are done. */
         "BNE flushDline\n\t"                           /* Repeat until done. */

         "exitLabel4:"
         "MOV r0, r0\n\t"

         : "+r" (addr), "+r" (len), "+r" (tmp)
         :
         :
        );
}

/* Function to lock a block of code into the I-Cache.
 * Please ensure that the I-Cache has been flushed before calling this function.
 */
uint32_t
CyU3PSysCacheIRegion (
        uint32_t *addr,
        uint32_t  len)
{
    uint32_t t1, t2;

    __asm__
        (
         "MOV   %3, #0\n\t"
         "CMP   %1, #0\n\t"
         "BEQ   statusReturn\n\t"                       /* If the region size is 0, then return. */

         "ADD   %1, %0, %1\n\t"                         /* Compute the end address. */
         "BIC   %0, %0, #0x1F\n\t"                      /* Align the start address to a cache line. */
         "MOV   %2, #0x1F\n\t"
         "TST   %1, %2\n\t"                             /* Test if the end address runs over a cache line. */
         "SUB   %1, %1, %0\n\t"                         /* Revert to original length value. */
         "MOV   %1, %1, lsr #5\n\t"                     /* Calculate number of cache lines. */
         "ADDNE %1, %1, #1\n\t"                         /* If there is a residue at the end, add one. */
         "CMP   %1, #0x3F\n\t"                          /* Check if the region can fit in one way. */
         "BHI   statusReturn\n\t"                       /* Error: Return. */

         "MRC   p15, 0, r12, c9, c0, 1\n\t"             /* Read I-Cache lock status. */
         "AND   %2, r12, #0x0F\n\t"                     /* Mask off bits other than lock status. */
         "MOV   %3, #1\n\t"
         "TST   r12, %3\n\t"                            /* Test way #0. */
         "MOVNE %3, %3, lsl #1\n\t"
         "TSTNE r12, %3\n\t"                            /* Test way #1. */
         "MOVNE %3, %3, lsl #1\n\t"
         "TSTNE r12, %3\n\t"                            /* Test way #2. */
         "MOVNE %3, %3, lsl #1\n\t"
         "BNE   statusReturn\n\t"                       /* Way #3 is reserved for normal cache function. */

         "CMP   %1, #0\n\t"                             /* Check for data to lock into the cache. */
         "BEQ   statusReturn\n\t"

         "MVN   %3, %3\n\t"                             /* Complement tmp1 for value with L set to 0 for target way. */
         "AND   %3, %3, #0x0F\n\t"                      /* Mask off non-L bits. */
         "BIC   r12, r12, #0x0F\n\t"                    /* Mask off L bits from register value. */
         "ADD   r12, r12, %3\n\t"                       /* Set the desired values to the L bits. */

         "MCR   p15, 0, r12, c0, c0, 1\n\t"             /* Update the L bits in cp15:c9 */

         "loadICacheLine:"
         "MCR   p15, 0, %0, c7, c13, 1\n\t"             /* Load cache line into the I-cache. */
         "ADD   %0, %0, #0x20\n\t"                      /* Move to the next cache line. */
         "SUBS  %1, %1, #1\n\t"                         /* Decrement number of cache lines. */
         "BNE   loadICacheLine\n\t"

         "MVN   %3, r12\n\t"                            /* Complement current value of cp15:c9 */
         "AND   %3, %3, #0x0F\n\t"                      /* Mask off non-L bits. */
         "ORR   %2, %2, %3\n\t"                         /* OR with original L bits saved above. */
         "BIC   r12, r12, #0x0F\n\t"
         "AND   r12, r12, %2\n\t"

         "MCR   p15, 0, r12, c9, c0, 1\n\t"             /* Update the lock status bits on cp15:c9 */

         "statusReturn:"
         "MOV   r0, %3\n\t"                             /* Move the status into r0. */

         : "+r" (addr), "+r" (len), "+r" (t1), "+r" (t2)
         :
         : "r12"
        );
}

/* Function to lock a block of data into the D-Cache.
 * Please ensure that the D-Cache has been cleaned and flushed before calling this function.
 */
uint32_t
CyU3PSysCacheDRegion (
        uint32_t *addr,
        uint32_t  len)
{
    uint32_t t1, t2;

    __asm__
        (
         "MOV   %3, #0\n\t"
         "CMP   %1, #0\n\t"
         "BEQ   dStatusReturn\n\t"                      /* If the region size is 0, then return. */

         "ADD   %1, %0, %1\n\t"                         /* Compute the end address. */
         "BIC   %0, %0, #0x1F\n\t"                      /* Align the start address to a cache line. */
         "MOV   %2, #0x1F\n\t"
         "TST   %1, %2\n\t"                             /* Test if the end address runs over a cache line. */
         "SUB   %1, %1, %0\n\t"                         /* Revert to original length value. */
         "MOV   %1, %1, lsr #5\n\t"                     /* Calculate number of cache lines. */
         "ADDNE %1, %1, #1\n\t"                         /* If there is a residue at the end, add one. */
         "CMP   %1, #0x3F\n\t"                          /* Check if the region can fit in one way. */
         "BHI   dStatusReturn\n\t"                      /* Error: Return. */

         "MRC   p15, 0, r12, c9, c0, 0\n\t"             /* Read D-Cache lock status. */
         "AND   %2, r12, #0x0F\n\t"                     /* Mask off bits other than lock status. */
         "MOV   %3, #1\n\t"
         "TST   r12, %3\n\t"                            /* Test way #0. */
         "MOVNE %3, %3, lsl #1\n\t"
         "TSTNE r12, %3\n\t"                            /* Test way #1. */
         "MOVNE %3, %3, lsl #1\n\t"
         "TSTNE r12, %3\n\t"                            /* Test way #2. */
         "MOVNE %3, %3, lsl #1\n\t"
         "BNE   dStatusReturn\n\t"                      /* Way #3 is reserved for normal cache function. */

         "CMP   %1, #0\n\t"                             /* Check for data to lock into the cache. */
         "BEQ   dStatusReturn\n\t"

         "MVN   %3, %3\n\t"                             /* Complement tmp1 for value with L set to 0 for target way. */
         "AND   %3, %3, #0x0F\n\t"                      /* Mask off non-L bits. */
         "BIC   r12, r12, #0x0F\n\t"                    /* Mask off L bits from register value. */
         "ADD   r12, r12, %3\n\t"                       /* Set the desired values to the L bits. */

         "MCR   p15, 0, r12, c0, c0, 0\n\t"             /* Update the L bits in cp15:c9 */

         "loadDCacheLine:"
         "LDR   %3, [%0], #0x20\n\t"                    /* Read a word of memory to cause the line to be cached. */
         "SUBS  %1, %1, #1\n\t"                         /* Decrement number of cache lines. */
         "BNE   loadDCacheLine\n\t"

         "MVN   %3, r12\n\t"                            /* Complement current value of cp15:c9 */
         "AND   %3, %3, #0x0F\n\t"                      /* Mask off non-L bits. */
         "ORR   %2, %2, %3\n\t"                         /* OR with original L bits saved above. */
         "BIC   r12, r12, #0x0F\n\t"
         "AND   r12, r12, %2\n\t"

         "MCR   p15, 0, r12, c9, c0, 0\n\t"             /* Update the lock status bits on cp15:c9 */

         "dStatusReturn:"
         "MOV r0, %3\n\t"

         : "+r" (addr), "+r" (len), "+r" (t1), "+r" (t2)
         :
         : "r12"
        );
}

#endif

void
CyU3PSysBarrierSync (
        void)
{
    uint32_t val = 0;

#ifdef CY_USE_ARMCC
    __asm
    {
        MCR p15, 0, val, c7, c10, 4;
    }
#else
    __asm__
        (
         "MCR   p15, 0, %0, c7, c10, 4\n\t"
         :
         : "r" (val)
         :
        );
#endif
}

void
CyU3PSysInitTCMs (
        void)
{
    uint32_t val;
   
    val = ((CYU3P_ITCM_BASE_ADDR & CYU3P_TCMREG_ADDRESS_MASK) | CYU3P_ITCM_SZ_EN);

#ifdef CY_USE_ARMCC
    __asm
    {
        MCR p15, 0, val, c9, c1, 1;
    }
#else
    __asm__
        (
         "MCR   p15, 0, %0, c9, c1, 1\n\t"
         :
         : "r" (val)
         :
        );
#endif

    val = ((CYU3P_DTCM_BASE_ADDR & CYU3P_TCMREG_ADDRESS_MASK) | CYU3P_DTCM_SZ_EN);

#ifdef CY_USE_ARMCC
    __asm
    {
        MCR p15, 0, val, c9, c1, 0;
    }
#else
    __asm__
        (
         "MCR   p15, 0, %0, c9, c1, 0\n\t"
         :
         : "r" (val)
         :
        );
#endif
}

#ifdef CY_USE_ARMCC

/* Function to unlock all I-cache entries.
 */
__asm void
CyU3PSysUnlockICache (
        void)
{
    MRC p15, 0, r0, c9, c0, 1
    BIC r0, r0, #0x0F                                   /* Clear all lock bits. */
    MCR p15, 0, r0, c9, c0, 1
}

/* Function to unlock all D-cache entries.
 */
__asm void
CyU3PSysUnlockDCache (
        void)
{
    MRC p15, 0, r0, c9, c0, 0
    BIC r0, r0, #0x0F                                   /* Clear all lock bits. */
    MCR p15, 0, r0, c9, c0, 0
}


#else

/* Function to unlock all I-cache entries.
 */
void
CyU3PSysUnlockICache (
        void)
{
    uint32_t tmp;

    __asm__
        (
         "MRC   p15, 0, %0, c9, c0, 1\n\t"
         "BIC   %0, %0, #0x0F\n\t"                      /* Clear all lock bits. */
         "MCR   p15, 0, %0, c9, c0, 1\n\t"
         : "+r" (tmp)
         :
         :
        );
}

/* Function to unlock all D-cache entries.
 */
void
CyU3PSysUnlockDCache (
        void)
{
    uint32_t tmp;

    __asm__
        (
         "MRC   p15, 0, %0, c9, c0, 0\n\t"
         "BIC   %0, %0, #0x0F\n\t"                      /* Clear all lock bits. */
         "MCR   p15, 0, %0, c9, c0, 0\n\t"
         : "+r" (tmp)
         :
         :
        );
}

#endif

void
CyU3PSysLockTLBEntry (
        uint32_t *addr)
{
    uint32_t val = 0;

    /* Lock the section entry for the ITCM into the TLB. */
#ifdef CY_USE_ARMCC
    __asm
    {
        MCR p15, 0, addr, c8, c7, 1;            /* Invalidate any existing entries for this address. */
        MRC p15, 0, val, c10, c0, 0;            /* Read the TLB lock-down register. */
        ORR val, val, #0x01;                    /* Set the preserve bit. */
        MCR p15, 0, val, c10, c0, 0;            /* Write to the TLB lock-down register. Next page table
                                                   walk places the result into the lock-down region of TLB. */
        LDR addr, [addr];                       /* Dummy load from the target address. */
        MRC p15, 0, val, c10, c0, 0;            /* Read the TLB lock-down register again. */
        BIC val, val, #0x01;                    /* Clear the preserve bit. */
        MCR p15, 0, val, c10, c0, 0;            /* Write back to the lock-down register. */
    }
#else
    __asm__
        (
         "MCR p15, 0, %0, c8, c7, 1\n\t"        /* Invalidate any existing entries for this address. */
         "MRC p15, 0, %1, c10, c0, 0\n\t"       /* Read the TLB lock-down register. */
         "ORR %1, %1, #0x01\n\t"                /* Set the preserve bit. */
         "MCR p15, 0, %1, c10, c0, 0\n\t"       /* Write to the TLB lock-down register. Next page table
                                                   walk places the result into the lock-down region of TLB. */
         "LDR %0, [%0]\n\t"                     /* Dummy load from the target address. */
         "MRC p15, 0, %1, c10, c0, 0\n\t"       /* Read the TLB lock-down register again. */
         "BIC %1, %1, #0x01\n\t"                /* Clear the preserve bit. */
         "MCR p15, 0, %1, c10, c0, 0\n\t"       /* Write back to the lock-down register. */
         : "+r" (addr), "+r" (val)
         :
         :
        );
#endif
}

/* Function to invalidate the TLB entry corresponding to the specified address. */
void
CyU3PSysFlushTLBEntry (
        uint32_t *addr)
{
#ifdef CY_USE_ARMCC
    __asm
    {
        MCR p15, 0, addr, c8, c7, 1;            /* Invalidate the TLB entry. */
    }
#else
    __asm__
        (
         "MCR   p15, 0, %0, c8, c7, 1\n\t"
         : "+r" (addr)
         :
         :
        );
#endif
}

/* Function to load page table maps for all valid entries into the TLB and lock them. */
void
CyU3PSysLoadTLB (
        void)
{
    CyU3PSysLockTLBEntry ((uint32_t *)CYU3P_ITCM_BASE_ADDR);
    CyU3PSysLockTLBEntry ((uint32_t *)CYU3P_DTCM_BASE_ADDR);
    CyU3PSysLockTLBEntry ((uint32_t *)CYU3P_SYSMEM_BASE_ADDR);
    CyU3PSysLockTLBEntry ((uint32_t *)CYU3P_MMIO_BASE_ADDR);
    CyU3PSysLockTLBEntry ((uint32_t *)CYU3P_ROM_BASE_ADDR);
    CyU3PSysLockTLBEntry ((uint32_t *)CYU3P_VIC_BASE_ADDR);
}

void
CyU3PInitPageTable (
        void)
{
    CyFx3DevInitPageTables ();
}

/*[]*/

