/*
 ## Cypress USB 3.0 Platform header file (cyfx3error.h)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2011-2012,
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

#ifndef __CYFX3ERROR_H__
#define __CYFX3ERROR_H__

#include <cyu3externcstart.h>

/** \file cyfx3error.h
    \brief This file lists the various error codes that can be returned by the
    APIs in the FX3 boot library.
 */

/** \brief List of error codes returned by the boot APIs.
*/
typedef enum CyFx3BootErrorCode_t
{
   CY_FX3_BOOT_SUCCESS = 0x0,                   /**< Success. */
   CY_FX3_BOOT_ERROR_BAD_ARGUMENT,              /**< One or more parameters to a function are invalid. */
   CY_FX3_BOOT_ERROR_NULL_POINTER,              /**< A null pointer has been passed in unexpectedly. */
   CY_FX3_BOOT_ERROR_TIMEOUT,                   /**< Timeout on relevant operation. */
   CY_FX3_BOOT_ERROR_NOT_SUPPORTED,             /**< Operation requested is not supported in current mode. */
   CY_FX3_BOOT_ERROR_NOT_CONFIGURED,            /**< The peripheral block being accessed has not been configured. */
   CY_FX3_BOOT_ERROR_BAD_DESCRIPTOR_TYPE,       /**< Invalid USB descriptor type. */
   CY_FX3_BOOT_ERROR_XFER_FAILURE,              /**< Data Transfer failed. */
   CY_FX3_BOOT_ERROR_NO_REENUM_REQUIRED,        /**< Indicates that the booter has successfully configured the FX3
                                                     device after control was transferred from the full firmware
                                                     application. The user need not go through the cycle of setting
                                                     the descriptors and issuing connect call if this error code is
                                                     returned by the CyFx3BootUsbStart () API. */
   CY_FX3_BOOT_ERROR_NOT_STARTED,               /**< Indicates that the block being configured has not been
                                                     initialized. */
   CY_FX3_BOOT_ERROR_MEMORY_ERROR,              /**< Indicates that the API was unable to find enough memory to
                                                     copy the descriptor set through the CyFx3BootUsbSetDesc() API. */
   CY_FX3_BOOT_ERROR_ABORTED,                   /**< Indicates that the USB control request being processed has been
                                                     aborted due to a USB reset or a new control request. */
   CY_FX3_BOOT_ERROR_INVALID_DMA_ADDR,          /**< Indicates that the address passed into a DMA transfer API is
                                                     invalid (could be a TCM address). */
   CY_FX3_BOOT_ERROR_FAILURE                    /**< Generic error code indicating any other failure. */
} CyFx3BootErrorCode_t;

#include <cyu3externcend.h>

#endif /* __CYFX3ERROR_H__ */

