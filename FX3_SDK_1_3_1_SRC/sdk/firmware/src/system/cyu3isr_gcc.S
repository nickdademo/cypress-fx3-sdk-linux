/*
  Cypress FX3 Interrupt Service Routines (cyu3isr.S)
  ## =====================================================
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
  ## =======================================================
 */

#define CY_U3P_EXCEPTION_VECTOR_LOCATION        0x00000000
#define CY_U3P_VIC_VECT_ADDRESS_BASE            0xFFFFF100

#define CY_U3P_VIC_ADDRESS_ADDRESS              0xffffff00
#define CY_U3P_VIC_INT_CLEAR_ADDRESS            0xfffff014
#define CY_U3P_VIC_INT_ENABLE_ADDRESS           0xfffff010

#define CY_U3P_VIC_GCTLCORE_MASK                0x00000001
#define CY_U3P_VIC_SWINTR_MASK                  0x00000002
#define CY_U3P_VIC_WD_TIMER_MASK                0x00000010
#define CY_U3P_VIC_PVT_TIMER_MASK               0x00000020
#define CY_U3P_VIC_PIB_DMA_MASK                 0x00000040
#define CY_U3P_VIC_PIB_CORE_MASK                0x00000080
#define CY_U3P_VIC_USB_DMA_MASK                 0x00000100
#define CY_U3P_VIC_USB_CORE_MASK                0x00000200
#define CY_U3P_VIC_USB_CTRL_MASK                0x00000400
#define CY_U3P_VIC_SIB_DMA_MASK                 0x00000800
#define CY_U3P_VIC_SIB0_CORE_MASK               0x00001000
#define CY_U3P_VIC_SIB1_CORE_MASK               0x00002000
#define CY_U3P_VIC_I2C_MASK                     0x00008000
#define CY_U3P_VIC_I2S_MASK                     0x00010000
#define CY_U3P_VIC_SPI_MASK                     0x00020000
#define CY_U3P_VIC_UART_MASK                    0x00040000
#define CY_U3P_VIC_GPIO_MASK                    0x00080000
#define CY_U3P_VIC_LPP_DMA_MASK                 0x00100000
#define CY_U3P_VIC_POWER_MASK                   0x00200000

/* Interrupt functions replicated from cyu3tx.h file. */
#define CyU3PIrqContextSave             _tx_thread_context_save
#define CyU3PIrqVectoredContextSave     _tx_thread_vectored_context_save
#define CyU3PIrqContextRestore          _tx_thread_context_restore
#define CyU3PIrqNestingStart            _tx_thread_irq_nesting_start
#define CyU3PIrqNestingStop             _tx_thread_irq_nesting_end

.extern __cyu3p_exception_vectors
.extern CyU3PIrqVectoredContextSave
.extern CyU3PIrqContextRestore
.extern CyU3PIrqNestingStart
.extern CyU3PIrqNestingStop

.section "CYU3P_ITCM_SECTION"
.code 32

/* Register the ISR addresses with the VIC. */
.global CyU3PVicSetupIntVectors
CyU3PVicSetupIntVectors:
    /* Copy the ARM exception vector table into its target location at the top of the I-TCM. */
    stmdb   sp!, {r0-r9, lr}                            /* Save registers onto the stack. */
    ldr     r0, =__cyu3p_exception_vectors              /* Address containing original vectors. */
    ldr     r1, =CY_U3P_EXCEPTION_VECTOR_LOCATION       /* Target address to copy the vectors to. */
    ldmia   r0!, {r2-r9}                                /* Read and save 256 bytes of code from this location. */
    stmia   r1!, {r2-r9}
    ldmia   r0!, {r2-r9}
    stmia   r1!, {r2-r9}
    ldmia   r0!, {r2-r9}
    stmia   r1!, {r2-r9}
    ldmia   r0!, {r2-r9}
    stmia   r1!, {r2-r9}

    ldr     r0, =CY_U3P_VIC_VECT_ADDRESS_BASE
    ldr     r1, =__CyU3PGCTLCoreIntr
    ldr     r2, =__CyU3PSWIIntr
    ldr     r3, =__CyU3PDummyIsr
    ldr     r4, =__CyU3PDummyIsr
    ldr     r5, =__CyU3PWDTIntr
    ldr     r6, =__CyU3PDummyIsr
    ldr     r7, =__CyU3PPportDmaIntr
    ldr     r8, =__CyU3PPportCoreIntr
    stmia   r0!, {r1-r8}                        /* Store the ISR vector addresses into the VIC. */

    ldr     r1, =__CyU3PUsbDmaIntr
    ldr     r2, =__CyU3PUsbCoreIntr
    ldr     r3, =__CyU3PDummyIsr
    ldr     r4, =__CyU3PSibDmaIntr
    ldr     r5, =__CyU3PS0CoreIntr
    ldr     r6, =__CyU3PS1CoreIntr
    ldr     r7, =__CyU3PDummyIsr
    ldr     r8, =__CyU3PI2CCoreIntr
    stmia   r0!, {r1-r8}                        /* Store the ISR vector addresses into the VIC. */

    ldr     r1, =__CyU3PI2SCoreIntr
    ldr     r2, =__CyU3PSpiCoreIntr
    ldr     r3, =__CyU3PUartCoreIntr
    ldr     r4, =__CyU3PGpioCoreIntr
    ldr     r5, =__CyU3PLppDmaIntr
    ldr     r6, =__CyU3PGCTLPowerIntr
    ldr     r7, =__CyU3PDummyIsr
    ldr     r8, =__CyU3PDummyIsr
    stmia   r0!, {r1-r6}                        /* Store the ISR vector addresses into the VIC. */

    ldmia   sp!, {r0-r9, pc}                    /* Restore register contents. */

