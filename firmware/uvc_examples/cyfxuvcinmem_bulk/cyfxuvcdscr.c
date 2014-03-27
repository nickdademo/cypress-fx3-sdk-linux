/*
 ## Cypress USB 3.0 Platform source file (cyfxuvcdscr.c)
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

/* This file contains the USB enumeration descriptors for the UVC (in memory) application example.
 * The descriptor arrays must be 32 byte aligned and multiple of 32 bytes if the D-cache is
 * turned on. If the linker used is not capable of supporting the aligned feature for this,
 * either the descriptors must be placed in a different section and the section should be 
 * 32 byte aligned and 32 byte multiple; or dynamically allocated buffer allocated using
 * CyU3PDmaBufferAlloc must be used, and the descriptor must be loaded into it. The example
 * assumes that the aligned attribute for 32 bytes is supported by the linker. Do not add
 * any other variables to this file other than USB descriptors. This is not the only
 * pre-requisite to enabling the D-cache. Refer to the documentation for
 * CyU3PDeviceCacheControl for more information.
 */

#include "cyfxuvcinmem.h"

/* Standard device descriptor for USB 3.0 */
const uint8_t CyFxUSB30DeviceDscr[] __attribute__ ((aligned (32))) =
{
    0x12,                           /* Descriptor size */
    CY_U3P_USB_DEVICE_DESCR,        /* Device descriptor type */
    0x00,0x03,                      /* USB 3.0 */
    0xEF,                           /* Device class */
    0x02,                           /* Device Sub-class */
    0x01,                           /* Device protocol */
    0x09,                           /* Maxpacket size for EP0 : 2^9 */
    0xB4,0x04,                      /* Vendor ID */
    0x22,0x47,                      /* Product ID */
    0x00,0x00,                      /* Device release number */
    0x01,                           /* Manufacture string index */
    0x02,                           /* Product string index */
    0x00,                           /* Serial number string index */
    0x01                            /* Number of configurations */
};

/* Standard device descriptor */
const uint8_t CyFxUSB20DeviceDscr[] __attribute__ ((aligned (32))) =
{
    0x12,                           /* Descriptor size */
    CY_U3P_USB_DEVICE_DESCR,        /* Device descriptor type */
    0x10,0x02,                      /* USB 2.10 */
    0xEF,                           /* Device class */
    0x02,                           /* Device sub-class */
    0x01,                           /* Device protocol */
    0x40,                           /* Maxpacket size for EP0 : 64 bytes */
    0xB4,0x04,                      /* Vendor ID */
    0x22,0x47,                      /* Product ID */
    0x00,0x00,                      /* Device release number */
    0x01,                           /* Manufacture string index */
    0x02,                           /* Product string index */
    0x00,                           /* Serial number string index */
    0x01                            /* Number of configurations */
};

/* Binary device object store descriptor */
const uint8_t CyFxUSBBOSDscr[] __attribute__ ((aligned (32))) =
{
    0x05,                           /* Descriptor size */
    CY_U3P_BOS_DESCR,               /* Device descriptor type */
    0x16,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x02,                           /* Number of device capability descriptors */

    /* USB 2.0 Extension */
    0x07,                           /* Descriptor size */
    CY_U3P_DEVICE_CAPB_DESCR,       /* Device capability type descriptor */
    CY_U3P_USB2_EXTN_CAPB_TYPE,     /* USB 2.0 extension capability type */
    0x02,0x00,0x00,0x00,            /* Supported device level features: LPM support  */

    /* SuperSpeed Device Capability */
    0x0A,                           /* Descriptor size */
    CY_U3P_DEVICE_CAPB_DESCR,       /* Device capability type descriptor */
    CY_U3P_SS_USB_CAPB_TYPE,        /* SuperSpeed device capability type */
    0x00,                           /* Supported device level features  */
    0x0E,0x00,                      /* Speeds supported by the device : SS, HS and FS */
    0x03,                           /* Functionality support */
    0x00,                           /* U1 device exit latency */
    0x00,0x00                       /* U2 device exit latency */
};

/* Standard device qualifier descriptor */
const uint8_t CyFxUSBDeviceQualDscr[] __attribute__ ((aligned (32))) =
{
    0x0A,                           /* descriptor size */
    CY_U3P_USB_DEVQUAL_DESCR,       /* Device qualifier descriptor type */
    0x00,0x02,                      /* USB 2.0 */
    0xEF,                           /* Device class */
    0x02,                           /* Device sub-class */
    0x01,                           /* Device protocol */
    0x40,                           /* Maxpacket size for EP0 : 64 bytes */
    0x01,                           /* Number of configurations */
    0x00                            /* Reserved */
};

