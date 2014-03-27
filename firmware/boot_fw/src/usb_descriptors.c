/*
 ## Cypress FX3 Boot Firmware Example Source file (usb_descriptors.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2011-2012,
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


/* Device, config and string descriptors. */
unsigned char gbDevDesc[] =
{
    0x12,                           /* Descriptor Size */
    0x01,                           /* Device Descriptor Type */
    0x10,0x02,                      /* USB 2.0 */
    0xFF,                           /* Device Class */
    0xFF,                           /* Device Sub-class */
    0xFF,                           /* Device protocol */
    0x40,                           /* Maxpacket size for EP0 : 64 bytes */
    0xB4,0x04,                      /* Vendor ID */
    0xF0,0x00,                      /* Product ID */
    0x00,0x00,                      /* Device release number */
    0x01,                           /* Manufacture string index */
    0x02,                           /* Product string index */
    0x00,                           /* Serial number string index */
    0x01                            /* Number of configurations */
};

unsigned char gbDevQualDesc[] =
{
    0x0A,                           /* Descriptor Size */
    0x06,                           /* Device Qualifier Descriptor Type */
    0x00,0x02,                      /* USB 2.0 */
    0xFF,                           /* Device Class */
    0xFF,                           /* Device Sub-class */
    0xFF,                           /* Device protocol */
    0x40,                           /* Maxpacket size for EP0 : 64 bytes */
    0x01,                           /* Number of configurations */
    0x00                            /* Reserved */
};

unsigned char gbCfgDesc[] =
{
    0x09,                           /* Descriptor Size */
    0x02,                           /* Configuration Descriptor Type */
    0x20,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x01,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* COnfiguration string index */
    0x80,                           /* Config characteristics - Bus powered */
    0x32,                           /* Max power consumption of device (in 2mA unit) : 100mA */

    /* Interface Descriptor */
    0x09,                           /* Descriptor size */
    0x04,                           /* Interface Descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x02,                           /* Number of end points */
    0xFF,                           /* Interface class */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Endpoint Descriptor for Producer EP */
    0x07,                           /* Descriptor size */
    0x05,                           /* Endpoint Descriptor Type */
    0x01,                           /* Endpoint address and description */
    0x02,                           /* Bulk End point Type */
    0x00,                           /* Max packet size = 512 bytes */
    0x02,
    0x00,                           /* Servicing interval for data transfers : NA for Bulk */

    /* Endpoint Descriptor for Consumer EP */
    0x07,                           /* Descriptor size */
    0x05,                           /* Endpoint Descriptor Type */
    0x81,                           /* Endpoint address and description */
    0x02,                           /* Bulk End point Type */
    0x00,                           /* Max packet size = 512 bytes */
    0x02,
    0x00                            /* Servicing interval for data transfers : NA for Bulk */
};

unsigned char gbLangIDDesc[] =
{
    0x04,                           /* Descriptor Size */
    0x03,                           /* Device Descriptor Type */
    0x09,0x04                       /* Language ID supported */
};

/* Standard Manufacturer String Descriptor */
unsigned char gbManufactureDesc[] =
{
    0x10,                           /* Descriptor Size */
    0x03,                           /* Device Descriptor Type */
    'C',0x00,
    'y',0x00,
    'p',0x00,
    'r',0x00,
    'e',0x00,
    's',0x00,
    's',0x00
};

unsigned char gbProductDesc[] =
{
    0x08,                           /* Descriptor Size */
    0x03,                           /* Device Descriptor Type */
    'F',0x00,
    'X',0x00,
    '3',0x00
};

unsigned char gbSerialNumDesc [] = 
{
    0x1A,                           /* bLength */
    0x03,                           /* bDescType */
    '0',0,'0',0,'0',0,'0',0,'0',0,'0',0,
    '0',0,'0',0,'0',0,'4',0,'B',0,'E',0,
    0,0,                            /* long word align */
};

/* SuperSpeed descriptors */

/* Binary Device Object Store Descriptor */
unsigned char gbBosDesc[] =
{
    0x05,                           /* Descriptor Size */
    0x0F,                           /* Device Descriptor Type */
    0x16,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x02,                           /* Number of device capability descriptors */

    /* USB 2.0 Extension */
    0x07,                           /* Descriptor Size */
    0x10,                           /* Device Capability Type descriptor */
    0x02,                           /* USB 2.0 Extension Capability Type */
    0x02,0x00,0x00,0x00,            /* Supported device level features - LPM Support */

    /* SuperSpeed Device Capability */
    0x0A,                           /* Descriptor Size */
    0x10,                           /* Device Capability Type descriptor */
    0x03,                           /* SuperSpeed Device Capability Type */
    0x00,                           /* Supported device level features  */
    0x0E,0x00,                      /* Speeds Supported by the device : SS, HS and FS */
    0x03,                           /* Functionality support */
    0x00,                           /* U1 Device Exit Latency */
    0x00,0x00                       /* U2 Device Exit Latency */
};