/* Actual handler for GCTL Core Interrupt */
.global __CyU3PGCTLCoreIntr
__CyU3PGCTLCoreIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_GCTLCORE_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PGctlCoreIntHandler
    bl      CyU3PIrqNestingStop

    ldr     r1, =CY_U3P_VIC_INT_ENABLE_ADDRESS
    ldr     r2, =CY_U3P_VIC_GCTLCORE_MASK
    str     r2, [r1]

    b       CyU3PIrqContextRestore

/* Actual handler for SWI Interrupt */
.global __CyU3PSWIIntr
__CyU3PSWIIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_SWINTR_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PSwiIntHandler
    bl      CyU3PIrqNestingStop

    ldr     r1, =CY_U3P_VIC_INT_ENABLE_ADDRESS
    ldr     r2, =CY_U3P_VIC_SWINTR_MASK
    str     r2, [r1]

    b       CyU3PIrqContextRestore

/* Actual handler for GCTL power interrupt */
.global __CyU3PGCTLPowerIntr
__CyU3PGCTLPowerIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_POWER_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PGctlPowerIntHandler
    bl      CyU3PIrqNestingStop

    ldr     r1, =CY_U3P_VIC_INT_ENABLE_ADDRESS
    ldr     r2, =CY_U3P_VIC_POWER_MASK
    str     r2, [r1]

    b       CyU3PIrqContextRestore

/* Actual handler for WDT */
.global __CyU3PWDTIntr
__CyU3PWDTIntr:
    stmdb   sp!, {r0-r3}                        /*  Save some scratch registers */
    mrs     r0, SPSR                            /*  Pickup saved SPSR */
    sub     lr, lr, #4                          /*  Adjust point of interrupt */
    stmdb   sp!, {r0, r10, r12, lr}             /*  Store other scratch registers */
    bl      CyU3PIrqVectoredContextSave         /*  Vectored context save */

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_WD_TIMER_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PWDTIntHandler                  /*  Actual interrupt handler */
    bl      CyU3PIrqNestingStop

    ldr     r1, =CY_U3P_VIC_INT_ENABLE_ADDRESS
    ldr     r2, =CY_U3P_VIC_WD_TIMER_MASK
    str     r2, [r1]

    b       CyU3PIrqContextRestore              /*  Restore context */

/* Actual handler for PIB DMA interrupt */
.global __CyU3PPportDmaIntr
__CyU3PPportDmaIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_PIB_DMA_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PPportDmaIntHandler
    bl      CyU3PIrqNestingStop

    ldr     r1, =CY_U3P_VIC_INT_ENABLE_ADDRESS
    ldr     r2, =CY_U3P_VIC_PIB_DMA_MASK
    str     r2, [r1]

    b       CyU3PIrqContextRestore

/* Actual handler for PIB Core interrupt */
.global __CyU3PPportCoreIntr
__CyU3PPportCoreIntr:
    stmdb   sp!, {r0-r3}                        /*  Save some scratch registers */
    mrs     r0, SPSR                            /*  Pickup saved SPSR */
    sub     lr, lr, #4                          /*  Adjust point of interrupt */
    stmdb   sp!, {r0, r10, r12, lr}             /*  Store other scratch registers */
    bl      CyU3PIrqVectoredContextSave         /*  Vectored context save */

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_PIB_CORE_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PPibIntHandler                  /*  Actual interrupt handler */
    bl      CyU3PIrqNestingStop

    ldr     r1, =CY_U3P_VIC_INT_ENABLE_ADDRESS
    ldr     r2, =CY_U3P_VIC_PIB_CORE_MASK
    str     r2, [r1]

    b       CyU3PIrqContextRestore              /*  Restore context */

/* Non pre-emptable handler for PIB core interrupts. */
.global __CyU3PPportCoreIntrHP
__CyU3PPportCoreIntrHP:
    stmdb   sp!, {r0-r3}                        /*  Save some scratch registers */
    mrs     r0, SPSR                            /*  Pickup saved SPSR */
    sub     lr, lr, #4                          /*  Adjust point of interrupt */
    stmdb   sp!, {r0, r10, r12, lr}             /*  Store other scratch registers */

    bl      CyU3PIrqVectoredContextSave         /*  Vectored context save */
    bl      CyU3PPibIntHandler                  /*  Actual interrupt handler */
    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]                            /*  Clear the interrupt. */
    b       CyU3PIrqContextRestore              /*  Restore context */