/* Standard super speed configuration descriptor */
const uint8_t CyFxUSBSSConfigDscr[] __attribute__ ((aligned (32))) =
{
    /* Configuration descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_CONFIG_DESCR,        /* Configuration descriptor type */
    0xC9,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x02,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* COnfiguration string index */
    0x80,                           /* Config characteristics - Bus powered */
    0x32,                           /* Max power consumption of device (in 8mA unit) : 400mA */

    /* Interface association descriptor */
    0x08,                           /* Descriptor size */
    CY_FX_INTF_ASSN_DSCR_TYPE,      /* Interface association descr type */
    0x00,                           /* I/f number of first VideoControl i/f */
    0x02,                           /* Number of video streaming i/f */
    0x0E,                           /* CC_VIDEO : Video i/f class code */
    0x03,                           /* SC_VIDEO_INTERFACE_COLLECTION : Subclass code */
    0x00,                           /* Protocol : Not used */
    0x00,                           /* String desc index for interface */

    /* Standard video control interface descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x01,                           /* Number of end points */
    0x0E,                           /* CC_VIDEO : Interface class */
    0x01,                           /* CC_VIDEOCONTROL : Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Class specific VC interface header descriptor */
    0x0D,                           /* Descriptor size */
    0x24,                           /* Class specific i/f header descriptor type */
    0x01,                           /* Descriptor sub type : VC_HEADER */
    0x00,0x01,                      /* Revision of class spec : 1.0 */
    0x50,0x00,                      /* Total size of class specific descriptors (till output terminal) */
    0x00,0x6C,0xDC,0x02,            /* Clock frequency : 48MHz */
    0x01,                           /* Number of streaming interfaces */
    0x01,                           /* Video streaming i/f 1 belongs to VC i/f */

    /* Input (camera) terminal descriptor */
    0x12,                           /* Descriptor size */
    0x24,                           /* Class specific interface desc type */
    0x02,                           /* Input Terminal Descriptor type */
    0x01,                           /* ID of this terminal */
    0x01,0x02,                      /* Camera terminal type */
    0x00,                           /* No association terminal */
    0x00,                           /* String desc index : Not used */
    0x00,0x00,                      /* No optical zoom supported */
    0x00,0x00,                      /* No optical zoom supported */
    0x00,0x00,                      /* No optical zoom supported */
    0x03,                           /* Size of controls field for this terminal : 3 bytes */
    0x00,0x00,0x00,                 /* No controls supported */

    /* Processing unit descriptor */
    0x0C,                           /* Descriptor size */
    0x24,                           /* Class specific interface desc type */
    0x05,                           /* Processing unit descriptor type */
    0x02,                           /* ID of this terminal */
    0x01,                           /* Source ID : 1 : conencted to input terminal */
    0x00,0x40,                      /* Digital multiplier */
    0x03,                           /* Size of controls field for this terminal : 3 bytes */
    0x00,0x00,0x00,                 /* No controls supported */
    0x00,                           /* String desc index : Not used */

    /* Extension unit descriptor */
    0x1C,                           /* Descriptor size */
    0x24,                           /* Class specific interface desc type */
    0x06,                           /* Extension unit descriptor type */
    0x03,                           /* ID of this terminal */
    0xFF,0xFF,0xFF,0xFF,            /* 16 byte GUID */
    0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,
    0x00,                           /* Number of controls in this terminal */
    0x01,                           /* Number of input pins in this terminal */
    0x02,                           /* Source ID : 2 : connected to proc unit */
    0x03,                           /* Size of controls field for this terminal : 3 bytes */
    0x00,0x00,0x00,                 /* No controls supported */
    0x00,                           /* String desc index : Not used */

    /* Output terminal descriptor */
    0x09,                           /* Descriptor size */
    0x24,                           /* Class specific interface desc type */
    0x03,                           /* Output terminal descriptor type */
    0x04,                           /* ID of this terminal */
    0x01,0x01,                      /* USB streaming terminal type */
    0x00,                           /* No association terminal */
    0x03,                           /* Source ID : 3 : Connected to extn Unit */
    0x00,                           /* String desc index : Not used */

    /* Video control status interrupt endpoint descriptor */
    0x07,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint descriptor type */
    CY_FX_EP_CONTROL_STATUS,        /* Endpoint address and description */
    CY_U3P_USB_EP_INTR,             /* Interrupt end point type */
    0x00,0x04,                      /* Max packet size = 1024 bytes */
    0x01,                           /* Servicing interval */

    /* Super speed endpoint companion descriptor */
    0x06,                           /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,       /* SS endpoint companion descriptor type */
    0x00,                           /* Max no. of packets in a Burst : 1 pp change */
    0x00,                           /* Mult.: Max number of packets : 1 */
    0x00, 0x04,                     /* Bytes per interval : 1024 */

    /* Class specific interrupt endpoint descriptor */
    0x05,                           /* Descriptor size */
    0x25,                           /* Class specific endpoint descriptor type */
    CY_U3P_USB_EP_INTR,             /* End point sub type */
    0x00,0x04,                      /* Max packet size = 1024 */

    /* Standard video streaming interface descriptor (Alternate setting 0) */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface descriptor type */
    0x01,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x01,                           /* Number of end points : zero bandwidth */
    0x0E,                           /* Interface class : CC_VIDEO */
    0x02,                           /* Interface sub class : CC_VIDEOSTREAMING */
    0x00,                           /* Interface protocol code : Undefined */
    0x00,                           /* Interface descriptor string index */

    /* Endpoint descriptor for streaming video data */
    0x07,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint descriptor type */
    CY_FX_EP_BULK_VIDEO,            /* Endpoint address and description */
    CY_U3P_USB_EP_BULK,             /* Bulk End Point */
    0x00, 0x04,                     /* Maximum packet size. */
    0x00,                           /* Servicing interval for data transfers */

    /* Super speed endpoint companion descriptor */
    0x06,                           /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,       /* SS endpoint companion descriptor type */
    0x0F,                           /* Max no. of packets in a Burst : 1 pp change */
    0x00,                           /* Mult.: Max number of packets : 1 */
    0x00,(0x04 * CY_FX_Bulk_BURST),	/* Bytes per interval : 1024 */

    /* Class-specific video streaming input header descriptor */
    0x0E,                           /* Descriptor size */
    0x24,                           /* Class-specific VS i/f Type */
    0x01,                           /* Descriptotor subtype : input header */
    0x01,                           /* 1 format desciptor follows */
    0x19,0x00,                      /* Total size of class specific VS descr */
    CY_FX_EP_BULK_VIDEO,            /* EP address for BULK video data */
    0x00,                           /* No dynamic format change supported */
    0x04,                           /* Output terminal ID : 4 */
    0x01,                           /* Still image capture method 1 supported */
    0x01,                           /* Hardware trigger supported for still image */
    0x00,                           /* Hardware to initiate still image capture */
    0x01,                           /* Size of controls field : 1 byte */
    0x00,                           /* D2 : Compression quality supported */

    /* Class specific VS format descriptor */
    0x0B,                           /* Descriptor size */
    0x24,                           /* Class-specific VS i/f type */
    0x06,                           /* Descriptotor subtype : VS_FORMAT_MJPEG */
    0x01,                           /* Format desciptor index */
    0x01,                           /* 1 Frame desciptor follows */
    0x01,                           /* Uses fixed size samples */
    0x01,                           /* Default frame index is 1 */
    0x00,                           /* Non interlaced stream not reqd. */
    0x00,                           /* Non interlaced stream not reqd. */
    0x00,                           /* Non interlaced stream */
    0x00,                           /* CopyProtect: duplication unrestricted */

    /* Class specific VS frame descriptor */
    0x1E,                           /* Descriptor size */
    0x24,                           /* Class-specific VS i/f type */
    0x07,                           /* Descriptor Subtype : VS_FRAME_MJPEG */
    0x01,                           /* Frame desciptor index */
    0x00,                           /* Still image capture method not supported */
    0xC2,0x01,                      /* Width of the frame :450(176) */
    0xF0,0x00,                      /* Height of the frame :250 144 */
    0x00,0xC0,0x5D,0x00,            /* Min bit rate bits/s */
    0x00,0xC0,0x5D,0x00,            /* Min bit rate bits/s */
    0x00,0x58,0x02,0x00,            /* Maximum video or still frame size in bytes */
    0x2A,0x2C,0x0A,0x00,            /* Default frame interval */
    0x01,                           /* Frame interval type : No of discrete intervals */
    0x2A,0x2C,0x0A,0x00             /* Frame interval 3 */
};

