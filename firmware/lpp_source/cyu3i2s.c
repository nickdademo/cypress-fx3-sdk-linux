/*
 ## Cypress EZ-USB FX3 Source file (cyu3i2s.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2012,
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

#include <cyu3i2s.h>
#include <cyu3error.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3lpp.h>
#include <cyu3utils.h>
#include <cyu3system.h>
#include <i2s_regs.h>

/*
 * @@I2S 
 * Summary
 * I2S driver and convenience API for the EZ-USB FX3 device.
 */

#define CY_U3P_I2S_DEFAULT_SAMPLE_RATE  (CY_U3P_I2S_SAMPLE_RATE_8KHz)   /* Default bit rate for I2S module. */
#define CY_U3P_I2S_TIMEOUT              (0xFFFFF)                       /* Default timeout for I2S response access. */

static CyBool_t glIsI2sDma = CyFalse;                                   /* Whether the I2S module is in DMA mode. */
static CyBool_t glIsI2sConfigured = CyFalse;                            /* Whether the I2S module has been configured. */
static CyBool_t glIsI2sActive = CyFalse;                                /* Whether the I2S module is active. */
static CyU3PI2sIntrCb_t glI2sIntrCb = NULL;                             /* Callback for I2S interrupts. */
static CyU3PMutex glI2sLock;                                            /* Mutex lock for I2S module access. */

/* Get the lock of the I2S */
static CyU3PReturnStatus_t
CyU3PI2sGetLock (
                 void)
{
    uint32_t waitOption = 0;

    if (!glIsI2sActive)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (CyU3PThreadIdentify () != NULL)
    {
        waitOption = CY_U3P_I2S_DEFAULT_LOCK_TIMEOUT;
    }
    if (CyU3PMutexGet (&glI2sLock, waitOption) != CY_U3P_SUCCESS)
    {
        return CY_U3P_ERROR_MUTEX_FAILURE;
    }

    return CY_U3P_SUCCESS;
}

/* Release the lock of the I2S */
static CyU3PReturnStatus_t
CyU3PI2sReleaseLock (
                     void)
{
    if (!glIsI2sActive)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (CyU3PMutexPut (&glI2sLock) != CY_U3P_SUCCESS)
    {
        return CY_U3P_ERROR_MUTEX_FAILURE;
    }
    return CY_U3P_SUCCESS;	
}

/* Register for the I2s Call Back function */
void 
CyU3PRegisterI2sCallBack (
                          CyU3PI2sIntrCb_t i2sIntrCb)
{    
    glI2sIntrCb = i2sIntrCb;
}

void
CyU3PI2sInt_ThreadHandler (
                           void)
{
    uint32_t mask, status;

    status = CyU3PI2sGetLock ();
    if (status != CY_U3P_SUCCESS)
    {        
        return;
    }

    /* Read out the interrupts and clear them. */
    mask = I2S->lpp_i2s_intr & I2S->lpp_i2s_intr_mask;
    I2S->lpp_i2s_intr = mask;

    if (glI2sIntrCb == NULL)
    {       
        CyU3PI2sReleaseLock ();
        return;
    }
    
    if (mask & CY_U3P_LPP_I2S_TXL_DONE)
    {
        glI2sIntrCb (CY_U3P_I2S_EVENT_TXL_DONE, (CyU3PI2sError_t)0);
    }

    if (mask & CY_U3P_LPP_I2S_TXR_DONE)
    {
        glI2sIntrCb (CY_U3P_I2S_EVENT_TXR_DONE, (CyU3PI2sError_t)0);
    }

    if (mask & CY_U3P_LPP_I2S_PAUSED)
    {
        glI2sIntrCb (CY_U3P_I2S_EVENT_PAUSED, (CyU3PI2sError_t)0);
    }

    if (mask & CY_U3P_LPP_I2S_ERROR)
    {
        glI2sIntrCb (CY_U3P_I2S_EVENT_ERROR, (CyU3PI2sError_t)((I2S->lpp_i2s_status & 
            CY_U3P_LPP_I2S_ERROR_CODE_MASK) >> CY_U3P_LPP_I2S_ERROR_CODE_POS));
    }
    
    CyU3PI2sReleaseLock ();
}

/* 
 * This function initializes the I2S module.
 */
