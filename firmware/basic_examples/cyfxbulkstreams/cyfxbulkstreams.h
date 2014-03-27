/*
 ## Cypress USB 3.0 Platform header file (cyfxbulkstreams.h)
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

/* This file contains the constants used by the bulk streams application example */

#ifndef _INCLUDED_CYFXBULKSTREAMS_H_
#define _INCLUDED_CYFXBULKSTREAMS_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3externcstart.h"

#define CY_FX_BULKSTREAMS_DMA_TX_SIZE        (0)                       /* DMA transfer size is set to infinite */
#define CY_FX_BULKSTREAMS_THREAD_STACK       (0x1000)                  /* Bulk loop application thread stack size */
#define CY_FX_BULKSTREAMS_THREAD_PRIORITY    (8)                       /* Bulk loop application thread priority */
#define CY_FX_BULKSTREAMS_PATTERN            (0xAA)                    /* 8-bit pattern to be loaded to the source buffers. */

/* Endpoint and socket definitions for the bulk source sink application */

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

/* Maximum stream count definitions: Only for super speed operation.
 * This needs to be a power of 2. */

#define CY_FX_EP_MAX_STREAMS_FIELD      (2)    /* Super speed maximum supported number of streams. */
#define CY_FX_EP_MAX_STREAMS            (1 << CY_FX_EP_MAX_STREAMS_FIELD) /* Super speed maximum supported number of streams. */

#ifdef CYMEM_256K

/* As we have only 32 KB of DMA buffer space, drop the buffering per DMA channel to 3 KB. */
#define CY_FX_BULKSTREAMS_DMA_BUF_COUNT         (3)

#else

/* Use upto 8 KB of buffering per DMA channel. */
#define CY_FX_BULKSTREAMS_DMA_BUF_COUNT         (8)

#endif

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

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYFXBULKSTREAMS_H_ */

/*[]*/
