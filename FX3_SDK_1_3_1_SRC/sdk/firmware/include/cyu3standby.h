/*
 ## Cypress USB 3.0 Platform header file (cyu3standby.h)
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

#ifndef _INCLUDED_CYU3P_STANDBY_H_
#define _INCLUDED_CYU3P_STANDBY_H_

#include <cyu3types.h>
#include <cyu3dma.h>
#include <cyu3externcstart.h>

/**************************************************************************
 ******************************* Macros ***********************************
 **************************************************************************/

/**************************************************************************
 ******************************* Data Types *******************************
 **************************************************************************/

/**************************************************************************
 *************************** Function prototypes **************************
 **************************************************************************/

/* Summary
 * Internal function invoked as part of firmware initialization.
 * The function is not expected to be explicitly invoked.
 *
 * Description
 * The function is invoked from the CyU3PFirmwareEntry to check
 * the source of boot. If this was a wake up from Standby then
 * it is expected to invoke CyU3PDeviceLeaveStandby. Otherwise
 * if this is a valid boot entry then should just return.
 *
 * Parameters
 * None
 *
 * Return value
 * None
 * 
 * See Also
 * CyU3PSysEnterStandby
 * CyU3PDeviceLeaveStandby
 */
void
CyU3PSysCheckBootState (
        void);

/* Summary
 * Internal function to put the device on standby mode. 
 * Not to be called outside of the library.
 *
 * Description
 * The function can be called only after all required registers
 * are backed up. The function simply backs up the stack pointers
 * and puts the device on standby. This is a non-returning call.
 *
 * Parameters
 * None
 *
 * Return value
 * None
 * 
 * See Also
 * CyU3PSysCheckBootState
 * CyU3PDeviceLeaveStandby
 */
void
CyU3PDeviceEnterStandby (
        void);

/* Summary
 * Internal function to restore the device from standby mode. 
 * Not to be called outside of the library.
 *
 * Description
 * The function should be called only from the library
 * and will restore the previously saved stack pointer and
 * return to the CyU3PSysEnterStandby function so that
 * the registers can be restored. This is a non-returning call.
 *
 * Parameters
 * None
 *
 * Return value
 * None
 * 
 * See Also
 * CyU3PSysCheckBootState
 * CyU3PDeviceEnterStandby
 */
void
CyU3PDeviceLeaveStandby (
        void);

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYU3P_STANDBY_H_ */

/*[]*/

