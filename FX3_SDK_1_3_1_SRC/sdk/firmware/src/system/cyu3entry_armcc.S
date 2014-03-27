; Cypress FX3 Firmware Entry code (cyu3entry_armcc.S)
; ## =====================================================
; ##
; ##  Copyright Cypress Semiconductor Corporation, 2010-2013,
; ##  All Rights Reserved
; ##  UNPUBLISHED, LICENSED SOFTWARE.
; ##
; ##  CONFIDENTIAL AND PROPRIETARY INFORMATION
; ##  WHICH IS THE PROPERTY OF CYPRESS.
; ##
; ##  Use of this file is governed
; ##  by the license agreement included in the file
; ##
; ##     <install>/license/license.txt
; ##
; ##  where <install> is the Cypress software
; ##  installation root directory path.
; ##
; ## =======================================================

CY_U3P_VIC_ADDRESS_OFFSET       EQU 0xFFFFFEE0
CY_U3P_VIC_INT_CLEAR_ADDRESS    EQU 0xFFFFF014
CY_U3P_INT_CLEAR_MASK           EQU 0xFFFFFFFF

CY_U3P_TMP_STACK_BASE           EQU 0x40001000  ; Free area used for temporary stack
CY_U3P_TMP_STACK_SIZE           EQU 0x1000      ; Only SVC stack

CY_U3P_STACK_BASE               EQU 0x10000000  ; D-TCM area ( 8KB )
CY_U3P_SVC_STACK_SIZE           EQU 0x1000      ; SVC stack size
CY_U3P_FIQ_STACK_SIZE           EQU 0x0200      ; FIQ stack size
CY_U3P_IRQ_STACK_SIZE           EQU 0x0400      ; IRQ stack size
CY_U3P_SYS_STACK_SIZE           EQU 0x0800      ; SYS stack size (used for nested interrupts)
CY_U3P_ABT_STACK_SIZE           EQU 0x0100      ; ABT stack size
CY_U3P_UND_STACK_SIZE           EQU 0x0100      ; UND stack size

ARM_FIQ_MODE                    EQU 0xD1        ; Disable Interrupts + FIQ mode
ARM_IRQ_MODE                    EQU 0xD2        ; Disable Interrupts + IRQ mode
ARM_SVC_MODE                    EQU 0xD3        ; Disable Interrupts + SVC mode
ARM_ABT_MODE                    EQU 0xD7        ; Disable Interrupts + ABT mode
ARM_UND_MODE                    EQU 0xDB        ; Disable Interrupts + UND mode
ARM_SYS_MODE                    EQU 0xDF        ; Disable Interrupts + SYS mode

    IMPORT CyU3PToolChainInit
    IMPORT CyU3PSysSetupMMU
    IMPORT CyU3PSysCheckBootState

;  The exception vectors imported below are to be defined by the application firmware. 
    IMPORT CyU3PUndefinedHandler
    IMPORT CyU3PPrefetchHandler
    IMPORT CyU3PAbortHandler

; FX3 firmware framework initialization.

    PRESERVE8
    AREA FX3FWINIT, CODE, READONLY

    EXPORT CyU3PSetupStackPtrs
