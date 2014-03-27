;  Cypress FX3 Interrupt Service Routines (cyu3isr_armcc.S)
;  ## =====================================================
;  ##
;  ##  Copyright Cypress Semiconductor Corporation, 2010-2013,
;  ##  All Rights Reserved
;  ##  UNPUBLISHED, LICENSED SOFTWARE.
;  ##
;  ##  CONFIDENTIAL AND PROPRIETARY INFORMATION
;  ##  WHICH IS THE PROPERTY OF CYPRESS.
;  ##
;  ##  Use of this file is governed
;  ##  by the license agreement included in the file
;  ##
;  ##     <install>/license/license.txt
;  ##
;  ##  where <install> is the Cypress software
;  ##  installation root directory path.
;  ##
;  ## =======================================================

CY_U3P_EXCEPTION_VECTOR_LOCATION        EQU 0x00000000
CY_U3P_VIC_VECT_ADDRESS_BASE            EQU 0xFFFFF100

CY_U3P_VIC_ADDRESS_ADDRESS              EQU 0xffffff00
CY_U3P_VIC_INT_CLEAR_ADDRESS            EQU 0xfffff014
CY_U3P_VIC_INT_ENABLE_ADDRESS           EQU 0xfffff010

CY_U3P_VIC_GCTLCORE_MASK                EQU 0x00000001
CY_U3P_VIC_SWINTR_MASK                  EQU 0x00000002
CY_U3P_VIC_WD_TIMER_MASK                EQU 0x00000010
CY_U3P_VIC_PVT_TIMER_MASK               EQU 0x00000020
CY_U3P_VIC_PIB_DMA_MASK                 EQU 0x00000040
CY_U3P_VIC_PIB_CORE_MASK                EQU 0x00000080
CY_U3P_VIC_USB_DMA_MASK                 EQU 0x00000100
CY_U3P_VIC_USB_CORE_MASK                EQU 0x00000200
CY_U3P_VIC_USB_CTRL_MASK                EQU 0x00000400
CY_U3P_VIC_SIB_DMA_MASK                 EQU 0x00000800
CY_U3P_VIC_SIB0_CORE_MASK               EQU 0x00001000
CY_U3P_VIC_SIB1_CORE_MASK               EQU 0x00002000
CY_U3P_VIC_I2C_MASK                     EQU 0x00008000
CY_U3P_VIC_I2S_MASK                     EQU 0x00010000
CY_U3P_VIC_SPI_MASK                     EQU 0x00020000
CY_U3P_VIC_UART_MASK                    EQU 0x00040000
CY_U3P_VIC_GPIO_MASK                    EQU 0x00080000
CY_U3P_VIC_LPP_DMA_MASK                 EQU 0x00100000
CY_U3P_VIC_POWER_MASK                   EQU 0x00200000

;  Interrupt functions replicated from cyu3tx.h file. 
#define CyU3PIrqContextSave             _tx_thread_context_save
#define CyU3PIrqVectoredContextSave     _tx_thread_vectored_context_save
#define CyU3PIrqContextRestore          _tx_thread_context_restore
#define CyU3PIrqNestingStart            _tx_thread_irq_nesting_start
#define CyU3PIrqNestingStop             _tx_thread_irq_nesting_end

    IMPORT __cyu3p_exception_vectors
    IMPORT CyU3PIrqVectoredContextSave
    IMPORT CyU3PIrqContextRestore
    IMPORT CyU3PIrqNestingStart
    IMPORT CyU3PIrqNestingStop

    IMPORT CyU3PGctlCoreIntHandler
    IMPORT CyU3PSwiIntHandler
    IMPORT CyU3PGctlPowerIntHandler
    IMPORT CyU3PWDTIntHandler
    IMPORT CyU3PPportDmaIntHandler
    IMPORT CyU3PPibIntHandler
    IMPORT CyU3PPibIntHandler
    IMPORT CyU3PUibIntHandler
    IMPORT CyU3PUsbDmaIntHandler
    IMPORT CyU3PSibDmaIntHandler
    IMPORT CyU3PSib0IntHandler
    IMPORT CyU3PSib1IntHandler
    IMPORT CyU3PI2cIntHandler
    IMPORT CyU3PI2sIntHandler
    IMPORT CyU3PSpiIntHandler
    IMPORT CyU3PUartIntHandler
    IMPORT CyU3PGpioCoreIntHandler
    IMPORT CyU3PLppDmaIntHandler

    PRESERVE8
    AREA CYU3P_ITCM_SECTION, CODE, READONLY