CyU3PReturnStatus_t 
CyU3PI2sInit (
              void)
{
    CyU3PReturnStatus_t status;
    /* Check the IO matrix */
    if (!CyU3PIsLppIOConfigured(CY_U3P_LPP_I2S))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (glIsI2sActive)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    CyU3PMutexCreate (&glI2sLock, CYU3P_NO_INHERIT);

    /* Set the clock freqency. This should precede the I2S power up.
    * Setting it to stereo fixed mode. */
    status = CyU3PI2sSetClock (CY_U3P_I2S_DEFAULT_SAMPLE_RATE * 64);
    if(status != CY_U3P_SUCCESS)	
    {
        return status;
    }

    /* Identify if the LPP block has been initialized. */
    status = CyU3PLppInit (CY_U3P_LPP_I2S,CyU3PI2sInt_ThreadHandler);
    if(status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Power on the I2S module */
    I2S->lpp_i2s_power &= ~(CY_U3P_LPP_I2S_RESETN);
    CyU3PBusyWait (10);
    I2S->lpp_i2s_power |= CY_U3P_LPP_I2S_RESETN;
    while (!(I2S->lpp_i2s_power & CY_U3P_LPP_I2S_ACTIVE));

    /* Mark the module active. */
    glIsI2sActive = CyTrue;

    return CY_U3P_SUCCESS;
}

/*
 * This function de-initializes the I2S 
 */
CyU3PReturnStatus_t
CyU3PI2sDeInit(
               void)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    if (!glIsI2sActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Reset the reset bit */
    I2S->lpp_i2s_power &= ~(CY_U3P_LPP_I2S_RESETN);
    CyU3PBusyWait (10);

    /* Mark the block as disabled. */
    glIsI2sActive = CyFalse;
    glIsI2sDma = CyFalse;
    glIsI2sConfigured = CyFalse;

    /* Identify if the LPP block has to be disabled. */
    status = CyU3PLppDeInit (CY_U3P_LPP_I2S);  

    /* Disable the I2s clock */
    CyU3PI2sStopClock();

    CyU3PMutexDestroy (&glI2sLock);
    return status;
}

/* 
 * configures and opens the I2S  
 */
CyU3PReturnStatus_t 
CyU3PI2sSetConfig (
                   CyU3PI2sConfig_t *config,
                   CyU3PI2sIntrCb_t cb)
{
    uint32_t temp, mult, timeout;
    CyU3PReturnStatus_t status;
 
    /* Check if the I2S is initiaized */
    if (!glIsI2sActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (config == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if (config->padMode >= CY_U2P_I2S_NUM_PAD_MODES)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (config->sampleWidth >= CY_U3P_I2S_NUM_BIT_WIDTH)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((!config->isDma) && (config->padMode !=
        CY_U3P_I2S_PAD_MODE_RIGHT_JUSTIFIED))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((!config->isDma) && (cb != NULL))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    status = CyU3PI2sGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    temp = 0;
    /* Mono or stereo */
    if (config->isMono)
    {
        temp = CY_U3P_LPP_I2S_MONO;
    }

    /* does LSB go first or MSB */
    if (config->isLsbFirst)
    {
        temp |= CY_U3P_LPP_I2S_ENDIAN;
    }

    /* Set the pad mode. */
    if (config->padMode != CY_U3P_I2S_PAD_MODE_CONTINUOUS)
    {
        temp |= CY_U3P_LPP_I2S_FIXED_SCK;
        /* This means that each sample (L + R)
        * is sent out in 64 clocks. clk *= 64 */
        mult = 64;
    }
    else
    {
        /* Each sample (L + R) is sent
        * with only required clocks. */
        switch (config->sampleWidth)
        {
        case CY_U3P_I2S_WIDTH_8_BIT: 
            mult = 8 * 2;
            break;
        case CY_U3P_I2S_WIDTH_16_BIT: 
            mult = 16 * 2;
            break;
        case CY_U3P_I2S_WIDTH_18_BIT: 
            mult = 18 * 2;
            break;
        case CY_U3P_I2S_WIDTH_24_BIT: 
            mult = 24 * 2;
            break;
        case CY_U3P_I2S_WIDTH_32_BIT: 
            mult = 32 * 2;
            break;
        default:
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }

        temp |= (config->padMode << CY_U3P_LPP_I2S_MODE_POS);
    }

    /* Dma mode or register mode trandfer */
    if (config->isDma)
    {
        glIsI2sDma = CyTrue;
        temp |= CY_U3P_LPP_I2S_DMA_MODE;
    }
    else
    {
        glIsI2sDma = CyFalse;
    }

    /* Set the bitWidth */
    temp |= (config->sampleWidth << CY_U3P_LPP_I2S_BIT_WIDTH_POS);

    /* Set the clock for I2S MCLK = bit clock * 4. */
    status = CyU3PI2sSetClock (config->sampleRate * mult * 4);

    if (status == CY_U3P_SUCCESS)
    {
        /* Wait for sometime for the clock to stabilize. */
        CyU3PBusyWait (10);

        /* Clear the TX_FIFO */
        I2S->lpp_i2s_config = CY_U3P_LPP_I2S_TX_CLEAR;
        /* Using timeout as a scratch variable. Read back to flush */
        timeout = I2S->lpp_i2s_config;
        /* Wait until both the TX pipes are cleared. */
        timeout = CY_U3P_I2S_TIMEOUT;
        while ((I2S->lpp_i2s_status & (CY_U3P_LPP_I2S_TXL_DONE | CY_U3P_LPP_I2S_TXR_DONE))
            != (CY_U3P_LPP_I2S_TXL_DONE | CY_U3P_LPP_I2S_TXR_DONE))
        {
            if (timeout-- == 0)
            {
                status = CY_U3P_ERROR_TIMEOUT;
                break;
            }
        }
    }

    if (status != CY_U3P_SUCCESS)
    {
        CyU3PI2sReleaseLock();
        return status;
    }

    /* Write the required configuration to the register */
    I2S->lpp_i2s_config = temp;

    /* Set the socket */
    I2S->lpp_i2s_socket = 
        ((CY_U3P_LPP_SOCKET_I2S_RIGHT << CY_U3P_LPP_I2S_RIGHT_SOCKET_POS) &
        CY_U3P_LPP_I2S_RIGHT_SOCKET_MASK) | (CY_U3P_LPP_SOCKET_I2S_LEFT &
        CY_U3P_LPP_I2S_LEFT_SOCKET_MASK);

    /* Register the callback */
    CyU3PRegisterI2sCallBack(cb);
    if (cb != NULL)
    {
        I2S->lpp_i2s_intr_mask = (CY_U3P_LPP_I2S_TXL_DONE |
            CY_U3P_LPP_I2S_TXR_DONE | CY_U3P_LPP_I2S_PAUSED |
            CY_U3P_LPP_I2S_ERROR);
    }

    glIsI2sConfigured = CyTrue;

    /* Enable the I2S config. */
    I2S->lpp_i2s_config |= (CY_U3P_LPP_I2S_ENABLE);

    CyU3PI2sReleaseLock();

    return CY_U3P_SUCCESS;
}

/*
 * Transmits data byte by byte over the I2S interface
 */
CyU3PReturnStatus_t 
CyU3PI2sTransmitBytes (
                       uint8_t *lData,
                       uint8_t *rData,
                       uint8_t lByteCount,
                       uint8_t rByteCount)
{
    uint8_t left, right;
    uint32_t timeout, status;

    /* Since this is just a read operation, lock is not
    * acquired. */
    if (glIsI2sDma)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (!glIsI2sConfigured)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (((lData == NULL) && (lByteCount != 0))
        || ((rData == NULL) && (rByteCount != 0)))
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    status = CyU3PI2sGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Copy the data to the fifo. */
    left = right = 0;
    while ((left < lByteCount) || (right < rByteCount))
    {
        timeout = CY_U3P_I2S_TIMEOUT;
        /* Wait for the tx_space interrupt bit */
        while (!((I2S->lpp_i2s_status & CY_U3P_LPP_I2S_TXL_SPACE) || 
            (I2S->lpp_i2s_status & CY_U3P_LPP_I2S_TXR_SPACE) ||  
            timeout == 0))
        {
            timeout--;
        }

        if (timeout == 0)
        {
            status = CY_U3P_ERROR_TIMEOUT;
            break;
        }

        /* copy the data to the left egress fifo if that is available */
        if ((I2S->lpp_i2s_status & CY_U3P_LPP_I2S_TXL_SPACE) && (left < lByteCount))
        {
            I2S->lpp_i2s_egress_data_left = lData[left];
            left++;
        }

        /* copy the data to the right egress fifo if that is available */
        if ((I2S->lpp_i2s_status & CY_U3P_LPP_I2S_TXR_SPACE) && (right < rByteCount))
        {
            I2S->lpp_i2s_egress_data_right = rData[right];
            right++;
        }
    }

    CyU3PI2sReleaseLock();

    return status;
}

CyU3PReturnStatus_t 
CyU3PI2sSetMute (CyBool_t isMute)
{
    uint32_t status;

    if (!glIsI2sConfigured)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    status = CyU3PI2sGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    if (isMute)
    {
        I2S->lpp_i2s_config |= CY_U3P_LPP_I2S_MUTE;
    }
    else
    {
        I2S->lpp_i2s_config &= ~CY_U3P_LPP_I2S_MUTE;
    }

    CyU3PI2sReleaseLock();

    return (CY_U3P_SUCCESS);
}

CyU3PReturnStatus_t 
CyU3PI2sSetPause (CyBool_t isPause)
{
    uint32_t status;

    if (!glIsI2sConfigured)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    status = CyU3PI2sGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    if (isPause)
    {
        I2S->lpp_i2s_config |= CY_U3P_LPP_I2S_PAUSE;
    }
    else
    {
        I2S->lpp_i2s_config &= ~CY_U3P_LPP_I2S_PAUSE;
    }

    CyU3PI2sReleaseLock();

    return (CY_U3P_SUCCESS);
}

/*[]*/

