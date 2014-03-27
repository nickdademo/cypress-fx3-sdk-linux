/*
 ## Cypress USB 3.0 Platform header file (cyfxusbi2cregmode.h)
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

/* This file contains the externants used by the USB I2C application. */

#ifndef _INCLUDED_CYFXUSBI2CREGMODE_H_
#define _INCLUDED_CYFXUSBI2CREGMODE_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3externcstart.h"

#define APPTHREAD_STACK       (0x0800)    /* App thread stack size */
#define APPTHREAD_PRIORITY    (8)         /* App thread priority */

/* This application uses EEPROM as the slave I2C device. The I2C EEPROM
 * part number used is 24LC256. The capacity of the EEPROM is 256K bits */
#define CY_FX_USBI2C_I2C_MAX_CAPACITY   (32 * 1024) /* Capacity in bytes */

/* The following constant is defined based on the page size that the I2C
 * device support. 24LC256 support 64 byte page write access. */
#define CY_FX_USBI2C_I2C_PAGE_SIZE      (64)

/* I2C Data rate */
#define CY_FX_USBI2C_I2C_BITRATE        (100000)

/* Give a timeout value of 5s for any programming. */
#define CY_FX_USB_I2C_TIMEOUT                (5000)

/* USB vendor requests supported by the application. */

/* USB vendor request to read the 8 byte firmware ID. This will return content 
 * of glFirmwareID array. */
#define CY_FX_RQT_ID_CHECK                      (0xB0)

/* USB vendor request to write to I2C EEPROM connected. The EEPROM page size is
 * fixed to 64 bytes. The I2C EEPROM address is provided in the value field. The
 * memory address to start writing is provided in the index field of the request.
 * The maximum allowed request length is 4KB. */
#define CY_FX_RQT_I2C_EEPROM_WRITE              (0xBA)

/* USB vendor request to read from I2C EEPROM connected. The EEPROM page size is
 * fixed to 64 bytes. The I2C EEPROM address is provided in the value field. The
 * memory address to start reading from is provided in the index field of the
 * request. The maximum allowed request length is 4KB. */
#define CY_FX_RQT_I2C_EEPROM_READ               (0xBB)

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

#endif /* _INCLUDED_CYFXUSBI2CREGMODE_H_ */

/*[]*/
