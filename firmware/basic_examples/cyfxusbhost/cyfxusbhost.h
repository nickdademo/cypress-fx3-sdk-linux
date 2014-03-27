/*
 ## Cypress USB 3.0 Platform header file (cyfxusbhost.h)
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

/* This file contains the externants used by the host application example */

#ifndef _INCLUDED_CYFXUSBHOST_H_
#define _INCLUDED_CYFXUSBHOST_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3usbhost.h"
#include "cyu3externcstart.h"

#define CY_FX_APPLN_THREAD_STACK        (0x1000)        /* Application thread stack size */
#define CY_FX_APPLN_THREAD_PRIORITY     (8)             /* Application thread priority */

#define CY_FX_HOST_POLL_INTERVAL        (100)           /* The polling interval for the DoWork function in clock ticks. */

#define CY_FX_HOST_EP0_SETUP_SIZE       (32)            /* EP0 setup packet buffer size made multiple of 32 bytes. */
#define CY_FX_HOST_EP0_BUFFER_SIZE      (512)           /* EP0 data transfer buffer size. */
#define CY_FX_HOST_EP0_WAIT_TIMEOUT     (5000)          /* EP0 request timeout in clock ticks. */
#define CY_FX_HOST_PERIPHERAL_ADDRESS   (1)             /* USB host mode peripheral address to be used. */
#define CY_FX_HOST_DMA_BUF_COUNT        (4)             /* Number of buffers to be allocated for DMA channel. */

#define CY_FX_HOST_VBUS_ENABLE_VALUE    (CyTrue)        /* GPIO value for driving VBUS. */
#define CY_FX_HOST_VBUS_DISABLE_VALUE   (!CY_FX_HOST_VBUS_ENABLE_VALUE)

#define CY_FX_HOST_OWNER_NONE           (0)             /* Host controller is not associated with a class driver. */
#define CY_FX_HOST_OWNER_MOUSE_DRIVER   (1)             /* Host controller associated with mouse driver. */
#define CY_FX_HOST_OWNER_MSC_DRIVER     (2)             /* Host controller associated with MSC driver. */

#define CY_FX_MSC_GET_MAX_LUN           (0xFE)          /* Setup request to retreive the MSC MAX_LUN. */
#define CY_FX_MSC_CBW_CSW_BUFFER_SIZE   (32)            /* Buffer size for receiving CBW / CSW. */
#define CY_FX_MSC_MAX_SECTOR_SIZE       (0x1000)        /* Maximum sector size supported. */
#define CY_FX_MSC_WAIT_TIMEOUT          (5000)          /* Timeout for MSC transfers. */
#define CY_FX_MSC_MAX_RETRY             (10)            /* Maximum retry count. */

#define CY_FX_MSC_TEST_SECTOR_OFFSET    (256)           /* Offset address from which the MSC test can run. */
#define CY_FX_MSC_TEST_SECTOR_COUNT     (10)            /* Number of sectors tested in a single run. */
#define CY_FX_MSC_TEST_INTERVAL         (60000)         /* Once every minute. */

extern uint8_t glEp0Buffer[];                           /* Buffer to send / receive data for EP0. */

/* Summary
   Helper function to send the setup packet.

   Description
   This function takes in the various fields
   of the setup packet and formats it into
   the eight byte setup packet and then sends
   it the USB host. It also waits for the
   transfer to complete.

   Return Value
   CY_U3P_SUCCESS - The call was successful.
   Non zero value means error.

 */
extern CyU3PReturnStatus_t
CyFxSendSetupRqt (
        uint8_t type,           /* Attributes of setup request . */
        uint8_t request,        /* The request code. */
        uint16_t value,         /* Value field. */
        uint16_t index,         /* Index field. */
        uint16_t length,        /* Length of the data phase. */
        uint8_t *buffer_p       /* Buffer pointer for the data phase. */
        );

/* Summary
   Function initializes the mouse driver.

   Description
   The function is invoked by the host controller driver
   when a HID mouse class device is detected.

   Return Value
   CY_U3P_SUCCESS - The call was successful.
   CY_U3P_ERROR_FAILURE - The driver initialization failed.
 */
extern CyU3PReturnStatus_t
CyFxMouseDriverInit (
        void);

/* Summary
   Function disables the mouse driver.

   Description
   The function is invoked by the host controller driver
   when the previously attached mouse device is disconnected.

   Return Value
   None.
 */
extern void
CyFxMouseDriverDeInit (
        void);

/* Summary
   Function initializes the MSC driver.

   Description
   The function is invoked by the host controller driver
   when a MSC class device is detected.

   Return Value
   CY_U3P_SUCCESS - The call was successful.
   CY_U3P_ERROR_FAILURE - The driver initialization failed.
 */
extern CyU3PReturnStatus_t
CyFxMscDriverInit (
        void);

/* Summary
   Function disables the MSC driver.

   Description
   The function is invoked by the host controller driver
   when the previously attached MSC device is disconnected.

   Return Value
   None.
 */
extern void
CyFxMscDriverDeInit (
        void);

/* Summary
   Function runs the MSC driver periodic tasks.

   Description
   The function is invoked by the host controller driver
   thread at a fixed interval.

   Return Value
   None.
 */
extern void
CyFxMscDriverDoWork (
        void);

#include "cyu3externcend.h"

#endif /* _INCLUDED_CYFXUSBHOST_H_ */

/*[]*/
