/*
 ## Cypress USB 3.0 Platform Source file (cyu3pib.c)
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
#include <cyu3pib.h>
#include <cywbpib.h>
#include <cyu3gpif.h>
#include <cyu3error.h>
#include <cyu3protocol.h>
#include <cyu3vic.h>
#include <cyu3mmu.h>
#include <cyu3utils.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3mbox.h>

#include <cyfx3_api.h>

/* 
 * Summary: 
 * This file implements PIB functionalities. PIB related Interrupt handling, Register initialization
 * Mailbox handling etc are done here. This contains code for PMMC and GPIF. 
 */
#define CY_U3P_PIB_QUEUE_SIZE           (64)        /* Queue Size in bytes */
#define CY_U3P_PIB_MSG_SIZE             (4)         /* Message size in byte */
#define CY_U3P_PIB_STACK_SIZE           (0x400)     /* 1K stack for PIB Thread */
#define CY_U3P_PIB_THREAD_PRIORITY      (4)         /* PIB thread priority = 4 */

#define CY_U3P_PMMC_PSN                 (0x01)      /* 32-bit product serial # */ 
#define CY_U3P_PMMC_PRV                 (0x01)      /* Product version # */ 
#define CY_U3P_PMMC_PNM_0               (0)         /* Lowest 8-bits of PNM */ 
#define CY_U3P_PMMC_PNM_1               (1)         /* 32 middle bits of PNM */ 
#define CY_U3P_PMMC_PNM_2               (2)         /* Upper 8 bits of PNM */ 
#define CY_U3P_PMMC_OID                 (3)         /* OEM ID. Must be changed */ 
#define CY_U3P_PMMC_CBX                 (0x01)      /* Device type: 1 = BGA */ 
#define CY_U3P_PMMC_MANUFACTURE_DATE    (0x92)      /* Manufacturing date and month */
#define CY_U3P_PMMC_MID                 (0x04)      /* Manufacturer ID. Must be changed */

/* CSD register */
#define CY_U3P_PMMC_CSD_LEN             (4)
const uint32_t glPmmcCsdValue[CY_U3P_PMMC_CSD_LEN] =
{
    0x06E00C01,
    0xC0038000,           
    0x415BF3FF,
    0x1001005A
};
/* Extended CSD register */
#define CY_U3P_PMMC_EXT_CSD_LEN         (6)
const uint32_t glPmmcExtCsdValue[CY_U3P_PMMC_EXT_CSD_LEN] = 
{
    0x40040000,
    0x03020200,
    0x00000404,
    0x32321414,
    0x00406464,
    0x00000000
};

CyBool_t            glPibActive = CyFalse;      /* Whether the PIB has been initialized. */
CyBool_t            glPibMmcMode = CyFalse;     /* Whether the PIB is configured in MMC slave mode. */
CyU3PPmmcState      glCurrentState;             /* Current State */
CyU3PPmmcIntrCb_t   glPmmcIntrCb = 0;           /* PMMC interrupt callback. */

CyU3PEvent          glPibEvt = {0};             /* Event flag for the Pib Module */
uint32_t            glPibMask = 0;              /* Mask for the event flag */
CyU3PThread         glPibThread;                /* Pib thread */
CyU3PQueue          glPibQueue;                 /* Pib Queue */
CyU3PPibIntrCb_t    glPibIntrCb = 0;            /* PIB interrupt callback. */
uint32_t            glPibIntrMask = 0;          /* Mask enabling PIB interrupt callbacks. */
CyU3PGpifEventCb_t  glGpifEvtCb = 0;            /* GPIF event callback. */
CyU3PGpifSMIntrCb_t glGpifSMCb = 0;             /* GPIF state machine callback. */

static void
CyU3PPibSocketInit (
        void)
{
    uint8_t i;

    for (i = 0; i < CY_U3P_DMA_PIB_NUM_SCK; i++)
    {
        PIB->sck[i].status = CY_U3P_PIB_SCK_STATUS_DEFAULT;
        PIB->sck[i].intr = ~CY_U3P_PIB_SCK_INTR_DEFAULT;
        PIB->sck[i].intr_mask = CY_U3P_PIB_SCK_INTR_MASK_DEFAULT;
    }
}

/* 
 * Pib Thread: This processes different requests from other fw modules .
 */
