/*
## Cypress USB 3.0 Platform source file (cycx3_uvcdscr.c)
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


/* This file contains the USB enumeration descriptors for the CX3 UVC YUY2 application example.
 * The descriptor arrays must be 32 byte aligned if the D-cache is turned on. If the linker
 * used is not capable of supporting the aligned feature for this, then dynamically allocated
 * buffer must be used, and the descriptor must be loaded into it. */

#include "cycx3_uvc.h"
#include "cyu3mipicsi.h"

/* Standard Device Descriptor for USB 3 */
const uint8_t CyCx3USB30DeviceDscr[] =
{
    0x12,                               /* Descriptor size */
    CY_U3P_USB_DEVICE_DESCR,            /* Device descriptor type */
    0x00, 0x03,                         /* USB 3.0 */
    0xEF,                               /* Device class */
    0x02,                               /* Device Sub-class */
    0x01,                               /* Device protocol */
    0x09,                               /* Maxpacket size for EP0 : 2^9 */
    0xB4, 0x04,                         /* Vendor ID */
    0xC3, 0x00,                         /* Product ID */
    0x00, 0x00,                         /* Device release number */
    0x01,                               /* Manufacture string index */
    0x02,                               /* Product string index */
    0x00,                               /* Serial number string index */
    0x01                                /* Number of configurations */
};

/* Standard Device Descriptor for USB 2 */
const uint8_t CyCx3USB20DeviceDscr[] =
{
    0x12,                               /* Descriptor size */
    CY_U3P_USB_DEVICE_DESCR,            /* Device descriptor type */
    0x10, 0x02,                         /* USB 2.1 */
    0xEF,                               /* Device class */
    0x02,                               /* Device sub-class */
    0x01,                               /* Device protocol */
    0x40,                               /* Maxpacket size for EP0 : 64 bytes */
    0xB4, 0x04,                         /* Vendor ID */
    0xC3, 0x00,                         /* Product ID */
    0x00, 0x00,                         /* Device release number */
    0x01,                               /* Manufacture string index */
    0x02,                               /* Product string index */
    0x00,                               /* Serial number string index */
    0x01                                /* Number of configurations */
};

/* Binary Device Object Store (BOS) Descriptor */
const uint8_t CyCx3USBBOSDscr[] =
{
    0x05,                               /* Descriptor size */
    CY_U3P_BOS_DESCR,                   /* Device descriptor type */
    0x16, 0x00,                         /* Length of this descriptor and all sub descriptors */
    0x02,                               /* Number of device capability descriptors */

    /* USB 2.0 Extension */
    0x07,                               /* Descriptor size */
    CY_U3P_DEVICE_CAPB_DESCR,           /* Device capability type descriptor */
    CY_U3P_USB2_EXTN_CAPB_TYPE,         /* USB 2.1 extension capability type */
    0x02, 0x00, 0x00, 0x00,             /* Supported device level features - LPM support */

    /* SuperSpeed Device Capability */
    0x0A,                               /* Descriptor size */
    CY_U3P_DEVICE_CAPB_DESCR,           /* Device capability type descriptor */
    CY_U3P_SS_USB_CAPB_TYPE,            /* SuperSpeed device capability type */
    0x00,                               /* Supported device level features  */
    0x0E, 0x00,                         /* Speeds supported by the device : SS, HS and FS */
    0x03,                               /* Functionality support */
    0x00,                               /* U1 device exit latency */
    0x00, 0x00                          /* U2 device exit latency */
};

/* Standard Device Qualifier Descriptor */
const uint8_t CyCx3USBDeviceQualDscr[] =
{
    0x0A,                               /* descriptor size */
    CY_U3P_USB_DEVQUAL_DESCR,           /* Device qualifier descriptor type */
    0x00, 0x02,                         /* USB 2.0 */
    0xEF,                               /* Device class */
    0x02,                               /* Device sub-class */
    0x01,                               /* Device protocol */
    0x40,                               /* Maxpacket size for EP0 : 64 bytes */
    0x01,                               /* Number of configurations */
    0x00                                /* Reserved */
};

