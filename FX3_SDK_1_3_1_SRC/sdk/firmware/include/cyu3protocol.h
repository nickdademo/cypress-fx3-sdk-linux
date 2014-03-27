/*
 ## Cypress USB 3.0 Platform header file (cyu3protocol.h)
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

#ifndef _INCLUDED_CYU3PPROTOCOL_H_
#define _INCLUDED_CYU3PPROTOCOL_H_

#include <cyu3types.h>
#include <cyu3externcstart.h>

/**************************************************************************
 ******************************* Data types *******************************
 **************************************************************************/
/* 
 * Source module ID 
 * This is in context of message passing between thread. Max element number is 8
 */
typedef enum
{
    CY_U3P_INT_MODULE_ID = 0,
    CY_U3P_DMA_MODULE_ID,
    CY_U3P_SYS_MODULE_ID,
    CY_U3P_STOR_MODULE_ID,
    CY_U3P_USB_MODULE_ID,
    CY_U3P_PIB_MODULE_ID,
    CY_U3P_LPP_MODULE_ID,
    CY_U3P_RESERVE_MODULE_ID,
    CY_U3P_USBHOST_MODULE_ID
} CyU3PModuleId_t;

/**************************************************************************
 ********************************* Macros *********************************
 **************************************************************************/

/* Each message to the firmware modules must have the following 16 bit header */
#define CY_U3P_MSG_MBOX         (1 << 15) /* The bit identifies message source as mailbox         */
#define CY_U3P_MSG_RQT          (1 << 14) /* The bit identifies message as request                */
#define CY_U3P_MSG_TAG_POS      (11)      /* Three bits of tag identifies the message             */
#define CY_U3P_MSG_TAG_MASK     (0x3800)  
#define CY_U3P_MSG_SRC_ID_POS   (8)       /* Three bit field of source module ID                  */
#define CY_U3P_MSG_SRC_ID_MASK  (0x0700)
#define CY_U3P_MSG_CXT_ID_POS   (8)       /* Three bit field of API context                       */
#define CY_U3P_MSG_CXT_ID_MASK  (0x0700)
#define CY_U3P_MSG_TASK_ID_POS  (0)       /* Eight bit field identifying the requested task       */
#define CY_U3P_MSG_TASK_ID_MASK (0x00FF)

/* The first four bits of the Event group for each module shall have the following signature */
#define CY_U3P_EVENT_QUEUE      (1 << 0)
#define CY_U3P_EVENT_RES1       (1 << 1)
#define CY_U3P_EVENT_RES2       (1 << 2)
#define CY_U3P_EVENT_RES3       (1 << 3)

/* Get information from message identifier (16 bit)*/
#define CyU3PMsgIsMbox(mId)                 (((mId) & CY_U3P_MSG_MBOX) ? CyTrue : CyFalse)
#define CyU3PMsgIsRequest(mId)              (((mId) & CY_U3P_MSG_RQT) ? CyTrue : CyFalse)
#define CyU3PMsgIsResponse(mId)             (((mId) & CY_U3P_MSG_RQT) ? CyFalse : CyTrue)

#define CyU3PMsgGetCodeTask(mId)            ((uint8_t)(((mId) & CY_U3P_MSG_TASK_ID_MASK) >> CY_U3P_MSG_TASK_ID_POS))
#define CyU3PMsgGetContextSource(mId)       ((uint8_t)(((mId) & CY_U3P_MSG_SRC_ID_MASK) >> CY_U3P_MSG_SRC_ID_POS))
#define CyU3PMsgGetTag(mId)                 ((uint8_t)(((mId) & CY_U3P_MSG_TAG_MASK) >> CY_U3P_MSG_TAG_POS))

/* Setting mailbox identifier */
#define CyU3PMsgSetTask(mId, task)          \
    ((mId) = ((mId) & ~CY_U3P_MSG_TASK_ID_MASK) | (((task) << CY_U3P_MSG_TASK_ID_POS) & CY_U3P_MSG_TASK_ID_MASK))