/* Standard High Speed Configuration Descriptor */
const uint8_t CyFxUSBHSConfigDscr[] __attribute__ ((aligned (32))) =
{
    /* Configuration descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_CONFIG_DESCR,        /* Configuration descriptor type */
    0xBD,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x02,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* COnfiguration string index */
    0x80,                           /* Config characteristics - bus powered */
    0xC8,                           /* Max power consumption of device (in 2mA unit) : 400mA */

    /* Interface association descriptor */
    0x08,                           /* Descriptor size */
    CY_FX_INTF_ASSN_DSCR_TYPE,      /* Interface association descr type */
    0x00,                           /* I/f number of first video control i/f */
    0x02,                           /* Number of video streaming i/f */
    0x0E,                           /* CC_VIDEO : Video i/f class code */
    0x03,                           /* SC_VIDEO_INTERFACE_COLLECTION : subclass code */
    0x00,                           /* Protocol : not used */
    0x00,                           /* String desc index for interface */

    /* Standard video control interface descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x01,                           /* Number of end points */
    0x0E,                           /* CC_VIDEO : Interface class */
    0x01,                           /* CC_VIDEOCONTROL : Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Class specific VC interface header descriptor */
    0x0D,                           /* Descriptor size */
    0x24,                           /* Class Specific I/f header descriptor type */
    0x01,                           /* Descriptor sub type : VC_HEADER */
    0x00,0x01,                      /* Revision of class spec : 1.0 */
    0x50,0x00,                      /* Total size of class specific descriptors (till output terminal) */
    0x00,0x6C,0xDC,0x02,            /* Clock frequency : 48MHz */
    0x01,                           /* Number of streaming interfaces */
    0x01,                           /* Video streaming I/f 1 belongs to VC i/f */

    /* Input (camera) terminal descriptor */
    0x12,                           /* Descriptor size */
    0x24,                           /* Class specific interface desc type */
    0x02,                           /* Input Terminal Descriptor type */
    0x01,                           /* ID of this terminal */
    0x01,0x02,                      /* Camera terminal type */
    0x00,                           /* No association terminal */
    0x00,                           /* String desc index : not used */
    0x00,0x00,                      /* No optical zoom supported */
    0x00,0x00,                      /* No optical zoom supported */
    0x00,0x00,                      /* No optical zoom supported */
    0x03,                           /* Size of controls field for this terminal : 3 bytes */
    0x00,0x00,0x00,                 /* No controls supported */

    /* Processing unit descriptor */
    0x0C,                           /* Descriptor size */
    0x24,                           /* Class specific interface desc type */
    0x05,                           /* Processing unit descriptor type */
    0x02,                           /* ID of this terminal */
    0x01,                           /* Source ID : 1 : conencted to input terminal */
    0x00,0x40,                      /* Digital multiplier */
    0x03,                           /* Size of controls field for this terminal : 3 bytes */
    0x00,0x00,0x00,                 /* No controls supported */
    0x00,                           /* String desc index : not used */

    /* Extension unit descriptor */
    0x1C,                           /* Descriptor size */
    0x24,                           /* Class specific interface desc type */
    0x06,                           /* Extension unit descriptor type */
    0x03,                           /* ID of this terminal */
    0xFF,0xFF,0xFF,0xFF,            /* 16 byte GUID */
    0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,
    0x00,                           /* Number of controls in this terminal */
    0x01,                           /* Number of input pins in this terminal */
    0x02,                           /* Source ID : 2 : connected to proc unit */
    0x03,                           /* Size of controls field for this terminal : 3 bytes */
    0x00,0x00,0x00,                 /* No controls supported */
    0x00,                           /* String desc index : not used */

    /* Output terminal descriptor */
    0x09,                           /* Descriptor size */
    0x24,                           /* Class specific interface desc type */
    0x03,                           /* Output terminal descriptor type */
    0x04,                           /* ID of this terminal */
    0x01,0x01,                      /* USB Streaming terminal type */
    0x00,                           /* No association terminal */
    0x03,                           /* Source ID : 3 : connected to extn unit */
    0x00,                           /* String desc index : not used */

    /* Video control status interrupt endpoint descriptor */
    0x07,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint descriptor type */
    CY_FX_EP_CONTROL_STATUS,        /* Endpoint address and description */
    CY_U3P_USB_EP_INTR,             /* Interrupt end point type */
    0x40,0x00,                      /* Max packet size = 64 bytes */
    0x08,                           /* Servicing interval : 8ms */

    /* Class specific interrupt endpoint descriptor */
    0x05,                           /* Descriptor size */
    0x25,                           /* Class specific endpoint descriptor type */
    CY_U3P_USB_EP_INTR,             /* End point sub type */
    0x40,0x00,                      /* Max packet size = 64 bytes */

    /* Standard video streaming interface descriptor (alternate setting 0) */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface descriptor type */
    0x01,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x01,                           /* Number of end points : zero bandwidth */
    0x0E,                           /* Interface class : CC_VIDEO */
    0x02,                           /* Interface sub class : CC_VIDEOSTREAMING */
    0x00,                           /* Interface protocol code : undefined */
    0x00,                           /* Interface descriptor string index */

    /* Endpoint descriptor for streaming video data */
    0x07,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint descriptor type */
    CY_FX_EP_BULK_VIDEO,            /* Endpoint address and description */
    CY_U3P_USB_EP_BULK,             /* Bulk Endpoint */
    0x00, 0x02,                     /* 512 Bytes Maximum Packet Size. */
    0x00,                           /* Servicing interval for data transfers */

    /* Class-specific video streaming input header descriptor */
    0x0E,                           /* Descriptor size */
    0x24,                           /* Class-specific VS i/f type */
    0x01,                           /* Descriptotor subtype : input header */
    0x01,                           /* 1 format desciptor follows */
    0x19,0x00,                      /* Total size of class specific VS descr */
    CY_FX_EP_BULK_VIDEO,            /* EP address for BULK video data */
    0x00,                           /* No dynamic format change supported */
    0x04,                           /* Output terminal ID : 4 */
    0x01,                           /* Still image capture method 1 supported */
    0x01,                           /* Hardware trigger supported for still image */
    0x00,                           /* Hardware to initiate still image capture */
    0x01,                           /* Size of controls field : 1 byte */
    0x00,                           /* D2 : Compression quality supported */

    /* Class specific VS format descriptor */
    0x0B,                           /* Descriptor size */
    0x24,                           /* Class-specific VS i/f type */
    0x06,                           /* Descriptotor subtype : VS_FORMAT_MJPEG */
    0x01,                           /* Format desciptor index */
    0x01,                           /* 1 Frame desciptor follows */
    0x01,                           /* Uses fixed size samples */
    0x01,                           /* Default frame index is 1 */
    0x00,                           /* Non interlaced stream not reqd. */
    0x00,                           /* Non interlaced stream not reqd. */
    0x00,                           /* Non interlaced stream */
    0x00,                           /* CopyProtect: duplication unrestricted */

    /* Class specific VS frame descriptor */
    0x1E,                           /* Descriptor size */
    0x24,                           /* Class-specific VS I/f Type */
    0x07,                           /* Descriptotor subtype : VS_FRAME_MJPEG */
    0x01,                           /* Frame desciptor index */
    0x00,                           /* Still image capture method not supported */
    0xC2,0x01,                      /* Width of the frame :450(176) */
    0xF0,0x00,                      /* Height of the frame :250 144 */
    0x00,0xC0,0x5D,0x00,            /* Min bit rate bits/s */
    0x00,0xC0,0x5D,0x00,            /* Max bit rate bits/s */
    0x00,0x58,0x02,0x00,            /* Maximum video or still frame size in bytes */
    0x2A,0x2C,0x0A,0x00,            /* Default frame interval */
    0x01,                           /* 1 Frame interval type : No of discrete intervals */
    0x2A,0x2C,0x0A,0x00             /* Frame interval 3 */
};