/* Standard Super Speed Configuration Descriptor */
const uint8_t CyCx3USBSSConfigDscr[] =
{
    /* Configuration Descriptor*/
    0x09,                               /* Descriptor Size */
    CY_U3P_USB_CONFIG_DESCR,            /* Configuration Descriptor Type */
    0x16, 0x01,                         /* Length of this descriptor and all sub descriptors */
    0x02,                               /* Number of interfaces */
    0x01,                               /* Configuration number */
    0x03,                               /* Configuration string index */
    0xC0,                               /* Config characteristics - Self Powered */
    0x0C,                               /* Max power consumption of device (in 8mA unit) : 96mA */

    /* Interface Association Descriptor */
    0x08,                               /* Descriptor Size */
    CX3_INTRFC_ASSN_DESCR,              /* Interface Association Descriptor Type */
    0x00,                               /* Interface number of the VideoControl interface 
                                           that is associated with this function*/
    0x02,                               /* Number of contiguous Video interfaces that are 
                                           associated with this function */
    0x0E,                               /* Video Interface Class Code: CC_VIDEO */
    0x03,                               /* Subclass code: SC_VIDEO_INTERFACE_COLLECTION*/
    0x00,                               /* Protocol: PC_PROTOCOL_UNDEFINED*/
    0x00,                               /* String Descriptor index for interface */

    /* Standard Video Control Interface Descriptor (Interface 0, Alternate Setting 0)*/
    0x09,                               /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,            /* Interface Descriptor type */
    0x00,                               /* Index of this Interface */
    0x00,                               /* Alternate setting number */
    0x01,                               /* Number of end points - 1 Interrupt Endpoint*/
    0x0E,                               /* Video Interface Class Code: CC_VIDEO  */
    0x01,                               /* Interface sub class: SC_VIDEOCONTROL */
    0x00,                               /* Interface protocol code: PC_PROTOCOL_UNDEFINED.*/
    0x00,                               /* Interface descriptor string index */

    /* Class specific VC Interface Header Descriptor */
    0x0D,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class Specific Interface Descriptor type: CS_INTERFACE */
    0x01,                               /* Descriptor Sub type: VC_HEADER */
#ifdef CX3_UVC_1_0_SUPPORT
    0x00, 0x01,                         /* Revision of UVC class spec: 1.0 - Legacy version */
#else
    0x10, 0x01,                         /* Revision of UVC class spec: 1.1 - Minimum version required 
                                           for USB Compliance. Not supported on Windows XP*/
#endif
    0x51, 0x00,                         /* Total Size of class specific descriptors 
                                           (till Output terminal) */
    0x00, 0x6C, 0xDC, 0x02,             /* Clock frequency : 48MHz(Deprecated) */
    0x01,                               /* Number of streaming interfaces */
    0x01,                               /*VideoStreaming interface 1 belongs to this 
                                          VideoControl interface*/

    /* Input (Camera) Terminal Descriptor */
    0x12,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* CS_INTERFACE */
    0x02,                               /* VC_INPUT_TERMINAL subtype */
    0x01,                               /* ID of this input terminal */
    0x01, 0x02,                         /* ITT_CAMERA type. This terminal is a camera 
                                           terminal representing the CCD sensor*/
    0x00,                               /* No association terminal */
    0x00,                               /* Unused */
    0x00, 0x00,                         /* No optical zoom supported */
    0x00, 0x00,                         /* No optical zoom supported */
    0x00, 0x00,                         /* No optical zoom supported */
    0x03,                               /* Size of controls field for this terminal : 3 bytes */
    0x00, 0x00, 0x00,                   /* No controls supported */

    /* Processing Unit Descriptor */
    0x0D,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class specific interface desc type */
    0x05,                               /* Processing Unit Descriptor type: VC_PROCESSING_UNIT*/
    0x02,                               /* ID of this unit */
    0x01,                               /* Source ID: 1: Conencted to input terminal */
    0x00, 0x40,                         /* Digital multiplier */
    0x03,                               /* Size of controls field for this terminal: 3 bytes */
    0x00, 0x00, 0x00,                   /* No controls supported */
    0x00,                               /* String desc index: Not used */
    0x00,                               /* Analog Video Standards Supported: None */
    
    /* Extension Unit Descriptor */
    0x1C,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class specific interface desc type */
    0x06,                               /* Extension Unit Descriptor type */
    0x03,                               /* ID of this terminal */
    0xFF, 0xFF, 0xFF, 0xFF,             /* 16 byte GUID */
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0x00,                               /* Number of controls in this terminal */
    0x01,                               /* Number of input pins in this terminal */
    0x02,                               /* Source ID : 2 : Connected to Proc Unit */
    0x03,                               /* Size of controls field for this terminal : 3 bytes */
    0x00, 0x00, 0x00,                   /* No controls supported */
    0x00,                               /* String descriptor index : Not used */

    /* Output Terminal Descriptor */
    0x09,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class specific interface desc type */
    0x03,                               /* Output Terminal Descriptor type */
    0x04,                               /* ID of this terminal */
    0x01, 0x01,                         /* USB Streaming terminal type */
    0x00,                               /* No association terminal */
    0x03,                               /* Source ID : 3 : Connected to Extn Unit */
    0x00,                               /* String desc index : Not used */

    /* Video Control Status Interrupt Endpoint Descriptor */
    0x07,                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,            /* Endpoint Descriptor Type */
    CX3_EP_CONTROL_STATUS,              /* Endpoint address and description: EP-2 IN */
    CY_U3P_USB_EP_INTR,                 /* Interrupt End point Type */
    0x40, 0x00,                         /* Max packet size: 64 bytes */
    0x01,                               /* Servicing interval */

    /* Super Speed Endpoint Companion Descriptor */
    0x06,                               /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,           /* SS Endpoint Companion Descriptor Type */
    0x00,                               /* Max no. of packets in a Burst: 1 */
    0x00,                               /* Attribute: N.A. */
    0x40,                               /* Bytes per interval: 1024 */
    0x00,

    /* Class Specific Interrupt Endpoint Descriptor */
    0x05,                               /* Descriptor size */
    0x25,                               /* Class Specific Endpoint Descriptor Type */
    CY_U3P_USB_EP_INTR,                 /* End point Sub Type */
    0x40, 0x00,                         /* Max packet size = 64 bytes */

    /* Standard Video Streaming Interface Descriptor (Interface 1, Alternate Setting 0) */
    0x09,                               /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,            /* Interface Descriptor type */
    0x01,                               /* Interface number: 1 */
    0x00,                               /* Alternate setting number: 0 */
    0x01,                               /* Number of end points: 1 Bulk Endpoint */
    0x0E,                               /* Interface class : CC_VIDEO */
    0x02,                               /* Interface sub class : SC_VIDEOSTREAMING */
    0x00,                               /* Interface protocol code : PC_PROTOCOL_UNDEFINED */
    0x00,                               /* Interface descriptor string index */

    /* Class-specific Video Streaming Input Header Descriptor */
    0x0E,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class-specific VS I/f Type */
    0x01,                               /* Descriptor Subtype: Input Header */
    0x01,                               /* 1 format desciptor follows */
    0x65,0x00,                          /* Total size of Class specific VS descr */
    CX3_EP_BULK_VIDEO,                  /* EP address for BULK video data: EP 3 IN */
    0x00,                               /* No dynamic format change supported */
    0x04,                               /* Output terminal ID : 4 */
    0x00,                               /* No Still image capture method supported */
    0x00,                               /* Hardware trigger NOT supported */
    0x00,                               /* Hardware to initiate still image capture not supported */
    0x01,                               /* Size of controls field : 1 byte */
    0x00,                               /* D2 : Compression quality supported - No compression*/

    /* Class specific Uncompressed VS format descriptor */
    0x1B,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class-specific VS interface Type */
    0x04,                               /* Subtype : VS_FORMAT_UNCOMPRESSED */
    0x01,                               /* Format desciptor index */
    0x03,                               /* Number of Frame Descriptors that follow this descriptor: 3 */
    
    /* GUID, globally unique identifier used to identify streaming-encoding format: YUY2  */
    0x59, 0x55, 0x59, 0x32,             /*MEDIASUBTYPE_YUY2 GUID: 32595559-0000-0010-8000-00AA00389B71 */
    0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0xAA,
    0x00, 0x38, 0x9B, 0x71,
    0x10,                               /* Number of bits per pixel: 16*/
    0x02,                               /* Optimum Frame Index for this stream: 2 (720p) */
    0x00,                               /* X dimension of the picture aspect ratio; Non-interlaced */
    0x00,                               /* Y dimension of the pictuer aspect ratio: Non-interlaced */
    0x00,                               /* Interlace Flags: Progressive scanning, no interlace */
    0x00,                               /* duplication of the video stream restriction: 0 - no restriction */

    /* Class specific Uncompressed VS Frame Descriptor 1 - 1080p@30fps */
    0x1E,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Descriptor type*/
    0x05,                               /* Subtype: Uncompressed frame interface*/
    0x01,                               /* Frame Descriptor Index: 1 */
    0x00,                               /* No Still image capture method supported */
    0x80, 0x07,                         /* Width in pixel:  1080 */	
    0x38, 0x04,                         /* Height in pixel: 1920 */
    0x00, 0x80, 0x53, 0x3B,             /* Min bit rate (bits/s): 1080 x 1920 x 2 x 30 x 8 = 995328000 */
    0x00, 0x80, 0x53, 0x3B,             /* Max bit rate (bits/s): Fixed rate so same as Min */
    0x00, 0x48, 0x3F, 0x00,             /* Maximum video or still frame size in bytes(Deprecated): 1920 x 1080 x 2 */
    0x15, 0x16, 0x05, 0x00,             /* Default frame interval (in 100ns units): (1/30)x10^7 */
    0x01,                               /* Frame interval type : No of discrete intervals */
    0x15, 0x16, 0x05, 0x00,             /* Frame interval 3: Same as Default frame interval */

    /* Class specific Uncompressed VS frame descriptor 2 - 720p@ 60fps*/
    0x1E,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Descriptor type*/
    0x05,                               /* Subtype: Uncompressed frame interface*/
    0x02,                               /* Frame Descriptor Index: 2 */
    0x00,                               /* No Still image capture method supported */
    0x00, 0x05,                         /* Width in pixel: 720 */
    0xD0, 0x02,                         /* Height in pixel: 1280 */
    0x00, 0x00, 0x57, 0x30,             /* Min bit rate @55fps (bits/s): 720 x 1280 x 2 x 55 x 8 = 811008000 */
    0x00, 0x00, 0xBC, 0x34,             /* Max bit rate @60fps (bits/s). 720 x 1280 x 2 x 60 x 8 884736000 */
    0x00, 0x20, 0x1C, 0x00,             /* Maximum video frame size in bytes(Deprecated): 1280 x 720 x 2   */
    0x0A, 0x8B, 0x02, 0x00,             /* Default frame interval (in 100ns units): (1/60)x10^7 */
    0x01,                               /* Frame interval type : No of discrete intervals */
    0x0A, 0x8B, 0x02, 0x00,             /* Frame interval 3: Same as Default frame interval */
    
    /* Class specific Uncompressed VS frame descriptor 3 - 5MP@15fps*/
    0x1E,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Descriptor type*/
    0x05,                               /* Subtype: Uncompressed frame interface*/
    0x03,                               /* Frame Descriptor Index: 3 */
    0x00,                               /* No Still image capture method supported */
    0x20, 0x0A,                         /* Width in pixel: 2592 */
    0x98, 0x07,                         /* Height in pixel: 1944 */
    0x00, 0xD0, 0x14, 0x48,             /* Min bit rate @15fps (bits/s): 2592 x 1944 x 2 x 15 x 8 = 1209323520 */
    0x00, 0xD0, 0x14, 0x48,             /* Max bit rate @15fps (bits/s): 2592 x 1944 x 2 x 15 x 8 = 1209323520 */
    0x00, 0xC6, 0x99, 0x00,             /* Maximum video frame size in bytes(Deprecated): 2592 x 1944 x 2 = 10077696  */
    0x2A, 0x2C, 0x0A, 0x00,             /* Default frame interval (in 100ns units): (1/15)x10^7 */
    0x01,                               /* Frame interval type : No of discrete intervals */
    0x2A, 0x2C, 0x0A, 0x00,             /* Frame interval 3: Same as Default frame interval */
    
    /* Endpoint Descriptor for BULK Streaming Video Data */
    0x07,                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,            /* Endpoint Descriptor Type */
    CX3_EP_BULK_VIDEO,                  /* Endpoint address and description: EP 3 IN */
    CY_U3P_USB_EP_BULK,                 /* BULK End point */
    CX3_EP_BULK_VIDEO_PKT_SIZE_L,       /* CX3_EP_BULK_VIDEO_PKT_SIZE_L */
    CX3_EP_BULK_VIDEO_PKT_SIZE_H,       /* CX3_EP_BULK_VIDEO_PKT_SIZE_H */
    0x00,                               /* Servicing interval for data transfers */

    /* Super Speed Endpoint Companion Descriptor */
    0x06,                               /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,           /* SS Endpoint Companion Descriptor Type */
    0x0F,                               /* Max number of packets per burst: 12 */
    0x00,                               /* Attribute: Streams not defined */
    0x00,                               /* No meaning for bulk */
    0x00
};

