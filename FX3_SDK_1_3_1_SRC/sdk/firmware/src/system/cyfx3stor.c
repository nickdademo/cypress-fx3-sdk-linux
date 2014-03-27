/*
 ## Cypress USB 3.0 Platform source file (cyfx3stor.c)
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

#include <cyu3regs.h>
#include <cyu3sib.h>
#include <cyu3sibpp.h>
#include <cyu3cardmgr.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3utils.h>
#include <cyu3error.h>
#include <cyu3protocol.h>
#include <cyu3vic.h>
#include <cyu3gpio.h>

typedef void (*CyU3PStorDriverFunction) (void);

CyU3PThread             glSibThread;                            /* SIB Thread Handle */
CyU3PEvent              glSibEvent;                             /* SIB Event Handle */
CyU3PSibIntfParams_t    glSibIntfParams[CY_U3P_SIB_NUM_PORTS];  /* Interface Control parameters */
CyU3PStorDriverFunction glSibDrvEntry = NULL;                   /* Storage driver entry function. */


/* Initialize the SIB Interface parameters to their default values. */
void
CyU3PSibInitIntfParams (
        uint8_t portId)
{
    CyU3PSibIntfParams_t *pIntfParams = &glSibIntfParams[portId];

    pIntfParams->resetGpio       = 0xFF;
    pIntfParams->cardDetType     = CY_U3P_SIB_DETECT_DAT_3;
    pIntfParams->writeProtEnable = CyTrue;
    pIntfParams->useDdr          = CyFalse;
    pIntfParams->lowVoltage      = CyFalse;
    pIntfParams->voltageSwGpio   = 0xFF;
    pIntfParams->lvGpioState     = CyFalse;
    pIntfParams->maxFreq         = CY_U3P_SIB_FREQ_104MHZ;
    pIntfParams->cardInitDelay   = 0;
}

void
CyU3PStorRegisterDriverEntry (
        CyU3PStorDriverFunction entry_p)
{
    glSibDrvEntry = entry_p;
}

/*
   Storage driver thread entry. All of the functional code here will be handled in a function
   registered by the storage library.
 */
void
CyU3PSibThreadEntry (
        uint32_t threadInput )
{
    /* Initialize the sib context structure and clear the interrupt register. */
    CyU3PSibInitIntfParams (0);
    CyU3PSibInitIntfParams (1);

    /* Send a sib init complete event to the sys thread. */
    CyU3PSysModuleInitCompleteEvt (CY_U3P_STOR_MODULE_ID);

    while (1)
    {
        /* Once the storage module has been started up, jump to the actual driver entry. */
        if (glSibDrvEntry != NULL)
            glSibDrvEntry ();
        else
            CyU3PThreadSleep (10);
    }
}

/*
   This function contains the instructions required to be called from CyU3PApplicationDefine
   function for the SIB module to be working. e.g memory allocation for SIB thread etc. This
   also calls SibInit function to initialize the SIB Module.
   */
void
CyU3PSibApplicationDefine (
        void)
{
    uint8_t *ptr = NULL;

    CyU3PEventCreate (&glSibEvent);
    glSibDrvEntry = NULL;

    /* SIB Queue Creation successful. */
    ptr = CyU3PMemAlloc (CY_U3P_SIB_STACK_SIZE);
    if (ptr != NULL)
    {
        CyU3PThreadCreate (&glSibThread, "06_SIB_THREAD", CyU3PSibThreadEntry, 0, ptr,
                CY_U3P_SIB_STACK_SIZE, CY_U3P_SIB_THREAD_PRIORITY, CY_U3P_SIB_THREAD_PRIORITY,
                CYU3P_NO_TIME_SLICE, CYU3P_AUTO_START);
    }
}

/*[]*/

