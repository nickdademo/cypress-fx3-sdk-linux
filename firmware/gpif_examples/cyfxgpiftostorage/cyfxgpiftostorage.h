/*
 ## Cypress FX3S Example Header file (cyfxgpiftostorage.h)
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

/* This file contains the constants and definitions for the GPIF to Storage example. */

#ifndef _INCLUDED_CYFXGPIFTOSTORAGE_H_
#define _INCLUDED_CYFXGPIFTOSTORAGE_H_

#include <cyu3types.h>
#include <cyu3usbconst.h>
#include <cyu3externcstart.h>

/* List of requests that are sent to FX3S through the mailbox registers.
   Most of these requests take parameters that are also encoded into the
   mailbox data.
 */
typedef enum CyFxStorAppRqtCode
{
    CYFXSTORRQT_INIT = 0,               /* Request to initialize storage. */
    CYFXSTORRQT_DEINIT,                 /* Request to de-init storage. */
    CYFXSTORRQT_PORTCFG,                /* Set storage port parameters. */
    CYFXSTORRQT_QUERYDEV,               /* Query device properties. */
    CYFXSTORRQT_QUERYUNIT,              /* Query storage partition properties. */
    CYFXSTORRQT_READ,                   /* Request to read data. */
    CYFXSTORRQT_WRITE,                  /* Request to write data. */
    CYFXSTORRQT_ECHO                    /* Echo request used to test mailbox communication. */
} CyFxStorAppRqtCode;

/* List of responses that are sent by the FX3S firmware through the mailbox registers.
   Most of these responses include parameters or data that are also encoded into
   the mailbox message.
 */
typedef enum CyFxStorAppRespCode
{
    CYFXSTORRESP_STATUS = 0,            /* Return status response sent for INIT, DEINIT, PORTCFG, READ and
                                           WRITE requests. */
    CYFXSTORRESP_READY,                 /* Event sent from the FX3S to notify the processor that it is
                                           ready to accept requests. */
    CYFXSTORRESP_DEVDATA,               /* Return data for a QUERYDEV request. */
    CYFXSTORRESP_UNITDATA,              /* Return data for a QUERYUNIT request. */
    CYFXSTORRESP_INSERT_EVT,            /* SD/MMC device inserted event. */
    CYFXSTORRESP_REMOVE_EVT,            /* SD/MMC device removal event. */
    CYFXSTORRESP_ECHO                   /* Echo command response. */
} CyFxStorAppRespCode;

#define CYFXSTORAPP_GPIF_WRSOCK         (0)     /* GPIF socket used for writes to storage device. */
#define CYFXSTORAPP_GPIF_RDSOCK         (1)     /* GPIF socket used for reads from storage device. */
#define CYFXSTORAPP_SIB_WRSOCK          (0)     /* SIB socket used for writes to storage device. */
#define CYFXSTORAPP_SIB_RDSOCK          (1)     /* SIB socket used for reads from storage device. */

#define CYFXSTORAPP_DMA_BUFSIZE         (512)   /* Size of DMA buffers used for GPIF to Storage transfer. */
#define CYFXSTORAPP_DMA_BUFCOUNT        (16)    /* Number of DMA buffers used for transfers in either direction. */

#define CYFXSTORAPP_XFER_TIMEOUT        (5000)  /* Timeout duration for data transfer requests. */

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYFXGPIFTOSTORAGE_H_ */

/*[]*/