/* Standard High Speed Configuration Descriptor */
const uint8_t CyCx3USBHSConfigDscr[] =
{
    /* Configuration descriptor */
    0x09,                               /* Descriptor size */
    CY_U3P_USB_CONFIG_DESCR,            /* Configuration descriptor type */
#ifdef CX3_UVC_1_0_SUPPORT
    0xEB,0x00,                          /* Length of this descriptor and all sub descriptors */
#else
    0xEC,0x00,                          /* Length of this descriptor and all sub descriptors */
#endif
    0x02,                               /* Number of interfaces */
    0x01,                               /* Configuration number */
    0x04,                               /* Configuration string index */
    0xC0,                               /* Config characteristics - self powered */
    0x32,                               /* Max power consumption of device (in 2mA unit) : 100mA */

    /* Interface Association Descriptor */
    0x08,                               /* Descriptor Size */
    CX3_INTRFC_ASSN_DESCR,              /* Interface Association Descriptor Type */
    0x00,                               /* Interface number of the VideoControl interface 
                                           that is associated with this function*/
    0x02,                               /* Number of contiguous Video interfaces that are 
                                           associated with this function */
    0x0E,                               /* Video Interface Class Code: CC_VIDEO */
    0x03,                               /* Subclass code: SC_VIDEO_INTERFACE_COLLECTION*/
    0x00,                               /* Protocol: PC_PROTOCOL_UNDEFINED*/
    0x00,                               /* String Descriptor index for interface */

    /* Standard Video Control Interface Descriptor (Interface 0, Alternate Setting 0)*/
    0x09,                               /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,            /* Interface Descriptor type */
    0x00,                               /* Index of this Interface */
    0x00,                               /* Alternate setting number */
    0x01,                               /* Number of end points - 1 Interrupt Endpoint*/
    0x0E,                               /* Video Interface Class Code: CC_VIDEO  */
    0x01,                               /* Interface sub class: SC_VIDEOCONTROL */
    0x00,                               /* Interface protocol code: PC_PROTOCOL_UNDEFINED.*/
    0x00,                               /* Interface descriptor string index */

    /* Class specific VC Interface Header Descriptor */
    0x0D,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class Specific Interface Descriptor type: CS_INTERFACE */
    0x01,                               /* Descriptor Sub type: VC_HEADER */
#ifdef CX3_UVC_1_0_SUPPORT
    0x00, 0x01,                         /* Revision of UVC class spec: 1.0 - Legacy version */
    0x50, 0x00,                         /* Total Size of class specific descriptors 
                                           (till Output terminal) */
#else
    0x10, 0x01,                         /* Revision of UVC class spec: 1.1 - Minimum version required 
                                           for USB Compliance. Not supported on Windows XP*/
    0x51, 0x00,                         /* Total Size of class specific descriptors 
                                           (till Output terminal) */
#endif
    0x00, 0x6C, 0xDC, 0x02,             /* Clock frequency : 48MHz(Deprecated) */
    0x01,                               /* Number of streaming interfaces */
    0x01,                               /*VideoStreaming interface 1 belongs to this 
                                          VideoControl interface*/

    /* Input (Camera) Terminal Descriptor */
    0x12,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* CS_INTERFACE */
    0x02,                               /* VC_INPUT_TERMINAL subtype */
    0x01,                               /* ID of this input terminal */
    0x01, 0x02,                         /* ITT_CAMERA type. This terminal is a camera 
                                           terminal representing the CCD sensor*/
    0x00,                               /* No association terminal */
    0x00,                               /* Unused */
    0x00, 0x00,                         /* No optical zoom supported */
    0x00, 0x00,                         /* No optical zoom supported */
    0x00, 0x00,                         /* No optical zoom supported */
    0x03,                               /* Size of controls field for this terminal : 3 bytes */
    0x00, 0x00, 0x00,                   /* No controls supported */

    /* Processing Unit Descriptor */
#ifdef CX3_UVC_1_0_SUPPORT
    0x0C,                               /* Descriptor size */
#else
    0x0D,                               /* Descriptor size */
#endif    
    CX3_CS_INTRFC_DESCR,                /* Class specific interface desc type */
    0x05,                               /* Processing Unit Descriptor type: VC_PROCESSING_UNIT*/
    0x02,                               /* ID of this unit */
    0x01,                               /* Source ID: 1: Conencted to input terminal */
    0x00, 0x40,                         /* Digital multiplier */
    0x03,                               /* Size of controls field for this terminal: 3 bytes */
    0x00, 0x00, 0x00,                   /* No controls supported */
    0x00,                               /* String desc index: Not used */
#ifndef CX3_UVC_1_0_SUPPORT
    0x00,                               /* Analog Video Standards Supported: None */
#endif    
    /* Extension Unit Descriptor */
    0x1C,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class specific interface desc type */
    0x06,                               /* Extension Unit Descriptor type */
    0x03,                               /* ID of this terminal */
    0xFF, 0xFF, 0xFF, 0xFF,             /* 16 byte GUID */
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0x00,                               /* Number of controls in this terminal */
    0x01,                               /* Number of input pins in this terminal */
    0x02,                               /* Source ID : 2 : Connected to Proc Unit */
    0x03,                               /* Size of controls field for this terminal : 3 bytes */
    0x00, 0x00, 0x00,                   /* No controls supported */
    0x00,                               /* String descriptor index : Not used */

    /* Output Terminal Descriptor */
    0x09,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class specific interface desc type */
    0x03,                               /* Output Terminal Descriptor type */
    0x04,                               /* ID of this terminal */
    0x01, 0x01,                         /* USB Streaming terminal type */
    0x00,                               /* No association terminal */
    0x03,                               /* Source ID : 3 : Connected to Extn Unit */
    0x00,                               /* String desc index : Not used */

    /* Video Control Status Interrupt Endpoint Descriptor */
    0x07,                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,            /* Endpoint Descriptor Type */
    CX3_EP_CONTROL_STATUS,              /* Endpoint address and description: EP-2 IN */
    CY_U3P_USB_EP_INTR,                 /* Interrupt End point Type */
    0x40, 0x00,                         /* Max packet size: 64 bytes */
    0x01,                               /* Servicing interval */

    /* Class Specific Interrupt Endpoint Descriptor */
    0x05,                               /* Descriptor size */
    0x25,                               /* Class specific endpoint descriptor type */
    CY_U3P_USB_EP_INTR,                 /* End point sub type */
    0x40,0x00,                          /* Max packet size = 64 bytes */

    /* Standard Video Streaming Interface Descriptor (Interface 1, Alternate Setting 0) */
    0x09,                               /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,            /* Interface descriptor type */
    0x01,                               /* Interface number: 1 */
    0x00,                               /* Alternate setting number: 0 */
    0x01,                               /* Number of end points: 1 Bulk endopoint*/
    0x0E,                               /* Interface class : CC_VIDEO */
    0x02,                               /* Interface sub class : SC_VIDEOSTREAMING */
    0x00,                               /* Interface protocol code : PC_PROTOCOL_UNDEFINED */
    0x00,                               /* Interface descriptor string index */

    /* Class-specific Video Streaming Input Header Descriptor */
    0x0E,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class-specific VS interface type */
    0x01,                               /* Descriptotor subtype : Input Header */
    0x01,                               /* 1 format desciptor follows */
    0x65, 0x00,                         /* Total size of class specific VS descr */
    CX3_EP_BULK_VIDEO,                  /* EP address for BULK video data: EP 3 IN  */
    0x00,                               /* No dynamic format change supported */
    0x04,                               /* Output terminal ID : 4 */
    0x00,                               /* No Still image capture method supported */
    0x00,                               /* Hardware trigger not supported */
    0x00,                               /* Hardware to initiate still image capture not supported*/
    0x01,                               /* Size of controls field : 1 byte */
    0x00,                               /* D2 : Compression quality supported - No compression */

    /* Class specific VS format descriptor */
    0x1B,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class-specific VS interface Type */
    0x04,                               /* Subtype : VS_FORMAT_UNCOMPRESSED  */
    0x01,                               /* Format desciptor index */
    0x02,                               /* Number of Frame Descriptors that follow this descriptor: 2 */
    /* GUID, globally unique identifier used to identify streaming-encoding format: YUY2  */
    0x59, 0x55, 0x59, 0x32,             /*MEDIASUBTYPE_YUY2 GUID: 32595559-0000-0010-8000-00AA00389B71 */
    0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0xAA,
    0x00, 0x38, 0x9B, 0x71,
    0x10,                               /* Number of bits per pixel: 16*/
    0x01,                               /* Optimum Frame Index for this stream: 1 (640x480) */
    0x00,                               /* X dimension of the picture aspect ratio; Non-interlaced */
    0x00,                               /* Y dimension of the pictuer aspect ratio: Non-interlaced */
    0x00,                               /* Interlace Flags: Progressive scanning, no interlace */
    0x00,                               /* duplication of the video stream restriction: 0 - no restriction */

    /* Class specific Uncompressed VS frame descriptor 1 - 640x480@60fps*/
    0x1E,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Descriptor type*/
    0x05,                               /* Subtype: Uncompressed frame interface*/
    0x01,                               /* Frame Descriptor Index: 1 */
    0x00,                               /* No Still image capture method supported */
    0x80, 0x02,                         /* Width in pixel:  640 */
    0xE0, 0x01,                         /* Height in pixel: 480 */
    0x00, 0x00, 0x94, 0x11,             /* Min bit rate (bits/s): 640 x 480 x 2 x 60 x 8 = 294912000 */
    0x00, 0x00, 0x94, 0x11,             /* Max bit rate (bits/s): Fixed rate so same as Min  */
    0x00, 0x60, 0x09, 0x00,             /* Maximum video or still frame size in bytes(Deprecated): 640 x 480 x 2*/
    0x0A, 0x8B, 0x02, 0x00,             /* Default frame interval (in 100ns units): (1/60)x10^7 */
    0x01,                               /* Frame interval type : No of discrete intervals */
    0x0A, 0x8B, 0x02, 0x00,             /* Frame interval 3: Same as Default frame interval */

    /* Class specific Uncompressed VS frame descriptor 2 - 640x480@60fps*/
    0x1E,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Descriptor type*/
    0x05,                               /* Subtype: Uncompressed frame interface*/
    0x02,                               /* Frame Descriptor Index: 2 */
    0x00,                               /* No Still image capture method supported */
    0x80, 0x02,                         /* Width in pixel:  640 */
    0xE0, 0x01,                         /* Height in pixel: 480 */
    0x00, 0x00, 0x94, 0x11,             /* Min bit rate (bits/s): 640 x 480 x 2 x 60 x 8 = 294912000 */
    0x00, 0x00, 0x94, 0x11,             /* Max bit rate (bits/s): Fixed rate so same as Min  */
    0x00, 0x60, 0x09, 0x00,             /* Maximum video or still frame size in bytes(Deprecated): 640 x 480 x 2*/
    0x0A, 0x8B, 0x02, 0x00,             /* Default frame interval (in 100ns units): (1/60)x10^7 */
    0x01,                               /* Frame interval type : No of discrete intervals */
    0x0A, 0x8B, 0x02, 0x00,             /* Frame interval 3: Same as Default frame interval */

    /* Endpoint descriptor for Bulk streaming video data */
    0x07,                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,            /* Endpoint Descriptor Type */
    CX3_EP_BULK_VIDEO,                  /* Endpoint address and description: EP 3 IN */
    CY_U3P_USB_EP_BULK,                 /* BULK End point */
    0x00,                               /* Packet Size: 512 bytes */
    0x02,
    0x00                                /* Servicing interval for data transfers */
};

