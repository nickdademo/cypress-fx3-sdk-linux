/*
 ## Cypress USB 3.0 Platform header file (cyfx3s_fatfs.h)
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

/* This file contains the constants and definitions for the FATFS application example */

#ifndef _INCLUDED_CYFXMSCDEMO_H_
#define _INCLUDED_CYFXMSCDEMO_H_

#include <cyu3types.h>
#include <cyu3usbconst.h>
#include <cyu3externcstart.h>

/* 
 * Endpoint and socket definitions for the FatFs integration example.
 * To change sockets, mention the appropriate socket number in the #defines.
 */

extern CyU3PDmaChannel glChHandleStorOut;               /* DMA channel for CPU -> SIB */
extern CyU3PDmaChannel glChHandleStorIn;                /* DMA channel for SIB -> CPU. */
extern CyU3PDmaChannel glChHandleUARTIN;                /* DMA channel for CPU -> UART */
extern CyU3PEvent      glStorDriverEvent;               /* Event structure used by the Block Driver. */

#define CY_FX_FATFS_DMA_BUF_COUNT       (4)             /* Number of DMA buffers used. */
#define CY_FX_FATFS_DMA_BUF_SIZE        (512)           /* Size of DMA buffers used. */

#define CY_FX_FATFS_THREAD_STACK        (0x0800)  	/* Application thread stack size = 2 KB */
#define CY_FX_FATFS_THREAD_PRIORITY     (8)       	/* Application thread priority */

/* Storage driver event callback. */
extern void
CyFxFatfsAppSibCallback (
        uint8_t             portId,
        CyU3PSibEventType   evt,
        CyU3PReturnStatus_t status);

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYFXMSCDEMO_H_ */

/*[]*/
