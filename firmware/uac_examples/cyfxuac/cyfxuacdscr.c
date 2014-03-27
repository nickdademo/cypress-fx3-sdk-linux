/*
 ## Cypress USB 3.0 Platform source file (cyfxuacdscr.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2012-2013
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

/* This file contains the USB enumeration descriptors for the UAC application example.
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

#include "cyfxuac.h"

/* Standard device descriptor for USB 3.0 */
const uint8_t CyFxUsb30DeviceDscr[] __attribute__ ((aligned (32))) =
{
    0x12,                           /* Descriptor size */
    CY_U3P_USB_DEVICE_DESCR,        /* Device descriptor type */
    0x00,0x03,                      /* USB 3.0 */
    0x00,                           /* Device class */
    0x00,                           /* Device Sub-class */
    0x00,                           /* Device protocol */
    0x09,                           /* Maxpacket size for EP0 : 2^9 */
    0xB4,0x04,                      /* Vendor ID  = 0x04B4 */
    0x22,0x47,                      /* Product ID = 0x4722 */
    0x01,0x00,                      /* Device release number */
    0x01,                           /* Manufacture string index */
    0x02,                           /* Product string index */
    0x00,                           /* Serial number string index */
    0x01                            /* Number of configurations */
};

/* Standard device descriptor */
const uint8_t CyFxUsb20DeviceDscr[] __attribute__ ((aligned (32))) =
{
    0x12,                           /* Descriptor size */
    CY_U3P_USB_DEVICE_DESCR,        /* Device descriptor type */
    0x10,0x02,                      /* USB 2.10 */
    0x00,                           /* Device class */
    0x00,                           /* Device sub-class */
    0x00,                           /* Device protocol */
    0x40,                           /* Maxpacket size for EP0 : 64 bytes */
    0xB4,0x04,                      /* Vendor ID  = 0x04B4 */
    0x22,0x47,                      /* Product ID = 0x4722 */
    0x01,0x00,                      /* Device release number */
    0x01,                           /* Manufacture string index */
    0x02,                           /* Product string index */
    0x00,                           /* Serial number string index */
    0x01                            /* Number of configurations */
};

