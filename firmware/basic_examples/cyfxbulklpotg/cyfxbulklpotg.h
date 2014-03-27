/*
 ## Cypress USB 3.0 Platform header file (cyfxbulklpotg.h)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2011,
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

/* This file contains the externants used by the OTG application example */

#ifndef _INCLUDED_CYFXUSBOTG_H_
#define _INCLUDED_CYFXUSBOTG_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3externcstart.h"

#define CY_FX_APPLN_THREAD_STACK       (0x1000) /* Application thread stack size */
#define CY_FX_APPLN_THREAD_PRIORITY    (8)      /* Application thread priority */

#define CY_FX_OTG_POLL_INTERVAL        (100)    /* The polling interval for the DoWork function in clock ticks. */

#define CY_FX_OTG_HNP_WAIT_TIME        (30000)  /* The wait time in clock ticks before the device requests for role change. */

#define CY_FX_OTG_STATUS_POLL_INTERVAL (1000)   /* Polling interval for OTG session request in host mode. */

#define CY_FX_OTG_VBUS_ENABLE_VALUE    (CyFalse)       /* GPIO value for driving VBUS. */
#define CY_FX_OTG_VBUS_DISABLE_VALUE   (CyTrue)        /* GPIO value for disabling VBUS. */

/* Summary
   Fatal error handler.

   Description
   The function does not return and is equivalent to an
   Abort function.
 */
extern void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t status    /* API return status */
        );

/* Summary
   The function completes HNP request and initiates role change
   when in device mode of operation.

   Description
   The function disables the currently running device stack,
   invokes the role change API and then starts the host stack.
 */
extern void
CyFxDeviceHnp (
        void);

/* Summary
   The function completes HNP request and initiates role change
   when in host mode of operation.

   Description
   The function disables the currently running host stack,
   invokes the role change API and then starts the device stack.
 */
extern void
CyFxHostHnp (
        void);

/* Host mode definitions. */

#define CY_FX_HOST_EP0_SETUP_SIZE       (32)     /* EP0 setup packet buffer size made multiple of 32 bytes. */
#define CY_FX_HOST_EP0_BUFFER_SIZE      (512)   /* EP0 data transfer buffer size. */
#define CY_FX_HOST_EP0_WAIT_TIMEOUT     (5000)  /* EP0 request timeout in clock ticks. */
#define CY_FX_HOST_PERIPHERAL_ADDRESS   (1)     /* USB host mode peripheral address to be used. */
#define CY_FX_HOST_DMA_BUF_COUNT        (4)     /* Number of buffers to be allocated for DMA channel. */
#define CY_FX_HOST_PERIPHERAL_VID       (0x04B4)/* VID of the peripheral device supported. */
#define CY_FX_HOST_PERIPHERAL_PID       (0x00F0)/* PID of the peripheral device supported. */
#define CY_FX_HOST_DATA_BYTE            (0x5A)  /* Data pattern sent out on the bus from host. */

/* Summary
   The function initializes the USB host stack implementation.

   Description
   The function initializes the USB host module and starts the
   bulkloop driver application.
 */
extern void
CyFxUsbHostStart (
        void);

/* Summary
   The function disables the USB host stack implementation.

   Description
   The function stops the bulkloop driver application and disables
   the USB host module.
 */
extern void
CyFxUsbHostStop (
        void);

/* Summary
   USB host stack helper function.

   Description
   This function is expected to be periodically called from
   the application thread to keep track of the host mode changes.
  */
extern void
CyFxUsbHostDoWork (
        void);

/* Device mode definitions. */
#define CY_FX_BULKLP_DMA_BUF_COUNT      (8)                       /* Bulk loop channel buffer count */
#define CY_FX_BULKLP_DMA_TX_SIZE        (0)                       /* DMA transfer size is set to infinite */

/* Endpoint and socket definitions for the bulkloop application */

/* To change the producer and consumer EP enter the appropriate EP numbers for the #defines.
 * In the case of IN endpoints enter EP number along with the direction bit.
 * For eg. EP 6 IN endpoint is 0x86
 *     and EP 6 OUT endpoint is 0x06.
 * To change sockets mention the appropriate socket number in the #defines. */

/* Note: For USB 2.0 the endpoints and corresponding sockets are one-to-one mapped
         i.e. EP 1 is mapped to UIB socket 1 and EP 2 to socket 2 so on */

#define CY_FX_EP_PRODUCER               0x01    /* EP 1 OUT */
#define CY_FX_EP_CONSUMER               0x81    /* EP 1 IN */

#define CY_FX_EP_PRODUCER_SOCKET        CY_U3P_UIB_SOCKET_PROD_1    /* Socket 1 is producer */
#define CY_FX_EP_CONSUMER_SOCKET        CY_U3P_UIB_SOCKET_CONS_1    /* Socket 1 is consumer */

/* Extern definitions for the USB Descriptors */
extern const uint8_t CyFxUSB20DeviceDscr[];
extern const uint8_t CyFxUSB30DeviceDscr[];
extern const uint8_t CyFxUSBDeviceQualDscr[];
extern const uint8_t CyFxUSBFSConfigDscr[];
extern const uint8_t CyFxUSBHSConfigDscr[];
extern const uint8_t CyFxUSBBOSDscr[];
extern const uint8_t CyFxUSBSSConfigDscr[];
extern const uint8_t CyFxUSBOtgDscr[];
extern const uint8_t CyFxUSBStringLangIDDscr[];
extern const uint8_t CyFxUSBManufactureDscr[];
extern const uint8_t CyFxUSBProductDscr[];

/* Summary
   The function initializes the USB device stack implementation.

   Description
   The function initializes the USB device module and starts the
   bulkloop application.
 */
extern void
CyFxUsbStart (
        void);

/* Summary
   The function disables the USB device stack implementation.

   Description
   The function stops the bulkloop application and disables
   the USB host module.
 */
extern void
CyFxUsbStop (
        void);

/* Summary
   USB device stack helper function.

   Description
   This function is expected to be periodically called from
   the application thread to keep track of the device mode changes.
  */
extern void
CyFxUsbDeviceDoWork (
        void);

#include "cyu3externcend.h"

#endif /* _INCLUDED_CYFXUSBOTG_H_ */

/*[]*/