void
CyU3PPibThreadEntry (
        uint32_t threadInput)
{
    uint32_t flag, status1;
    uint16_t msg[CY_U3P_PIB_MSG_SIZE/2];

    /* Send Sys Module Init Event */
    CyU3PSysModuleInitCompleteEvt (CY_U3P_PIB_MODULE_ID);

    /* Configure PIB */
    PIB->config = (PIB->config & CY_U3P_PIB_DEVICE_ID_MASK) | CY_U3P_PIB_ENABLE | CY_U3P_PIB_MMIO_ENABLE |
            CY_U3P_PIB_PMMC_RESETN;

    glPibMask = (CY_U3P_EVENT_QUEUE | CY_U3P_PIB_PP_CFG_EVT | CY_U3P_PIB_ERROR_EVT | CY_U3P_PIB_DLLLOCK_EVT);

    /* Run the following code repeatedly */
    while (1)
    {
        /* Wait for some event to happen */
        status1 = CyU3PEventGet (&glPibEvt, glPibMask, CYU3P_EVENT_OR_CLEAR, &flag, CYU3P_WAIT_FOREVER);
        flag &= glPibMask;
        if (status1 != CY_U3P_SUCCESS)
        {
            continue;
        }

        /* Some event has happened. The event flag is copied to the flag local variable. 
         * Process each event and toggle the bit for the corresponding bit. Do this till 
         * all bits are set to zero */
        while (flag)
        {
            if (flag & CY_U3P_PIB_PP_CFG_EVT)
            {
                /* If there is a registered PP_CONFIG callback, call it. Then clear the CFGMODE bit. */
                if ((glPibIntrCb) && ((glPibIntrMask & CYU3P_PIB_INTR_PPCONFIG) != 0))
                    glPibIntrCb (CYU3P_PIB_INTR_PPCONFIG, (uint16_t)PIB->pp_config);

                PIB->config &= ~CY_U3P_PIB_PP_CFGMODE;
                flag &= ~CY_U3P_PIB_PP_CFG_EVT;
            }

            if (flag & CY_U3P_PIB_ERROR_EVT)
            {
                /* If there is a registered PIB interrupt callback and the PIB_ERROR interrupt is enabled,
                   call the function. */
                if ((glPibIntrCb) && ((glPibIntrMask & CYU3P_PIB_INTR_ERROR) != 0))
                {
                    /* Check for errors that need to be reported and clean up the error codes. */
                    uint16_t arg = (uint16_t)PIB->error;

                    /* If there is a GPIF state error, clear all other errors. */
                    if (CYU3P_GET_GPIF_ERROR_TYPE (arg) == CYU3P_GPIF_ERR_INVALID_STATE)
                        arg = CYU3P_GPIF_ERR_INVALID_STATE;

                    /* Ignore Direction errors for non PP-mode cases. */
                    if (((PIB->gpif_config & CY_U3P_GPIF_CONF_PP_MODE) != 0) || (arg > CYU3P_PIB_ERR_THR3_DIRECTION))
                        glPibIntrCb (CYU3P_PIB_INTR_ERROR, arg);
                }

                /* If ERROR interrupt callbacks are still enabled, ensure that the interrupt is turned on. */
                if (glPibIntrMask & CYU3P_PIB_INTR_ERROR)
                    PIB->intr_mask |= (CY_U3P_PIB_INTR_PIB_ERR | CY_U3P_PIB_INTR_GPIF_ERR | CY_U3P_PIB_INTR_MMC_ERR);
                flag &= ~CY_U3P_PIB_ERROR_EVT;
            }

            if (flag & CY_U3P_PIB_DLLLOCK_EVT)
            {
                if ((glPibIntrCb) && (glPibIntrMask & CYU3P_PIB_INTR_DLL_UPDATE) != 0)
                    glPibIntrCb (CYU3P_PIB_INTR_DLL_UPDATE, CyFx3PibGetDllStatus ());
                flag &= ~CY_U3P_PIB_DLLLOCK_EVT;
            }

            if (flag & CY_U3P_EVENT_QUEUE)
            {
                /* Check whether Mbox or some other message is been received. */
                status1 = CyU3PQueueReceive (&glPibQueue, msg, CYU3P_NO_WAIT);
                if (status1 == CY_U3P_SUCCESS)
                {
                    if (CyU3PMsgIsRequest(msg[0]))
                    {
                        /* Is a request */
                        switch (CyU3PMsgGetCodeTask (msg[0]))
                        {
                        case CY_U3P_TASK_GPIF_EVENT:
                            /* Send the GPIF event to the application. */
                            if (glGpifEvtCb)
                            {
                                glGpifEvtCb ((CyU3PGpifEventType)CYU3P_LSB (msg[1]), CYU3P_MSB (msg[1]));
                            }
                            break;

                        default:
                            break;
                        }
                    }
                }
                else
                {
                    flag &= ~CY_U3P_EVENT_QUEUE;
                }
            }
        }
    }
}

void
CyU3PPibRegisterCallback (
        CyU3PPibIntrCb_t cbFunc,
        uint32_t         intMask)
{
    glPibIntrCb   = cbFunc;
    glPibIntrMask = intMask;

    /* Make sure that any existing interrupts are cleared up. */
    PIB->intr = 0xFFFFFFFF;

    /* Enable PIB and GPIF error interrupts if a corresponding callback is requested. */
    if (intMask & CYU3P_PIB_INTR_ERROR)
        PIB->intr_mask |= (CY_U3P_PIB_INTR_PIB_ERR | CY_U3P_PIB_INTR_GPIF_ERR | CY_U3P_PIB_INTR_MMC_ERR);
    else
        PIB->intr_mask &= ~(CY_U3P_PIB_INTR_PIB_ERR | CY_U3P_PIB_INTR_GPIF_ERR | CY_U3P_PIB_INTR_MMC_ERR);

    /* Enable DLL interrupts if a corresponding callback is requested. */
    if (intMask & CYU3P_PIB_INTR_DLL_UPDATE)
        PIB->intr_mask |= (CY_U3P_PIB_INTR_DLL_LOCKED | CY_U3P_PIB_INTR_DLL_LOST_LOCK);
    else
        PIB->intr_mask &= ~(CY_U3P_PIB_INTR_DLL_LOCKED | CY_U3P_PIB_INTR_DLL_LOST_LOCK);
}

/*
 * This function contains the instructions required to be called from CyU3PApplicationDefine 
 * function for the PIB module to be working. e.g memory allocation for Pib thread etc.
 */