/* Binary device object store descriptor */
const uint8_t CyFxUsbBOSDscr[] __attribute__ ((aligned (32))) =
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
const uint8_t CyFxUsbDeviceQualDscr[] __attribute__ ((aligned (32))) =
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
const uint8_t CyFxUsbSSConfigDscr[] __attribute__ ((aligned (32))) =
{
    /* Configuration descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_CONFIG_DESCR,        /* Configuration descriptor type */
    0x6A,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x02,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* Configuration string index */
    0x80,                           /* Config characteristics - Bus powered */
    0x32,                           /* Max power consumption of device (in 8mA unit) : 400mA */

    /* Standard Audio Control Interface Descriptor */
    0x09,                           /* Descriptor Size */
    0x04,                           /* Interface Descriptor Type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting */
    0x00,                           /* Number of endpoints - 0 endpoints */
    0x01,                           /* Interface Class - Audio */
    0x01,                           /* Interface SubClass - Audio Control */
    0x00,                           /* Interface Protocol - Unused */
    0x00,                           /* Interface string index */

    /* Class Specific Audio Control Interface Descriptor */
    0x09,                           /* Descriptor Size */
    0x24,                           /* Descriptor Type - CS_INTERFACE */
    0x01,                           /* Descriptor SubType - Header */
    0x00, 0x01,                     /* Revision of class specification - 1.0 */
    0x1E, 0x00,                     /* Total size of class specific descriptors */
    0x01,                           /* Number of streaming Interfaces - 1 */
    0x01,                           /* Audio Streaming interface 1 belongs to this AudioControl Interface */

    /* Input terminal descriptor */
    0x0C,                           /* Descriptor size in bytes */
    0x24,                           /* CS Interface Descriptor */
    0x02,                           /* Input Terminal Descriptor subtype */
    0x01,                           /* ID Of the input terminal */
    0x01, 0x02,                     /* Microphone - terminal type */
    0x00,                           /* Association terminal - None */
    0x02,                           /* Number of channels - 2 */
    0x03, 0x00,                     /* spatial location of the logical channels - Front Left and Front Right */
    0x00,                           /* Channel names - Unused */
    0x00,                           /* String index for this descriptor - None */

    /* Output terminal descriptor */
    0x09,                           /* Descriptor size */
    0x24,                           /* Class specific interface desc type */
    0x03,                           /* Output terminal descriptor type */
    0x02,                           /* ID of this terminal */
    0x01, 0x01,                     /* Output terminal type: USB Streaming */
    0x00,                           /* Association terminal - Unused */
    0x01,                           /* Id of the terminal/unit to which this is connected - Input terminal id */
    0x00,                           /* String desc index : Not used */

    /* Standard Audio Streaming Interface Descriptor (Alternate setting 0) */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface descriptor type */
    0x01,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x00,                           /* Number of end points : zero bandwidth */
    0x01,                           /* Interface class : Audio */
    0x02,                           /* Interface sub class : Audio Streaming */
    0x00,                           /* Interface protocol code : Unused */
    0x00,                           /* Interface descriptor string index */

    /* Standard Audio Streaming Interface descriptor (Alternate setting 1) */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface descriptor type */
    0x01,                           /* Interface number */
    0x01,                           /* Alternate setting number */
    0x01,                           /* Number of end points : 1 ISO EP */
    0x01,                           /* Interface Audio class */
    0x02,                           /* Interface Audio sub class - Audio Streaming */
    0x00,                           /* Interface protocol code : Unused */
    0x00,                           /* Interface descriptor string index */

    /* Class-specific Audio Streaming General Interface descriptor */
    0x07,                           /* Descriptor size */
    0x24,                           /* Class-specific AS i/f Type */
    0x01,                           /* Descriptotor subtype : AS General */
    0x02,                           /* Terminal Link - Output terminal id */                           
    0x01,                           /* Interface delay */
    0x01, 0x00,                     /* Audio data format - PCM */

    /* Class specific AS Format descriptor - Type I Format Descriptor */
    0x0B,                           /* Descriptor size */
    0x24,                           /* Class-specific Interface Descriptor Type */
    0x02,                           /* Format Type Descriptor subtype */
    0x01,                           /* PCM FORMAT_TYPE_I */
    0x02,                           /* Number of channels - 2 */
    0x02,                           /* Subframe size - 2 bytes per audio subframe */
    0x10,                           /* Bit resolution - 16 bits */
    0x01,                           /* Number of samping frequencies - 1 */
    0x80, 0xBB, 0x00,               /* Sampling frequency - 48000 Hz */
    
    /* Endpoint descriptor for ISO streaming Audio data */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint descriptor type */
    CY_FX_EP_ISO_AUDIO,             /* Endpoint address and description */
    0x05,                           /* ISO End point : Async */
    0x60, 0x00,                     /* Transaction size - 96 bytes */
    0x03,                           /* Servicing interval for data transfers */
    0x00,                           /* bRefresh */ 
    0x00,                           /* bSynchAddress */

    /* Super speed endpoint companion descriptor */
    0x06,                           /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,       /* SS endpoint companion descriptor type */
    0x00,                           /* Max no. of packets in a burst : 1 */
    0x00,                           /* Mult.: Max number of packets : 1 */
    0x60, 0x00,                     /* Bytes per interval : 1024 */

    /* Class Specific AS Isochronous Audio Data Endpoint Descriptor */
    0x07,                           /* Descriptor size in bytes */
    0x25,                           /* CS_ENDPOINT descriptor type */
    0x01,                           /* EP_GENERAL sub-descriptor type */
    0x00,                           /* bmAttributes - None  */
    0x00,                           /* bLockDelayUnits - Unused */
    0x00, 0x00                      /* wLockDelay - unused */
};