/* Standard Full Speed Configuration Descriptor*/
const uint8_t CyCx3USBFSConfigDscr[] =
{
    /* Configuration descriptor */
    0x09,                               /* Descriptor size */
    CY_U3P_USB_CONFIG_DESCR,            /* Configuration descriptor type */
#ifdef CX3_UVC_1_0_SUPPORT
    0xEB,0x00,                          /* Length of this descriptor and all sub descriptors */
#else
    0xEC,0x00,                          /* Length of this descriptor and all sub descriptors */
#endif
    0x02,                               /* Number of interfaces */
    0x01,                               /* Configuration number */
    0x05,                               /* Configuration string index */
    0xC0,                               /* Config characteristics - self powered */
    0x32,                               /* Max power consumption of device (in 2mA unit) : 100mA */

    /* Interface Association Descriptor */
    0x08,                               /* Descriptor Size */
    CX3_INTRFC_ASSN_DESCR,              /* Interface Association Descriptor Type */
    0x00,                               /* Interface number of the VideoControl interface 
                                           that is associated with this function*/
    0x02,                               /* Number of contiguous Video interfaces that are 
                                           associated with this function */
    0x0E,                               /* Video Interface Class Code: CC_VIDEO */
    0x03,                               /* Subclass code: SC_VIDEO_INTERFACE_COLLECTION*/
    0x00,                               /* Protocol: PC_PROTOCOL_UNDEFINED*/
    0x00,                               /* String Descriptor index for interface */

    /* Standard Video Control Interface Descriptor (Interface 0, Alternate Setting 0)*/
    0x09,                               /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,            /* Interface Descriptor type */
    0x00,                               /* Index of this Interface */
    0x00,                               /* Alternate setting number */
    0x01,                               /* Number of end points - 1 Interrupt Endpoint*/
    0x0E,                               /* Video Interface Class Code: CC_VIDEO  */
    0x01,                               /* Interface sub class: SC_VIDEOCONTROL */
    0x00,                               /* Interface protocol code: PC_PROTOCOL_UNDEFINED.*/
    0x00,                               /* Interface descriptor string index */

    /* Class specific VC Interface Header Descriptor */
    0x0D,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class Specific Interface Descriptor type: CS_INTERFACE */
    0x01,                               /* Descriptor Sub type: VC_HEADER */
#ifdef CX3_UVC_1_0_SUPPORT
    0x00, 0x01,                         /* Revision of UVC class spec: 1.0 - Legacy version */
    0x50, 0x00,                         /* Total Size of class specific descriptors 
                                           (till Output terminal) */
#else
    0x10, 0x01,                         /* Revision of UVC class spec: 1.1 - Minimum version required 
                                           for USB Compliance. Not supported on Windows XP*/
    0x51, 0x00,                         /* Total Size of class specific descriptors 
                                           (till Output terminal) */
#endif
    0x00, 0x6C, 0xDC, 0x02,             /* Clock frequency : 48MHz(Deprecated) */
    0x01,                               /* Number of streaming interfaces */
    0x01,                               /*VideoStreaming interface 1 belongs to this 
                                          VideoControl interface*/

    /* Input (Camera) Terminal Descriptor */
    0x12,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* CS_INTERFACE */
    0x02,                               /* VC_INPUT_TERMINAL subtype */
    0x01,                               /* ID of this input terminal */
    0x01, 0x02,                         /* ITT_CAMERA type. This terminal is a camera 
                                           terminal representing the CCD sensor*/
    0x00,                               /* No association terminal */
    0x00,                               /* Unused */
    0x00, 0x00,                         /* No optical zoom supported */
    0x00, 0x00,                         /* No optical zoom supported */
    0x00, 0x00,                         /* No optical zoom supported */
    0x03,                               /* Size of controls field for this terminal : 3 bytes */
    0x00, 0x00, 0x00,                   /* No controls supported */

    /* Processing Unit Descriptor */
#ifdef CX3_UVC_1_0_SUPPORT
    0x0C,                               /* Descriptor size */
#else
    0x0D,                               /* Descriptor size */
#endif    
    CX3_CS_INTRFC_DESCR,                /* Class specific interface desc type */
    0x05,                               /* Processing Unit Descriptor type: VC_PROCESSING_UNIT*/
    0x02,                               /* ID of this unit */
    0x01,                               /* Source ID: 1: Conencted to input terminal */
    0x00, 0x40,                         /* Digital multiplier */
    0x03,                               /* Size of controls field for this terminal: 3 bytes */
    0x00, 0x00, 0x00,                   /* No controls supported */
    0x00,                               /* String desc index: Not used */
#ifndef CX3_UVC_1_0_SUPPORT
    0x00,                               /* Analog Video Standards Supported: None */
#endif    

    
    /* Extension Unit Descriptor */
    0x1C,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class specific interface desc type */
    0x06,                               /* Extension Unit Descriptor type */
    0x03,                               /* ID of this terminal */
    0xFF, 0xFF, 0xFF, 0xFF,             /* 16 byte GUID */
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0x00,                               /* Number of controls in this terminal */
    0x01,                               /* Number of input pins in this terminal */
    0x02,                               /* Source ID : 2 : Connected to Proc Unit */
    0x03,                               /* Size of controls field for this terminal : 3 bytes */
    0x00, 0x00, 0x00,                   /* No controls supported */
    0x00,                               /* String descriptor index : Not used */

    /* Output Terminal Descriptor */
    0x09,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class specific interface desc type */
    0x03,                               /* Output Terminal Descriptor type */
    0x04,                               /* ID of this terminal */
    0x01, 0x01,                         /* USB Streaming terminal type */
    0x00,                               /* No association terminal */
    0x03,                               /* Source ID : 3 : Connected to Extn Unit */
    0x00,                               /* String desc index : Not used */

    /* Video Control Status Interrupt Endpoint Descriptor */
    0x07,                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,            /* Endpoint Descriptor Type */
    CX3_EP_CONTROL_STATUS,              /* Endpoint address and description: EP-2 IN */
    CY_U3P_USB_EP_INTR,                 /* Interrupt End point Type */
    0x40, 0x00,                         /* Max packet size: 64 bytes */
    0x01,                               /* Servicing interval */

    /* Class Specific Interrupt Endpoint Descriptor */
    0x05,                               /* Descriptor size */
    0x25,                               /* Class specific endpoint descriptor type */
    CY_U3P_USB_EP_INTR,                 /* End point sub type */
    0x40,0x00,                          /* Max packet size = 64 bytes */

    /* Standard Video Streaming Interface Descriptor (Interface 1, Alternate Setting 0) */
    0x09,                               /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,            /* Interface descriptor type */
    0x01,                               /* Interface number: 1 */
    0x00,                               /* Alternate setting number: 0 */
    0x01,                               /* Number of end points: 1 Bulk endopoint*/
    0x0E,                               /* Interface class : CC_VIDEO */
    0x02,                               /* Interface sub class : SC_VIDEOSTREAMING */
    0x00,                               /* Interface protocol code : PC_PROTOCOL_UNDEFINED */
    0x00,                               /* Interface descriptor string index */

    /* Class-specific Video Streaming Input Header Descriptor */
    0x0E,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class-specific VS interface type */
    0x01,                               /* Descriptotor subtype : Input Header */
    0x01,                               /* 1 format desciptor follows */
    0x65, 0x00,                         /* Total size of class specific VS descr */
    CX3_EP_BULK_VIDEO,                  /* EP address for BULK video data: EP 3 IN  */
    0x00,                               /* No dynamic format change supported */
    0x04,                               /* Output terminal ID : 4 */
    0x00,                               /* No Still image capture method supported */
    0x00,                               /* Hardware trigger not supported */
    0x00,                               /* Hardware to initiate still image capture not supported*/
    0x01,                               /* Size of controls field : 1 byte */
    0x00,                               /* D2 : Compression quality supported - No compression */

    /* Class specific VS format descriptor */
    0x1B,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Class-specific VS interface Type */
    0x04,                               /* Subtype : VS_FORMAT_UNCOMPRESSED  */
    0x01,                               /* Format desciptor index */
    0x02,                               /* Number of Frame Descriptors that follow this descriptor: 2 */
    /* GUID, globally unique identifier used to identify streaming-encoding format: YUY2  */
    0x59, 0x55, 0x59, 0x32,             /*MEDIASUBTYPE_YUY2 GUID: 32595559-0000-0010-8000-00AA00389B71 */
    0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0xAA,
    0x00, 0x38, 0x9B, 0x71,
    0x10,                               /* Number of bits per pixel: 16*/
    0x02,                               /* Optimum Frame Index for this stream: 1 (320x240) */
    0x00,                               /* X dimension of the picture aspect ratio; Non-interlaced */
    0x00,                               /* Y dimension of the pictuer aspect ratio: Non-interlaced */
    0x00,                               /* Interlace Flags: Progressive scanning, no interlace */
    0x00,                               /* duplication of the video stream restriction: 0 - no restriction */

   /* Class specific Uncompressed VS Frame Descriptor 1 - 320x240@5fps*/
    0x1E,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Descriptor type*/
    0x05,                               /* Subtype: Uncompressed frame interface*/
    0x01,                               /* Frame Descriptor Index: 1 */
    0x00,                               /* No Still image capture method supported */
    0x40, 0x01,                         /* Width in pixel:  320 */
    0xF0, 0x00,                         /* Height in pixel: 240 */
    0x00, 0xC0, 0x5D, 0x00,             /* Min bit rate (bits/s): 320 x 240 x 2 x 5 x 8 = 6144000 */
    0x00, 0xC0, 0x5D, 0x00,             /* Max bit rate (bits/s): Fixed rate so same as Min  */
    0x00, 0x58, 0x02, 0x00,             /* Maximum video or still frame size in bytes(Deprecated): 320 x 240 x 2*/
    0x80, 0x84, 0x1E, 0x00,             /* Default frame interval (in 100ns units): (1/5)x10^7 */
    0x01,                               /* Frame interval type : No of discrete intervals */
    0x80, 0x84, 0x1E, 0x00,             /* Frame interval 3: Same as Default frame interval */

    /* Class specific Uncompressed VS frame descriptor 2 - 320 x 240 @2fps - Needed only for compliance. 
     * The USBCV30 USB 2.0 UVC Tests 'Standard VS Interface Descriptor Test - Device Configured' and 
     * 'Standard VS Interface Descriptor Test - Device Addressed', Fail if only one VS Frame descriptor 
     * is provided. 
     */
    0x1E,                               /* Descriptor size */
    CX3_CS_INTRFC_DESCR,                /* Descriptor type*/
    0x05,                               /* Subtype: Uncompressed frame interface*/
    0x02,                               /* Frame Descriptor Index: 1 */
    0x00,                               /* No Still image capture method supported */
    0x40, 0x01,                         /* Width in pixel:  320 */
    0xF0, 0x00,                         /* Height in pixel: 240 */
    0x00, 0x80, 0x25, 0x00,             /* Min bit rate (bits/s): 320 x 240 x 2 x 2 x 8 = 2457600 */
    0x00, 0x80, 0x25, 0x00,             /* Max bit rate (bits/s): Fixed rate so same as Min  */
    0x00, 0x58, 0x02, 0x00,             /* Maximum video or still frame size in bytes(Deprecated): 320 x 240 x 2*/
    0x40, 0x4B, 0x4C, 0x00,             /* Default frame interval (in 100ns units): (1/5)x10^7 */
    0x01,                               /* Frame interval type : No of discrete intervals */
    0x40, 0x4B, 0x4C, 0x00,             /* Frame interval 3: Same as Default frame interval */

    /* Endpoint descriptor for Bulk streaming video data */
    0x07,                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,            /* Endpoint Descriptor Type */
    CX3_EP_BULK_VIDEO,                  /* Endpoint address and description: EP 3 IN */
    CY_U3P_USB_EP_BULK,                 /* BULK End point */
    0x40,                               /* EP Packet Size: 64 bytes */
    0x00,
    0x00                                /* Servicing interval for data transfers */
};

