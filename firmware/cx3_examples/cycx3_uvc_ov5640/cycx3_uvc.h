/*
## Cypress USB 3.0 Platform header file (cycx3_uvc.h)
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

#ifndef _INCLUDED_CYCX3_UVC_H_
#define _INCLUDED_CYCX3_UVC_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3os.h"
#include "cyu3utils.h"
#include "cyu3mipicsi.h"

#include "cyu3externcstart.h"

/* This header file comprises of the UVC application constants and
 * the video frame configurations */

/* Uncomment the following line to provide verbose debug logging. */
/* #define CX3_DEBUG_ENABLED       1 */
/* Uncomment the following line to run the mipi error thread */
/* #define CX3_ERROR_THREAD_ENABLE 1 */
/* Uncomment the following line to run as UVC 1.0 device. 
 * Default is UVC 1.1 device.
 */
/* #define CX3_UVC_1_0_SUPPORT     1  */
#define UVC_APP_THREAD_STACK                    (0x1000)        /* Thread stack size */
#define UVC_APP_THREAD_PRIORITY                 (8)             /* Thread priority */

#ifdef CX3_ERROR_THREAD_ENABLE
#define UVC_MIPI_ERROR_THREAD_STACK             (0x1000)        /* Thread stack size */
#define UVC_MIPI_ERROR_THREAD_PRIORITY          (15)            /* Thread priority */
#endif
/* Endpoint definition for UVC application */
#define CX3_EP_BULK_VIDEO                       (0x83)          /* EP 1 IN */
#define CX3_EP_CONTROL_STATUS                   (0x82)          /* EP 2 IN */

#define CX3_EP_VIDEO_CONS_SOCKET                (CY_U3P_UIB_SOCKET_CONS_3)      /* Consumer socket 1 */
#define CX3_PRODUCER_PPORT_SOCKET_0             (CY_U3P_PIB_SOCKET_0)           /* P-port Socket 0 is producer */
#define CX3_PRODUCER_PPORT_SOCKET_1             (CY_U3P_PIB_SOCKET_1)           /* P-port Socket 0 is producer */

/* UVC descriptor types */
#define CX3_INTRFC_ASSN_DESCR                   (11)            /* Interface association descriptor type. */
#define CX3_CS_INTRFC_DESCR                     (0x24)          /* Class Specific Interface Descriptor type: CS_INTERFACE */
/* UVC video streaming endpoint packet Size */
#define CX3_EP_BULK_VIDEO_PKT_SIZE              (0x400)

/* UVC Buffer Parameters*/
#define CX3_UVC_DATA_BUF_SIZE                   (0x5FF0)        /* DMA Buffer Data Size Used: 12272 Bytes*/
#define CX3_UVC_PROD_HEADER                     (12)            /* UVC DMA Buffer Header Size */
#define CX3_UVC_PROD_FOOTER                     (4)             /* UVC DMA Buffer Footer Size */

/* UVC Buffer size - Will map to bulk Transaction size */
#define CX3_UVC_STREAM_BUF_SIZE                 (CX3_UVC_DATA_BUF_SIZE + CX3_UVC_PROD_HEADER + CX3_UVC_PROD_FOOTER)

/* UVC Buffer count */
#define CX3_UVC_STREAM_BUF_COUNT                (4)

/* Low byte - UVC video streaming endpoint packet size */
#define CX3_EP_BULK_VIDEO_PKT_SIZE_L            CY_U3P_GET_LSB(CX3_EP_BULK_VIDEO_PKT_SIZE)

/* High byte - UVC video streaming endpoint packet size */
#define CX3_EP_BULK_VIDEO_PKT_SIZE_H            CY_U3P_GET_MSB(CX3_EP_BULK_VIDEO_PKT_SIZE)

#define CX3_UVC_HEADER_LENGTH                   (CX3_UVC_PROD_HEADER)           /* Maximum number of header bytes in UVC */
#define CX3_UVC_HEADER_DEFAULT_BFH              (0x8C)                          /* Default BFH(Bit Field Header) for the UVC Header */

#ifdef CX3_UVC_1_0_SUPPORT
#define CX3_UVC_MAX_PROBE_SETTING               (26)            /* UVC 1.0 Maximum number of bytes in Probe Control */
#define CX3_UVC_MAX_PROBE_SETTING_ALIGNED       (32)            /* Maximum number of bytes in Probe Control aligned to 32 byte */
#else
#define CX3_UVC_MAX_PROBE_SETTING               (34)            /* UVC 1.1 Maximum number of bytes in Probe Control */
#define CX3_UVC_MAX_PROBE_SETTING_ALIGNED       (64)            /* Maximum number of bytes in Probe Control aligned to 32 byte */
#endif

