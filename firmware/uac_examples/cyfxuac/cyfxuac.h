/*
 ## Cypress USB 3.0 Platform header file (cyfxuac.h)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2012-2013,
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

#ifndef _INCLUDED_CYFXUAC_H_
#define _INCLUDED_CYFXUAC_H_

#include <cyu3externcstart.h>
#include <cyu3types.h>
#include <cyu3usbconst.h>

/* This header file comprises of the UAC application constants */

#define CY_FX_UAC_APP_THREAD_STACK     (0x1000)        /* Thread stack size */
#define CY_FX_UAC_APP_THREAD_PRIORITY  (8)             /* Thread priority */

/* Endpoint definition for UAC application */
#define CY_FX_EP_ISO_AUDIO             (0x81)          /* EP 1 IN */
#define CY_FX_EP_AUDIO_CONS_SOCKET     (CY_U3P_UIB_SOCKET_CONS_1) /* Consumer socket 1 */

/* UAC streaming endpoint packet Count */
#define CY_FX_EP_ISO_AUDIO_PKTS_COUNT  (0x01)

/* UAC Buffer count */
#define CY_FX_UAC_STREAM_BUF_COUNT     (16)

#define CY_FX_UAC_STREAM_INTERFACE     (uint8_t)(1)   /* Streaming Interface : Alternate setting 1 */

/* Extern definitions of the USB Enumeration constant arrays used for the Application */
extern const uint8_t CyFxUsb20DeviceDscr[];
extern const uint8_t CyFxUsb30DeviceDscr[];
extern const uint8_t CyFxUsbDeviceQualDscr[];
extern const uint8_t CyFxUsbFSConfigDscr[];
extern const uint8_t CyFxUsbHSConfigDscr[];
extern const uint8_t CyFxUsbBOSDscr[];
extern const uint8_t CyFxUsbSSConfigDscr[];
extern const uint8_t CyFxUsbStringLangIDDscr[];
extern const uint8_t CyFxUsbManufactureDscr[];
extern const uint8_t CyFxUsbProductDscr[];

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYFXUAC_H_ */

/*[]*/