/* Standard language ID string descriptor */
const uint8_t CyCx3USBStringLangIDDscr[] =
{
    0x04,                               /* Descriptor size */
    CY_U3P_USB_STRING_DESCR,            /* Device descriptor type */
    0x09,0x04                           /* Language ID supported */
};

/* Standard manufacturer string descriptor */
const uint8_t CyCx3USBManufactureDscr[] =
{
    0x10,                               /* Descriptor size */
    CY_U3P_USB_STRING_DESCR,            /* Device descriptor type */
    'C',0x00,                           /* Manufacturer String */
    'y',0x00,
    'p',0x00,
    'r',0x00,
    'e',0x00,
    's',0x00,
    's',0x00
};

/* Standard product string descriptor */
const uint8_t CyCx3USBProductDscr[] =
{
    0x10,                               /* Descriptor Size */
    CY_U3P_USB_STRING_DESCR,            /* Device descriptor type */
    'C', 0x00,                          /* Product Descriptor String */
    'X', 0x00,
    '3', 0x00,
    '-', 0x00,
    'U', 0x00,
    'V', 0x00,
    'C', 0x00
};

/* Standard product string descriptor */
const uint8_t CyCx3USBConfigSSDscr[] =
{
    0x10,                               /* Descriptor Size */
    CY_U3P_USB_STRING_DESCR,            /* Device descriptor type */
    'U', 0x00,                          /* Super-Speed Configuration Descriptor */
    'S', 0x00,
    'B', 0x00,
    '-', 0x00,
    '3', 0x00,
    '.', 0x00,
    '0', 0x00
};
/* Standard product string descriptor */
const uint8_t CyCx3USBConfigHSDscr[] =
{
    0x10,                               /* Descriptor Size */
    CY_U3P_USB_STRING_DESCR,            /* Device descriptor type */
    'U', 0x00,                          /* High-Speed Configuration Descriptor */
    'S', 0x00,
    'B', 0x00,
    '-', 0x00,
    '2', 0x00,
    '.', 0x00,
    '1', 0x00
};
/* Standard product string descriptor */
const uint8_t CyCx3USBConfigFSDscr[] =
{
    0x10,                               /* Descriptor Size */
    CY_U3P_USB_STRING_DESCR,            /* Device descriptor type */
    'U', 0x00,                          /* Full-Speed Configuration Descriptor */
    'S', 0x00,
    'B', 0x00,
    '-', 0x00,
    '1', 0x00,
    '.', 0x00,
    '1', 0x00
};



