/*
 ## Cypress USB 3.0 Platform header file (cyu3regs.h)
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

#ifndef _INCLUDED_CYU3REGS_H_
#define _INCLUDED_CYU3REGS_H_

/*@@USB 3.0 Device registers
 * This file is the top-level include for all of the USB 3.0 device
 * register headers. Separate files document the registers specific
 * to each peripheral block within the device, and they are all
 * included here.
 */

#include <gctl_regs.h>
#include <gctlaon_regs.h>
#include <sock_regs.h>
#include <gpif_regs.h>
#include <gpio_regs.h>
#include <i2c_regs.h>
#include <i2s_regs.h>
#include <lpp_regs.h>
#include <pib_regs.h>
#include <pmmc_regs.h>
#include <sib_regs.h>
#include <spi_regs.h>
#include <uart_regs.h>
#include <uib_regs.h>
#include <uibin_regs.h>
#include <usb3lnk_regs.h>
#include <usb3prot_regs.h>
#include <vic_regs.h>

/* Summary
 * Macro to get the current value of a register.
 *
 * Parameters
 * addr         : Address of the register to be read.
 */
#define CYU3P_REG_GET_VALUE(addr)               (*((uvint32_t *)(addr)))

/* Summary
 * Macro to update the value of a register.
 *
 * Parameters
 * addr         : Address of the register to be updated.
 * value        : Value to be written into the register.
 */
#define CYU3P_REG_SET_VALUE(addr, value)        *((uvint32_t *)(addr)) = (value)

/* Summary
 * Macro to get the value of a field within a register.
 *
 * Parameters
 * addr         : Address of the register.
 * mask         : Mask that selects the relevant bits from the register
 * pos          : Position of the least significant bit of the field
 */
#define CYU3P_REG_GET_FIELD(addr, mask, pos)    ((*((uvint32_t *)(addr)) & (mask)) >> (pos))

/* Summary
 * Macro to update the value of a field within a register, without affecting
 * any of the other fields.
 *
 * Parameters
 * addr         : Address of the register.
 * mask         : Mask that selects the relevant bits in the register.
 * pos          : Position of the least significant bit of the field.
 * value        : Value to be set to the field.
 */
#define CYU3P_REG_SET_FIELD(addr, mask, pos, value)     \
    *((uvint32_t *)(addr)) = ((*((uvint32_t *)(addr)) & ~(mask)) | ((value) << (pos)))

/* Summary
 * Macro to get the LS word from a double word.
 */
#define CYU3P_LSW(dw)                   ((uint16_t)((dw) & UINT16_MAX))

/* Summary
 * Macro to get the HS word from a double word.
 */
#define CYU3P_MSW(dw)                   ((uint16_t)((dw) >> 16))

/* Summary
 * Macro to get the LS byte from a word.
 */
#define CYU3P_LSB(w)                    ((uint8_t)((w) & UINT8_MAX))

/* Summary
 * Macro to get the MS byte from a word.
 */
#define CYU3P_MSB(w)                    ((uint8_t)((w) >> 8))

#endif /* _INCLUDED_CYU3REGS_H_ */

/*[]*/