CyU3PSetupStackPtrs
    ldr r1, =CY_U3P_STACK_BASE      ;  Load the stack base address 
    sub r1, r1, #8                  ;  Prevent overflow 

    ldr r2, =CY_U3P_SYS_STACK_SIZE  ;  Pickup stack size 
    mov r3, #ARM_SYS_MODE           ;  Build SYS mode CPSR 
    msr CPSR_cxsf, r3               ;  Enter SYS mode 
    add r1, r1, r2                  ;  Calculate start of SYS stack 
    bic r1, r1, #7                  ;  Ensure 8-byte alignment 
    mov sp, r1                      ;  Setup SYS stack pointer 
    mov r10, #0                     ;  Clear SYS mode sl 
    mov r11, #0                     ;  Clear SYS fp 

    ldr r2, =CY_U3P_ABT_STACK_SIZE  ;  Pickup stack size 
    mov r3, #ARM_ABT_MODE           ;  Build ABT mode CPSR 
    msr CPSR_cxsf, r3               ;  Enter ABT mode 
    add r1, r1, r2                  ;  Calculate start of ABT stack 
    bic r1, r1, #7                  ;  Ensure 8-byte alignment 
    mov sp, r1                      ;  Setup ABT stack pointer 
    mov r10, #0                     ;  Clear ABT mode sl 
    mov r11, #0                     ;  Clear ABT fp 

    ldr r2, =CY_U3P_UND_STACK_SIZE  ;  Pickup stack size 
    mov r3, #ARM_UND_MODE           ;  Build UND mode CPSR 
    msr CPSR_cxsf, r3               ;  Enter UND mode 
    add r1, r1, r2                  ;  Calculate start of UND stack 
    bic r1, r1, #7                  ;  Ensure 8-byte alignment 
    mov sp, r1                      ;  Setup UND stack pointer 
    mov r10, #0                     ;  Clear UND mode sl 
    mov r11, #0                     ;  Clear UND fp 
   
    ldr r2, =CY_U3P_FIQ_STACK_SIZE  ;  Pickup stack size 
    mov r0, #ARM_FIQ_MODE           ;  Build FIQ mode CPSR 
    msr CPSR_c, r0                  ;  Enter FIQ mode 
    add r1, r1, r2                  ;  Calculate start of FIQ stack 
    bic r1, r1, #7                  ;  Ensure 8-byte alignment 
    mov sp, r1                      ;  Setup FIQ stack pointer 
    mov sl, #0                      ;  Clear sl 
    mov fp, #0                      ;  Clear fp 

    ldr r2, =CY_U3P_IRQ_STACK_SIZE  ;  Pickup IRQ stack size 
    mov r0, #ARM_IRQ_MODE           ;  Build IRQ mode CPSR 
    msr CPSR_c, r0                  ;  Enter IRQ mode 
    add r1, r1, r2                  ;  Calculate start of IRQ stack 
    bic r1, r1, #7                  ;  Ensure 8-byte alignment 
    mov sp, r1                      ;  Setup IRQ stack pointer 

    ldr r2, =CY_U3P_SVC_STACK_SIZE  ;  Pickup System stack size 
    mov r0, #ARM_SVC_MODE           ;  Build SVC mode CPSR 
    msr CPSR_c, r0                  ;  Enter SVC mode 
    add r1, r1, r2                  ;  Calculate start of SVC stack 
    bic r1, r1, #7                  ;  Ensure 8-byte alignment 
    mov sp, r1                      ;  Setup SVC stack pointer 
    bx  lr

    EXPORT CyU3PFirmwareEntry
CyU3PFirmwareEntry
    ldr r1, =CY_U3P_VIC_INT_CLEAR_ADDRESS       ;  VIC INT disable address 
    ldr r0, =CY_U3P_INT_CLEAR_MASK              ;  Mask for clearing all interrupts 
    str r0, [r1]                                ;  Disable all interrupts 

    ldr r1, =CY_U3P_TMP_STACK_BASE              ;  Initial stack area 
    sub r1, r1, #8                              ;  Prevent overflow 
    ldr r2, =CY_U3P_TMP_STACK_SIZE              ;  Pickup stack size 
    mov r0, #ARM_SVC_MODE                       ;  Build SVC mode CPSR 
    msr CPSR_c, r0                              ;  Enter SVC mode 
    add r1, r1, r2                              ;  Calculate start of SVC stack 
    bic r1, r1, #7                              ;  Ensure 8-byte alignment 
    mov sp, r1                                  ;  Setup SVC stack pointer 
 
    bl  CyU3PSysSetupMMU                        ;  Setup memory interfaces 
    bl  CyU3PSysCheckBootState                  ;  If this is a wakeup from standby, restore TCM contents. 
    bl  CyU3PSetupStackPtrs                     ;  Setup device stack 
    b   CyU3PToolChainInit                      ;  Jump to toolchain initialization routine 

    PRESERVE8
    AREA CYU3P_EXCEPTION_VECTORS, CODE, READONLY
    ENTRY
    
    EXPORT __cyu3p_exception_vectors
__cyu3p_exception_vectors
    ldr pc, =CyU3PFirmwareEntry
    ldr pc, =CyU3PUndefinedHandler
    ldr pc, =__CyU3PSWIHandler
    ldr pc, =CyU3PPrefetchHandler
    ldr pc, =CyU3PAbortHandler
    ldr pc, =__CyU3PReservedHandler
    ldr pc,  [PC, #CY_U3P_VIC_ADDRESS_OFFSET]
    ldr pc, =__CyU3PFiqHandler

; The following exceptions are not expected to occur on this setup.
;  The handlers are left in place for completion.

    EXPORT __CyU3PSWIHandler
__CyU3PSWIHandler
    b   __CyU3PSWIHandler

    EXPORT __CyU3PReservedHandler
__CyU3PReservedHandler
    b   __CyU3PReservedHandler

    EXPORT __CyU3PFiqHandler
__CyU3PFiqHandler
    b   __CyU3PFiqHandler

    END