void
CyU3PPibApplicationDefine (
        void)
{
    uint8_t *pointer;

    /* Global variable initialization. */
    glPibActive  = CyFalse;
    glPibMmcMode = CyFalse;

    glPibMask     = 0;
    glMboxCb      = 0;
    glGpifEvtCb   = 0;
    glGpifSMCb    = 0;
    glPibIntrCb   = 0;
    glPibIntrMask = 0;
    glPmmcIntrCb  = 0;

    /* Define the pport thread */
    pointer = CyU3PMemAlloc (CY_U3P_PIB_STACK_SIZE);
    CyU3PThreadCreate (&glPibThread, "03_PIB_THREAD", CyU3PPibThreadEntry, 0, pointer, 
        CY_U3P_PIB_STACK_SIZE, CY_U3P_PIB_THREAD_PRIORITY, CY_U3P_PIB_THREAD_PRIORITY, 
        CYU3P_NO_TIME_SLICE, CYU3P_AUTO_START);

    /* The RD_MBOX can be considered empty at start-up. */
    CyU3PEventCreate (&glPibEvt);
    CyU3PEventSet (&glPibEvt, CY_U3P_PIB_RD_EMPTY_EVT, CYU3P_EVENT_OR);

    pointer = CyU3PMemAlloc (CY_U3P_PIB_QUEUE_SIZE);
    CyU3PQueueCreate (&glPibQueue, CY_U3P_PIB_MSG_SIZE/4, pointer, CY_U3P_PIB_QUEUE_SIZE);
}