/* UVC Probe Control Settings */
uint8_t glProbeCtrl[CX3_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                         /* bmHint : No fixed parameters */
    0x01,                               /* Use 1st Video format index */
    0x01,                               /* Use 1st Video frame index */
    0x0A, 0x8B, 0x02, 0x00,             /* Desired frame interval in 100ns */
    0x00, 0x00,                         /* Key frame rate in key frame/video frame units */
    0x00, 0x00,                         /* PFrame rate in PFrame / key frame units */
    0x00, 0x00,                         /* Compression quality control */
    0x00, 0x00,                         /* Window size for average bit rate */
    0x00, 0x00,                         /* Internal video streaming i/f latency in ms */
    0x00, 0x48, 0x3F, 0x00,             /* Max video frame size in bytes */ 
#ifdef CX3_UVC_1_0_SUPPORT 
    0x00, 0x90, 0x00, 0x00              /* No. of bytes device can rx in single payload: 32KB */
#else
    /* UVC 1.1 Probe Control has additional fields from UVC 1.0 */
    0x00, 0x90, 0x00, 0x00,             /* No. of bytes device can rx in single payload: 32KB */
    0x00, 0x60, 0xE3, 0x16,             /* Device Clock */
    0x00,                               /* Framing Information - Ignored for uncompressed format*/
    0x00,                               /* Preferred payload format version */
    0x00,                               /* Minimum payload format version */
    0x00                                /* Maximum payload format version */
#endif
};