;  Actual handler for GCTL Core Interrupt 
    EXPORT __CyU3PGCTLCoreIntr
__CyU3PGCTLCoreIntr
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

;  Actual handler for SWI Interrupt 
    EXPORT __CyU3PSWIIntr
__CyU3PSWIIntr
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

;  Actual handler for GCTL power interrupt 
    EXPORT __CyU3PGCTLPowerIntr
__CyU3PGCTLPowerIntr
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

;  Actual handler for WDT 
    EXPORT __CyU3PWDTIntr
__CyU3PWDTIntr
    stmdb   sp!, {r0-r3}                        ;   Save some scratch registers 
    mrs     r0, SPSR                            ;   Pickup saved SPSR 
    sub     lr, lr, #4                          ;   Adjust point of interrupt 
    stmdb   sp!, {r0, r10, r12, lr}             ;   Store other scratch registers 
    bl      CyU3PIrqVectoredContextSave         ;   Vectored context save 

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_WD_TIMER_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PWDTIntHandler                  ;   Actual interrupt handler 
    bl      CyU3PIrqNestingStop

    ldr     r1, =CY_U3P_VIC_INT_ENABLE_ADDRESS
    ldr     r2, =CY_U3P_VIC_WD_TIMER_MASK
    str     r2, [r1]

    b       CyU3PIrqContextRestore              ;   Restore context 

;  Actual handler for PIB DMA interrupt 
    EXPORT __CyU3PPportDmaIntr
__CyU3PPportDmaIntr
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

;  Actual handler for PIB Core interrupt 
    EXPORT __CyU3PPportCoreIntr
__CyU3PPportCoreIntr
    stmdb   sp!, {r0-r3}                        ;   Save some scratch registers 
    mrs     r0, SPSR                            ;   Pickup saved SPSR 
    sub     lr, lr, #4                          ;   Adjust point of interrupt 
    stmdb   sp!, {r0, r10, r12, lr}             ;   Store other scratch registers 
    bl      CyU3PIrqVectoredContextSave         ;   Vectored context save 

    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]
    ldr     r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS
    ldr     r2, =CY_U3P_VIC_PIB_CORE_MASK
    str     r2, [r1]

    bl      CyU3PIrqNestingStart
    bl      CyU3PPibIntHandler                  ;   Actual interrupt handler 
    bl      CyU3PIrqNestingStop

    ldr     r1, =CY_U3P_VIC_INT_ENABLE_ADDRESS
    ldr     r2, =CY_U3P_VIC_PIB_CORE_MASK
    str     r2, [r1]

    b       CyU3PIrqContextRestore              ;   Restore context 

;  Non pre-emptable handler for PIB core interrupts. 
    EXPORT __CyU3PPportCoreIntrHP
__CyU3PPportCoreIntrHP
    stmdb   sp!, {r0-r3}                        ;   Save some scratch registers 
    mrs     r0, SPSR                            ;   Pickup saved SPSR 
    sub     lr, lr, #4                          ;   Adjust point of interrupt 
    stmdb   sp!, {r0, r10, r12, lr}             ;   Store other scratch registers 

    bl      CyU3PIrqVectoredContextSave         ;   Vectored context save 
    bl      CyU3PPibIntHandler                  ;   Actual interrupt handler 
    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]                            ;   Clear the interrupt. 
    b       CyU3PIrqContextRestore              ;   Restore context 

;  The USB core interrupt does not allow other interrupts to pre-empt it. 
    EXPORT __CyU3PUsbCoreIntr
__CyU3PUsbCoreIntr
    stmdb   sp!, {r0-r3}                        ;   Save some scratch registers 
    mrs     r0, SPSR                            ;   Pickup saved SPSR 
    sub     lr, lr, #4                          ;   Adjust point of interrupt 
    stmdb   sp!, {r0, r10, r12, lr}             ;   Store other scratch registers 

    bl      CyU3PIrqVectoredContextSave         ;   Vectored context save 
    bl      CyU3PUibIntHandler                  ;   Actual interrupt handler 
    ldr     r1, =CY_U3P_VIC_ADDRESS_ADDRESS
    mov     r2, #0
    str     r2, [r1]                            ;   Clear the interrupt. 
    b       CyU3PIrqContextRestore              ;   Restore context 