#define CX3_UVC_HEADER_FRAME                    (0)                             /* Normal frame indication */
#define CX3_UVC_HEADER_EOF                      (uint8_t)(1 << 1)               /* End of frame indication */
#define CX3_UVC_HEADER_FRAME_ID                 (uint8_t)(1 << 0)               /* Frame ID toggle bit */

#define CX3_USB_UVC_SET_REQ_TYPE                (uint8_t)(0x21)                 /* UVC interface SET request type */
#define CX3_USB_UVC_GET_REQ_TYPE                (uint8_t)(0xA1)                 /* UVC Interface GET request type */
#define CX3_USB_UVC_GET_CUR_REQ                 (uint8_t)(0x81)                 /* UVC GET_CUR request */
#define CX3_USB_UVC_SET_CUR_REQ                 (uint8_t)(0x01)                 /* UVC SET_CUR request */
#define CX3_USB_UVC_GET_MIN_REQ                 (uint8_t)(0x82)                 /* UVC GET_MIN Request */
#define CX3_USB_UVC_GET_MAX_REQ                 (uint8_t)(0x83)                 /* UVC GET_MAX Request */
#define CX3_USB_UVC_GET_RES_REQ                 (uint8_t)(0x84)                 /* UVC GET_RES Request */
#define CX3_USB_UVC_GET_INFO_REQ                (uint8_t)(0x86)                 /* UVC GET_INFO Request */
#define CX3_USB_UVC_GET_DEF_REQ                 (uint8_t)(0x87)                 /* UVC GET_DEF Request */

#define CX3_UVC_VS_PROBE_CONTROL                (0x0100)                        /* Video Stream Probe Control Request */
#define CX3_UVC_VS_COMMIT_CONTROL               (0x0200)                        /* Video Stream Commit Control Request */
#define CX3_UVC_VC_REQUEST_ERROR_CODE_CONTROL   (0x0200)                        /* Request Control Error Code*/
#define CX3_UVC_ERROR_INVALID_CONTROL           (0x06)                          /* Error indicating invalid control requested*/
#define CX3_UVC_STREAM_INTERFACE                (0x01)                          /* Streaming Interface : Alternate setting 1 */
#define CX3_UVC_CONTROL_INTERFACE               (0x00)                          /* Control Interface: Alternate Setting 0 */

#define CX3_GPIF_SWITCH_TIMEOUT                 (2)             /* Timeout setting for the switch operation in GPIF clock cycles */
#define CX3_INVALID_GPIF_STATE                  (257)           /* Invalid state for use in CyU3PGpifSMSwitch calls */

/* Extern definitions of the USB Enumeration constant arrays used for the Application */
extern const uint8_t CyCx3USB20DeviceDscr[];
extern const uint8_t CyCx3USB30DeviceDscr[];
extern const uint8_t CyCx3USBDeviceQualDscr[];
extern const uint8_t CyCx3USBFSConfigDscr[];
extern const uint8_t CyCx3USBHSConfigDscr[];
extern const uint8_t CyCx3USBBOSDscr[];
extern const uint8_t CyCx3USBSSConfigDscr[];
extern const uint8_t CyCx3USBStringLangIDDscr[];
extern const uint8_t CyCx3USBManufactureDscr[];
extern const uint8_t CyCx3USBProductDscr[];
extern const uint8_t CyCx3USBConfigSSDscr[];
extern const uint8_t CyCx3USBConfigHSDscr[];
extern const uint8_t CyCx3USBConfigFSDscr[];

/* Extern definitions of the Video frame data */

/* UVC Probe Control Setting */
extern uint8_t glProbeCtrl[CX3_UVC_MAX_PROBE_SETTING];
extern const uint8_t gl720pProbeCtrl[CX3_UVC_MAX_PROBE_SETTING];
extern const uint8_t gl1080pProbeCtrl[CX3_UVC_MAX_PROBE_SETTING];
extern const uint8_t glVga60ProbeCtrl[CX3_UVC_MAX_PROBE_SETTING];
extern uint8_t const gl5MpProbeCtrl[CX3_UVC_MAX_PROBE_SETTING];
/************************************/
/* MIPI Configuration parameters */
/************************************/

extern CyU3PMipicsiCfg_t cfgUvc1080p30NoMclk, cfgUvc720p60NoMclk,  cfgUvcVgaNoMclk, cfgUvc5Mp15NoMclk;

#include "cyu3externcend.h"


#endif /* _INCLUDED_CYCX3_UVC_H_ */