CyU3PReturnStatus_t
CyU3PGpifInit (
        CyBool_t doInit)
{
    if (doInit)
    {
        /* Enable GPIF and stay in Config mode. */
        PIB->config = (PIB->config & CY_U3P_PIB_DEVICE_ID_MASK) | CY_U3P_PIB_PMMC_RESETN | CY_U3P_PIB_MMIO_ENABLE |
           CY_U3P_PIB_ENABLE | CY_U3P_PIB_PP_CFGMODE;

#ifndef CYU3P_DEBUG
        /* Disable PP access to MMIO registers in release builds. */
        PIB->config &= ~CY_U3P_PIB_MMIO_ENABLE;
#endif /* CYU3P_DEBUG */
    }

    /* Clear all GPIF IRQs and disable all GPIF interrupts (for now). */ 
    PIB->gpif_intr      = 0xFFFFFFFF;
    PIB->gpif_intr_mask = CY_U3P_PIB_GPIF_INTR_MASK_DEFAULT;

    PIB->intr_mask = (CY_U3P_PIB_INTR_GPIF_INTERRUPT | CY_U3P_PIB_INTR_CONFIG_CHANGE);
    CyU3PSysBarrierSync ();

    PIB->config = (PIB->config & CY_U3P_PIB_DEVICE_ID_MASK) | CY_U3P_PIB_ENABLE | CY_U3P_PIB_MMIO_ENABLE |
        CY_U3P_PIB_PMMC_RESETN;
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PPmmcInit (
        CyBool_t doInit)
{
    if (doInit)
    {
        /* Regular PMMC initialization for API based usage. */

        /* Clear all PMMC IRQs */ 
        PMMC->intr          = 0xFFFFFFFF;
        PMMC->intr_mask     = 0;
        PMMC->sck_direction = 0xFFFF0000;
        PIB->intr_mask      = 0;

        /* Force the state */   
        PMMC->config &= ~CY_U3P_PIB_CSR_CURRENT_STATE_MASK; 

        /* Init the ID register */ 
        PMMC->cid0 = 
            CY_U3P_PIB_PMMC_NOT_USED |                                  /* Always 1 */
            (CY_U3P_PMMC_MANUFACTURE_DATE << CY_U3P_PIB_PMMC_MDT_POS) | /* Manufacturing date */
            (CY_U3P_PMMC_PSN << CY_U3P_PIB_PMMC_PSN_L_POS);             /* LSB 16bit of PSN code */

        PMMC->cid1 = 
            (CY_U3P_PMMC_PSN >> CY_U3P_PIB_PMMC_PSN_H_POS) |            /* Prod serial top 16 bit# */ 
            (CY_U3P_PMMC_PRV << CY_U3P_PIB_PMMC_PRV_POS) |              /* 8 bit Prod version # */ 
            (CY_U3P_PMMC_PNM_0 << CY_U3P_PIB_PMMC_PNM_L_POS);           /* 6 ASCII chrs Prod Name */ 

        PMMC->cid2 = CY_U3P_PMMC_PNM_1;                                 /* 8-40 of Product Name */ 

        PMMC->cid3 = 
            (CY_U3P_PMMC_PNM_2 << CY_U3P_PIB_PMMC_PNM_H_POS) |          /* last 8 bits of Prod Name */  
            (CY_U3P_PMMC_OID << CY_U3P_PIB_PMMC_OID_POS) |              /* 8-bit OID # */ 
            (CY_U3P_PMMC_CBX << CY_U3P_PIB_PMMC_CBX_POS) |              /* Device Package type */
            (CY_U3P_PMMC_MID << CY_U3P_PIB_PMMC_MID_POS) ;              /* Manufacturer ID */ 


        /* Init PMMC-CSD Card Specific Data register here */
        PMMC->csd0 = glPmmcCsdValue[0];
        PMMC->csd1 = glPmmcCsdValue[1];
        PMMC->csd2 = glPmmcCsdValue[2];
        PMMC->csd3 = glPmmcCsdValue[3];

        /* Setup extended CSD register */
        PMMC->ext_csd0 = glPmmcExtCsdValue[0];
        PMMC->ext_csd1 = glPmmcExtCsdValue[1];
        PMMC->ext_csd2 = glPmmcExtCsdValue[2];
        PMMC->ext_csd3 = glPmmcExtCsdValue[3];
        PMMC->ext_csd4 = glPmmcExtCsdValue[4];
        PMMC->ext_csd5 = glPmmcExtCsdValue[5];

        /* Init OCR register */ 
        PMMC->ocr = CY_U3P_PIB_PMMC_OCR_DEFAULT;

        /* Init CSR register */ 
        PMMC->csr = CY_U3P_PIB_PMMC_CSR_DEFAULT;  

        /* Set a default PMMC block length */
        /* May not be necessary since Boot-ROM or host will set */ 
        PMMC->block_len = CY_U3P_PIB_PMMC_BLOCK_LEN_DEFAULT;
        PMMC->dir_sock  = 0x00000000;

        /* Set the proper INTR mask in PMMC module */
        PMMC->intr_mask = (CY_U3P_PIB_PMMC_GO_IDLE | CY_U3P_PIB_PMMC_CMD1 | CY_U3P_PIB_PMMC_CMD5_SLEEP |
                CY_U3P_PIB_PMMC_CMD5_AWAKE | CY_U3P_PIB_PMMC_CMD6 | CY_U3P_PIB_PMMC_CMD12 | CY_U3P_PIB_PMMC_CMD15 |
                CY_U3P_PIB_PMMC_NEW_CMD_RCVD | CY_U3P_PIB_PMMC_WR_SCK_NOT_RDY | CY_U3P_PIB_PMMC_RD_SCK_NOT_RDY);

        /* Set the proper INTR mask in PIB module */
        PIB->intr_mask = (CY_U3P_PIB_INTR_PMMC_INTR1);
    }
    else
    {
        PMMC->intr_mask = 0x00000000;
    }

    /* Set the OCR ready status to indicate that the firmware is ready for operation. */
    PMMC->ocr |= CY_U3P_PIB_PMMC_OCR_STATUS;
    CyU3PSysBarrierSync ();

    PIB->config = (CY_U3P_PIB_ENABLE | CY_U3P_PIB_PCFG | CY_U3P_PIB_PMMC_RESETN) |
        (PIB->config & (CY_U3P_PIB_MMIO_ENABLE | CY_U3P_PIB_DEVICE_ID_MASK));
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PPibSelectMmcSlaveMode (
        void)
{
    if (glPibActive)
        return CY_U3P_ERROR_ALREADY_STARTED;

    glPibMmcMode = CyTrue;
    return CY_U3P_SUCCESS;
}

/*
 * Pib module init function: This initializes pib module e.g. check whether to support 
 * pass-through (in case of PMMC), initializes registers, interrupts, socket allocation 
 * if required etc.
 */
CyU3PReturnStatus_t
CyU3PPibInit (
        CyBool_t doInit,
        CyU3PPibClock_t *pibClock)
{
    uint32_t status = CY_U3P_SUCCESS;

    /* If the part in use is SD3/SD2, the PIB is not supported and trying to use this will cause failures. */
    if (!CyFx3DevIsGpifSupported ())
        return CY_U3P_ERROR_NOT_SUPPORTED;

    if (glPibActive)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    /* Null pointer check */
    if (pibClock == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    /* Check for valid Clock Source */
    if (pibClock->clkSrc >= CY_U3P_NUM_CLK_SRC)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Check for valid divider */
    if ((pibClock->clkDiv < 2) || (pibClock->clkDiv > 1024))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    glPibActive  = CyTrue;

    /* Set the frequency and enable it. This should precede the power up */
    GCTL->pib_core_clk = (((uint32_t)pibClock->clkSrc << CY_U3P_GCTL_PIBCLK_SRC_POS) & CY_U3P_GCTL_PIBCLK_SRC_MASK) |
        ((((uint32_t)pibClock->clkDiv - 1) << CY_U3P_GCTL_PIBCLK_DIV_POS) & CY_U3P_GCTL_PIBCLK_DIV_MASK);

    /* Check HalfDiv */
    if (pibClock->isHalfDiv)
    {
        GCTL->pib_core_clk |= CY_U3P_GCTL_PIBCLK_HALFDIV;
    }

    /* Enable clock */
    GCTL->pib_core_clk |= CY_U3P_GCTL_PIBCLK_CLK_EN;

    /* Power up the Pport and wait for reset to be completed. */
    CyFx3PibPowerOn ();

    /* Check DLL enable */
    if (pibClock->isDllEnable)
    {
        CyFx3PibDllEnable ();
    }

    /* Selectively enable the required P-port interface. */
    if (glPibMmcMode)
    {
        status = CyU3PPmmcInit (doInit);
    }
    else
    {
        status = CyU3PGpifInit (doInit);
    }

    /* By default, set all sockets in packet mode. */
    PIB->eop_eot = 0xFFFFFFFF;

    /* Enable the interrupts from the PIB. */
    if (status == CY_U3P_SUCCESS)
    {
        CyU3PPibSocketInit ();

        CyU3PVicEnableInt (CY_U3P_VIC_PIB_CORE_VECTOR);
        CyU3PVicEnableInt (CY_U3P_VIC_PIB_DMA_VECTOR);
    }

    return status;
}

/* De-initialize the PIB. Deinitialize the PIB VIC. */
CyU3PReturnStatus_t
CyU3PPibDeInit (
        void)
{
    if (!glPibActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    CyU3PVicDisableInt (CY_U3P_VIC_PIB_CORE_VECTOR);
    CyU3PVicDisableInt (CY_U3P_VIC_PIB_DMA_VECTOR);

    /* Power the PIB block off and turn the clock off. */
    CyFx3PibPowerOff ();
    CyU3PBusyWait (10);
    GCTL->pib_core_clk &= ~CY_U3P_GCTL_PIBCLK_CLK_EN;

    glPibActive  = CyFalse;
    glPibMmcMode = CyFalse;

    return CY_U3P_SUCCESS;
}

/*
 * This function sets the Drive Strength for the Pport.
 */
CyU3PReturnStatus_t
CyU3PSetPportDriveStrength (
            CyU3PDriveStrengthState_t pportDriveStrength
            )
{
    /* Check for parameter validity. */
    if (pportDriveStrength > CY_U3P_DS_FULL_STRENGTH)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Reset the current Pport Drive strength */
    GCTL->ds &= ~(CY_U3P_PDS_MASK);

    /* Set the requested Drive strength */
    GCTL->ds |= (((uint32_t)pportDriveStrength << CY_U3P_PDS_POS) & CY_U3P_PDS_MASK);

    return CY_U3P_SUCCESS;

}

void
CyU3PPibSelectIntSources (
        CyBool_t pibSockEn,
        CyBool_t gpifIntEn,
        CyBool_t pibErrEn,
        CyBool_t mboxIntEn,
        CyBool_t wakeupEn)
{
    uint32_t mask = 0;

    if (pibSockEn)
        mask |= CY_U3P_PIB_SOCK_AGG_AL | CY_U3P_PIB_SOCK_AGG_AH | CY_U3P_PIB_SOCK_AGG_BL | CY_U3P_PIB_SOCK_AGG_BH;
    if (gpifIntEn)
        mask |= CY_U3P_PIB_GPIF_INT;
    if (pibErrEn)
        mask |= CY_U3P_PIB_PIB_ERR | CY_U3P_PIB_GPIF_ERR;
    if (mboxIntEn)
        mask |= CY_U3P_PIB_RD_MB_FULL;
    if (wakeupEn)
        mask |= CY_U3P_PIB_WAKEUP;

    PIB->pp_intr_mask = mask;
}

extern void
__CyU3PPportCoreIntrHP (
        void);
extern void
__CyU3PPportCoreIntr (
        void);

CyU3PReturnStatus_t
CyU3PPibSetInterruptPriority (
        CyBool_t isHigh)
{
    /* Has to be called from a thread. */
    if (CyU3PThreadIdentify () == NULL)
        return CY_U3P_ERROR_INVALID_CALLER;

    /* Disable the PIB core interrupt vector. */
    CyU3PVicDisableInt (CY_U3P_VIC_PIB_CORE_VECTOR);
    CyU3PBusyWait (10);

    /* Update the interrupt vector location and priority. */
    if (isHigh)
    {
        VIC->vect_priority[CY_U3P_VIC_PIB_CORE_VECTOR] = 0;
        VIC->vec_address[CY_U3P_VIC_PIB_CORE_VECTOR]   = (uint32_t)__CyU3PPportCoreIntrHP;
    }
    else
    {
        VIC->vect_priority[CY_U3P_VIC_PIB_CORE_VECTOR] = CY_U3P_VIC_VECT_PRIORITY_DEFAULT;
        VIC->vec_address[CY_U3P_VIC_PIB_CORE_VECTOR]   = (uint32_t)__CyU3PPportCoreIntr;
    }

    CyU3PSysBarrierSync ();
    CyU3PBusyWait (10);

    /* Enable the PIB core interrupt vector. */
    CyU3PVicEnableInt (CY_U3P_VIC_PIB_CORE_VECTOR);
    return CY_U3P_SUCCESS;
}

void
CyU3PPmmcRegisterCallback (
        CyU3PPmmcIntrCb_t cbFunc)
{
    glPmmcIntrCb = cbFunc;
}

CyU3PReturnStatus_t
CyU3PPmmcEnableDirectAccess (
        CyBool_t enable,
        uint8_t  wrSock,
        uint8_t  rdSock)
{
    if (!glPibActive)
        return CY_U3P_ERROR_NOT_STARTED;
    if (!glPibMmcMode)
        return CY_U3P_ERROR_INVALID_SEQUENCE;
    if ((rdSock >= 16) || (wrSock < 16) || (wrSock >= 32))
        return CY_U3P_ERROR_BAD_ARGUMENT;

    if (enable)
    {
        PMMC->intr_mask |= (CY_U3P_PIB_PMMC_WR_DIRECT | CY_U3P_PIB_PMMC_RD_DIRECT);
        PMMC->dir_sock   = (wrSock << CY_U3P_PIB_PMMC_WR_SOCK_POS) | (rdSock << CY_U3P_PIB_PMMC_RD_SOCK_POS) |
            CY_U3P_PIB_PMMC_ACCESS_ENABLE;
    }
    else
    {
        PMMC->intr_mask &= ~(CY_U3P_PIB_PMMC_WR_DIRECT | CY_U3P_PIB_PMMC_RD_DIRECT);
        PMMC->dir_sock   = 0;
    }

    return CY_U3P_SUCCESS;
}

extern void
CyU3PPibIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));
extern void
CyU3PGpifIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));
extern void
CyU3PPmmcIntHandler (
        void) __attribute__ ((section ("CYU3P_ITCM_SECTION")));
