/****************************************************************************
 *
 * File: vic_regs.h
 *
 * Copyright (c) 2010-13 Cypress Semiconductor. All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION WHICH IS THE PROPERTY OF CYPRESS.
 *
 * Description:
 *   Vectored Interrupt Controller (VIC) registers for the EZ-USB FX3 Device.
 *
 *   This file is auto generated from register spreadsheet.
 *   DO NOT MODIFY THIS FILE
 *
 * Use of this file is governed by the license agreement included in the file
 *
 *   <install>/license/license.txt
 *
 * where <install> is the Cypress software installation root directory path.
 *
 ****************************************************************************/

#ifndef _INCLUDED_VIC_REGS_H_
#define _INCLUDED_VIC_REGS_H_

#include <cyu3types.h>

#define VIC_BASE_ADDR                            (0xfffff000)

typedef struct
{
    uvint32_t irq_status;                         /* 0xfffff000 */
    uvint32_t fiq_status;                         /* 0xfffff004 */
    uvint32_t raw_status;                         /* 0xfffff008 */
    uvint32_t int_select;                         /* 0xfffff00c */
    uvint32_t int_enable;                         /* 0xfffff010 */
    uvint32_t int_clear;                          /* 0xfffff014 */
    uvint32_t reserved1;                          /* 0xfffff018 */
    uvint32_t reserved2;                          /* 0xfffff01c */
    uvint32_t protect;                            /* 0xfffff020 */
    uvint32_t priority_mask;                      /* 0xfffff024 */
    uvint32_t reserved3;                          /* 0xfffff028 */
    uvint32_t rsrvd0[53];
    uvint32_t vec_address[32];                    /* 0xfffff100 */
    uvint32_t rsrvd1[32];
    uvint32_t vect_priority[32];                  /* 0xfffff200 */
    uvint32_t rsrvd2[800];
    uvint32_t address;                            /* 0xffffff00 */
} VIC_REGS_T, *PVIC_REGS_T;

#define VIC        ((PVIC_REGS_T) VIC_BASE_ADDR)


/*
   IRQ Status after masking
 */
#define CY_U3P_VIC_IRQ_STATUS_ADDRESS                       (0xfffff000)
#define CY_U3P_VIC_IRQ_STATUS                               (*(uvint32_t *)(0xfffff000))
#define CY_U3P_VIC_IRQ_STATUS_DEFAULT                       (0x00000000)

/*
   1: IRQ interrupt has been raised at the line correponding to the bit position.
 */
#define CY_U3P_IRQ_STATUS_MASK                              (0xffffffff) /* <0:31> RW:R:0:No */
#define CY_U3P_IRQ_STATUS_POS                               (0)



/*
   FIQ Status after masking
 */
#define CY_U3P_VIC_FIQ_STATUS_ADDRESS                       (0xfffff004)
#define CY_U3P_VIC_FIQ_STATUS                               (*(uvint32_t *)(0xfffff004))
#define CY_U3P_VIC_FIQ_STATUS_DEFAULT                       (0x00000000)

/*
   1: FIQ interrupt has been raised at the line correponding to the bit position.
 */
#define CY_U3P_FIQ_STATUS_MASK                              (0xffffffff) /* <0:31> RW:R:0:No */
#define CY_U3P_FIQ_STATUS_POS                               (0)



/*
   IRQ Status before masking
 */
#define CY_U3P_VIC_RAW_STATUS_ADDRESS                       (0xfffff008)
#define CY_U3P_VIC_RAW_STATUS                               (*(uvint32_t *)(0xfffff008))
#define CY_U3P_VIC_RAW_STATUS_DEFAULT                       (0x00000000)

/*
   1: FIQ/IRQ interrupt has been raised at the line correponding to the bit
   position regardless of its masking configuration.
 */
#define CY_U3P_RAW_STATUS_MASK                              (0xffffffff) /* <0:31> RW:R:X:No */
#define CY_U3P_RAW_STATUS_POS                               (0)



/*
   IRQ/FIQ Designation Register
 */
#define CY_U3P_VIC_INT_SELECT_ADDRESS                       (0xfffff00c)
#define CY_U3P_VIC_INT_SELECT                               (*(uvint32_t *)(0xfffff00c))
#define CY_U3P_VIC_INT_SELECT_DEFAULT                       (0x00000000)

/*
   1: Designate the line corresponding to the bit position to be FIQ. The
   SW shall ensure that this register is a power of 2 (one FIQ only). The
   software shall write to this registers only after disabling the interrupts
   it intends to change.
 */
#define CY_U3P_FIQ_NIRQ_MASK                                (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_FIQ_NIRQ_POS                                 (0)



/*
   Interrupt Enable Register
 */
#define CY_U3P_VIC_INT_ENABLE_ADDRESS                       (0xfffff010)
#define CY_U3P_VIC_INT_ENABLE                               (*(uvint32_t *)(0xfffff010))
#define CY_U3P_VIC_INT_ENABLE_DEFAULT                       (0x00000000)

/*
   1: Enable the interrupt at this bit position. All interrupts are disabled
   at reset. SW cannot write 0 here to disable interrupts. Use the VIC_INT_CLEAR
   register for this purpose.
 */
#define CY_U3P_INT_ENABLE_MASK                              (0xffffffff) /* <0:31> R:RW1S:0:No */
#define CY_U3P_INT_ENABLE_POS                               (0)



/*
   Interrupt Clear Register
 */
#define CY_U3P_VIC_INT_CLEAR_ADDRESS                        (0xfffff014)
#define CY_U3P_VIC_INT_CLEAR                                (*(uvint32_t *)(0xfffff014))
#define CY_U3P_VIC_INT_CLEAR_DEFAULT                        (0x00000000)

