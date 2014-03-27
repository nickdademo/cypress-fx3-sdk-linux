/*
 ## Cypress USB 3.0 Platform header file (cyfxslfifosync.h)
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

/* This file contains the constants and definitions used by the Slave FIFO application example */

#ifndef _INCLUDED_CYFXSLFIFOASYNC_H_
#define _INCLUDED_CYFXSLFIFOASYNC_H_

#include "cyu3externcstart.h"
#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3dma.h"

/* 16/32 bit GPIF Configuration select */
/* Set CY_FX_SLFIFO_GPIF_16_32BIT_CONF_SELECT = 0 for 16 bit GPIF data bus.
 * Set CY_FX_SLFIFO_GPIF_16_32BIT_CONF_SELECT = 1 for 32 bit GPIF data bus.
 */
#define CY_FX_SLFIFO_GPIF_16_32BIT_CONF_SELECT (0)

/* Select the number of address lines which intern select the NUmber of sockets used at P-Port 
 * and also the number of bulk end points with the equation.
 * Number of socket = Number of bulk endpoints = 2^(CY_FX_NUMBER_OF_ADDR_LINES-1)
 * 
 * Following is the table of parameters that varies with respect to the 
 * number of address lines
 * -------------------------------------------------------------------
 * |   CY_FX_NUMBER_OF_ADDR_LINES     |  2  |  3    |  4    |   5    |
 * |   Receive Data Socket            |  1  | 1-3   | 1-7   | 1-15   |
 * |   Send Data socket               |  3  | 5-7   | 9-15  | 17-31  |
 * |   OUT Bulk Endpoints             |  1  | 1-3   | 1-7   | 1-15   | 
 * |   IN Bulk Endpoints              |  81 | 81-83 | 81-87 | 81-8F  |
 * -------------------------------------------------------------------
 */
#define CY_FX_NUMBER_OF_ADDR_LINES 5

#define CY_FX_SLFIFO_DMA_BUF_COUNT      (2)                       /* Slave FIFO channel buffer count */
#define CY_FX_SLFIFO_DMA_TX_SIZE        (0)	                  /* DMA transfer size is set to infinite */
#define CY_FX_SLFIFO_DMA_RX_SIZE        (0)			  /* DMA transfer size is set to infinite */
#define CY_FX_SLFIFO_THREAD_STACK       (0x0400)                  /* Slave FIFO application thread stack size */
#define CY_FX_SLFIFO_THREAD_PRIORITY    (8)                       /* Slave FIFO application thread priority */

/* Endpoint and socket definitions for the Slave FIFO application */

/* To change the Producer and Consumer EP enter the appropriate EP numbers for the #defines.
 * In the case of IN endpoints enter EP number along with the direction bit.
 * For eg. EP 6 IN endpoint is 0x86
 *     and EP 6 OUT endpoint is 0x06.
 * To change sockets mention the appropriate socket number in the #defines. */

/* Note: For USB 2.0 the endpoints and corresponding sockets are one-to-one mapped
         i.e. EP 1 is mapped to UIB socket 1 and EP 2 to socket 2 so on */


#define CY_FX_EP_PRODUCER_1                0x01    /* EP 1 OUT  */
#define CY_FX_EP_PRODUCER_2                0x02    /* EP 2 OUT  */
#define CY_FX_EP_PRODUCER_3                0x03    /* EP 3 OUT  */
#define CY_FX_EP_PRODUCER_4                0x04    /* EP 4 OUT  */
#define CY_FX_EP_PRODUCER_5                0x05    /* EP 5 OUT */
#define CY_FX_EP_PRODUCER_6                0x06    /* EP 6 OUT */
#define CY_FX_EP_PRODUCER_7                0x07    /* EP 7 OUT */
#define CY_FX_EP_PRODUCER_8                0x08    /* EP 8 OUT */
#define CY_FX_EP_PRODUCER_9                0x09    /* EP 9 OUT */
#define CY_FX_EP_PRODUCER_10               0x0A    /* EP 10 OUT */
#define CY_FX_EP_PRODUCER_11               0x0B    /* EP 11 OUT */
#define CY_FX_EP_PRODUCER_12               0x0C    /* EP 12 OUT */
#define CY_FX_EP_PRODUCER_13               0x0D    /* EP 13 OUT */
#define CY_FX_EP_PRODUCER_14               0x0E    /* EP 14 OUT */
#define CY_FX_EP_PRODUCER_15               0x0F    /* EP 15 OUT */