extern CyU3PReturnStatus_t
CyU3PPibMsgSend (
        uint32_t *msg,
        uint32_t waitOption,
        CyBool_t priority) __attribute__ ((section ("CYU3P_ITCM_SECTION")));

/* 
* Pib interrupt handler: This check the cause of the interrupt then calls appropiate 
* function. If there is nothing to do then it just clears the interrupt then returns.
*/
void
CyU3PPibIntHandler (
        void)
{
    uint32_t activeIntr;

    activeIntr = PIB->intr & PIB->intr_mask;

    if (activeIntr & CY_U3P_PIB_INTR_RD_MB_EMPTY)
    {
        /* Trigger the PIB event flag's RD_EMPTY event */
        CyU3PEventSet (&glPibEvt, CY_U3P_PIB_RD_EMPTY_EVT, CYU3P_EVENT_OR);

        /* If callback is registered then call the callback */
        if (glMboxCb != NULL)
        {
            glMboxCb (CyFalse);
        }
        PIB->intr = CY_U3P_PIB_INTR_RD_MB_EMPTY;
    }

    if (activeIntr & CY_U3P_PIB_INTR_WR_MB_FULL)
    {
        /* If callback is registered then call the callback */
        if (glMboxCb != NULL)
        {
            glMboxCb (CyTrue);
        }
        PIB->intr = CY_U3P_PIB_INTR_WR_MB_FULL;
    }

    if (activeIntr & CY_U3P_PIB_INTR_GPIF_INTERRUPT)
    {
        /* Call the GPIF specific interrupt handler. There is no need
         * to clear the interrupt at the PIB level.
         */
        CyU3PGpifIntHandler ();
    }
    if (activeIntr & CY_U3P_PIB_INTR_PMMC_INTR1)
    {
        CyU3PPmmcIntHandler ();
        PIB->intr = CY_U3P_PIB_INTR_PMMC_INTR1;
    }

    if (activeIntr & CY_U3P_PIB_INTR_CONFIG_CHANGE)
    {
        /* Set an event that will trigger the callback. */
        CyU3PEventSet (&glPibEvt, CY_U3P_PIB_PP_CFG_EVT, CYU3P_EVENT_OR);

        /* Clear the interrupt. */
        PIB->intr = CY_U3P_PIB_INTR_CONFIG_CHANGE;
    }

    if (activeIntr & CY_U3P_PIB_INTR_DLL_LOCKED)
    {
        CyU3PEventSet (&glPibEvt, CY_U3P_PIB_DLLLOCK_EVT, CYU3P_EVENT_OR);
        PIB->intr = CY_U3P_PIB_INTR_DLL_LOCKED;
    }

    if (activeIntr & CY_U3P_PIB_INTR_DLL_LOST_LOCK)
    {
        CyU3PEventSet (&glPibEvt, CY_U3P_PIB_DLLLOCK_EVT, CYU3P_EVENT_OR);
        PIB->intr = CY_U3P_PIB_INTR_DLL_LOCKED;
    }

    if (activeIntr & (CY_U3P_PIB_INTR_PIB_ERR | CY_U3P_PIB_INTR_GPIF_ERR | CY_U3P_PIB_INTR_MMC_ERR))
    {
        CyU3PEventSet (&glPibEvt, CY_U3P_PIB_ERROR_EVT, CYU3P_EVENT_OR);

        /* Disable the interrupt until the callback has been invoked. */
        PIB->intr_mask &= ~(CY_U3P_PIB_INTR_PIB_ERR | CY_U3P_PIB_INTR_GPIF_ERR | CY_U3P_PIB_INTR_MMC_ERR);
        PIB->intr       = (CY_U3P_PIB_INTR_PIB_ERR | CY_U3P_PIB_INTR_GPIF_ERR | CY_U3P_PIB_INTR_MMC_ERR);
    }
}

