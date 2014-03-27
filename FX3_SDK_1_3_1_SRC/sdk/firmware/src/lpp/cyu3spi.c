/*
 ## Cypress EZ-USB FX3 Source file (cyu3spi.c)
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

#include <cyu3spi.h>
#include <cyu3error.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3lpp.h>
#include <cyu3utils.h>
#include <cyu3system.h>
#include <spi_regs.h>

/*
 * @@SPI 
 * Summary
 * SPI driver and convenience API for the EZ-USB FX3 device.
 */

#define CY_U3P_SPI_DEFAULT_CLK          (1000000)               /* Default SPI clock in Hz */
#define CY_U3P_SPI_TIMEOUT              (0xFFFFF)

static CyBool_t glIsSpiConfigured = CyFalse;
static CyBool_t glIsSpiActive = CyFalse;
static CyU3PSpiIntrCb_t glSpiIntrCb = NULL;     /* Callback from SPI event notification */
static CyU3PMutex glSpiLock;			/* Mutex lock for SPI access APIs. */

/* Get the lock for the SPI before any operation */
static CyU3PReturnStatus_t
CyU3PSpiGetLock (
                 void)
{
    uint32_t waitOption = 0;

    if (!glIsSpiActive)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (CyU3PThreadIdentify () != NULL)
    {
        waitOption = CY_U3P_SPI_DEFAULT_LOCK_TIMEOUT;
    }

    if (CyU3PMutexGet (&glSpiLock, waitOption) != CY_U3P_SUCCESS)
    {
        return CY_U3P_ERROR_MUTEX_FAILURE;
    }

    return CY_U3P_SUCCESS;
}

/* Release the SPI lock */
static CyU3PReturnStatus_t
CyU3PSpiReleaseLock (
                     void)
{
    if (!glIsSpiActive)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (CyU3PMutexPut (&glSpiLock) != CY_U3P_SUCCESS)
    {
        return CY_U3P_ERROR_MUTEX_FAILURE;
    }

    return CY_U3P_SUCCESS;	
}

/* Registering for the call back function */
void 
CyU3PRegisterSpiCallBack (
                          CyU3PSpiIntrCb_t spiIntrCb)
{    
    glSpiIntrCb = spiIntrCb;
}

void
CyU3PSpiInt_ThreadHandler (
                           void)
{
    uint32_t mask, status;

    status = CyU3PSpiGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
       return;
    }

    /* Read out the interrupts and clear them. */
    mask = SPI->lpp_spi_intr & SPI->lpp_spi_intr_mask;
    SPI->lpp_spi_intr = mask;

    if (glSpiIntrCb == NULL)
    {       
        CyU3PSpiReleaseLock();
        return;
    }
   
    if (mask & CY_U3P_LPP_SPI_RX_DONE)
    {
        glSpiIntrCb (CY_U3P_SPI_EVENT_RX_DONE, CY_U3P_SPI_ERROR_NONE);
    }

    if (mask & CY_U3P_LPP_SPI_TX_DONE)
    {
        glSpiIntrCb (CY_U3P_SPI_EVENT_TX_DONE, CY_U3P_SPI_ERROR_NONE);
    }

    if (mask & CY_U3P_LPP_SPI_ERROR)
    {
        glSpiIntrCb (CY_U3P_SPI_EVENT_ERROR, (CyU3PSpiError_t)((SPI->lpp_spi_status & 
            CY_U3P_LPP_SPI_ERROR_CODE_MASK) >> CY_U3P_LPP_SPI_ERROR_CODE_POS));
    }
    CyU3PSpiReleaseLock ();
}

/* 
 * This function initializes the SPI module
 */