/* Standard full speed configuration descriptor : full speed is not supported. */
const uint8_t CyFxUSBFSConfigDscr[] __attribute__ ((aligned (32))) =
{
    /* Configuration descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_CONFIG_DESCR,        /* Configuration descriptor type */
    0x09,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x00,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* COnfiguration string index */
    0x80,                           /* Config characteristics - bus powered */
    0x32                            /* Max power consumption of device (in 2mA unit) : 100mA */
};

/* Standard language ID string descriptor */
const uint8_t CyFxUSBStringLangIDDscr[] __attribute__ ((aligned (32))) =
{
    0x04,                           /* Descriptor size */
    CY_U3P_USB_STRING_DESCR,        /* Device descriptor type */
    0x09,0x04                       /* Language ID supported */
};

/* Standard manufacturer string descriptor */
const uint8_t CyFxUSBManufactureDscr[] __attribute__ ((aligned (32))) =
{
    0x10,                           /* Descriptor size */
    CY_U3P_USB_STRING_DESCR,        /* Device descriptor type */
    'C',0x00,
    'y',0x00,
    'p',0x00,
    'r',0x00,
    'e',0x00,
    's',0x00,
    's',0x00
};

/* Standard product string descriptor */
const uint8_t CyFxUSBProductDscr[] __attribute__ ((aligned (32))) =
{
    0x08,                           /* Descriptor Size */
    CY_U3P_USB_STRING_DESCR,        /* Device descriptor type */
    'F',0x00,
    'X',0x00,
    '3',0x00
};

/* Place this buffer as the last buffer so that no other variable / code shares
 * the same cache line. Do not add any other variables / arrays in this file.
 * This will lead to variables sharing the same cache line. */
const uint8_t CyFxUsbDscrAlignBuffer[32] __attribute__ ((aligned (32)));

/* [ ] */

