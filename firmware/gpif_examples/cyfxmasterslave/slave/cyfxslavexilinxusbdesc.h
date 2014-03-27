/*
 ## Cypress FX3 Application Example Source file (cyfxusbdscr.c)
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

/* This file contains the USB descriptors for the Xilinx FPGA configuration test firmware. */

#include "cyfxslavexilinx.h"
#include "cyu3usbconst.h"


/* Standard Device Descriptor for USB 2.0 */
const uint8_t CyFxUSB20DeviceDscr[] =
{
    0x12,                               /* Descriptor Size */
    CY_U3P_USB_DEVICE_DESCR,            /* Device Descriptor Type */
    0x10, 0x02,                         /* USB 2.1 */
    0x00,                               /* Device Class - Specified in Intf Descriptor */
    0x00,                               /* Device Sub-class */
    0x00,                               /* Device protocol */
    0x40,                               /* Maxpacket size for EP0 : 64 bytes */
    0xB4, 0x04,                         /* Vendor ID */
    0xF2, 0x00,                         /* Product ID */
    0x00, 0x00,                         /* Device release number */
    0x01,                               /* Manufacturer string index */
    0x02,                               /* Product string index */
    0x00,                               /* Serial number string index */
    0x01                                /* Number of configurations */
};

/* Standard Device Descriptor for USB 3.0 */
const uint8_t CyFxUSB30DeviceDscr[] =
{
    0x12,                               /* Descriptor Size */
    CY_U3P_USB_DEVICE_DESCR,            /* Device Descriptor Type */
    0x00, 0x03,                         /* USB 3.0 */
    0x00,                               /* Device Class - Specified in Intf descriptor */
    0x00,                               /* Device Sub-class */
    0x00,                               /* Device protocol */
    0x09,                               /* Maxpacket size for EP0 : 2^9 */
    0xB4, 0x04,                         /* Vendor ID */
    0xF2, 0x00,                         /* Product ID */
    0x00, 0x00,                         /* Device release number */
    0x01,                               /* Manufacture string index */
    0x02,                               /* Product string index */
    0x00,                               /* Serial number string index */
    0x01                                /* Number of configurations */
};

/* Standard Device Qualifier Descriptor */
const uint8_t CyFxUSBDeviceQualDscr[] =
{
    0x0A,                               /* Descriptor Size */
    CY_U3P_USB_DEVQUAL_DESCR,           /* Device Qualifier Descriptor Type */
    0x00, 0x02,                         /* USB 2.0 */
    0x00,                               /* Device Class */
    0x00,                               /* Device Sub-class */
    0x00,                               /* Device protocol */
    0x40,                               /* Maxpacket size for EP0 : 64 bytes */
    0x01,                               /* Number of configurations */
    0x00                                /* Reserved */
};

/* Standard Full Speed Configuration Descriptor */
const uint8_t CyFxUSBFSConfigDscr[] =
{
    /* Configuration Descriptor Type */
    0x09,                           /* Descriptor Size */
    CY_U3P_USB_CONFIG_DESCR,        /* Configuration Descriptor Type */
    0x19, 0x00,                     /* Length of this descriptor and all sub descriptors */
    0x01,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* Configuration string index */
    0xC0,                           /* Config characteristics - Self powered */
    0x01,                           /* Max power consumption of device (in 2 mA unit) : 2 mA */

    /* Interface Descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface Descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x01,                           /* Number of end points */
    0xFF,                           /* Interface class - Vendor specific */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* No Interface String */

    /* Endpoint Descriptor for Producer EP */
    0x07,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint Descriptor Type */
    CY_FX_EP_CONSUMER,              /* Endpoint address and description */
    CY_U3P_USB_EP_BULK,             /* Bulk End point Type */
    0x40, 0x00,                     /* Max packet size = 64 bytes */
    0x00,                           /* Servicing interval for data transfers : NA for Bulk */
};

/* Standard High Speed Configuration Descriptor */
const uint8_t CyFxUSBHSConfigDscr[] =
{
    /* Configuration Descriptor Type */
    0x09,                           /* Descriptor Size */
    CY_U3P_USB_CONFIG_DESCR,        /* Configuration Descriptor Type */
    0x19, 0x00,                     /* Length of this descriptor and all sub descriptors */
    0x01,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* Configuration string index */
    0xC0,                           /* Config characteristics - Self powered */
    0x01,                           /* Max power consumption of device (in 2 mA unit) : 2 mA */

    /* Interface Descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface Descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x01,                           /* Number of end points */
    0xFF,                           /* Interface class - Vendor specific */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* No Interface String */

    /* Endpoint Descriptor for Producer EP */
    0x07,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint Descriptor Type */
    CY_FX_EP_CONSUMER,              /* Endpoint address and description */
    CY_U3P_USB_EP_BULK,             /* Bulk End point Type */
    0x00, 0x02,                     /* Max packet size = 512 bytes */
    0x00,                           /* Servicing interval for data transfers : NA for Bulk */
};