/* GPIF interrupt handler. Sends a message to the PIB thread which will then raise the event
 * to the application level.
 */
void
CyU3PGpifIntHandler (
        void)
{
    uint32_t intrVal;
    uint32_t msg, id;

    /* Identify that the message originated from an ISR within the firmware. */
    id = CY_U3P_MSG_RQT | (CY_U3P_INT_MODULE_ID << CY_U3P_MSG_SRC_ID_POS) | CY_U3P_TASK_GPIF_EVENT;

    intrVal = PIB->gpif_intr & PIB->gpif_intr_mask;
    while (intrVal != 0)
    {
        if (intrVal & CY_U3P_GPIF_INTR_GPIF_DONE)
        {
            msg = (CY_U3P_PIB_GPIF_WAVEFORM_CTRL_STAT & CY_U3P_GPIF_CURRENT_STATE_MASK) |
                (CYU3P_GPIF_EVT_END_STATE << 16) | id;
            CyU3PPibMsgSend (&msg, CYU3P_NO_WAIT, CyFalse);

            PIB->gpif_intr = CY_U3P_GPIF_INTR_GPIF_DONE;
        }

        if (intrVal & CY_U3P_GPIF_INTR_GPIF_INTR)
        {
            if (glGpifSMCb)
                glGpifSMCb ((uint8_t) (CY_U3P_PIB_GPIF_STATUS >> CY_U3P_GPIF_STATUS_INTERRUPT_STATE_POS));

            msg = (CY_U3P_PIB_GPIF_STATUS & CY_U3P_GPIF_STATUS_INTERRUPT_STATE_MASK) |
                (CYU3P_GPIF_EVT_SM_INTERRUPT << 16) | id;
            CyU3PPibMsgSend (&msg, CYU3P_NO_WAIT, CyFalse);

            PIB->gpif_intr = CY_U3P_GPIF_INTR_GPIF_INTR;
        }

        if (intrVal & CY_U3P_GPIF_INTR_SWITCH_TIMEOUT)
        {
            msg = (CYU3P_GPIF_EVT_SWITCH_TIMEOUT << 16) | id;
            CyU3PPibMsgSend (&msg, CYU3P_NO_WAIT, CyFalse);

            PIB->gpif_intr = CY_U3P_GPIF_INTR_SWITCH_TIMEOUT;
        }

        if (intrVal & CY_U3P_GPIF_INTR_ADDR_COUNT_HIT)
        {
            msg = (CYU3P_GPIF_EVT_ADDR_COUNTER << 16) | id;
            CyU3PPibMsgSend (&msg, CYU3P_NO_WAIT, CyFalse);

            PIB->gpif_intr = CY_U3P_GPIF_INTR_ADDR_COUNT_HIT;
        }

        if (intrVal & CY_U3P_GPIF_INTR_DATA_COUNT_HIT)
        {
            msg = (CYU3P_GPIF_EVT_DATA_COUNTER << 16) | id;
            CyU3PPibMsgSend (&msg, CYU3P_NO_WAIT, CyFalse);

            PIB->gpif_intr = CY_U3P_GPIF_INTR_DATA_COUNT_HIT;
        }

        if (intrVal & CY_U3P_GPIF_INTR_CTRL_COUNT_HIT)
        {
            msg = (CYU3P_GPIF_EVT_CTRL_COUNTER << 16) | id;
            CyU3PPibMsgSend (&msg, CYU3P_NO_WAIT, CyFalse);

            PIB->gpif_intr = CY_U3P_GPIF_INTR_CTRL_COUNT_HIT;
        }

        if (intrVal & CY_U3P_GPIF_INTR_ADDR_COMP_HIT)
        {
            msg = (CYU3P_GPIF_EVT_ADDR_COMP << 16) | id;
            CyU3PPibMsgSend (&msg, CYU3P_NO_WAIT, CyFalse);

            PIB->gpif_intr = CY_U3P_GPIF_INTR_ADDR_COMP_HIT;
        }

        if (intrVal & CY_U3P_GPIF_INTR_DATA_COMP_HIT)
        {
            msg = (CYU3P_GPIF_EVT_DATA_COMP << 16) | id;
            CyU3PPibMsgSend (&msg, CYU3P_NO_WAIT, CyFalse);

            PIB->gpif_intr = CY_U3P_GPIF_INTR_DATA_COMP_HIT;
        }

        if (intrVal & CY_U3P_GPIF_INTR_CTRL_COMP_HIT)
        {
            msg = (CYU3P_GPIF_EVT_CTRL_COMP << 16) | id;
            CyU3PPibMsgSend (&msg, CYU3P_NO_WAIT, CyFalse);

            PIB->gpif_intr = CY_U3P_GPIF_INTR_CTRL_COMP_HIT;
        }

        if (intrVal & CY_U3P_GPIF_INTR_EG_DATA_EMPTY_MASK)
        {
            /* Find which threads have active eg_data_empty status and update the events. */
            msg = ((intrVal & CY_U3P_GPIF_INTR_EG_DATA_EMPTY_MASK) >> (CY_U3P_GPIF_INTR_EG_DATA_EMPTY_POS - 8));
            CyU3PEventSet (&glPibEvt, msg, CYU3P_EVENT_OR);

            PIB->gpif_intr = CY_U3P_GPIF_INTR_EG_DATA_EMPTY_MASK;
        }

        if (intrVal & CY_U3P_GPIF_INTR_IN_DATA_VALID_MASK)
        {
            /* Find which threads have active in_data_empty status and update the events. */
            msg = ((intrVal & CY_U3P_GPIF_INTR_IN_DATA_VALID_MASK) >> (CY_U3P_GPIF_INTR_IN_DATA_VALID_POS - 8));
            CyU3PEventSet (&glPibEvt, msg, CYU3P_EVENT_OR);

            PIB->gpif_intr = CY_U3P_GPIF_INTR_IN_DATA_VALID_MASK;
        }

        if (intrVal & CY_U3P_GPIF_INTR_WAVEFORM_BUSY)
        {
            /* Just clear the interrupt as it is not likely to be of interest. */
            PIB->gpif_intr = CY_U3P_GPIF_INTR_WAVEFORM_BUSY;
        }

        if (intrVal & CY_U3P_GPIF_INTR_CRC_ERROR)
        {
            /* Send CRC error notification event to PIB/GPIF thread. */
            msg = (CYU3P_GPIF_EVT_CRC_ERROR << 16) | id;
            CyU3PPibMsgSend (&msg, CYU3P_NO_WAIT, CyFalse);

            PIB->gpif_intr = CY_U3P_GPIF_INTR_CRC_ERROR;
        }

        intrVal = PIB->gpif_intr & PIB->gpif_intr_mask;
    }
}

