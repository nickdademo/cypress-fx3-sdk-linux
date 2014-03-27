/*
 ## Cypress FX3S Example header file (cyfx3s_sdiouart.h)
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

#ifndef _INCLUDED_CYFX3S_SDIOUART_H_
#define _INCLUDED_CYFX3S_SDIOUART_H_

/* This file contains the constants used by the USB to SDIO-UART application example */

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3sib.h"
#include "cyu3cardmgr_fx3s.h"
#include "cyu3uart.h"
#include "cyu3externcstart.h"

#define CY_FX_SDIOUART_THREAD_STACK             (1024)
#define CY_FX_SDIOUART_THREAD_PRIORITY          (8)

/* Endpoint and socket definitions for the USB-UART application */
#define CY_FX_EP_PRODUCER               (0x02)                             /* EP 2 OUT */
#define CY_FX_EP_CONSUMER               (0x82)                             /* EP 2 IN */
#define CY_FX_EP_INTERRUPT              (0x81)                             /* EP 1 INTR */

#define CY_FX_EP_PRODUCER1_SOCKET       (CY_U3P_UIB_SOCKET_PROD_2)
#define CY_FX_EP_CONSUMER1_SOCKET       (CY_U3P_SIB_SOCKET_0)
#define CY_FX_EP_PRODUCER2_SOCKET       (CY_U3P_SIP_SOCKET_1)
#define CY_FX_EP_CONSUMER2_SOCKET       (CY_U3P_UIB_SOCKET_CONS_2)
#define CY_FX_EP_INTR_CONSUMER1_SOCKET  (CY_U3P_UIB_SOCKET_CONS_1)
#define CY_TRNSFR_NONE                  (0x00)
#define CY_TRNSFR_UART_TO_USB           (0x01)
#define CY_TRNSFR_USB_TO_UART           (0x02)

#define CY_FX_SDIOUART_DMA_BUF_COUNT    (8)

/* Arasan SDIO-UART Specific Registers */
/* Line status register bits */
#define CY_SDIOUART_DATA_READY          (0x01)
#define CY_SDIOUART_OVERRUN_ERROR       (0x02)
#define CY_SDIOUART_PARITY_ERROR        (0x04)
#define CY_SDIOUART_FRAMING_ERRROR      (0x08)
#define CY_SDIOUART_BREAK_INTR          (0x10)
#define CY_SDIOUART_TX_HLD_REG_EMPTY    (0x20)
#define CY_SDIOUART_TX_EMPTY            (0x40)
#define CY_SDIOUART_ERR_FIFO            (0x80)

/* in DLAB=0 and read */
#define CY_SDIOUART_RBR                 (0x00)
#define CY_SDIOUART_IER                 (0x01)
#define CY_SDIOUART_IIR                 (0x02)
#define CY_SDIOUART_LCR                 (0x03)
#define CY_SDIOUART_MCR                 (0x04)
#define CY_SDIOUART_LSR                 (0x05)
#define CY_SDIOUART_MSR                 (0x06)
#define CY_SDIOUART_SCR                 (0x07)

/* modem status register */
#define CY_SDIOUART_CTS                 (0x10)
#define CY_SDIOUART_DSR                 (0x20)
#define CY_SDIOUART_RI                  (0x40)
#define CY_SDIOUART_DCD                 (0x80)

/* in dlab=0 and write */
#define CY_SDIOUART_THR                 (0x00)
#define CY_SDIOUART_IER                 (0x01)
#define CY_SDIOUART_FCR                 (0x02)
#define CY_SDIOUART_MCR                 (0x04)
#define CY_SDIOUART_SCR                 (0x07)
#define CY_SDIOUART_EFIFOER             (0x08)
#define CY_SDIOUART_TCR                 (0x09)
#define CY_SDIOUART_RCR                 (0x0A)

#define CY_SDIOUART_DLAB_MASK           (0x80)