/* Standard High Speed Configuration Descriptor */
const uint8_t CyFxUsbHSConfigDscr[] __attribute__ ((aligned (32))) =
{
    /* Configuration descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_CONFIG_DESCR,        /* Configuration descriptor type */
    0x64, 0x00,                     /* Length of this descriptor and all sub descriptors */
    0x02,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* Configuration string index */
    0x80,                           /* Config characteristics - bus powered */
    0x32,                           /* Max power consumption of device (in 2mA unit) : 100mA */

    /* Standard Audio Control Interface Descriptor */
    0x09,                           /* Descriptor Size */
    0x04,                           /* Interface Descriptor Type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting */
    0x00,                           /* Number of endpoints - 0 endpoints */
    0x01,                           /* Interface Class - Audio */
    0x01,                           /* Interface SubClass - Audio Control */
    0x00,                           /* Interface Protocol - Unused */
    0x00,                           /* Interface string index */

    /* Class Specific Audio Control Interface Descriptor */
    0x09,                           /* Descriptor Size */
    0x24,                           /* Descriptor Type - CS_INTERFACE */
    0x01,                           /* Descriptor SubType - Header */
    0x00, 0x01,                     /* Revision of class specification - 1.0 */
    0x1E, 0x00,                     /* Total size of class specific descriptors */
    0x01,                           /* Number of streaming Interfaces - 1 */
    0x01,                           /* Audio Streaming interface 1 belongs to this AudioControl Interface */

    /* Input terminal descriptor */
    0x0C,                           /* Descriptor size in bytes */
    0x24,                           /* CS Interface Descriptor */
    0x02,                           /* Input Terminal Descriptor subtype */
    0x01,                           /* ID Of the input terminal */
    0x01, 0x02,                     /* Microphone - terminal type */
    0x00,                           /* Association terminal - None */
    0x02,                           /* Number of channels - 2 */
    0x03, 0x00,                     /* spatial location of the logical channels - Front Left and Front Right */
    0x00,                           /* Channel names - Unused */
    0x00,                           /* String index for this descriptor - None */

    /* Output terminal descriptor */
    0x09,                           /* Descriptor size */
    0x24,                           /* Class specific interface desc type */
    0x03,                           /* Output terminal descriptor type */
    0x02,                           /* ID of this terminal */
    0x01, 0x01,                     /* Output terminal type: USB Streaming */
    0x00,                           /* Association terminal - Unused */
    0x01,                           /* Id of the terminal/unit to which this is connected - Input terminal id */
    0x00,                           /* String desc index : Not used */

    /* Standard Audio Streaming Interface Descriptor (Alternate setting 0) */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface descriptor type */
    0x01,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x00,                           /* Number of end points : zero bandwidth */
    0x01,                           /* Interface class : Audio */
    0x02,                           /* Interface sub class : Audio Streaming */
    0x00,                           /* Interface protocol code : Unused */
    0x00,                           /* Interface descriptor string index */

    /* Standard Audio Streaming Interface descriptor (Alternate setting 1) */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface descriptor type */
    0x01,                           /* Interface number */
    0x01,                           /* Alternate setting number */
    0x01,                           /* Number of end points : 1 ISO EP */
    0x01,                           /* Interface Audio class */
    0x02,                           /* Interface Audio sub class - Audio Streaming */
    0x00,                           /* Interface protocol code : Unused */
    0x00,                           /* Interface descriptor string index */

    /* Class-specific Audio Streaming General Interface descriptor */
    0x07,                           /* Descriptor size */
    0x24,                           /* Class-specific AS i/f Type */
    0x01,                           /* Descriptotor subtype : AS General */
    0x02,                           /* Terminal Link - Output terminal id */                           
    0x01,                           /* Interface delay */
    0x01, 0x00,                     /* Audio data format - PCM */

    /* Class specific AS Format descriptor - Type I Format Descriptor */
    0x0B,                           /* Descriptor size */
    0x24,                           /* Class-specific Interface Descriptor Type */
    0x02,                           /* Format Type Descriptor subtype */
    0x01,                           /* PCM FORMAT_TYPE_I */
    0x02,                           /* Number of channels - 2 */
    0x02,                           /* Subframe size - 2 bytes per audio subframe */
    0x10,                           /* Bit resolution - 16 bits */
    0x01,                           /* Number of samping frequencies - 1 */
    0x80, 0xBB, 0x00,               /* Sampling frequency - 48000 Hz */
    
    /* Endpoint descriptor for ISO streaming Audio data */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint descriptor type */
    0x81,                           /* Endpoint address and description */
    0x05,                           /* ISO End point : Async */
    0x60, 0x00,                     /* Transaction size - 96 bytes per frame */
    0x03,                           /* Servicing interval for data transfers */
    0x00,                           /* bRefresh */ 
    0x00,                           /* bSynchAddress */

    /* Class Specific AS Isochronous Audio Data Endpoint Descriptor */
    0x07,                           /* Descriptor size in bytes */
    0x25,                           /* CS_ENDPOINT descriptor type */
    0x01,                           /* EP_GENERAL sub-descriptor type */
    0x00,                           /* bmAttributes - None  */
    0x00,                           /* bLockDelayUnits - Unused */
    0x00, 0x00                      /* wLockDelay - unused */
};

