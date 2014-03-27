/*
 ## Cypress USB 3.0 Platform header file (cyfxuvcinmem.h)
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

#ifndef _INCLUDED_CYFXUVCINMEM_H_
#define _INCLUDED_CYFXUVCINMEM_H_

#include <cyu3externcstart.h>
#include <cyu3types.h>
#include <cyu3usbconst.h>

/* This header file comprises of the UVC application contants and
 * the video frame configurations */

#define UVC_APP_THREAD_STACK           (0x1000)        /* Thread stack size */
#define UVC_APP_THREAD_PRIORITY        (8)             /* Thread priority */

/* Endpoint definition for UVC application */
#define CY_FX_EP_ISO_VIDEO              0x83           /* EP 3 IN */
#define CY_FX_EP_VIDEO_CONS_SOCKET      (CY_U3P_UIB_SOCKET_CONS_0 | (CY_FX_EP_ISO_VIDEO & 0x7F)) /* Consumer socket 3 */
#define CY_FX_EP_CONTROL_STATUS         0x82           /* EP 2 IN */

/* UVC descriptor types */
#define CY_FX_INTF_ASSN_DSCR_TYPE       (11)           /* Interface association descriptor type. */

/* UVC video streaming endpoint packet Size */
#define CY_FX_EP_ISO_VIDEO_PKT_SIZE    (0x400)

/* UVC video streaming endpoint packet Count */
#define CY_FX_EP_ISO_VIDEO_PKTS_COUNT  (3)

#define CY_FX_UVC_MAX_VID_FRAMES       (4)             /* Maximum number of video frames */

/* UVC Buffer size - Will map to ISO Transaction size */
#define CY_FX_UVC_STREAM_BUF_SIZE      (CY_FX_EP_ISO_VIDEO_PKTS_COUNT * CY_FX_EP_ISO_VIDEO_PKT_SIZE)

/* UVC Buffer count */
#define CY_FX_UVC_STREAM_BUF_COUNT     (10)

/* Low byte - UVC video streaming endpoint packet size */
#define CY_FX_EP_ISO_VIDEO_PKT_SIZE_L  (uint8_t)(CY_FX_EP_ISO_VIDEO_PKT_SIZE & 0x00FF)

/* High byte - UVC video streaming endpoint packet size and No. of ISO packets */
#define CY_FX_EP_ISO_VIDEO_PKT_SIZE_H  (uint8_t)(((CY_FX_EP_ISO_VIDEO_PKT_SIZE & 0xFF00) >> 8)  \
                                                 | ((CY_FX_EP_ISO_VIDEO_PKTS_COUNT-1) << 3))

/* Mult setting for USB 3.0. */
#define CY_FX_EP_ISO_VIDEO_SS_MULT      (CY_FX_EP_ISO_VIDEO_PKTS_COUNT)

/* Burst setting for USB 3.0. UVC-Iso does not support burst as yet. */
#define CY_FX_EP_ISO_VIDEO_SS_BURST     (1)

#define CY_FX_UVC_MAX_HEADER           (12)         /* Maximum number of header bytes in UVC */
#define CY_FX_UVC_HEADER_DEFAULT_BFH   (0x8C)       /* Default BFH(Bit Field Header) for the UVC Header */

#define CY_FX_UVC_MAX_PROBE_SETTING    (26)         /* Maximum number of bytes in Probe Control */
#define CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED    (32) /* Maximum number of bytes in Probe Control aligned to 32 byte */

#define CY_FX_UVC_HEADER_FRAME         (0)                    /* Normal frame indication */
#define CY_FX_UVC_HEADER_EOF           (uint8_t)(1 << 1)      /* End of frame indication */
#define CY_FX_UVC_HEADER_FRAME_ID      (uint8_t)(1 << 0)      /* Frame ID toggle bit */

#define CY_FX_USB_UVC_SET_REQ_TYPE     (uint8_t)(0x21)        /* UVC interface SET request type */
#define CY_FX_USB_UVC_GET_REQ_TYPE     (uint8_t)(0xA1)        /* UVC Interface GET request type */
#define CY_FX_USB_UVC_GET_CUR_REQ      (uint8_t)(0x81)        /* UVC GET_CUR request */
#define CY_FX_USB_UVC_SET_CUR_REQ      (uint8_t)(0x01)        /* UVC SET_CUR request */

#define CY_FX_UVC_STREAM_INTERFACE     (uint8_t)(1)           /* Streaming Interface : Alternate setting 1 */

/* Extern definitions of the USB Enumeration constant arrays used for the Application */
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

/* Extern definitions of the Video frame data */

/* UVC Probe Control Setting */
extern const uint8_t glProbeCtrl[CY_FX_UVC_MAX_PROBE_SETTING];
 
/* Video frame lengths */
extern const uint32_t glVidFrameLen[CY_FX_UVC_MAX_VID_FRAMES];

/* MJPEG Video Frames */
extern const uint8_t glUVCVidFrames[];

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYFXUVCINMEM_H_ */

/*[]*/