/*
   1:Disable the interrupt at this bit position.  Mask it if designated IRQ.
 */
#define CY_U3P_INT_CLEAR_MASK                               (0xffffffff) /* <0:31> R:W1C:NA:No */
#define CY_U3P_INT_CLEAR_POS                                (0)



/*
   Reserved Register
 */
#define CY_U3P_VIC_RESERVED1_ADDRESS                        (0xfffff018)
#define CY_U3P_VIC_RESERVED1                                (*(uvint32_t *)(0xfffff018))
#define CY_U3P_VIC_RESERVED1_DEFAULT                        (0x00000000)

/*
   RESERVED
 */
#define CY_U3P_RESERVED1_MASK                               (0xffffffff) /* <0:31> R:R:NA:No */
#define CY_U3P_RESERVED1_POS                                (0)



/*
   Reserved Register
 */
#define CY_U3P_VIC_RESERVED2_ADDRESS                        (0xfffff01c)
#define CY_U3P_VIC_RESERVED2                                (*(uvint32_t *)(0xfffff01c))
#define CY_U3P_VIC_RESERVED2_DEFAULT                        (0x00000000)

/*
   RESERVED
 */
#define CY_U3P_RESERVED2_MASK                               (0xffffffff) /* <0:31> R:R:NA:No */
#define CY_U3P_RESERVED2_POS                                (0)



/*
   VIC Registers Access Control Bit
 */
#define CY_U3P_VIC_PROTECT_ADDRESS                          (0xfffff020)
#define CY_U3P_VIC_PROTECT                                  (*(uvint32_t *)(0xfffff020))
#define CY_U3P_VIC_PROTECT_DEFAULT                          (0x00000000)

/*
   This register can only be accessed in protected mode. 1: Allow read-writes
   only in previledged mode. (HPROT[1] =1). 0: User mode accesses allowed.
 */
#define CY_U3P_VIC_PROT                                     (1u << 0) /* <0:0> R:RW:0:No */



/*
   Per-Priority Interrupt Mask Register
 */
#define CY_U3P_VIC_PRIORITY_MASK_ADDRESS                    (0xfffff024)
#define CY_U3P_VIC_PRIORITY_MASK                            (*(uvint32_t *)(0xfffff024))
#define CY_U3P_VIC_PRIORITY_MASK_DEFAULT                    (0x0000ffff)

/*
   0: Interrupt at the priority level = bit position is masked. 1: Unmasked.
 */
#define CY_U3P_PRIO_MASK_MASK                               (0x0000ffff) /* <0:15> R:RW:0xFFFF:No */
#define CY_U3P_PRIO_MASK_POS                                (0)



/*
   Reserved Register
 */
#define CY_U3P_VIC_RESERVED3_ADDRESS                        (0xfffff028)
#define CY_U3P_VIC_RESERVED3                                (*(uvint32_t *)(0xfffff028))
#define CY_U3P_VIC_RESERVED3_DEFAULT                        (0x0000000f)

#define CY_U3P_RESRVED3_MASK                                (0xffffffff) /* <0:31> R:R:0xF:No */
#define CY_U3P_RESRVED3_POS                                 (0)



/*
   Interrupt Vector Register
 */
#define CY_U3P_VIC_VEC_ADDRESS_ADDRESS(n)                   (0xfffff100 + ((n) * (0x0004)))
#define CY_U3P_VIC_VEC_ADDRESS(n)                           (*(uvint32_t *)(0xfffff100 + ((n) * 0x0004)))
#define CY_U3P_VIC_VEC_ADDRESS_DEFAULT                      (0x00000000)

/*
   The firmware shall access this register only after disabling the corresponding
   interrupt. Holds the address to the ISR for interrupt number =  the position
   of this register in the bank of 32 registers.
 */
#define CY_U3P_ISR_ADD_MASK                                 (0xffffffff) /* <0:31> R:RW:0:No */
#define CY_U3P_ISR_ADD_POS                                  (0)



/*
   Interrupt Priority Register
 */
#define CY_U3P_VIC_VECT_PRIORITY_ADDRESS(n)                 (0xfffff200 + ((n) * (0x0004)))
#define CY_U3P_VIC_VECT_PRIORITY(n)                         (*(uvint32_t *)(0xfffff200 + ((n) * 0x0004)))
#define CY_U3P_VIC_VECT_PRIORITY_DEFAULT                    (0x0000000f)

/*
   The firmware shall access this register only after disabling the corresponding
   interrupt. Assigns one of the 16 priorities to the interrupt number =
   the position of this regsiter in the bank of 32 registers. When an two
   inteerupts with same priority arrive at the VIC, the one connected to
   the lower numbered line wins.
 */
#define CY_U3P_VIC_INT_PRI_MASK                             (0x0000000f) /* <0:3> R:RW:0xF:No */
#define CY_U3P_VIC_INT_PRI_POS                              (0)



/*
   Active ISR Address Register
 */
#define CY_U3P_VIC_ADDRESS_ADDRESS                          (0xffffff00)
#define CY_U3P_VIC_ADDRESS                                  (*(uvint32_t *)(0xffffff00))
#define CY_U3P_VIC_ADDRESS_DEFAULT                          (0x00000000)

/*
    Upon interruption, SW shall read this register. This marks the active
   interrupt as being serviced, which prevents interrupts of equal or lower
   priority from raising interrupt . Upon completion, the SW shall write
   any value to this register, which clears the active interrupt. SW shall
   not access this register at any other times.
 */
#define CY_U3P_ACTIVE_ISR_MASK                              (0xffffffff) /* <0:31> RW:RW:0:No */
#define CY_U3P_ACTIVE_ISR_POS                               (0)

#endif /* _INCLUDED_VIC_REGS_H_ */

/*[]*/