/* The USB core interrupt does not allow other interrupts to pre-empt it. */
.global __CyU3PUsbCoreIntr
__CyU3PUsbCoreIntr:
    stmdb   sp!, {r0-r3}                        /*  Save some scratch registers */
    mrs     r0, SPSR                            /*  Pickup saved SPSR */
    sub     lr, lr, #4                          /*  Adjust point of interrupt */
    stmdb   sp!, {r0, r10, r12, lr}             /*  Store other scratch registers */

    bl      CyU3PIrqVectoredContextSave         /*  Vectored context save */
    bl      CyU3PUibIntHandler                  /*  Actual interrupt handler */
    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]                            /*  Clear the interrupt. */
    b       CyU3PIrqContextRestore              /*  Restore context */

/* Actual handler for UIB DMA interrupt */
.global __CyU3PUsbDmaIntr
__CyU3PUsbDmaIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_USB_DMA_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PUsbDmaIntHandler
    bl      CyU3PIrqNestingStop

    ldr     r1, =CY_U3P_VIC_INT_ENABLE_ADDRESS
    ldr     r2, =CY_U3P_VIC_USB_DMA_MASK
    str     r2, [r1]

    b       CyU3PIrqContextRestore

/* Actual handler for SIB DMA interrupt */
.global __CyU3PSibDmaIntr
__CyU3PSibDmaIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_SIB_DMA_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PSibDmaIntHandler
    bl      CyU3PIrqNestingStop

    ldr     r1, =CY_U3P_VIC_INT_ENABLE_ADDRESS
    ldr     r2, =CY_U3P_VIC_SIB_DMA_MASK
    str     r2, [r1]

    b       CyU3PIrqContextRestore

/* SIB0 Core Interrupt. */
.global __CyU3PS0CoreIntr
__CyU3PS0CoreIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_SIB0_CORE_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PSib0IntHandler         /* Interrupt will only be re-enabled later on. */
    bl      CyU3PIrqNestingStop
    b       CyU3PIrqContextRestore

/* SIB1 Core Interrupt. */
.global __CyU3PS1CoreIntr
__CyU3PS1CoreIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_SIB1_CORE_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PSib1IntHandler         /* Interrupt will only be re-enabled later on. */
    bl      CyU3PIrqNestingStop
    b       CyU3PIrqContextRestore

/* I2C core interrupt */
.global __CyU3PI2CCoreIntr
__CyU3PI2CCoreIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_I2C_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PI2cIntHandler          /* Interrupt will only be re-enabled later on. */
    bl      CyU3PIrqNestingStop
    b       CyU3PIrqContextRestore

/* I2S core interrupt */
.global __CyU3PI2SCoreIntr
__CyU3PI2SCoreIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_I2S_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PI2sIntHandler
    bl      CyU3PIrqNestingStop
    b       CyU3PIrqContextRestore

/* SPI core interrupt */
.global __CyU3PSpiCoreIntr
__CyU3PSpiCoreIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_SPI_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PSpiIntHandler
    bl      CyU3PIrqNestingStop
    b       CyU3PIrqContextRestore

/* UART core interrupt */
.global __CyU3PUartCoreIntr
__CyU3PUartCoreIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_UART_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PUartIntHandler
    bl      CyU3PIrqNestingStop

    b       CyU3PIrqContextRestore

/* Actual handler for GPIO core interrupt */
.global __CyU3PGpioCoreIntr
__CyU3PGpioCoreIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_GPIO_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PGpioCoreIntHandler
    bl      CyU3PIrqNestingStop

    ldr     r1, =CY_U3P_VIC_INT_ENABLE_ADDRESS
    ldr     r2, =CY_U3P_VIC_GPIO_MASK
    str     r2, [r1]
    b       CyU3PIrqContextRestore

/* Actual handler for LPP DMA interrupt */
.global __CyU3PLppDmaIntr
__CyU3PLppDmaIntr:
    stmdb   sp!, {r0-r3}
    mrs     r0, SPSR
    sub     lr, lr, #4
    stmdb   sp!, {r0, r10, r12, lr}
    bl      CyU3PIrqVectoredContextSave

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_LPP_DMA_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PLppDmaIntHandler
    bl      CyU3PIrqNestingStop

    ldr     r1, =CY_U3P_VIC_INT_ENABLE_ADDRESS
    ldr     r2, =CY_U3P_VIC_LPP_DMA_MASK
    str     r2, [r1]

    b       CyU3PIrqContextRestore

.global __CyU3PDummyIsr
__CyU3PDummyIsr:
    b       __CyU3PDummyIsr

.end