#define CyU3PMsgSetSource(mId, source)    \
    ((mId) = ((mId) & ~CY_U3P_MSG_SRC_ID_MASK) | (((source) << CY_U3P_MSG_SRC_ID_POS) & CY_U3P_MSG_SRC_ID_MASK))

#define CyU3PMsgSetTag(mId, tag)            \
    ((mId) = ((mId) & ~CY_U3P_MSG_TAG_MASK) | (((tag) << CY_U3P_MSG_TAG_POS) & CY_U3P_MSG_TAG_MASK))

#define CyU3PMsgSetRequest(mId)              ((mId) |= CY_U3P_MSG_RQT)

#define CyU3PMsgSetResponse(mId)             ((mId) &= ~CY_U3P_MSG_RQT)

/* The following are the DMA module task / event messages */

/* Summary
 * DMA interrupt event.
 *
 * Description
 * The event is queued only by the DMA interrupt handler.
 *
 * Length (in transfers)
 * 4
 *
 * Request
 * WORD0:
 *      BIT 0-15 : General firmware message format
 *      BIT 16-31: ID of the interrupting socket.
 * WORD1:
 *      Channel handle for the DMA channel to which the event is intended.
 * WORD2:
 *      Socket interrupt status
 * WORD3:
 *      BIT 0-15 : Active descriptor for the interrupt.
 *
 * Response
 * No response for an event.
 *
 * See Also
 * None
 */
#define CY_U3P_TASK_DMA_INT     (0)

/* The following are the System module task / event messages */

/* Summary
 * Sleep Request
 *
 * Description
 * The request is to go to sleep state. i.e. low power state.
 *
 * Length (in transfers)
 * 1
 *
 * Request
 * WORD0:
 *      BIT 0-15 : General firmware message format
 *      BIT 16-31: 0:Wake up request
 *                 1:Goto Sleep request
 *
 * Response
 * CY_U3P_RESP_AWAKE_COMPLETE
 *
 * See Also
 * None
 */
#define CY_U3P_TASK_SLEEP_AWAKE     (0)

/* Summary
 * API Debug request
 *
 * Description
 * When the API debug is enabled, The mailbox are sent to the system thread irrespective 
 * of the context ID. In the mailbox the code is changed to this task ID so that the syatem 
 * thread cab distinguish these mailboc requests and can send faulty responses.
 *
 * Length (in transfers)
 * 1
 *
 * Request
 * WORD0:
 *      BIT 0-15 : General mailbox format
 *
 * Response
 * NONE
 *
 * See Also
 * None
 */
#define CY_U3P_TASK_API_DEBUG     (1)

/* Summary
 * Awake complete event
 *
 * Description
 * The response is sent from system in response of CY_U3P_TASK_SLEEP_AWAKE request.
 *
 * Length (in transfers)
 * 1
 *
 * Request
 * WORD0:
 *      BIT 0-15 : General firmware message format
 *      BIT 16-31: 0:Response for Wake up request
 *                 1:Response for Sleep request
 *
 * Response
 * None
 *
 * See Also
 * None
 */
#define CY_U3P_RESP_SLEEP_AWAKE_COMPLETE   (0)

/* Summary
 * GPIF Event
 *
 * Description
 * The request is sent by the ISR to the driver when there is a GPIF event to be
 * sent to the application.
 *
 * Length (in transfers)
 * 1
 *
 * Request
 * WORD0:
 *      Bits  0 - 15 : General firmware message format
 *      Bits 16 - 23 : Identifies specific GPIF event.
 *      Bits 24 - 31 : Identifies GPIF state that triggered the event.
 *
 * Response
 * No response for an ISR triggered event.
 */
#define CY_U3P_TASK_GPIF_EVENT                  (4)

/* Following are the events and responses defined for LPP module */

/* Summary
 * I2C Event
 *
 * Description
 * The request is sent by the ISR to the driver when there is a I2C event to be
 * sent to the application.
 *
 * Length (in transfers)
 * 1
 *
 * Request
 * WORD0:
 *      Bits  0 - 15 : General firmware message format
 *      Bits 16 - 23 : Identifies specific I2C event.
 *      Bits 24 - 31 : Error given in status register.
 *
 * Response
 * No response for an ISR triggered event.
 */