/* UVC Probe Control Setting - 1080p@30FPS */
uint8_t const gl1080pProbeCtrl[CX3_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                         /* bmHint : No fixed parameters */
    0x01,                               /* Use 1st Video format index */
    0x01,                               /* Use 1st Video frame index */
    0x15, 0x16, 0x05, 0x00,             /* Desired frame interval in 100ns = (1/30)x10^7 */
    0x00, 0x00,                         /* Key frame rate in key frame/video frame units */
    0x00, 0x00,                         /* PFrame rate in PFrame / key frame units */
    0x00, 0x00,                         /* Compression quality control */
    0x00, 0x00,                         /* Window size for average bit rate */
    0x00, 0x00,                         /* Internal video streaming i/f latency in ms */
    0x00, 0x48, 0x3F, 0x00,             /* Max video frame size in bytes = 1920 x 1080 x 2 */ 
#ifdef CX3_UVC_1_0_SUPPORT 
    0x00, 0x90, 0x00, 0x00              /* No. of bytes device can rx in single payload: 36KB */
#else
    /* UVC 1.1 Probe Control has additional fields from UVC 1.0 */
    0x00, 0x90, 0x00, 0x00,             /* No. of bytes device can rx in single payload: 36KB */
    0x00, 0x60, 0xE3, 0x16,             /* Device Clock */
    0x00,                               /* Framing Information - Ignored for uncompressed format*/
    0x00,                               /* Preferred payload format version */
    0x00,                               /* Minimum payload format version */
    0x00                                /* Maximum payload format version */
#endif
};

/* UVC Probe Control Setting - 720p@60fps*/
uint8_t const gl720pProbeCtrl[CX3_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                         /* bmHint : No fixed parameters */
    0x01,                               /* Use 1st Video format index */
    0x02,                               /* Use 2nd Video frame index */
    0x0A, 0x8B, 0x02, 0x00,             /* Desired frame interval in 100ns = (1/60)x10^7*/
    0x00, 0x00,                         /* Key frame rate in key frame/video frame units */
    0x00, 0x00,                         /* PFrame rate in PFrame / key frame units */
    0x00, 0x00,                         /* Compression quality control */
    0x00, 0x00,                         /* Window size for average bit rate */
    0x00, 0x00,                         /* Internal video streaming i/f latency in ms */
    0x00, 0x20, 0x1C, 0x00,             /* Max video frame size in bytes: 720 x 1280 x 2*/
#ifdef CX3_UVC_1_0_SUPPORT 
    0x00, 0x90, 0x00, 0x00              /* No. of bytes device can rx in single payload: 36KB */
#else
    /* UVC 1.1 Probe Control has additional fields from UVC 1.0 */
    0x00, 0x90, 0x00, 0x00,             /* No. of bytes device can rx in single payload: 36KB */
    0x00, 0x60, 0xE3, 0x16,             /* Device Clock */
    0x00,                               /* Framing Information - Ignored for uncompressed format*/
    0x00,                               /* Preferred payload format version */
    0x00,                               /* Minimum payload format version */
    0x00                                /* Maximum payload format version */
#endif
};