/* Binary Device Object Store Descriptor */
const uint8_t CyFxUSBBOSDscr[] =
{
    0x05,                           /* Descriptor Size */
    CY_U3P_BOS_DESCR,               /* Device Descriptor Type */
    0x16, 0x00,                     /* Length of this descriptor and all sub descriptors */
    0x02,                           /* Number of device capability descriptors */

    /* USB 2.0 Extension */
    0x07,                           /* Descriptor Size */
    CY_U3P_DEVICE_CAPB_DESCR,       /* Device Capability Type descriptor */
    CY_U3P_USB2_EXTN_CAPB_TYPE,     /* USB 2.0 Extension Capability Type */
    0x00, 0x00,
    0x00, 0x00,                     /* Supported device level features  */

    /* SuperSpeed Device Capability */
    0x0A,                           /* Descriptor Size */
    CY_U3P_DEVICE_CAPB_DESCR,       /* Device Capability Type descriptor */
    CY_U3P_SS_USB_CAPB_TYPE,        /* SuperSpeed Device Capability Type */
    0x00,                           /* Supported device level features  */
    0x0E,0x00,                      /* Speeds Supported by the device : SS, HS and FS */
    0x03,                           /* Functionality support */
    0x0A,                           /* U1 Device Exit Latency */
    0xFF, 0x07                      /* U2 Device Exit Latency */
};

/* Standard Super Speed Configuration Descriptor */
const uint8_t CyFxUSBSSConfigDscr[] =
{
    /* Configuration Descriptor Type */
    0x09,                           /* Descriptor Size */
    CY_U3P_USB_CONFIG_DESCR,        /* Configuration Descriptor Type */
    0x1F, 0x00,                     /* Length of this descriptor and all sub descriptors */
    0x01,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* Configuration string index */
    0xC0,                           /* Config characteristics - Self powered. */
    0x01,                           /* Max power consumption of device (in 8mA unit) : 8 mA */

    /* Interface Descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface Descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x01,                           /* Number of end points */
    0xFF,                           /* Interface class - Vendor specific */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Endpoint Descriptor for Producer EP */
    0x07,                           /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint Descriptor Type */
    CY_FX_EP_CONSUMER,              /* Endpoint address and description */
    CY_U3P_USB_EP_BULK,             /* Bulk End point Type */
    0x00, 0x04,                     /* Max packet size = 1024 bytes */
    0x00,                           /* Servicing interval for data transfers : NA for Bulk */

    /* Super Speed Endpoint Companion Descriptor for Producer EP */
    0x06,                           /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,       /* SS Endpoint Companion Descriptor Type */
    0x00,                           /* Max no. of packets in a Burst : 0: Burst 1 packet at a time */
    0x00,                           /* Max streams for Bulk EP = 0 (No streams) */
    0x00, 0x00
};


/* Standard Language ID String Descriptor */
const uint8_t CyFxUSBStringLangIDDscr[] =
{
    0x04,                           /* Descriptor Size */
    CY_U3P_USB_STRING_DESCR,        /* Device Descriptor Type */
    0x09, 0x04                      /* Language ID supported - US English */
};

/* Standard Manufacturer String Descriptor */
const uint8_t CyFxUSBManufactureDscr[] =
{
    0x10,                           /* Descriptor Size */
    CY_U3P_USB_STRING_DESCR,        /* Device Descriptor Type */
    'C', 0x00,
    'y', 0x00,
    'p', 0x00,
    'r', 0x00,
    'e', 0x00,
    's', 0x00,
    's', 0x00
};


/* Standard Product String Descriptor */
const uint8_t CyFxUSBProductDscr[] =
{
    0x24,                           /* Descriptor Size */
    CY_U3P_USB_STRING_DESCR,        /* Device Descriptor Type */
    'F', 0x00,
    'X', 0x00,
    '3', 0x00,
    ' ', 0x00,
    'F', 0x00,
    'p', 0x00,
    'g', 0x00,
    'a', 0x00,
    ' ', 0x00,
    'E', 0x00,
    'm', 0x00,
    'u', 0x00,
    'l', 0x00,
    'a', 0x00,
    't', 0x00,
    'o', 0x00,
    'r', 0x00
};

/*[]*/