#define CY_FX_EP_CONSUMER_1                0x81    /* EP 1 IN  */
#define CY_FX_EP_CONSUMER_2                0x82    /* EP 2 IN  */
#define CY_FX_EP_CONSUMER_3                0x83    /* EP 3 IN  */
#define CY_FX_EP_CONSUMER_4                0x84    /* EP 4 IN  */
#define CY_FX_EP_CONSUMER_5                0x85    /* EP 5 IN  */
#define CY_FX_EP_CONSUMER_6                0x86    /* EP 6 IN  */
#define CY_FX_EP_CONSUMER_7                0x87    /* EP 7 IN  */
#define CY_FX_EP_CONSUMER_8                0x88    /* EP 8 IN  */
#define CY_FX_EP_CONSUMER_9                0x89    /* EP 9 IN  */
#define CY_FX_EP_CONSUMER_10               0x8A    /* EP 10 IN */
#define CY_FX_EP_CONSUMER_11               0x8B    /* EP 11 IN */
#define CY_FX_EP_CONSUMER_12               0x8C    /* EP 12 IN */
#define CY_FX_EP_CONSUMER_13               0x8D    /* EP 13 IN */
#define CY_FX_EP_CONSUMER_14               0x8E    /* EP 14 IN */
#define CY_FX_EP_CONSUMER_15               0x8F    /* EP 15 IN */


#define CY_FX_PRODUCER_USB_SOCKET_BASE CY_U3P_UIB_SOCKET_PROD_0
#define CY_FX_CONSUMER_USB_SOCKET_BASE CY_U3P_UIB_SOCKET_CONS_0

#if (CY_FX_NUMBER_OF_ADDR_LINES == 2)

/* Length of this descriptor and all sub descriptors */
#define CY_U3P_HS_LENGTH_DESCRIPTOR 0x20,0x00
#define CY_U3P_FS_LENGTH_DESCRIPTOR 0x20,0x00
#define CY_U3P_SS_LENGTH_DESCRIPTOR 0x2C,0x00

/* Number of end points */
#define CY_U3P_NUMBER_OF_ENDPOINTS 0x02

/* PPORT Socket Base Address */
#define CY_FX_PRODUCER_PPORT_SOCKET_BASE CY_U3P_PIB_SOCKET_2
#define CY_FX_CONSUMER_PPORT_SOCKET_BASE CY_U3P_PIB_SOCKET_0

#endif

#if (CY_FX_NUMBER_OF_ADDR_LINES == 3)

/* Length of this descriptor and all sub descriptors */
#define CY_U3P_HS_LENGTH_DESCRIPTOR 0x3C,0x00
#define CY_U3P_FS_LENGTH_DESCRIPTOR 0x3C,0x00
#define CY_U3P_SS_LENGTH_DESCRIPTOR 0x60,0x00

/* Number of end points */
#define CY_U3P_NUMBER_OF_ENDPOINTS 0x06

/* PPORT Socket Base Address */
#define CY_FX_PRODUCER_PPORT_SOCKET_BASE CY_U3P_PIB_SOCKET_4
#define CY_FX_CONSUMER_PPORT_SOCKET_BASE CY_U3P_PIB_SOCKET_0

#endif

#if (CY_FX_NUMBER_OF_ADDR_LINES == 4)
/* Length of this descriptor and all sub descriptors */
#define CY_U3P_HS_LENGTH_DESCRIPTOR 0x74,0x00
#define CY_U3P_FS_LENGTH_DESCRIPTOR 0x74,0x00
#define CY_U3P_SS_LENGTH_DESCRIPTOR 0xC8,0x00

/* Number of end points */
#define CY_U3P_NUMBER_OF_ENDPOINTS 0x0E

/* PPORT Socket Base Address */
#define CY_FX_PRODUCER_PPORT_SOCKET_BASE CY_U3P_PIB_SOCKET_8
#define CY_FX_CONSUMER_PPORT_SOCKET_BASE CY_U3P_PIB_SOCKET_0
#endif

#if (CY_FX_NUMBER_OF_ADDR_LINES == 5)

/* Length of this descriptor and all sub descriptors */
#define CY_U3P_HS_LENGTH_DESCRIPTOR 0xE4,0x00
#define CY_U3P_FS_LENGTH_DESCRIPTOR 0xE4,0x00
#define CY_U3P_SS_LENGTH_DESCRIPTOR 0x98,0x01

/* Number of end points */
#define CY_U3P_NUMBER_OF_ENDPOINTS 0x1E