#define CY_U3P_TASK_I2C_EVENT                  (0)

/**************************************************************************
 ******************************* FUNCTION *********************************
 **************************************************************************/

/* 
 * Summary
 * Module initialization event function.
 * 
 * Descrption
 * Each function does their initialization in the beginning of their thread entry function.
 * Then they should call this function to let system thread to know their completion of 
 * initialization process.
 *
 * Parameters
 * module   : This is an enum defined in protocol.h. This is to specify which module has 
 *            called this function
 * 
 * Returns
 * None
 *
 * See Also
 * None
 */
extern void
CyU3PSysModuleInitCompleteEvt (
       CyU3PModuleId_t module);

/* Summary
 * This is the message queuing function for System thread.
 *
 * Description
 * This function is called by other ISRs and Threads to queue messages.
 *
 * Parameters
 * msg:         Message pointer    
 * waitOption:  Wait time in ticks. For ISRs this should be zero.
 * priority:    Priority decides if to send the message to the front of the queue or not.  
 *
 * Return value
 * CY_U3P_SUCCESS
 * OS_ERROR
 * 
 * See Also
 * None
 *
 */
extern CyU3PReturnStatus_t
CyU3PSysMsgSend (
        uint32_t *msg,
        uint32_t waitOption,
        CyBool_t priority);

/* Summary
 * Enqueues messages to the storage thread.
 *
 * Description
 * This function queues up the messages to the storage thread. Invoked by
 * the other firmware modules.
 *
 * Parameters
 * msg          : Message to be passed to the Storage Thread
 * waitOption   : CYU3P_NO_WAIT, CYU3P_WAIT_FOREVER or timeout value
 * priority     : Indicates a high priority message.
 *
 * Return value
 * CY_U3P_SUCCESS or Queue related error codes
 * */
extern CyU3PReturnStatus_t
CyU3PSibMsgSend (
        uint32_t *msg,
        uint32_t waitOption,
        CyBool_t priority);

/* Summary
 * This is the message queuing function for Pport thread.
 *
 * Description
 * This function is called by other ISRs and Threads to queue messages.
 *
 * Parameters
 * msg:         Message pointer    
 * waitOption:  Wait time in ticks. For ISRs this should be zero.
 * priority:    Priority decides if to send the message to the front of the queue or not.  
 *
 * Return value
 * CY_U3P_SUCCESS
 * OS_ERROR
 * 
 * See Also
 * None
 *
 */
extern CyU3PReturnStatus_t
CyU3PPibMsgSend (
        uint32_t *msg,
        uint32_t waitOption,
        CyBool_t priority) __attribute__ ((section ("CYU3P_ITCM_SECTION")));

/* Summary
 * This is the message queuing function for LPP thread.
 *
 * Description
 * This function is called by other ISRs and Threads to queue messages.
 *
 * Parameters
 * msg:         Message pointer    
 * waitOption:  Wait time in ticks. For ISRs this should be zero.
 * priority:    Priority decides if to send the message to the front of the queue or not.  
 *
 * Return value
 * CY_U3P_SUCCESS
 * OS_ERROR
 * 
 * See Also
 * None
 *
 */
extern CyU3PReturnStatus_t
CyU3PLppMsgSend (
        uint32_t *msg,
        uint32_t waitOption,
        CyBool_t priority);

/* Summary
 * This is the message queuing function for Uport thread.
 *
 * Description
 * This function is called by other ISRs and Threads to queue messages.
 *
 * Parameters
 * msg:         Message pointer    
 * waitOption:  Wait time in ticks. For ISRs this should be zero.
 * priority:    Priority decides if to send the message to the front of the queue or not.  
 *
 * Return value
 * CY_U3P_SUCCESS
 * OS_ERROR
 * 
 * See Also
 * None
 *
 */
extern CyU3PReturnStatus_t
CyU3PUibMsgSend (
        uint32_t *msg,
        uint32_t waitOption,
        CyBool_t priority);

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYU3PPROTOCOL_H_ */

/*[]*/