/* UVC Probe Control Setting - VGA @30fps (High Speed)*/
uint8_t const glVga60ProbeCtrl[CX3_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                         /* bmHint : No fixed parameters */
    0x01,                               /* Use 1st Video format index */
    0x02,                               /* Use 2nd Video frame index */
    0x0A, 0x8B, 0x02, 0x00,             /* Desired frame interval in 100ns = (1/60)x10^7 */
    0x00, 0x00,                         /* Key frame rate in key frame/video frame units */
    0x00, 0x00,                         /* PFrame rate in PFrame / key frame units */
    0x00, 0x00,                         /* Compression quality control */
    0x00, 0x00,                         /* Window size for average bit rate */
    0x00, 0x00,                         /* Internal video streaming i/f latency in ms */
    0x00, 0x60, 0x09, 0x00,             /* Max video frame size in bytes: 640 x 480 x 2*/
#ifdef CX3_UVC_1_0_SUPPORT 
    0x00, 0x90, 0x00, 0x00              /* No. of bytes device can rx in single payload: 36KB */
#else
    /* UVC 1.1 Probe Control has additional fields from UVC 1.0 */
    0x00, 0x90, 0x00, 0x00,             /* No. of bytes device can rx in single payload: 36KB */
    0x00, 0x60, 0xE3, 0x16,             /* Device Clock */
    0x00,                               /* Framing Information - Ignored for uncompressed format*/
    0x00,                               /* Preferred payload format version */
    0x00,                               /* Minimum payload format version */
    0x00                                /* Maximum payload format version */
#endif
};
/* UVC Probe Control Setting - 5Mp@15FPS */
uint8_t const gl5MpProbeCtrl[CX3_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                         /* bmHint : No fixed parameters */
    0x01,                               /* Use 1st Video format index */
    0x03,                               /* Use 1st Video frame index */
    0x2A, 0x2C, 0x0A, 0x00,             /* Desired frame interval in 100ns = (1/15)x10^7 */
    0x00, 0x00,                         /* Key frame rate in key frame/video frame units */
    0x00, 0x00,                         /* PFrame rate in PFrame / key frame units */
    0x00, 0x00,                         /* Compression quality control */
    0x00, 0x00,                         /* Window size for average bit rate */
    0x00, 0x00,                         /* Internal video streaming i/f latency in ms */
    0x00, 0xC6, 0x99, 0x00,             /* Max video frame size in bytes = 2592 x 1944 x 2 */ 
#ifdef CX3_UVC_1_0_SUPPORT 
    0x00, 0x90, 0x00, 0x00              /* No. of bytes device can rx in single payload: 36KB */
#else
    /* UVC 1.1 Probe Control has additional fields from UVC 1.0 */
    0x00, 0x90, 0x00, 0x00,             /* No. of bytes device can rx in single payload: 36KB */
    0x00, 0x60, 0xE3, 0x16,             /* Device Clock */
    0x00,                               /* Framing Information - Ignored for uncompressed format*/
    0x00,                               /* Preferred payload format version */
    0x00,                               /* Minimum payload format version */
    0x00                                /* Maximum payload format version */
#endif
};
/* Configuration parameters for 5Mp @15FPS for the OV5640 sensor */
CyU3PMipicsiCfg_t cfgUvc5Mp15NoMclk =  { 
    CY_U3P_CSI_DF_YUV422_8_2,
    2,
    1,
    64,
    CY_U3P_CSI_PLL_FRS_500_1000M, 
    CY_U3P_CSI_PLL_CLK_DIV_8,
    CY_U3P_CSI_PLL_CLK_DIV_8,
    0,
    CY_U3P_CSI_PLL_CLK_DIV_2,
    2592,
    0x01
};

/* Configuration parameters for 1080p @30FPS for the OV5640 sensor */
CyU3PMipicsiCfg_t cfgUvc1080p30NoMclk =  { 
    CY_U3P_CSI_DF_YUV422_8_2,     /* dataFormat   */ 
    2,                          /* numDataLanes */
    1,                        /* pllPrd       */
    62,                         /* pllFbd       */
    CY_U3P_CSI_PLL_FRS_500_1000M, /* pllFrs      */
    CY_U3P_CSI_PLL_CLK_DIV_8,   /* csiRxClkDiv  */
    CY_U3P_CSI_PLL_CLK_DIV_8,   /* parClkDiv    */
    0x00,                       /* mclkCtl      */
    CY_U3P_CSI_PLL_CLK_DIV_8,   /* mClkRefDiv   */
    1920,                       /* hResolution  */
    0x01                       /* fifoDelay    */
};

/* Configuration parameters for 720p @60FPS for the OV5640 sensor */
CyU3PMipicsiCfg_t cfgUvc720p60NoMclk =  { 
    CY_U3P_CSI_DF_YUV422_8_2,     /* dataFormat   */
    2,                          /* numDataLanes */
    1,                        /* pllPrd       */
    62,                         /* pllFbd       */
    CY_U3P_CSI_PLL_FRS_250_500M, /* pllFrs      */
    CY_U3P_CSI_PLL_CLK_DIV_4,   /* csiRxClkDiv  */
    CY_U3P_CSI_PLL_CLK_DIV_4,   /* parClkDiv    */
    0x00,                       /* mclkCtl      */
    CY_U3P_CSI_PLL_CLK_DIV_8,   /* mClkRefDiv   */
    1280,                       /* hResolution  */
    0x01                       /* fifoDelay    */
};

/* Configuration parameters for VGA for the OV5640 sensor */
CyU3PMipicsiCfg_t cfgUvcVgaNoMclk =  { 
    CY_U3P_CSI_DF_YUV422_8_2,       /* dataFormat   */    
    1,                            /* numDataLanes */    
    0x1,                          /* pllPrd       */    
    90,                           /* pllFbd       */    
    CY_U3P_CSI_PLL_FRS_125_250M,   /* pllFrs      */            
    CY_U3P_CSI_PLL_CLK_DIV_2,     /* csiRxClkDiv  */    
    CY_U3P_CSI_PLL_CLK_DIV_8,     /* parClkDiv    */    
    0x00,                         /* mclkCtl      */    
    CY_U3P_CSI_PLL_CLK_DIV_8,     /* mClkRefDiv   */    
    640,                          /* hResolution  */    
    0x01                          /* fifoDelay    */    
};

/* [ ] */