/* PPORT Socket Base Address */
#define CY_FX_PRODUCER_PPORT_SOCKET_BASE CY_U3P_PIB_SOCKET_16
#define CY_FX_CONSUMER_PPORT_SOCKET_BASE CY_U3P_PIB_SOCKET_0


#endif
/* Producer socket used by the U-Port */
#define CY_FX_PRODUCER_1_USB_SOCKET     CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_1     /* USB Socket 1 is producer */
#define CY_FX_PRODUCER_2_USB_SOCKET     CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_2     /* USB Socket 2 is producer */
#define CY_FX_PRODUCER_3_USB_SOCKET     CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_3     /* USB Socket 3 is producer */
#define CY_FX_PRODUCER_4_USB_SOCKET     CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_4     /* USB Socket 4 is producer */
#define CY_FX_PRODUCER_5_USB_SOCKET     CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_5     /* USB Socket 5 is producer */
#define CY_FX_PRODUCER_6_USB_SOCKET     CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_6     /* USB Socket 6 is producer */
#define CY_FX_PRODUCER_7_USB_SOCKET     CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_7     /* USB Socket 7 is producer */
#define CY_FX_PRODUCER_8_USB_SOCKET     CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_8     /* USB Socket 8 is producer */
#define CY_FX_PRODUCER_9_USB_SOCKET     CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_9     /* USB Socket 9 is producer */
#define CY_FX_PRODUCER_10_USB_SOCKET    CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_10    /* USB Socket 10 is producer */
#define CY_FX_PRODUCER_11_USB_SOCKET    CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_11    /* USB Socket 11 is producer */
#define CY_FX_PRODUCER_12_USB_SOCKET    CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_12    /* USB Socket 12 is producer */
#define CY_FX_PRODUCER_13_USB_SOCKET    CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_13    /* USB Socket 13 is producer */
#define CY_FX_PRODUCER_14_USB_SOCKET    CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_14    /* USB Socket 14 is producer */
#define CY_FX_PRODUCER_15_USB_SOCKET    CY_FX_PRODUCER_USB_SOCKET_BASE + CY_FX_EP_PRODUCER_15    /* USB Socket 15 is producer */

/* Consumer socket used by the U-Port */
#define CY_FX_CONSUMER_1_USB_SOCKET     CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_1 & 0xF)    /* USB Socket 1 is consumer */
#define CY_FX_CONSUMER_2_USB_SOCKET     CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_2 & 0xF)    /* USB Socket 2 is consumer */
#define CY_FX_CONSUMER_3_USB_SOCKET     CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_3 & 0xF)    /* USB Socket 3 is consumer */
#define CY_FX_CONSUMER_4_USB_SOCKET     CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_4 & 0xF)    /* USB Socket 4 is consumer */
#define CY_FX_CONSUMER_5_USB_SOCKET     CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_5 & 0xF)    /* USB Socket 5 is consumer */
#define CY_FX_CONSUMER_6_USB_SOCKET     CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_6 & 0xF)    /* USB Socket 6 is consumer */
#define CY_FX_CONSUMER_7_USB_SOCKET     CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_7 & 0xF)    /* USB Socket 7 is consumer */
#define CY_FX_CONSUMER_8_USB_SOCKET     CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_8 & 0xF)    /* USB Socket 8 is consumer */
#define CY_FX_CONSUMER_9_USB_SOCKET     CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_9 & 0xF)    /* USB Socket 9 is consumer */
#define CY_FX_CONSUMER_10_USB_SOCKET    CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_10 & 0xF)   /* USB Socket 10 is consumer */
#define CY_FX_CONSUMER_11_USB_SOCKET    CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_11 & 0xF)   /* USB Socket 11 is consumer */
#define CY_FX_CONSUMER_12_USB_SOCKET    CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_12 & 0xF)   /* USB Socket 12 is consumer */
#define CY_FX_CONSUMER_13_USB_SOCKET    CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_13 & 0xF)   /* USB Socket 13 is consumer */
#define CY_FX_CONSUMER_14_USB_SOCKET    CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_14 & 0xF)   /* USB Socket 14 is consumer */
#define CY_FX_CONSUMER_15_USB_SOCKET    CY_FX_CONSUMER_USB_SOCKET_BASE + (CY_FX_EP_CONSUMER_15 & 0xF)   /* USB Socket 15 is consumer */



