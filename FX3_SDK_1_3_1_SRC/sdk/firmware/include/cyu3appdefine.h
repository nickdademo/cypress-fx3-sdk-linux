/*
 ## Cypress USB 3.0 Platform header file (cyu3appdefine.h)
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

#ifndef _INCLUDED_CYU3P_APPDEFINE_H_
#define _INCLUDED_CYU3P_APPDEFINE_H_

#include <cyu3externcstart.h>

/*
   Summary
   This file contains application definition of all modules. These are called 
   from CyU3PApplicationDefine function.
 */

/**************************************************************************
 *************************** Function prototypes **************************
 **************************************************************************/

/* Summary
   DMA module definition.
  
   Description
   This function defines the DMA module OS primitives.
   It is an internal function and should not be invoked.
  
   Return value
   None
  
   See Also
   CyU3PApplicationDefine
 */
extern void
CyU3PDmaApplicationDefine (
        void);

/* Summary
   Debug module definition.
  
   Description
   This function defines the debug module OS primitives.
   It is an internal function and should not be invoked.
  
   Return value
   None
  
   See Also
   CyU3PApplicationDefine
 */
extern void
CyU3PDebugApplicationDefine (
        void);

/* Summary
   SIB Application define function
  
   Description
   This function contains the instructions required to be called from CyU3PApplicationDefine 
   function for the SIB module to be working. e.g memory allocation for SIB thread etc.
  
   Return value
   None
  
   See Also
   CyU3PApplicationDefine
 */
extern void
CyU3PSibApplicationDefine (
        void);

/* Summary
   PIB Application definition
  
   Description
   This function contains the instructions required to be called from CyU3PApplicationDefine 
   function for the PIB module to be working. e.g memory allocation for PIB thread etc.  
  
   Return value
   None
  
   See Also
   CyU3PApplicationDefine
 */
extern void
CyU3PPibApplicationDefine (
        void);

/* Summary
   LPP Application definition
  
   Description
   This function contains the instructions required to be called from CyU3PApplicationDefine 
   function for the LPP module to be working. e.g memory allocation for LPP thread etc. 
  
   Return value
   None
  
   See Also
   CyU3PApplicationDefine
 */
extern void
CyU3PLppApplicationDefine (
        void);

/* Summary
   UIB Application definition
  
   Description
   This function contains the instructions required to be called from CyU3PApplicationDefine 
   function for the UIB module to be working. e.g memory allocation for UIB thread etc.
  
   Return value
   None
  
   See Also
   CyU3PApplicationDefine
 */
extern void 
CyU3PUibApplicationDefine(
        void);

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYU3P_APPDEFINE_H_ */

/*[]*/
