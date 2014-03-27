/*
  Cypress FX3 2-Stage Bootloader Startup code (cyfx3startup.S)
  ## =====================================================
  ##
  ##  Copyright Cypress Semiconductor Corporation, 2010-2012,
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
  ## =======================================================
 */

#define ITCM_SETUP      0x00000015      /* ITCM_BASE = 0x00000000, size = 16K, enable */
#define DTCM_SETUP      0x10000011      /* DTCM_BASE = 0x10000000, size = 8K Enable */
#define VIC_ADDRESS     0xFFFFFF00      /* VIC VECTOR */
#define INT_CLEAR       0xFFFFF014
#define ITCM_BASE       0x00000000

/* Stack size definitions */
#define FX3_STACK_BASE  0x10000000      /* DTCM base address */
#define SVC_STACK_LEN   0x0200          /* 512 bytes */
#define FIQ_STACK_LEN   0x0400          /* 1 KB */
#define IRQ_STACK_LEN   0x0100          /* 256 bytes */
#define ABT_STACK_LEN   0x0080          /* 128 bytes */
#define UIM_STACK_LEN   0x0080          /* 128 bytes */
#define SYS_STACK_LEN   0x1800          /* 6 KB */

#define ICACHE_EN_BIT          0x1000
#define BOOTER_SIG_ADDRESS     0x40002000
#define REENUM_CONTROL_ADDRESS 0x40002FFC
#define REENUM_CONTROL_VALUE   0xB0

#define SVC_MODE 0x13
#define SYS_MODE 0x1F
#define IRQ_MODE 0x12
#define FIQ_MODE 0x11
#define ABT_MODE 0xD7   /* IRQ and FIQ disabled. */
#define UIM_MODE 0xDB   /* IRQ and FIQ disabled. */

.extern usb_isr
.extern main
.extern __main

.global __user_initial_stackheap
.global Reset_Handler
.global jump

.section "RESET"
.code 32

/* Interrupt vectors. */
Reset_Handler:
    ldr  pc, ResetVEC
    ldr  pc, UndefinedVEC
    ldr  pc, SWI_VEC
    ldr  pc, PrefetchVEC
    ldr  pc, DataAbortVEC
    ldr  pc, ReservedVEC
    ldr  pc, [pc, #-0x120]      /* Vector from VicVectAddr for PL192 */
    ldr  pc, FIQ_VEC

ResetVEC:
.word ResetInit

UndefinedVEC:
.word UndefinedISR

SWI_VEC:
.word SwiISR

PrefetchVEC:
.word PrefetchISR

DataAbortVEC:
.word AbortISR

ReservedVEC:
.word ReservedISR

IRQ_VEC:
.word 0

FIQ_VEC:
.word usb_isr

SwiISR:
UndefinedISR:
ReservedISR:
PrefetchISR:
AbortISR:
    b   AbortISR

ResetInit:
/* Invalidate TLBs */
    mov  r0,  #0
    mcr  p15, 0, r0, c8, c7, 0  /* Invalidate both caches */
    mcr  p15, 0, r0, c7, c7, 0
    mrc  p15, 0, r0, c1, c0, 0  /* Read Control Reg */
    bic  r0, r0, #1             /* Disable MMU */
    bic  r0, r0, #(1<<13)        
    orr  r0, r0, #ICACHE_EN_BIT /* Enable instruction cache */
    mcr  p15, 0, r0, c1, c0, 0  /* Write Control Reg */

    ldr  r0,=DTCM_SETUP
    mcr  p15, 0, r0, c9, c1, 0  /* Write DTCM */
    ldr  r0,=ITCM_SETUP
    mcr  p15, 0, r0, c9, c1, 1  /* Write ITCM */

/* Initialize the stacks */
    ldr  R1, =FX3_STACK_BASE    /* Stack base address = DTCM */
    sub  R1, R1, #8             /* Prevent overflow at the top of the DTCM */

    msr  CPSR_c, #SVC_MODE      /* Supervisor (SVC) mode stack */
    ldr  R2, =SVC_STACK_LEN
    add  R1, R1, R2
    bic  R1, R1, #7
    mov  SP, R1

    msr  CPSR_c, #IRQ_MODE      /* Interrupt (IRQ) mode stack */
    ldr  R2, =IRQ_STACK_LEN
    add  R1, R1, R2
    bic  R1, R1, #7
    mov  SP, R1

    msr  CPSR_c, #FIQ_MODE      /* Fast Interrupt (FIQ) mode stack */
    ldr  R2, =FIQ_STACK_LEN
    add  R1, R1, R2
    bic  R1, R1, #7
    mov  SP, R1

    msr  CPSR_c, #ABT_MODE
    ldr  R2, =ABT_STACK_LEN
    add  R1, R1, R2
    bic  R1, R1, #7
    mov  SP, R1

    msr  CPSR_c, #UIM_MODE
    ldr  R2, =UIM_STACK_LEN
    add  R1, R1, R2
    bic  R1, R1, #7
    mov  SP, R1

    msr  CPSR_c, #SYS_MODE      /* System (SYS) mode stack */
    ldr  R2, =SYS_STACK_LEN
    add  R1, R1, R2
    bic  R1, R1, #7
    mov  SP, R1

    ldr     r8, =ITCM_BASE
    ldr     r9, =Reset_Handler
    ldmia   r9!, {r0-r7}
    stmia   r8!, {r0-r7}
    ldmia   r9!, {r0-r7}
    stmia   r8!, {r0-r7}

    ldmia   r9!, {r0-r1}
    stmia   r8!, {r0-r1}

    ldr     r0, =INT_CLEAR      /* Build address of IRQ status register */
    mov     r1, #0xFFFFFFFF     /* Clear all interrupts */
    str     r1, [r0]

    b       __main              /* Jump to main in c */

jump:
    bx      r0

.end

/*[]*/