CyU3PReturnStatus_t 
CyU3PSpiInit (
              void)
{
    CyU3PReturnStatus_t status;
    /* Check the IO matrix */
    if (!CyU3PIsLppIOConfigured(CY_U3P_LPP_SPI))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    if (glIsSpiActive)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    /* Set the clock freqency. This should precede the SPI power up */
    status = CyU3PSpiSetClock (CY_U3P_SPI_DEFAULT_CLK); 
    if( status != CY_U3P_SUCCESS)	
    {
        return status;
    }
    
    CyU3PMutexCreate (&glSpiLock, CYU3P_NO_INHERIT);
    /* Identify if the LPP block has been initialized. */
    status = CyU3PLppInit (CY_U3P_LPP_SPI,CyU3PSpiInt_ThreadHandler);
    if(status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Power on the SPI module */
    SPI->lpp_spi_power &= ~(CY_U3P_LPP_SPI_RESETN);
    CyU3PBusyWait (10);
    SPI->lpp_spi_power |= CY_U3P_LPP_SPI_RESETN;
    /* Wait till the active bit is set */
    while (!(SPI->lpp_spi_power & CY_U3P_LPP_SPI_ACTIVE));

    /* Mark the module active. */
    glIsSpiActive = CyTrue;

    return CY_U3P_SUCCESS;
}

/*
 * This function de-initializes the SPI 
 */
CyU3PReturnStatus_t
CyU3PSpiDeInit(
               void)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    if (!(glIsSpiActive))
    {
        /* Have not started */
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Reset the reset bit */
    SPI->lpp_spi_power &= ~(CY_U3P_LPP_SPI_RESETN);
    CyU3PBusyWait (10);

    /* Mark the block as disabled. */
    glIsSpiActive = CyFalse;
    glIsSpiConfigured = CyFalse;
   
    /* Identify if the LPP block has to be disabled. */
    status = CyU3PLppDeInit (CY_U3P_LPP_SPI);    

    CyU3PSpiStopClock();
    
    CyU3PMutexDestroy (&glSpiLock);
    
    return status;
}

/* 
 * configures and opens the SPI
 */
CyU3PReturnStatus_t 
CyU3PSpiSetConfig (
                   CyU3PSpiConfig_t *config,
                   CyU3PSpiIntrCb_t cb)
{
    uint32_t temp, timeout;
    
    /* Check if the SPI is initiaized */
    if (!(glIsSpiActive))
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }
    if (config == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if ((config->leadTime == CY_U3P_SPI_SSN_LAG_LEAD_ZERO_CLK) ||
        (config->leadTime >= CY_U3P_SPI_NUM_SSN_LAG_LEAD))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (((config->cpha) && (config->lagTime == CY_U3P_SPI_SSN_LAG_LEAD_ZERO_CLK))
        || (config->lagTime >= CY_U3P_SPI_NUM_SSN_LAG_LEAD))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (config->ssnCtrl >= CY_U3P_SPI_NUM_SSN_CTRL)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((config->wordLen < 3) || (config->wordLen > 32))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    temp = CyU3PSpiGetLock ();    
    if (temp != CY_U3P_SUCCESS)
    {
        return temp;
    }

    /* If the SPI block is still enabled, return an error indicating bad API call sequence. */
    if ((SPI->lpp_spi_config & CY_U3P_LPP_SPI_ENABLE) != 0)
    {
        temp = CY_U3P_ERROR_INVALID_SEQUENCE;
    }

    /* Set the clock for Spi */
    if (temp == CY_U3P_SUCCESS)
        temp = CyU3PSpiSetClock (config->clock);

    if (temp == CY_U3P_SUCCESS)
    {
        /* Clear the TX and RX_FIFO */
        SPI->lpp_spi_config = (CY_U3P_LPP_SPI_TX_CLEAR | CY_U3P_LPP_SPI_RX_CLEAR);

        /* Wait until the SPI block is disabled and the RX_DATA bit is clear and TX_DONE bit is set. */
        timeout = CY_U3P_SPI_TIMEOUT;
        while ((SPI->lpp_spi_status & CY_U3P_LPP_SPI_RX_DATA) != 0)
        {
            if (timeout-- == 0)
            {
                temp = CY_U3P_ERROR_TIMEOUT;
                break;
            }
        }
    }
    if (temp == CY_U3P_SUCCESS)
    {
        while ((SPI->lpp_spi_status & CY_U3P_LPP_SPI_TX_DONE) == 0)
        {
            if (timeout-- == 0)
            {
                temp = CY_U3P_ERROR_TIMEOUT;
                break;
            }
        }
    }

    if (temp != CY_U3P_SUCCESS)
    {
        CyU3PSpiReleaseLock();
        return temp;
    }

    SPI->lpp_spi_config = 0;
    temp = 0;

    /* Setting the endian ness. */
    if (config->isLsbFirst)
    {
        temp |= CY_U3P_LPP_SPI_ENDIAN;
    }
    /* Setting the clock polarity nad clock phase */
    if (config->cpol)
    {
        temp |= CY_U3P_LPP_SPI_CPOL;
    }
    if (config->cpha)
    {
        temp |= CY_U3P_LPP_SPI_CPHA;
    }

    /* Set the clock parameter (e.g. Lead and Lag time) */
    temp |= ((config->leadTime << CY_U3P_LPP_SPI_LEAD_POS) & CY_U3P_LPP_SPI_LEAD_MASK);
    temp |= ((config->lagTime << CY_U3P_LPP_SPI_LAG_POS) & CY_U3P_LPP_SPI_LAG_MASK);

    /* SSN polarity. */
    if (config->ssnPol)
    {
        temp |= CY_U3P_LPP_SPI_SSPOL;
    }

    /* Set the SSN control */
    if (config->ssnCtrl == CY_U3P_SPI_SSN_CTRL_NONE)
    {
        temp |= CY_U3P_LPP_SPI_DESELECT;
    }
    else
    {
        temp |= (config->ssnCtrl << CY_U3P_LPP_SPI_SSNCTRL_POS);
    }

    /* Set the SSN bit high as default value (if FW control is chosen) */
    temp |= CY_U3P_LPP_SPI_SSN_BIT;

    /* Set the word length */
    temp |= ((config->wordLen << CY_U3P_LPP_SPI_WL_POS) & CY_U3P_LPP_SPI_WL_MASK);

    /* Write into the config register */
    SPI->lpp_spi_config = temp;

    SPI->lpp_spi_socket = 
        ((CY_U3P_LPP_SOCKET_SPI_CONS & CY_U3P_LPP_SPI_EGRESS_SOCKET_MASK) |
        ((CY_U3P_LPP_SOCKET_SPI_PROD << CY_U3P_LPP_SPI_INGRESS_SOCKET_POS) & 
        CY_U3P_LPP_SPI_INGRESS_SOCKET_MASK));

    /* Read back to flush */
    temp = SPI->lpp_spi_config;

    CyU3PRegisterSpiCallBack(cb);
    if (cb != NULL)
    {
        SPI->lpp_spi_intr |= CY_U3P_LPP_SPI_TX_DONE;
        SPI->lpp_spi_intr_mask = (CY_U3P_LPP_SPI_TX_DONE |
            CY_U3P_LPP_SPI_RX_DONE | CY_U3P_LPP_SPI_ERROR);
    }
    else
    {
        SPI->lpp_spi_intr_mask = 0;
    }

    glIsSpiConfigured = CyTrue;

    CyU3PSpiReleaseLock();

    return CY_U3P_SUCCESS;
}

/* 
 * Resets the FIFO.
 * Leaves SPI block disabled at the end.
 */
static CyU3PReturnStatus_t
CyU3PSpiResetFifo (
                   CyBool_t isTx,
                   CyBool_t isRx
                   )
{
    uint32_t intrMask;
    uint32_t ctrlMask = 0;
    uint32_t temp;

    /* No lock is acquired or error checked as this is
    * an internal function. Locks need to be acquired
    * prior to this call. */

    /* Temporarily disable interrupts. */
    intrMask = SPI->lpp_spi_intr_mask;
    SPI->lpp_spi_intr_mask = 0;

    if (isTx)
    {
        ctrlMask = CY_U3P_LPP_SPI_TX_CLEAR;
    }
    if (isRx)
    {
        ctrlMask |= CY_U3P_LPP_SPI_RX_CLEAR;
    }

    /* Disable the SPI block and reset. */
    temp = ~(CY_U3P_LPP_SPI_RX_ENABLE | CY_U3P_LPP_SPI_TX_ENABLE |
        CY_U3P_LPP_SPI_DMA_MODE | CY_U3P_LPP_SPI_ENABLE);
    SPI->lpp_spi_config &= temp;
    while ((SPI->lpp_spi_config & CY_U3P_LPP_SPI_ENABLE) != 0);

    /* Clear the FIFOs and wait until they have been cleared. */
    SPI->lpp_spi_config |= ctrlMask;
    if (isTx)
    {
        while ((SPI->lpp_spi_status & CY_U3P_LPP_SPI_TX_DONE) == 0);
    }
    if (isRx)
    {
        while ((SPI->lpp_spi_status & CY_U3P_LPP_SPI_RX_DATA) != 0);
    }
    SPI->lpp_spi_config &= ~ctrlMask;

    /* Clear all interrupts and re-enable them. */
    SPI->lpp_spi_intr |= CY_U3P_LPP_SPI_TX_DONE;
    SPI->lpp_spi_intr_mask = intrMask;

    return CY_U3P_SUCCESS;
}

/* 
 * Aserts / deasserts the SSN line.
 */
CyU3PReturnStatus_t
CyU3PSpiSetSsnLine (CyBool_t isHigh)
{
    uint32_t temp, status;

    status = CyU3PSpiGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Check if SSN is fw controlled. */
    temp = SPI->lpp_spi_config;
    if ((((temp & CY_U3P_LPP_SPI_SSNCTRL_MASK) >> CY_U3P_LPP_SPI_SSNCTRL_POS) !=
        CY_U3P_SPI_SSN_CTRL_FW) || (temp & CY_U3P_LPP_SPI_DESELECT))
    {
        CyU3PSpiReleaseLock();
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    if (isHigh)
    {
        SPI->lpp_spi_config |= CY_U3P_LPP_SPI_SSN_BIT;
    }
    else
    {
        SPI->lpp_spi_config &= ~CY_U3P_LPP_SPI_SSN_BIT;
    }

    CyU3PSpiReleaseLock();

    return CY_U3P_SUCCESS;
}

/*
 * Transmits data word by word over
 * the SPI interface.
 */
CyU3PReturnStatus_t 
CyU3PSpiTransmitWords (
                       uint8_t *data, 
                       uint32_t byteCount)
{
    uint8_t  wordLen;
    uint32_t intrMask, i;
    uint32_t temp, timeout;
    CyU3PReturnStatus_t status;

    if (!glIsSpiConfigured)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (byteCount == 0)
    {
        return CY_U3P_SUCCESS;
    }
    if (data == NULL)
    {   
        return CY_U3P_ERROR_NULL_POINTER;
    }

    status = CyU3PSpiGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Get the wordLen from the config register and convert it to byte length. */
    temp = SPI->lpp_spi_config;
    wordLen = ((temp & CY_U3P_LPP_SPI_WL_MASK) >> CY_U3P_LPP_SPI_WL_POS);
    if ((wordLen & 0x07) != 0)
    {
        wordLen = (wordLen >> 3) + 1;
    }
    else
    {
        wordLen = (wordLen >> 3);
    }
    if ((byteCount % wordLen) != 0)
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Check if the DMA mode is running. */
    if ((temp & (CY_U3P_LPP_SPI_DMA_MODE | CY_U3P_LPP_SPI_ENABLE))
        == (CY_U3P_LPP_SPI_DMA_MODE | CY_U3P_LPP_SPI_ENABLE))
    {
        status = CY_U3P_ERROR_ALREADY_STARTED;
    }

    if (status != CY_U3P_SUCCESS)
    {
        CyU3PSpiReleaseLock();
        return status;
    }

    CyU3PSpiResetFifo (CyTrue, CyFalse);

    /* Disable interrupts. */
    intrMask = SPI->lpp_spi_intr_mask;
    SPI->lpp_spi_intr_mask = 0;

    /* Enable the TX. */
    SPI->lpp_spi_config |= CY_U3P_LPP_SPI_TX_ENABLE;

    /* Re-enable SPI block. */
    SPI->lpp_spi_config |= CY_U3P_LPP_SPI_ENABLE;

    for (i = 0; i < byteCount; i += wordLen)
    {
        /* Wait for the tx_space bit in status register */
        timeout = CY_U3P_SPI_TIMEOUT;
        while (!(SPI->lpp_spi_status & CY_U3P_LPP_SPI_TX_SPACE))
        {
            if (timeout-- == 0)
            {
                status = CY_U3P_ERROR_TIMEOUT;
                break;
            }
        }

        if (status != CY_U3P_SUCCESS)
        {
            break;
        }

        /* Copy data to be written into local variable.
        * The padding required is to nearest byte. 
        * Do fallthrough switch instead of a loop. */
        temp = 0;
        switch (wordLen)
        {
        case 4:
            temp = (data[i + 3] << 24);
        case 3:
            temp |= (data[i + 2] << 16);
        case 2:
            temp |= (data[i + 1] << 8);
        case 1:
            temp |= data[i];
        default:
            break;
        }

        SPI->lpp_spi_egress_data = temp;

        /* wait for the TX_DONE */
        timeout = CY_U3P_SPI_TIMEOUT;
        while (!(SPI->lpp_spi_intr & CY_U3P_LPP_SPI_TX_DONE))
        {
            if (timeout-- == 0)
            {
                status = CY_U3P_ERROR_TIMEOUT;
                break;
            }
        }

        if (status != CY_U3P_SUCCESS)
        {
            break;
        }

        /* Clear the TX_DONE interrupt. */
        SPI->lpp_spi_intr = CY_U3P_LPP_SPI_TX_DONE;
    }

    /* Disable the TX. */
    SPI->lpp_spi_config &= ~CY_U3P_LPP_SPI_TX_ENABLE;

    /* Clear all interrupts and restore interrupt mask. */
    SPI->lpp_spi_intr |= CY_U3P_LPP_SPI_TX_DONE;
    SPI->lpp_spi_intr_mask = intrMask;

    /* Wait until the SPI block is no longer busy and disable. */
    while ((SPI->lpp_spi_status & CY_U3P_LPP_SPI_BUSY) != 0);
    SPI->lpp_spi_config &= ~CY_U3P_LPP_SPI_ENABLE;

    CyU3PSpiReleaseLock();

    return status;
}

/*
 * Receive data word by word over the SPI interface.
 */
CyU3PReturnStatus_t 
CyU3PSpiReceiveWords (
                      uint8_t *data,
                      uint32_t byteCount)
{
    uint8_t  wordLen;
    uint32_t intrMask;
    uint32_t i, temp, timeout;
    CyU3PReturnStatus_t status;

    if (!glIsSpiConfigured)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (byteCount == 0)
    {
        return CY_U3P_SUCCESS;
    }
    if (data == NULL)
    {   
        return CY_U3P_ERROR_NULL_POINTER;
    }

    status = CyU3PSpiGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Get the wordLen from the config register and convert it to byte length. */
    temp = SPI->lpp_spi_config;
    wordLen = ((temp & CY_U3P_LPP_SPI_WL_MASK) >> CY_U3P_LPP_SPI_WL_POS);
    if ((wordLen & 0x07) != 0)
    {
        wordLen = (wordLen >> 3) + 1;
    }
    else
    {
        wordLen = (wordLen >> 3);
    }
    if ((byteCount % wordLen) != 0)
    {
        status = CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Check if the DMA mode is running. */
    if ((temp & (CY_U3P_LPP_SPI_DMA_MODE | CY_U3P_LPP_SPI_ENABLE))
        == (CY_U3P_LPP_SPI_DMA_MODE | CY_U3P_LPP_SPI_ENABLE))
    {
        status = CY_U3P_ERROR_ALREADY_STARTED;
    }

    if (status != CY_U3P_SUCCESS)
    {
        CyU3PSpiReleaseLock();
        return status;
    }

    CyU3PSpiResetFifo (CyTrue, CyTrue);

    /* Disable interrupts. */
    intrMask = SPI->lpp_spi_intr_mask;
    SPI->lpp_spi_intr_mask = 0;

    /* Enable TX and RX. */
    SPI->lpp_spi_config |= (CY_U3P_LPP_SPI_TX_ENABLE | CY_U3P_LPP_SPI_RX_ENABLE);

    /* Re-enable SPI block. */
    SPI->lpp_spi_config |= CY_U3P_LPP_SPI_ENABLE;

    for (i = 0; i < byteCount; i += wordLen)
    {
        /* Wait for the tx_space bit in status register */
        timeout = CY_U3P_SPI_TIMEOUT;
        while (!(SPI->lpp_spi_status & CY_U3P_LPP_SPI_TX_SPACE))
        {
            if (timeout-- == 0)
            {
                status = CY_U3P_ERROR_TIMEOUT;
                break;
            }
        }

        if (status != CY_U3P_SUCCESS)
        {
            break;
        }

        /* Transmit zero. */
        SPI->lpp_spi_egress_data = 0;

        /* wait for the TX_DONE and RX_DATA. */
        timeout = CY_U3P_SPI_TIMEOUT;
        temp = CY_U3P_LPP_SPI_RX_DATA | CY_U3P_LPP_SPI_TX_DONE;
        while ((SPI->lpp_spi_intr & temp) != temp)
        {
            if (timeout-- == 0)
            {
                status = CY_U3P_ERROR_TIMEOUT;
                break;
            }
        }

        if (status != CY_U3P_SUCCESS)
        {
            break;
        }

        /* Clear interrupt bits. */
        SPI->lpp_spi_intr = temp;

        /* Copy the data from the fifo. The padding
        * required is to nearest byte. Do fallthrough
        * switch instead of a loop. */
        temp = SPI->lpp_spi_ingress_data;
        switch (wordLen)
        {
        case 4:
            data[i + 3] = (uint8_t)((temp >> 24) & 0xFF);
        case 3:
            data[i + 2] = (uint8_t)((temp >> 16) & 0xFF);
        case 2:
            data[i + 1] = (uint8_t)((temp >> 8) & 0xFF);
        case 1:
            data[i] = (uint8_t)(temp & 0xFF);
        default:
            break;
        }
    }

    /* Disable the TX and RX. */
    SPI->lpp_spi_config &= ~(CY_U3P_LPP_SPI_TX_ENABLE | CY_U3P_LPP_SPI_RX_ENABLE);

    /* Clear all interrupts and restore interrupt mask. */
    SPI->lpp_spi_intr |= CY_U3P_LPP_SPI_TX_DONE;
    SPI->lpp_spi_intr_mask = intrMask;

    /* Wait until the SPI block is no longer busy and disable. */
    while ((SPI->lpp_spi_status & CY_U3P_LPP_SPI_BUSY) != 0);
    SPI->lpp_spi_config &= ~CY_U3P_LPP_SPI_ENABLE;

    CyU3PSpiReleaseLock();

    return status;
}

/* 
 * Enables the transferring data.
 */
CyU3PReturnStatus_t
CyU3PSpiSetBlockXfer (
                      uint32_t txSize,
                      uint32_t rxSize)
{
    uint32_t temp;

    if (!glIsSpiConfigured)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if ((txSize == 0) && (rxSize == 0))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    temp = CyU3PSpiGetLock ();
    if (temp != CY_U3P_SUCCESS)
    {
        return temp;
    }

    /* Allow starting of DMA transfer if TX and RX is idle. */
    temp = SPI->lpp_spi_config;
    if ((temp & (CY_U3P_LPP_SPI_DMA_MODE | CY_U3P_LPP_SPI_ENABLE))
        == (CY_U3P_LPP_SPI_DMA_MODE | CY_U3P_LPP_SPI_ENABLE))
    {
        /* This is a continuation of the previous transfer. */
        if (((SPI->lpp_spi_tx_byte_count != 0) && (temp & CY_U3P_LPP_SPI_TX_ENABLE)) ||
            ((SPI->lpp_spi_rx_byte_count != 0) && (temp & CY_U3P_LPP_SPI_RX_ENABLE)))
        {
            /* This is a single instance. So release lock here. */
            CyU3PSpiReleaseLock();
            return CY_U3P_ERROR_ALREADY_STARTED;
        }
    }

    CyU3PSpiResetFifo (CyTrue, CyTrue);

    /* Enable DMA mode. */
    SPI->lpp_spi_config |= CY_U3P_LPP_SPI_DMA_MODE;

    /* Update the counters. */
    SPI->lpp_spi_tx_byte_count = txSize;
    SPI->lpp_spi_rx_byte_count = rxSize;

    temp = 0;
    if (txSize != 0)
    {
        temp |= CY_U3P_LPP_SPI_TX_ENABLE;
    }
    if (rxSize != 0)
    {
        temp |= CY_U3P_LPP_SPI_RX_ENABLE;
    }

    /* Enable Tx and Rx as required. */
    SPI->lpp_spi_config |= temp;

    /* Enable the SPI block. */
    SPI->lpp_spi_config |= CY_U3P_LPP_SPI_ENABLE;

    CyU3PSpiReleaseLock();

    return CY_U3P_SUCCESS;
}

/* 
 * Disable the transferring data.
 */
CyU3PReturnStatus_t
CyU3PSpiDisableBlockXfer (
                          CyBool_t txDisable,
                          CyBool_t rxDisable)
{
    uint32_t temp;

    if (!glIsSpiConfigured)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    temp = CyU3PSpiGetLock ();
    if (temp != CY_U3P_SUCCESS)
    {
        return temp;
    }

    /* Fail request if DMA is not running. */
    temp = SPI->lpp_spi_config;
    if ((temp & (CY_U3P_LPP_SPI_DMA_MODE | CY_U3P_LPP_SPI_ENABLE))
        != (CY_U3P_LPP_SPI_DMA_MODE | CY_U3P_LPP_SPI_ENABLE))
    {
        /* This is a single instance. So release lock here. */
        CyU3PSpiReleaseLock();
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    if (txDisable)
    {
        temp &= ~CY_U3P_LPP_SPI_TX_ENABLE;
    }
    if (rxDisable)
    {
        temp &= ~CY_U3P_LPP_SPI_RX_ENABLE;
    }

    SPI->lpp_spi_config = temp;

    /* If both RX and TX are disabled, then disable the block. */
    if ((temp & (CY_U3P_LPP_SPI_TX_ENABLE | CY_U3P_LPP_SPI_RX_ENABLE)) == 0)
    {
        while ((SPI->lpp_spi_status & CY_U3P_LPP_SPI_BUSY) != 0);
        SPI->lpp_spi_config &= ~CY_U3P_LPP_SPI_ENABLE;
    }

    CyU3PSpiReleaseLock();

    return CY_U3P_SUCCESS;
}

/* Wait for data transfer */
CyU3PReturnStatus_t
CyU3PSpiWaitForBlockXfer (
                          CyBool_t isRead)
{
    uint32_t timeout, status;
    uint32_t mask, error_mask;

    if (!glIsSpiConfigured)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    status = CyU3PSpiGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    if (isRead)
    {
        mask = CY_U3P_LPP_SPI_RX_DONE;
    }
    else
    {
        mask = CY_U3P_LPP_SPI_TX_DONE;
    }
    error_mask = CY_U3P_LPP_SPI_ERROR;
    timeout = CY_U3P_SPI_TIMEOUT;
    while (timeout != 0)
    {
        /* Check for the DONE bit and error interrupt */
        if (SPI->lpp_spi_intr & mask)
        {
            /* Data transfer is done */
            SPI->lpp_spi_intr = mask;
            break;
        }

        if (SPI->lpp_spi_intr & error_mask)
        {
            /* Error has happened */
            SPI->lpp_spi_intr = error_mask;
            status = CY_U3P_ERROR_FAILURE;
            break;
        }
        CyU3PBusyWait (10);
        timeout--;
    }

    if (timeout == 0)
    {
        status = CY_U3P_ERROR_TIMEOUT;
    }

    CyU3PSpiReleaseLock();

    return status;
}

/*[]*/