;  Actual handler for UIB DMA interrupt 
    EXPORT __CyU3PUsbDmaIntr
__CyU3PUsbDmaIntr
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

;  Actual handler for SIB DMA interrupt 
    EXPORT __CyU3PSibDmaIntr
__CyU3PSibDmaIntr
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

;  SIB0 Core Interrupt. 
    EXPORT __CyU3PS0CoreIntr
__CyU3PS0CoreIntr
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
    bl      CyU3PSib0IntHandler         ;  Interrupt will only be re-enabled later on. 
    bl      CyU3PIrqNestingStop
    b       CyU3PIrqContextRestore

;  SIB1 Core Interrupt. 
    EXPORT __CyU3PS1CoreIntr
__CyU3PS1CoreIntr
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
    bl      CyU3PSib1IntHandler         ;  Interrupt will only be re-enabled later on. 
    bl      CyU3PIrqNestingStop
    b       CyU3PIrqContextRestore

;  I2C core interrupt 
    EXPORT __CyU3PI2CCoreIntr
__CyU3PI2CCoreIntr
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
    bl      CyU3PI2cIntHandler          ;  Interrupt will only be re-enabled later on. 
    bl      CyU3PIrqNestingStop
    b       CyU3PIrqContextRestore

;  I2S core interrupt 
    EXPORT __CyU3PI2SCoreIntr
__CyU3PI2SCoreIntr
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

;  SPI core interrupt 
    EXPORT __CyU3PSpiCoreIntr
__CyU3PSpiCoreIntr
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

;  UART core interrupt 
    EXPORT __CyU3PUartCoreIntr
__CyU3PUartCoreIntr
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

;  Actual handler for GPIO core interrupt 
    EXPORT __CyU3PGpioCoreIntr
__CyU3PGpioCoreIntr
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

;  Actual handler for LPP DMA interrupt 
    EXPORT __CyU3PLppDmaIntr
__CyU3PLppDmaIntr
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

    EXPORT __CyU3PDummyIsr
__CyU3PDummyIsr
    b       __CyU3PDummyIsr

;  Register the ISR addresses with the VIC. 
    EXPORT CyU3PVicSetupIntVectors
CyU3PVicSetupIntVectors
    ;  Copy the ARM exception vector table into its target location at the top of the I-TCM. 
    stmdb   sp!, {r0-r9, lr}                            ;  Save registers onto the stack. 
    ldr     r0, =__cyu3p_exception_vectors              ;  Address containing original vectors. 
    ldr     r1, =CY_U3P_EXCEPTION_VECTOR_LOCATION       ;  Target address to copy the vectors to. 
    ldmia   r0!, {r2-r9}                                ;  Read and save 256 bytes of code from this location. 
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
    stmia   r0!, {r1-r8}                        ;  Store the ISR vector addresses into the VIC. 

    ldr     r1, =__CyU3PUsbDmaIntr
    ldr     r2, =__CyU3PUsbCoreIntr
    ldr     r3, =__CyU3PDummyIsr
    ldr     r4, =__CyU3PSibDmaIntr
    ldr     r5, =__CyU3PS0CoreIntr
    ldr     r6, =__CyU3PS1CoreIntr
    ldr     r7, =__CyU3PDummyIsr
    ldr     r8, =__CyU3PI2CCoreIntr
    stmia   r0!, {r1-r8}                        ;  Store the ISR vector addresses into the VIC. 

    ldr     r1, =__CyU3PI2SCoreIntr
    ldr     r2, =__CyU3PSpiCoreIntr
    ldr     r3, =__CyU3PUartCoreIntr
    ldr     r4, =__CyU3PGpioCoreIntr
    ldr     r5, =__CyU3PLppDmaIntr
    ldr     r6, =__CyU3PGCTLPowerIntr
    ldr     r7, =__CyU3PDummyIsr
    ldr     r8, =__CyU3PDummyIsr
    stmia   r0!, {r1-r6}                        ;  Store the ISR vector addresses into the VIC. 

    ldmia   sp!, {r0-r9, pc}                    ;  Restore register contents. 

    END