/* Producer socket used by the P-Port */
#define CY_FX_PRODUCER_1_PPORT_SOCKET     CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_1     /* P-port Socket 1 is producer */
#define CY_FX_PRODUCER_2_PPORT_SOCKET     CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_2     /* P-port Socket 2 is producer */
#define CY_FX_PRODUCER_3_PPORT_SOCKET     CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_3     /* P-port Socket 3 is producer */
#define CY_FX_PRODUCER_4_PPORT_SOCKET     CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_4     /* P-port Socket 4 is producer */
#define CY_FX_PRODUCER_5_PPORT_SOCKET     CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_5     /* P-port Socket 5 is producer */
#define CY_FX_PRODUCER_6_PPORT_SOCKET     CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_6     /* P-port Socket 6 is producer */
#define CY_FX_PRODUCER_7_PPORT_SOCKET     CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_7     /* P-port Socket 7 is producer */
#define CY_FX_PRODUCER_8_PPORT_SOCKET     CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_8     /* P-port Socket 8 is producer */
#define CY_FX_PRODUCER_9_PPORT_SOCKET     CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_9     /* P-port Socket 9 is producer */
#define CY_FX_PRODUCER_10_PPORT_SOCKET    CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_10    /* P-port Socket 10 is producer */
#define CY_FX_PRODUCER_11_PPORT_SOCKET    CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_11    /* P-port Socket 11 is producer */
#define CY_FX_PRODUCER_12_PPORT_SOCKET    CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_12    /* P-port Socket 12 is producer */
#define CY_FX_PRODUCER_13_PPORT_SOCKET    CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_13    /* P-port Socket 13 is producer */
#define CY_FX_PRODUCER_14_PPORT_SOCKET    CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_14    /* P-port Socket 14 is producer */
#define CY_FX_PRODUCER_15_PPORT_SOCKET    CY_FX_PRODUCER_PPORT_SOCKET_BASE + CY_FX_EP_PRODUCER_15    /* P-port Socket 15 is producer */


/* Consumer socket used by the P-Port */
#define CY_FX_CONSUMER_1_PPORT_SOCKET     CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_1  & 0xF)   /* P-port Socket 1 is CONSUMER */
#define CY_FX_CONSUMER_2_PPORT_SOCKET     CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_2  & 0xF)   /* P-port Socket 2 is CONSUMER */
#define CY_FX_CONSUMER_3_PPORT_SOCKET     CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_3  & 0xF)   /* P-port Socket 3 is CONSUMER */
#define CY_FX_CONSUMER_4_PPORT_SOCKET     CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_4  & 0xF)   /* P-port Socket 4 is CONSUMER */
#define CY_FX_CONSUMER_5_PPORT_SOCKET     CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_5  & 0xF)   /* P-port Socket 5 is CONSUMER */
#define CY_FX_CONSUMER_6_PPORT_SOCKET     CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_6  & 0xF)   /* P-port Socket 6 is CONSUMER */
#define CY_FX_CONSUMER_7_PPORT_SOCKET     CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_7  & 0xF)   /* P-port Socket 7 is CONSUMER */
#define CY_FX_CONSUMER_8_PPORT_SOCKET     CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_8  & 0xF)   /* P-port Socket 8 is CONSUMER */
#define CY_FX_CONSUMER_9_PPORT_SOCKET     CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_9  & 0xF)   /* P-port Socket 9 is CONSUMER */
#define CY_FX_CONSUMER_10_PPORT_SOCKET    CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_10 & 0xF)   /* P-port Socket 10 is CONSUMER */
#define CY_FX_CONSUMER_11_PPORT_SOCKET    CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_11 & 0xF)   /* P-port Socket 11 is CONSUMER */
#define CY_FX_CONSUMER_12_PPORT_SOCKET    CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_12 & 0xF)   /* P-port Socket 12 is CONSUMER */
#define CY_FX_CONSUMER_13_PPORT_SOCKET    CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_13 & 0xF)   /* P-port Socket 13 is CONSUMER */
#define CY_FX_CONSUMER_14_PPORT_SOCKET    CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_14 & 0xF)   /* P-port Socket 14 is CONSUMER */
#define CY_FX_CONSUMER_15_PPORT_SOCKET    CY_FX_CONSUMER_PPORT_SOCKET_BASE + (CY_FX_EP_CONSUMER_15 & 0xF)   /* P-port Socket 15 is CONSUMER */


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
extern const uint8_t CyFxProducerEndPoint[];
extern const uint8_t CyFxConsumerEndPoint[];
extern const uint16_t CyFxProducerSocket[];
extern const uint16_t CyFxConsumerSocket[];

#include "cyu3externcend.h"

#endif /* _INCLUDED_CYFXSLFIFOASYNC_H_ */

/*[]*/