/* Standard full speed configuration descriptor : full speed is not supported. */
const uint8_t CyFxUsbFSConfigDscr[] __attribute__ ((aligned (32))) =
{
    /* Configuration descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_CONFIG_DESCR,        /* Configuration descriptor type */
    0x64,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x02,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* Configuration string index */
    0x80,                           /* Config characteristics - bus powered */
    0x32,                           /* Max power consumption of device (in 2mA unit) : 100mA */

    /* Standard Audio Control Interface Descriptor */
    0x09,                           /* Descriptor Size */
    0x04,                           /* Interface Descriptor Type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting */
    0x00,                           /* Number of endpoints - 0 endpoints */
    0x01,                           /* Interface Class - Audio */
    0x01,                           /* Interface SubClass - Audio Control */
    0x00,                           /* Interface Protocol - Unused */
    0x00,                           /* Interface string index */

    /* Class Specific Audio Control Interface Descriptor */
    0x09,                           /* Descriptor Size */
    0x24,                           /* Descriptor Type - CS_INTERFACE */
    0x01,                           /* Descriptor SubType - Header */
    0x00, 0x01,                     /* Revision of class specification - 1.0 */
    0x1E, 0x00,                     /* Total size of class specific descriptors */
    0x01,                           /* Number of streaming Interfaces - 1 */
    0x01,                           /* Audio Streaming interface 1 belongs to this AudioControl Interface */

    /* Input terminal descriptor */
    0x0C,                           /* Descriptor size in bytes */
    0x24,                           /* CS Interface Descriptor */
    0x02,                           /* Input Terminal Descriptor subtype */
    0x01,                           /* ID Of the input terminal */
    0x01, 0x02,                     /* Microphone - terminal type */
    0x00,                           /* Association terminal - None */
    0x02,                           /* Number of channels - 2 */
    0x03, 0x00,                     /* spatial location of the logical channels - Front Left and Front Right */
    0x00,                           /* Channel names - Unused */
    0x00,                           /* String index for this descriptor - None */

    /* Output terminal descriptor */
    0x09,                           /* Descriptor size */
    0x24,                           /* Class specific interface desc type */
    0x03,                           /* Output terminal descriptor type */
    0x02,                           /* ID of this terminal */
    0x01, 0x01,                     /* Output terminal type: USB Streaming */
    0x00,                           /* Association terminal - Unused */
    0x01,                           /* Id of the terminal/unit to which this is connected - Input terminal id */
    0x00,                           /* String desc index : Not used */

    /* Standard Audio Streaming Interface Descriptor (Alternate setting 0) */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface descriptor type */
    0x01,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x00,                           /* Number of end points : zero bandwidth */
    0x01,                           /* Interface class : Audio */
    0x02,                           /* Interface sub class : Audio Streaming */
    0x00,                           /* Interface protocol code : Unused */
    0x00,                           /* Interface descriptor string index */

    /* Standard Audio Streaming Interface descriptor (Alternate setting 1) */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface descriptor type */
    0x01,                           /* Interface number */
    0x01,                           /* Alternate setting number */
    0x01,                           /* Number of end points : 1 ISO EP */
    0x01,                           /* Interface Audio class */
    0x02,                           /* Interface Audio sub class - Audio Streaming */
    0x00,                           /* Interface protocol code : Unused */
    0x00,                           /* Interface descriptor string index */

    /* Class-specific Audio Streaming General Interface descriptor */
    0x07,                           /* Descriptor size */
    0x24,                           /* Class-specific AS i/f Type */
    0x01,                           /* Descriptotor subtype : AS General */
    0x02,                           /* Terminal Link - Output terminal id */                           
    0x01,                           /* Interface delay */
    0x01, 0x00,                     /* Audio data format - PCM */

    /* Class specific AS Format descriptor - Type I Format Descriptor */
    0x0B,                           /* Descriptor size */
    0x24,                           /* Class-specific Interface Descriptor Type */
    0x02,                           /* Format Type Descriptor subtype */
    0x01,                           /* PCM FORMAT_TYPE_I */
    0x02,                           /* Number of channels - 2 */
    0x02,                           /* Subframe size - 4 bytes per audio subframe */
    0x10,                           /* Bit resolution - 32 bits */
    0x01,                           /* Number of samping frequencies - 1 */
    0x80, 0xBB, 0x00,               /* Sampling frequency - 48000 Hz */
    
    /* Endpoint descriptor for ISO streaming Audio data */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint descriptor type */
    CY_FX_EP_ISO_AUDIO,             /* Endpoint address and description */
    0x05,                           /* ISO End point : Async */
    0xC0, 0x00,                     /* Transaction size - 192 bytes per frame */
    0x01,                           /* Servicing interval for data transfers */
    0x00,                           /* bRefresh */ 
    0x00,                           /* bSynchAddress */

    /* Class Specific AS Isochronous Audio Data Endpoint Descriptor */
    0x07,                           /* Descriptor size in bytes */
    0x25,                           /* CS_ENDPOINT descriptor type */
    0x01,                           /* EP_GENERAL sub-descriptor type */
    0x00,                           /* bmAttributes - None  */
    0x00,                           /* bLockDelayUnits - Unused */
    0x00, 0x00                      /* wLockDelay - unused */
};

/* Standard language ID string descriptor */
const uint8_t CyFxUsbStringLangIDDscr[] __attribute__ ((aligned (32))) =
{
    0x04,                           /* Descriptor size */
    CY_U3P_USB_STRING_DESCR,        /* Device descriptor type */
    0x09,0x04                       /* Language ID supported */
};

/* Standard manufacturer string descriptor */
const uint8_t CyFxUsbManufactureDscr[] __attribute__ ((aligned (32))) =
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
const uint8_t CyFxUsbProductDscr[] __attribute__ ((aligned (32))) =
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

