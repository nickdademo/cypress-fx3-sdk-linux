/*
 ## Cypress FX3 Application Example Header File (cyfxfpgaconftest.h)
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

/* This file contains the constants and definitions used by the
 * Xilinx FPGA Serial Slave configuration emulator example for
 * the FX3 device.
 */

#ifndef _INCLUDED_CYFXFPGACONFTEST_H_
#define _INCLUDED_CYFXFPGACONFTEST_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3gpif.h"
#include "cyu3externcstart.h"

#define CY_FX_APP_THREAD_STACK_SIZE     (0x0400)                /* Application thread stack size */
#define CY_FX_APP_THREAD_PRIORITY       (8)                     /* Application thread priority */

#define CY_FX_APP_DMA_BUF_COUNT         (4)                    /* Use sixteen buffers on USB side. */

/* Endpoint and socket definitions for the application */

/* To change the Producer and Consumer EP enter the appropriate EP numbers for the #defines.
 * In the case of IN endpoints enter EP number along with the direction bit.
 * For eg. EP 6 IN endpoint is 0x86
 *     and EP 6 OUT endpoint is 0x06.
 * To change sockets mention the appropriate socket number in the #defines. */
#define CY_FX_EP_CONSUMER               0x81                            /* EP 1 IN */
#define CY_FX_EP_CONSUMER_USB_SOCKET    CY_U3P_UIB_SOCKET_CONS_1        /* USB Socket 1 is the consumer */
#define CY_FX_EP_PRODUCER_PPORT_SOCKET  CY_U3P_PIB_SOCKET_0             /* P-port Socket 0 is the producer */

/* GPIOs used for control signaling during the FPGA configuration protocol. */
#define CY_FX_FPGA_PROGRAM_GPIO         19                              /* PROGRAM input signal. */
#define CY_FX_FPGA_INITB_GPIO           17                              /* INIT_B output signal. */
#define CY_FX_FPGA_DONE_GPIO            20                              /* DONE output signal. */
#define CY_FX_FPGA_COMMIT_GPIO          22                              /* COMMIT Input Signal */

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

#endif /* _INCLUDED_CYFXFPGACONFTEST_H_ */

/*[]*/

