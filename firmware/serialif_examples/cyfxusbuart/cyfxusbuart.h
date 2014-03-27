/*
 ## Cypress USB 3.0 Platform header file (cyfxusbuart.h)
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

/* This file contains the constants used by the USB-UART application example */

#ifndef _INCLUDED_CYFXUSBUART_H_
#define _INCLUDED_CYFXUSBUART_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3externcstart.h"

#define  CY_FX_USBUART_DMA_BUF_COUNT      (8)
#define  CY_FX_USBUART_THREAD_STACK       (1000)
#define  CY_FX_USBUART_THREAD_PRIORITY     (8)

/* Endpoint and socket definitions for the USB-UART application */

#define CY_FX_EP_PRODUCER               0x02                             /* EP 2 OUT */
#define CY_FX_EP_CONSUMER               0x82                             /* EP 2 IN */
#define CY_FX_EP_INTERRUPT              0x81                             /* EP 1 INTR */

#define CY_FX_EP_PRODUCER1_SOCKET        CY_U3P_UIB_SOCKET_PROD_2
#define CY_FX_EP_CONSUMER1_SOCKET        CY_U3P_LPP_SOCKET_UART_CONS    
#define CY_FX_EP_PRODUCER2_SOCKET        CY_U3P_LPP_SOCKET_UART_PROD        
#define CY_FX_EP_CONSUMER2_SOCKET        CY_U3P_UIB_SOCKET_CONS_2
#define CY_FX_EP_INTR_CONSUMER1_SOCKET   CY_U3P_UIB_SOCKET_CONS_1

/* Descriptor Types */
#define CY_FX_BOS_DSCR_TYPE             15
#define CY_FX_DEVICE_CAPB_DSCR_TYPE     16
#define CY_FX_SS_EP_COMPN_DSCR_TYPE     48

/* Device Capability Type Codes */
#define CY_FX_WIRELESS_USB_CAPB_TYPE    1
#define CY_FX_USB2_EXTN_CAPB_TYPE       2
#define CY_FX_SS_USB_CAPB_TYPE          3
#define CY_FX_CONTAINER_ID_CAPBD_TYPE   4

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

#endif /* _INCLUDED_CYFXUSBUART_H_ */

/*[]*/

