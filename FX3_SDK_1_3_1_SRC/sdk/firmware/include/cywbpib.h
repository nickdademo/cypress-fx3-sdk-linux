/*
 ## Cypress USB 3.0 Platform Header file (cywbpib.h)
 ## =====================================
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
 ## =====================================
*/

#ifndef _INCLUDED_CYWB_PIB_H_
#define _INCLUDED_CYWB_PIB_H_

#include <cyu3os.h>
#include <cyu3types.h>
#include <cyu3system.h>
#include <cyu3pib.h>
#include <cyu3externcstart.h>

/**************************************************************************
 ********************************Data Types********************************
 **************************************************************************/

/* PIB event flag position */
#define CY_U3P_PIB_RD_EMPTY_EVT         (1 << 4)        /* RD_MAILBOX empty event position in glPibEvt */
#define CY_U3P_PIB_PP_CFG_EVT           (1 << 5)        /* PP_CONFIG register write event. */
#define CY_U3P_PIB_ERROR_EVT            (1 << 6)        /* PIB_ERROR event. */
#define CY_U3P_PIB_DLLLOCK_EVT          (1 << 7)        /* DLL lock/unlock event. */

/* GPIF ingress-data / egress-data event positions. */
#define CY_U3P_PIB_THR0_EGDATA_EVT_POS  (8)
#define CY_U3P_PIB_THR1_EGDATA_EVT_POS  (9)
#define CY_U3P_PIB_THR2_EGDATA_EVT_POS  (10)
#define CY_U3P_PIB_THR3_EGDATA_EVT_POS  (11)
#define CY_U3P_PIB_THR0_INDATA_EVT_POS  (12)
#define CY_U3P_PIB_THR1_INDATA_EVT_POS  (13)
#define CY_U3P_PIB_THR2_INDATA_EVT_POS  (14)
#define CY_U3P_PIB_THR3_INDATA_EVT_POS  (15)

/* Direct read and write socket */
#define CY_U3P_PMMC_DIRECT_RD_SOCKET    (15)            /* Read direct socket */
#define CY_U3P_PMMC_DIRECT_WR_SOCKET    (31)            /* Write direct socket */

/**************************************************************************
 *************************** Function prototypes **************************
 **************************************************************************/

/* Summary
 * Handles PIB interrupts
 *
 * Description
 * This function handles PIB interrupts. If the task is small then then it is handles 
 * there itself otherwise the task is queued to the PIB thread.
 */
extern void
CyU3PPibIntHandler (
        void);

/* Summary
 * Handles PMMC interrupts
 *
 * Description
 * This function handles Pmmc interrupts.
 */
extern void
CyU3PPmmcIntHandler (
         void);

/* Summary
 * Handles GPIF interrupts.
 *
 * Description
 * This function handles interrupts from the GPIF block.
 */
extern void
CyU3PGpifIntHandler (
        void);

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYWB_PIB_H_ */

/*[]*/

