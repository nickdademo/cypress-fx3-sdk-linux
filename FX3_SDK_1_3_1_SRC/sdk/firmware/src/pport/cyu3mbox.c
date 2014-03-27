/*
 ## Cypress USB 3.0 Platform Source file (cyu3mbox.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2013,
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

#include <cyu3mbox.h>
#include <cyu3regs.h>
#include <cyu3pib.h>
#include <cywbpib.h>
#include <cyu3os.h>
#include <cyu3protocol.h>
#include <cyu3error.h>
#include <cyu3utils.h>

/* Global variables. Restricted to file scope. */
static CyU3PMutex  glMBoxLock;          /* Mailbox Lock for writing into RD_MAILBOX register */
CyU3PMboxCb_t glMboxCb = NULL;          /* Callback to be called after mailbox intr has happened */

/*
 * Initiate the mailbox related structures
 */
void
CyU3PMboxInit (CyU3PMboxCb_t callback)
{
    uint32_t status;

    /* Create the lock for RD_MAILBOX */
    status = CyU3PMutexCreate (&glMBoxLock, CYU3P_NO_INHERIT);
    CyU3PBusyWait(10);
    if (status != CY_U3P_SUCCESS)
        return;

    glMboxCb = callback;
    /* Unmask the rd/wr interrupt */
    PIB->intr_mask |= (CY_U3P_PIB_INTR_WR_MB_FULL | CY_U3P_PIB_INTR_RD_MB_EMPTY);
}

/*
 * De-inits the mailbox structure
 */
void CyU3PMboxDeInit (void)
{
    /* Destroy the lock for RD_MAILBOX */
    CyU3PMutexDestroy (&glMBoxLock);
    glMboxCb = NULL;
    /*Unmask the rd/wr interrupt */
    PIB->intr_mask &= ~(CY_U3P_PIB_INTR_WR_MB_FULL | CY_U3P_PIB_INTR_RD_MB_EMPTY);
}

/* 
 * Reset Mailbox structures 
 */
void CyU3PMboxReset (void)
{
    /* Clear the interrupt */
    PIB->intr = CY_U3P_PIB_INTR_WR_MB_FULL;

    /* Clear the event flag which notifies Mailbox RD empty */
    CyU3PEventSet (&glPibEvt, CY_U3P_PIB_RD_EMPTY_EVT, CYU3P_EVENT_AND);

    /* Unmask the rd/wr interrupt */
    PIB->intr_mask |= (CY_U3P_PIB_INTR_WR_MB_FULL | CY_U3P_PIB_INTR_RD_MB_EMPTY);
}

CyU3PReturnStatus_t
CyU3PMboxWait (
	void)
{
    CyU3PReturnStatus_t status;
    uint32_t flag;

    status = CyU3PMutexGet (&glMBoxLock, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return CY_U3P_ERROR_FAILURE;
    }   

    /* Wait till the mailbox becomes empty */
    status = CyU3PEventGet (&glPibEvt, CY_U3P_PIB_RD_EMPTY_EVT, CYU3P_EVENT_AND, &flag, CYU3P_WAIT_FOREVER);
    CyU3PMutexPut (&glMBoxLock);

    return status;
}

/*
 * This function sends a mailbox message to the AP. The caller should break-up the message into
 * 8 byte values, and send each separately. This function returns error getting lock fails.
 */
CyU3PReturnStatus_t
CyU3PMboxWrite (CyU3PMbox *mbox)
{
    uint32_t status, flag;

    /* Error checking */
    if (mbox == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    /* Lock */
    status = CyU3PMutexGet (&glMBoxLock, CYU3P_WAIT_FOREVER);
    if (status != CY_U3P_SUCCESS)
    {
        return CY_U3P_ERROR_FAILURE;
    }   
    /* Wait till the mailbox becomes empty */
    status = CyU3PEventGet (&glPibEvt, CY_U3P_PIB_RD_EMPTY_EVT, CYU3P_EVENT_AND_CLEAR, &flag, CYU3P_WAIT_FOREVER);
    
    if (status == CY_U3P_SUCCESS)
    {
        PIB->rd_mailbox0 = mbox->w0;
        PIB->rd_mailbox1 = mbox->w1;
    }

    CyU3PMutexPut (&glMBoxLock);
    return status;
}

/*
 * This function reads mailbox message from AP. The function should be called when it is sured that the
 * data is readily available.
 */
CyU3PReturnStatus_t
CyU3PMboxRead (CyU3PMbox *mbox)
{
    /* Error checking */
    if (mbox == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    mbox->w0 = PIB->wr_mailbox0;
    mbox->w1 = PIB->wr_mailbox1;

    return CY_U3P_SUCCESS;
}

/*[]*/