/* 
 * Pmmc interrupt handler: This handles the pmmc interrupt. It checks the cause of this
 * interrupt by looking at the PMMC intr register 
 */
void
CyU3PPmmcIntHandler (
        void)
{
    uint8_t index, value;
    uint32_t mask;

    /* Store the interrupt status and then clear the interrupt. */
    mask = (PMMC->intr & PMMC->intr_mask);
    PMMC->intr = mask;

    if (mask & CY_U3P_PIB_PMMC_GO_IDLE)
    {
        if (glCurrentState == CY_U3P_PMMC_WAITIRQ)
        {
            /* State will transit from IRQ to Standby state. */
            glCurrentState = CY_U3P_PMMC_STANDBY;
        }
        else
        {
            /* Re initialize the maibox */
            CyU3PMboxReset ();
            glCurrentState = CY_U3P_PMMC_IDLE;
        }

        /* Set the OCR ready bit */
        PMMC->ocr |= CY_U3P_PIB_PMMC_OCR_STATUS;

        if (glPmmcIntrCb)
            glPmmcIntrCb (CYU3P_PMMC_GOIDLE_CMD, 0);
    }

    if (mask & CY_U3P_PIB_PMMC_CMD1)
    {
        if (glCurrentState == CY_U3P_PMMC_WAITIRQ)
        {
            glCurrentState = CY_U3P_PMMC_STANDBY;
        }
        else
        {
            glCurrentState = CY_U3P_PMMC_READY;
        }

        /* Set the OCR last bit */
        PMMC->ocr |= CY_U3P_PIB_PMMC_OCR_STATUS;
    }

    if (mask & CY_U3P_PIB_PMMC_CMD4)
    {
        glCurrentState = CY_U3P_PMMC_STANDBY;
    }

    if (mask & CY_U3P_PIB_PMMC_CMD5_SLEEP)
    {
        if (glPmmcIntrCb)
            glPmmcIntrCb (CYU3P_PMMC_CMD5_SLEEP, 0);
        glCurrentState = CY_U3P_PMMC_SLEEP;
    }

    if (mask & CY_U3P_PIB_PMMC_CMD5_AWAKE)
    {
        if (glPmmcIntrCb)
            glPmmcIntrCb (CYU3P_PMMC_CMD5_AWAKE, 0);
        glCurrentState = CY_U3P_PMMC_STANDBY;
    }

    if (mask & CY_U3P_PIB_PMMC_CMD6)
    {
        if (glPmmcIntrCb)
            glPmmcIntrCb (CYU3P_PMMC_CMD6_SWITCH, PMMC->arg);
        index = (PMMC->arg & (0xFF << 16)) >> 16;
        value = (PMMC->arg & (0xFF << 8)) >> 8;

        switch ((PMMC->arg & 0x03000000) >> 24)
        {
            case 0: /* Switch command set */
                /* Do not Support swich command set */
                PMMC->csr |= CY_U3P_PIB_CSR_SWITCH_ERROR;

                /* Clear the busy bit */
                PMMC->busy = CY_U3P_PIB_PMMC_BUSY_CLEAR;
                break;

            case 3: /* Write byte */
                if ((index == 183) && (value < 3))
                {
                    PMMC->ext_csd0 &= ~CY_U3P_PIB_EXT_CSD0_BUS_WIDTH_MASK;
                    PMMC->ext_csd0 |= value;
                    break;
                }
                else if (index == 185)
                {
                    if (value == 1)
                    {
                        PMMC->ext_csd0 |= (0x1  << CY_U3P_PIB_EXT_CSD0_HS_TIMING_POS)
                            & CY_U3P_PIB_EXT_CSD0_HS_TIMING_MASK; 
                        break;
                    }
                    else if (value == 0)
                    {
                        PMMC->ext_csd0 &= ~CY_U3P_PIB_EXT_CSD0_HS_TIMING_MASK;
                        break;
                    }
                }
                else if ((index == 191) && (value == 0))
                {
                    /* default is std mmc command */
                    break;
                }
                /* If you are  here the there is a switch error */
                PMMC->csr |= CY_U3P_PIB_CSR_SWITCH_ERROR;
                break;

            default: /* Set bits or Clear bits*/
                if ((index != 185) || (value > 1))
                {   /* Only supports HS_TIMING byte to set*/
                    PMMC->csr |= CY_U3P_PIB_CSR_SWITCH_ERROR;
                    break;
                }
                if (value == 1)
                {
                    PMMC->ext_csd0 |= (0x1  << CY_U3P_PIB_EXT_CSD0_HS_TIMING_POS) 
                        & CY_U3P_PIB_EXT_CSD0_HS_TIMING_MASK;
                }
                else
                {
                    PMMC->ext_csd0 &= ~CY_U3P_PIB_EXT_CSD0_HS_TIMING_MASK; 
                }
                break;
        }

        glCurrentState = CY_U3P_PMMC_STANDBY;

        /* Clear the busy bit */
        PMMC->busy = CY_U3P_PIB_PMMC_BUSY_CLEAR;
    }

    if (mask & CY_U3P_PIB_PMMC_CMD12)
    {
        if (glPmmcIntrCb)
            glPmmcIntrCb (CYU3P_PMMC_CMD12_STOP, PMMC->blk_addr);
    }

    if (mask & CY_U3P_PIB_PMMC_NEW_CMD_RCVD)
    {
        /* Wait for card select call before sending the init complete event to the system thread. */
        if ((PMMC->idx & CY_U3P_PIB_PMMC_CMD_MASK) == 7)
        {
            if (glPmmcIntrCb)
                glPmmcIntrCb (CYU3P_PMMC_CMD7_SELECT, PMMC->arg);
        }
    }

    if (mask & (CY_U3P_PIB_PMMC_WR_SCK_NOT_RDY | CY_U3P_PIB_PMMC_RD_SCK_NOT_RDY))
    {
        if (glPmmcIntrCb)
            glPmmcIntrCb (CYU3P_PMMC_SOCKET_NOT_READY, PMMC->blk_addr);
    }

    if (mask & CY_U3P_PIB_PMMC_WR_DIRECT)
    {
        if (glPmmcIntrCb)
            glPmmcIntrCb (CYU3P_PMMC_DIRECT_WRITE, PMMC->blk_addr);
    }

    if (mask & CY_U3P_PIB_PMMC_RD_DIRECT)
    {
        if (glPmmcIntrCb)
            glPmmcIntrCb (CYU3P_PMMC_DIRECT_READ, PMMC->blk_addr);
    }

    if (mask & CY_U3P_PIB_PMMC_CMD15)
    {
        if (glPmmcIntrCb)
            glPmmcIntrCb (CYU3P_PMMC_CMD15_INACTIVE, 0);
    }
}

/*
 * Message Queueing function to the system thread
 */
CyU3PReturnStatus_t
CyU3PPibMsgSend (
        uint32_t *msg,
        uint32_t waitOption,
        CyBool_t priority)
{
    uint32_t status;

    if (priority == CyTrue)
    {
        status = CyU3PQueuePrioritySend (&glPibQueue, msg, waitOption);
    }
    else
    {
        status = CyU3PQueueSend (&glPibQueue, msg, waitOption);
    }

    /* Set the Queue event */
    if (status == CY_U3P_SUCCESS)
        status = CyU3PEventSet (&glPibEvt, CY_U3P_EVENT_QUEUE, CYU3P_EVENT_OR);

    return status;
}

/*[]*/

