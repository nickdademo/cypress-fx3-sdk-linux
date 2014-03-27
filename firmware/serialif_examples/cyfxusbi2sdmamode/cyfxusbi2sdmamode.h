/*
 ## Cypress USB 3.0 Platform header file (cyfxusbi2sdmamode.h)
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

/* This file contains the externants used by the I2S application. */

#ifndef _INCLUDED_CYFXUSBI2SDMAMODE_H_
#define _INCLUDED_CYFXUSBI2SDMAMODE_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3externcstart.h"

#define APPTHREAD_STACK       (0x0800)    /* App thread stack size */
#define APPTHREAD_PRIORITY    (8)         /* App thread priority */

#define CY_FX_I2S_DMA_BUF_COUNT      (8)  /* I2S channel buffer count */

/* Endpoint and socket definitions for the I2S application */

/* To change the producer and EPs enter the appropriate EP numbers for the #defines.
 * In the case of IN endpoints enter EP number along with the direction bit.
 * For eg. EP 6 IN endpoint is 0x86
 *     and EP 6 OUT endpoint is 0x06.
 * To change sockets mention the appropriate socket number in the #defines. */

/* Note: For USB 2.0 the endpoints and corresponding sockets are one-to-one mapped
         i.e. EP 1 is mapped to UIB socket 1 and EP 2 to socket 2 so on */

#define CY_FX_EP_PRODUCER_1             0x01    /* EP 1 OUT */
#define CY_FX_EP_PRODUCER_2             0x02    /* EP 2 OUT */

#define CY_FX_EP_PRODUCER_1_SOCKET        CY_U3P_UIB_SOCKET_PROD_1    /* Socket 1 is producer */
#define CY_FX_EP_PRODUCER_2_SOCKET        CY_U3P_UIB_SOCKET_PROD_2    /* Socket 2 is producer */

/* Extern definitions for the USB Descriptors */
extern const uint8_t CyFxUSB20DeviceDscr[];
extern const uint8_t CyFxUSB30DeviceDscr[];
extern const uint8_t CyFxUSBDeviceQualDscr[];
extern const uint8_t CyFxUSBFSConfigDscr[];
extern const uint8_t CyFxUSBHSConfigDscr[];
extern const uint8_t CyFxUSBBOSDscr[];
extern const uint8_t CyFxUSBSSConfigDscr[];
extern const uint8_t CyFxUSBStringLangIDDscr[];
extern const uint8_t CyFxUSBManufactureDscr[];
extern const uint8_t CyFxUSBProductDscr[];

#include "cyu3externcend.h"

#endif /* _INCLUDED_CYFXUSBI2SDMAMODE_H_ */

/*[]*/