/* Standard Super Speed Configuration Descriptor */
unsigned char gbSsConfigDesc[] =
{
    /* Configuration Descriptor Type */
    0x09,                           /* Descriptor Size */
    0x02,                           /* Configuration Descriptor Type */
    0x2C,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x01,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* COnfiguration string index */
    0x80,                           /* Config characteristics - D6: Self power; D5: Remote Wakeup */
    0x32,                           /* Max power consumption of device (in 8mA unit) : 400mA */

    /* Interface Descriptor */
    0x09,                           /* Descriptor size */
    0x04,                           /* Interface Descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x02,                           /* Number of end points */
    0xFF,                           /* Interface class */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Endpoint Descriptor for Producer EP */
    0x07,                           /* Descriptor size */
    0x05,                           /* Endpoint Descriptor Type */
    0x01,                           /* Endpoint address and description */
    0x02,                           /* Bulk End point Type */
    0x00,0x04,                      /* Max packet size = 1024 bytes */
    0x00,                           /* Servicing interval for data transfers : NA for Bulk */

    /* Super Speed Endpoint Companion Descriptor for Producer EP */
    0x06,                           /* Descriptor size */
    0x30,                           /* SS Endpoint Companion Descriptor Type */
    0x00,                           /* Max no. of packets in a Burst : 0: Burst 1 packet at a time */
    0x00,                           /* Max streams for Bulk EP = 0 (No streams)*/
    0x00,0x00,                      /* Service interval for the EP : NA for Bulk */

    /* Endpoint Descriptor for Consumer EP */
    0x07,                           /* Descriptor size */
    0x05,                           /* Endpoint Descriptor Type */
    0x81,                           /* Endpoint address and description */
    0x02,                           /* Bulk End point Type */
    0x00,0x04,                      /* Max packet size = 1024 bytes */
    0x00,                           /* Servicing interval for data transfers : NA for Bulk */

    /* Super Speed Endpoint Companion Descriptor for Consumer EP */
    0x06,                           /* Descriptor size */
    0x30,                           /* SS Endpoint Companion Descriptor Type */
    0x00,                           /* Max no. of packets in a Burst : 0: Burst 1 packet at a time */
    0x00,                           /* Max streams for Bulk EP = 0 (No streams)*/
    0x00,0x00                       /* Service interval for the EP : NA for Bulk */
};

/* Standard Device Descriptor for USB 3.0 */
unsigned char gbSsDevDesc[] =
{
    0x12,                           /* Descriptor Size */
    0x01,                           /* Device Descriptor Type */
    0x00,0x03,                      /* USB 3.0 */
    0xFF,                           /* Device Class */
    0xFF,                           /* Device Sub-class */
    0xFF,                           /* Device protocol */
    0x09,                           /* Maxpacket size for EP0 : 2^9 */
    0xB4,0x04,                      /* Vendor ID */
    0xF0,0x00,                      /* Product ID */
    0x00,0x00,                      /* Device release number */
    0x01,                           /* Manufacture string index */
    0x02,                           /* Product string index */
    0x00,                           /* Serial number string index */
    0x01                            /* Number of configurations */
};

/* Standard Full Speed Configuration Descriptor */
unsigned char gbFsConfigDesc[] =
{
    /* Configuration Descriptor Type */
    0x09,                           /* Descriptor Size */
    0x02,                           /* Configuration Descriptor Type */
    0x20,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x01,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* COnfiguration string index */
    0x80,                           /* Config characteristics - Bus powered */
    0x32,                           /* Max power consumption of device (in 2mA unit) : 100mA */

    /* Interface Descriptor */
    0x09,                           /* Descriptor size */
    0x04,                           /* Interface Descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x02,                           /* Number of end points */
    0xFF,                           /* Interface class */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Endpoint Descriptor for Producer EP */
    0x07,                           /* Descriptor size */
    0x05,                           /* Endpoint Descriptor Type */
    0x01,                           /* Endpoint address and description */
    0x02,                           /* Bulk End point Type */
    0x40,0x00,                      /* Max packet size = 64 bytes */
    0x00,                           /* Servicing interval for data transfers : NA for Bulk */

    /* Endpoint Descriptor for Consumer EP */
    0x07,                           /* Descriptor size */
    0x05,                           /* Endpoint Descriptor Type */
    0x81,                           /* Endpoint address and description */
    0x02,                           /* Bulk End point Type */
    0x40,0x00,                      /* Max packet size = 64 bytes */
    0x00                            /* Servicing interval for data transfers : NA for Bulk */
};