/* in DLAB =1 */
#define CY_SDIOUART_DRL                 (0x00)
#define CY_SDIOUART_DRM                 (0x01)

/* Maximum baud rate supported. */
#define CY_SDIOUART_MAXBAUD             (921600)

/* Clock dividers used to select various baud rates. */
#define CY_SDIOUART_BAUD_1200           (CY_SDIOUART_MAXBAUD/1200)
#define CY_SDIOUART_BAUD_2400           (CY_SDIOUART_MAXBAUD/2400)
#define CY_SDIOUART_BAUD_4800           (CY_SDIOUART_MAXBAUD/4800)
#define CY_SDIOUART_BAUD_9600           (CY_SDIOUART_MAXBAUD/9600)
#define CY_SDIOUART_BAUD_14400          (CY_SDIOUART_MAXBAUD/14400)
#define CY_SDIOUART_BAUD_19200          (CY_SDIOUART_MAXBAUD/19200)
#define CY_SDIOUART_BAUD_38400          (CY_SDIOUART_MAXBAUD/38400)
#define CY_SDIOUART_BAUD_57600          (CY_SDIOUART_MAXBAUD/57600)
#define CY_SDIOUART_BAUD_115200         (CY_SDIOUART_MAXBAUD/115200)

/* Descriptor Types */
#define CY_FX_BOS_DSCR_TYPE             (15)
#define CY_FX_DEVICE_CAPB_DSCR_TYPE     (16)
#define CY_FX_SS_EP_COMPN_DSCR_TYPE     (48)

/* Device Capability Type Codes */
#define CY_FX_WIRELESS_USB_CAPB_TYPE    (1)
#define CY_FX_USB2_EXTN_CAPB_TYPE       (2)
#define CY_FX_SS_USB_CAPB_TYPE          (3)
#define CY_FX_CONTAINER_ID_CAPBD_TYPE   (4)

/* CDC-ACM bRequest codes. */
#define SET_LINE_CODING                 (0x20)
#define GET_LINE_CODING                 (0x21)
#define SET_CONTROL_LINE_STATE          (0x22)

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


/* Structure to store the FBR for the SDIO device */
typedef struct CyFxSdioFbr
{
    uint8_t  isInitialized;     /* Set if Function has been initialized. */
    uint8_t  interfaceCode;     /* Standard Function Interface Code */
    uint8_t  extInterfaceCode;  /* Extended Function Interface Code */
    uint8_t  supportCSA;        /* Function support for the CSA */
    uint8_t  supportWakeUp;     /* Function supports Wake up or not */
    uint8_t  csaProperty;       /* CSA Property Byte for the Function (See SDIO Spec for details) */
    uint16_t maxIoBlockSize;    /* IO block size for the function */
    uint32_t addrCIS;           /* Pointer to the CIS register for the function */
    uint32_t addrCSA;           /* Pointer to the CSA register for the function */
    uint32_t cardPSN;           /* Card Product serial number  */
    uint32_t csaSize;           /* CSA Size in Bytes */
} CyFxSdioFbr_t;


/* Structure to Store SDIO Card Data */
typedef struct CyFxSdioCardData
{
    uint8_t             isSDIO;         /* Set if Card is an SDIo Card*/
    CyU3PSdioCardRegs_t cardInfo;       /* Pointer to structure storing card information read from the CCCR registers*/
    CyFxSdioFbr_t       fbr[7];         /* Pointers to FBR objects for the functions */
} CyFxSdioCardData_t;

/* Structure to store UART specific configuration and state parameters */
typedef struct UARTSettings
{
    uint8_t uartExists;                 /* Set if UART function is found on SDIO card. */
    uint8_t sdioPort;                   /* Card port on which UART Function exists. */
    uint8_t sdioFunction;               /* Function number for UART function. */
} UARTSettings_t;

#include "cyu3externcend.h"

#endif /* _INCLUDED_CYFX3S_SDIOUART_H_ */

/*[]*/

