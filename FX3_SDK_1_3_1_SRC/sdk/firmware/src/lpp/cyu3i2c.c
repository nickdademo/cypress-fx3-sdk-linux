/*
 ## Cypress EZ-USB FX3 Source file (cyu3i2c.c)
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

#include <cyu3i2c.h>
#include <cyu3error.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3gpio.h>
#include <cyu3utils.h>
#include <cyu3lpp.h>
#include <i2c_regs.h>

/*
 * @@I2C 
 * Summary
 * I2C driver and convenience API for the EZ-USB FX3 device.
 */

#define CY_U3P_I2C_DEFAULT_BIT_RATE     (100000)                /* Default bit rate. */
#define CY_U3P_I2C_TIMEOUT              (0x7FFF)                /* Default response read timeout. */

static CyBool_t glIsI2cDma = CyFalse;                           /* Whether the I2C block is in DMA mode. */
static CyBool_t glIsI2cConfigured = CyFalse;                    /* Whether the I2C block has been configured. */
static CyBool_t glIsI2cActive = CyFalse;     
static CyU3PI2cIntrCb_t glI2cIntrCb = NULL;                     /* Callback for I2C event notifications. */
static CyU3PMutex glI2cLock;                    /* Mutex lock for I2C block access. */

/* This variable stores the I2C Status register when an error is encountered.
 * This is read and processed in I2CGetErrorCode API.
 * This is cleared in Init, DeInit, SetConfig and SendCommand
 */
static uint32_t glI2cStatus;

/* Get the lock for the I2C */
static CyU3PReturnStatus_t
CyU3PI2cGetLock (
                 void)
{
    uint32_t waitOption = 0;

    if (!glIsI2cActive)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (CyU3PThreadIdentify () != NULL)
    {
        waitOption = CY_U3P_I2C_DEFAULT_LOCK_TIMEOUT;
    }

    if (CyU3PMutexGet (&glI2cLock, waitOption) != CY_U3P_SUCCESS)
    {
        return CY_U3P_ERROR_MUTEX_FAILURE;
    }

    return CY_U3P_SUCCESS;
}

static CyU3PReturnStatus_t 
CyU3PI2cReleaseLock(
                    void)
{
    if (!glIsI2cActive)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (CyU3PMutexPut (&glI2cLock) != CY_U3P_SUCCESS)
    {
        return CY_U3P_ERROR_MUTEX_FAILURE;
    }
    return CY_U3P_SUCCESS;	
}

/* Register for the I2c Call Back function */
void 
CyU3PRegisterI2cCallBack (
                          CyU3PI2cIntrCb_t i2cIntrCb)
{
    glI2cIntrCb = i2cIntrCb;
}

void
CyU3PI2cInt_ThreadHandler (
                           void)
{
    uint32_t mask, status;

    status = CyU3PI2cGetLock ();
    if (status != CY_U3P_SUCCESS)
    {        
        return;
    }

    /* Read out the interrupts and clear them. */
    mask = I2C->lpp_i2c_intr & I2C->lpp_i2c_intr_mask;
    I2C->lpp_i2c_intr = mask;

    if (glI2cIntrCb == NULL)
    {     
        CyU3PI2cReleaseLock ();
        return;
    }
    
    if (mask & CY_U3P_LPP_I2C_RX_DONE)
    {
        glI2cIntrCb (CY_U3P_I2C_EVENT_RX_DONE, (CyU3PI2cError_t)0);
    }

    if (mask & CY_U3P_LPP_I2C_TX_DONE)
    {
        glI2cIntrCb (CY_U3P_I2C_EVENT_TX_DONE, (CyU3PI2cError_t)0);
    }

    if (mask & CY_U3P_LPP_I2C_TIMEOUT)
    {
        glI2cIntrCb (CY_U3P_I2C_EVENT_TIMEOUT, (CyU3PI2cError_t)0);
    }

    if (mask & CY_U3P_LPP_I2C_LOST_ARBITRATION)
    {
        glI2cIntrCb (CY_U3P_I2C_EVENT_LOST_ARBITRATION, (CyU3PI2cError_t)0);
    }

    if (mask & CY_U3P_LPP_I2C_ERROR)
    {
        glI2cIntrCb (CY_U3P_I2C_EVENT_ERROR, (CyU3PI2cError_t)((I2C->lpp_i2c_status & 
            CY_U3P_LPP_I2C_ERROR_CODE_MASK) >> CY_U3P_LPP_I2C_ERROR_CODE_POS));
    }
    
    CyU3PI2cReleaseLock ();
}

/* 
 * This function initializes the I2C module
 */
CyU3PReturnStatus_t
CyU3PI2cInit (
              void)
{
    CyU3PReturnStatus_t status;
    /* Check the IO matrix */
    if (!CyU3PIsLppIOConfigured(CY_U3P_LPP_I2C))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (glIsI2cActive)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    /* Set the clock frequency. This should preceed
    * the I2C power up */
    status = CyU3PI2cSetClock (CY_U3P_I2C_DEFAULT_BIT_RATE);
    if(status != CY_U3P_SUCCESS)	
    {
        return status;
    }

    /* Initialize the LPP block if not already started. */
    status = CyU3PLppInit (CY_U3P_LPP_I2C,CyU3PI2cInt_ThreadHandler);
    if(status != CY_U3P_SUCCESS)
    {
        return status;
    }
    CyU3PMutexCreate (&glI2cLock, CYU3P_NO_INHERIT);

    /* Power on the I2C module */
    I2C->lpp_i2c_power = 0;
    CyU3PBusyWait (10);
    I2C->lpp_i2c_power |= CY_U3P_LPP_I2C_RESETN;
    while (!(I2C->lpp_i2c_power & CY_U3P_LPP_I2C_ACTIVE));

    /* Clear the error status */
    glI2cStatus = 0;

    /* Mark the module as active. */
    glIsI2cActive = CyTrue;

    return CY_U3P_SUCCESS;
}

/*
 * This function de-initializes the I2C 
 */
CyU3PReturnStatus_t
CyU3PI2cDeInit (
                void)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    if (!glIsI2cActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Reset the reset bit */
    I2C->lpp_i2c_power &= ~(CY_U3P_LPP_I2C_RESETN);
    CyU3PBusyWait (10);

    /* Mark the block as disabled. */
    glIsI2cActive = CyFalse;
    glIsI2cDma = CyFalse;
    glIsI2cConfigured = CyFalse;

    /* Clear the error status */
    glI2cStatus = 0;

    /* Deinit the LPP Block */
    status = CyU3PLppDeInit (CY_U3P_LPP_I2C);

    CyU3PI2cStopClock();
    
    CyU3PMutexDestroy (&glI2cLock);

    return status;
}

/* 
 * configures and opens the I2C.
 */
CyU3PReturnStatus_t 
CyU3PI2cSetConfig (
                   CyU3PI2cConfig_t *config,
                   CyU3PI2cIntrCb_t cb)
{
    uint32_t temp, timeout;

    /* Check if the I2C is initiaized */
    if (!glIsI2cActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (config == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if ((!config->isDma) && (cb != NULL))
    {
        /* Callback is allowed only in DMA mode. */
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    temp = CyU3PI2cGetLock ();
    if (temp != CY_U3P_SUCCESS)
    {
        return temp;
    }

    /* Clear the error status */
    glI2cStatus = 0;

    /* Setup clock for the UART block according
    * to the baud rate. */
    temp = CyU3PI2cSetClock (config->bitRate);

    if (temp == CY_U3P_SUCCESS)
    {
        /* Disable and Clear the TX and RX_FIFO. */
        I2C->lpp_i2c_config = (CY_U3P_LPP_I2C_TX_CLEAR |
            CY_U3P_LPP_I2C_RX_CLEAR);

        /* Wait till the FIFOs becomes empty */
        timeout = CY_U3P_I2C_TIMEOUT;
        while ((I2C->lpp_i2c_status & CY_U3P_LPP_I2C_TX_DONE) == 0)
        {
            if (timeout-- == 0)
            {
                temp = CY_U3P_ERROR_BLOCK_FAILURE;
                break;
            }
        }
    }

    if (temp == CY_U3P_SUCCESS)
    {
        timeout = CY_U3P_I2C_TIMEOUT;
        while ((I2C->lpp_i2c_status & CY_U3P_LPP_I2C_RX_DATA) != 0)
        {
            if (timeout-- == 0)
            {
                temp = CY_U3P_ERROR_BLOCK_FAILURE;
                break;
            }
        }
    }

    if (temp != CY_U3P_SUCCESS)
    {
        CyU3PI2cReleaseLock();
        return temp;
    }

    I2C->lpp_i2c_config = 0;

    /* Update the configuration. */
    temp = 0;
    if (config->bitRate <= CY_U3P_I2C_DEFAULT_BIT_RATE)
    {
        /* We have to use clock with duty cycle of 50% */
        temp = CY_U3P_LPP_I2C_I2C_100KHZ;
    }

    /* Setting the transfer mode and the timeout register */
    if (config->isDma)
    {
        temp |= CY_U3P_LPP_I2C_DMA_MODE;
        glIsI2cDma = CyTrue;
        I2C->lpp_i2c_dma_timeout = config->dmaTimeout;
    }
    else
    {
        glIsI2cDma = CyFalse;
        I2C->lpp_i2c_dma_timeout = 0xFFFFU;
    }

    I2C->lpp_i2c_config = temp;

    I2C->lpp_i2c_socket = ((CY_U3P_LPP_SOCKET_I2C_CONS & CY_U3P_LPP_I2C_EGRESS_SOCKET_MASK) |
        ((CY_U3P_LPP_SOCKET_I2C_PROD << CY_U3P_LPP_I2C_INGRESS_SOCKET_POS) 
        & CY_U3P_LPP_I2C_INGRESS_SOCKET_MASK));

    I2C->lpp_i2c_timeout = config->busTimeout;
    /* Clear all stale interrupts. */
    I2C->lpp_i2c_intr = 0xFFFFFFFFU;

    CyU3PRegisterI2cCallBack(cb);
    if (cb != NULL)
    {
        /* Enable the interrupts. */
        I2C->lpp_i2c_intr_mask = (CY_U3P_LPP_I2C_RX_DONE |
            CY_U3P_LPP_I2C_TX_DONE | CY_U3P_LPP_I2C_TIMEOUT |
            CY_U3P_LPP_I2C_LOST_ARBITRATION | CY_U3P_LPP_I2C_ERROR);
    }
    else
    {
        /* Disable the interrupt */
        I2C->lpp_i2c_intr_mask = 0;
    }

    glIsI2cConfigured = CyTrue;

    /* Finally enable the block. */
    I2C->lpp_i2c_config |= CY_U3P_LPP_I2C_ENABLE;

    CyU3PI2cReleaseLock();

    return CY_U3P_SUCCESS;
}

static CyU3PReturnStatus_t
CyU3PI2cErrorRecovery (CyBool_t isClearFifo)
{
    uint32_t timeout, mask, clearMask = 0;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;


    /* Save the error status, mutex is already acquired by caller */
    glI2cStatus = I2C->lpp_i2c_status;

    /* Wait for the block to be idle before disabling. */
    timeout = CY_U3P_I2C_TIMEOUT;
    while ((I2C->lpp_i2c_status & CY_U3P_LPP_I2C_BUSY) && (timeout-- > 0));

    /* Save the interrupt mask and disable all interrupts. */
    mask = I2C->lpp_i2c_intr_mask;
    I2C->lpp_i2c_intr_mask = 0;
    I2C->lpp_i2c_intr = ~0;

    I2C->lpp_i2c_config &= ~CY_U3P_LPP_I2C_ENABLE;
    CyU3PBusyWait (5);

    if (isClearFifo)
    {
        I2C->lpp_i2c_byte_count = 0;

        if ((I2C->lpp_i2c_status & CY_U3P_LPP_I2C_TX_DONE) == 0)
        {
            clearMask = CY_U3P_LPP_I2C_TX_CLEAR;
        }
        if ((I2C->lpp_i2c_status & CY_U3P_LPP_I2C_RX_DATA) != 0)
        {
            clearMask = CY_U3P_LPP_I2C_RX_CLEAR;
        }

        /* Disable and Clear the TX and RX_FIFO. */
        I2C->lpp_i2c_config |= clearMask;

        /* Wait till the FIFOs becomes empty */
        timeout = CY_U3P_I2C_TIMEOUT;
        while ((I2C->lpp_i2c_status & CY_U3P_LPP_I2C_TX_DONE) == 0)
        {
            if (timeout-- == 0)
            {
                status = CY_U3P_ERROR_BLOCK_FAILURE;
                break;
            }
        }

        if (status == CY_U3P_SUCCESS)
        {
            timeout = CY_U3P_I2C_TIMEOUT;
            while ((I2C->lpp_i2c_status & CY_U3P_LPP_I2C_RX_DATA) != 0)
            {
                if (timeout-- == 0)
                {
                    status = CY_U3P_ERROR_BLOCK_FAILURE;
                    break;
                }
            }
        }

        /* Disable the bits. */
        I2C->lpp_i2c_config &= ~(CY_U3P_LPP_I2C_TX_CLEAR |
            CY_U3P_LPP_I2C_RX_CLEAR);
        CyU3PBusyWait (1);
    }

    I2C->lpp_i2c_config |= CY_U3P_LPP_I2C_ENABLE;
    CyU3PBusyWait (5);

    /* Clear all interrupts and re-enable interrupts. */
    I2C->lpp_i2c_intr = ~0;
    I2C->lpp_i2c_intr_mask = mask;

    return status;
}

/* Send the command for read and write */
static CyU3PReturnStatus_t
MyI2cSendCommand (
        CyU3PI2cPreamble_t *preamble,
        uint32_t byteCount,
        CyBool_t isRead)
{
    CyU3PReturnStatus_t status;
    uint32_t timeout;
    uint32_t value[2];

    /* Clear the error status */
    glI2cStatus = 0;

    /* If the I2C Bus-busy error is set, reset the I2C block. */
    if (I2C->lpp_i2c_status & CY_U3P_LPP_I2C_BUS_BUSY)
    {
        status = CyU3PI2cErrorRecovery (CyFalse);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }
    }

    /* Check if the bus and block are not busy. */
    timeout = CY_U3P_I2C_TIMEOUT;
    while ((I2C->lpp_i2c_status & (CY_U3P_LPP_I2C_BUSY | CY_U3P_LPP_I2C_BUS_BUSY)) && (timeout-- > 0));

    if (I2C->lpp_i2c_status & (CY_U3P_LPP_I2C_BUSY | CY_U3P_LPP_I2C_BUS_BUSY))
    {
        /* The I2c device is busy or our block has gotten stuck. */
        return CY_U3P_ERROR_TIMEOUT;
    }

    I2C->lpp_i2c_preamble_rpt = 0;  /* Disable repeat */

    /* Copy the preamble into a local array and then copy
     * to the register to prevent unaligned writes in register
     * area. */
    CyU3PMemCopy (((uint8_t *)(value)), preamble->buffer, preamble->length);

    /* Update the register. */
    I2C->lpp_i2c_preamble_data0 = value[0];
    I2C->lpp_i2c_preamble_data1 = value[1];

    /* Clear all conditions. */
    I2C->lpp_i2c_command = 0;
    /* Clear the interrupts */
    I2C->lpp_i2c_intr = ~0;
    /* Clear all sticky bits in status register. */
    I2C->lpp_i2c_status = CY_U3P_LPP_I2C_TIMEOUT | CY_U3P_LPP_I2C_LOST_ARBITRATION | CY_U3P_LPP_I2C_ERROR;

    /* Set the I2C byte count */
    I2C->lpp_i2c_byte_count = byteCount;

    /* Update the start stop locations. */
    I2C->lpp_i2c_preamble_ctrl = preamble->ctrlMask;

    if (isRead)
    {
        /* Set the command register */
        I2C->lpp_i2c_command = preamble->length /* length of preamble. */
            | CY_U3P_LPP_I2C_READ               /* Mark it as a read command */
            | CY_U3P_LPP_I2C_NAK_LAST           /* Send NAK at the last byte of read data. */
            | CY_U3P_LPP_I2C_START_FIRST        /* Send the start before first byte of preamble */
            | CY_U3P_LPP_I2C_STOP_LAST;         /* Send the start after last byte of data */
    }
    else
    {
        /* Set the command register */
        I2C->lpp_i2c_command = preamble->length /* length of preamble. */
            | CY_U3P_LPP_I2C_START_FIRST        /* Send the start before first byte of preamble */
            | CY_U3P_LPP_I2C_STOP_LAST;         /* Send the stop after last byte of data. */
    }

    /* Set the preamble_valid to start sending the command. This should be done separately. */
    I2C->lpp_i2c_command |= CY_U3P_LPP_I2C_PREAMBLE_VALID;
    return CY_U3P_SUCCESS;
}

/* Send the command for read and write */
CyU3PReturnStatus_t
CyU3PI2cSendCommand (
        CyU3PI2cPreamble_t *preamble,
        uint32_t byteCount,
        CyBool_t isRead)
{
    CyU3PReturnStatus_t status;

    if (!glIsI2cConfigured)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    /* Check the parameters. */
    if (preamble == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if ((preamble->length == 0) || (preamble->length > 8))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    status = CyU3PI2cGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    status = MyI2cSendCommand (preamble, byteCount, isRead);

    /* Release the lock. */
    CyU3PI2cReleaseLock();

    return status;
}

/*
 * Transmits data byte by byte over the I2C interface.
 */
CyU3PReturnStatus_t 
CyU3PI2cTransmitBytes (
        CyU3PI2cPreamble_t *preamble,
        uint8_t *data, 
        uint32_t byteCount,
        uint32_t retryCount)
{
    CyU3PReturnStatus_t status;
    uint32_t index, timeout, flag, temp;

    if (!glIsI2cConfigured)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (glIsI2cDma)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if ((data == NULL) || (preamble == NULL))
    {   
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if ((preamble->length == 0) || (preamble->length > 8))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (byteCount == 0)
    {
        return CY_U3P_SUCCESS;
    }

    status = CyU3PI2cGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    flag = CY_U3P_LPP_I2C_ERROR | CY_U3P_LPP_I2C_TIMEOUT | CY_U3P_LPP_I2C_LOST_ARBITRATION;

    do 
    {
        status = CY_U3P_SUCCESS;

        /* Copy the first data byte to the fifo before sending the command. */
        I2C->lpp_i2c_egress_data = data[0];

        /* Send the command to write in to the device */
        status = MyI2cSendCommand (preamble, byteCount, CyFalse);
        if (status != CY_U3P_SUCCESS)
        {
            break;
        }

        index = 1;
        while (index < byteCount)
        {
            timeout = CY_U3P_I2C_TIMEOUT;

            /* Wait for the tx_space or error bits. */
            while ((I2C->lpp_i2c_status & (flag | CY_U3P_LPP_I2C_TX_SPACE)) == 0)
            {
                if (timeout-- == 0)
                {
                    status = CY_U3P_ERROR_LOST_ARBITRATION;
                    break;
                }
            }

            if (I2C->lpp_i2c_status & flag)
            {
                /* An error has been encountered. Retry from the beginning. */
                status = CY_U3P_ERROR_FAILURE;
            }

            /* Break out of the loop if there was an error. */
            if (status != CY_U3P_SUCCESS)
            {
                break;
            }

            /* Add the next data byte */
            I2C->lpp_i2c_egress_data = data[index];
            index++;
        }

        if ((byteCount == index) && (status == CY_U3P_SUCCESS))
        {
            /* If this is the last data, wait for the FIFO to be drained. */
            timeout = CY_U3P_I2C_TIMEOUT;
            while ((I2C->lpp_i2c_intr & (flag | CY_U3P_LPP_I2C_TX_DONE)) == 0)
            {
                if (timeout-- == 0)
                {
                    status = CY_U3P_ERROR_LOST_ARBITRATION;
                    break;
                }
            }

            if (I2C->lpp_i2c_status & flag)
            {
                /* An error has been encountered. Retry from the beginning. */
                status = CY_U3P_ERROR_FAILURE;
            }
        }

        /* If there was an error, clear all failures. */
        if (status != CY_U3P_SUCCESS)
        {
            if (CyU3PI2cErrorRecovery (CyTrue))
            {
                CyU3PI2cReleaseLock();
                return CY_U3P_ERROR_BLOCK_FAILURE;
            }
        }

    } while ((status != CY_U3P_SUCCESS) && (retryCount-- != 0));

    /* Return correct error code. */
    if (status == CY_U3P_ERROR_FAILURE)
    {
        temp = I2C->lpp_i2c_status;
        if (temp & CY_U3P_LPP_I2C_TIMEOUT)
        {
            status = CY_U3P_ERROR_TIMEOUT;
        }
        if (temp & CY_U3P_LPP_I2C_LOST_ARBITRATION)
        {
            status = CY_U3P_ERROR_LOST_ARBITRATION;
        }
    }

    /* Release the lock. */
    CyU3PI2cReleaseLock();
    return status;
}

/*
 * Receive data byte by byte over the I2C interface
 */
CyU3PReturnStatus_t 
CyU3PI2cReceiveBytes (
                      CyU3PI2cPreamble_t *preamble,
                      uint8_t *data,
                      uint32_t byteCount,
                      uint32_t retryCount)
{
    CyU3PReturnStatus_t status;
    uint32_t temp;
    uint32_t index, flag;
    uint32_t timeout;

    if (!glIsI2cConfigured)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (glIsI2cDma)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if ((data == NULL) || (preamble == NULL))
    {   
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if ((preamble->length == 0) || (preamble->length > 8))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (byteCount == 0)
    {
        return CY_U3P_SUCCESS;
    }

    status = CyU3PI2cGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    flag = CY_U3P_LPP_I2C_ERROR | CY_U3P_LPP_I2C_TIMEOUT | CY_U3P_LPP_I2C_LOST_ARBITRATION;

    do 
    {
        /* Send the command to read */
        status = MyI2cSendCommand (preamble, byteCount, CyTrue);
        if (status != CY_U3P_SUCCESS)
        {
            break;
        }

        index = 0;

        while (index < byteCount)
        {
            /* Wait for the RX_DATA or error bits. */
            timeout = CY_U3P_I2C_TIMEOUT;
            while ((I2C->lpp_i2c_status & (flag | CY_U3P_LPP_I2C_RX_DATA)) == 0)
            {
                if (timeout-- == 0)
                {
                    status = CY_U3P_ERROR_LOST_ARBITRATION;
                    break;
                }
            }

            if (I2C->lpp_i2c_status & flag)
            {
                /* An error has been encountered. Retry from the beginning. */
                status = CY_U3P_ERROR_FAILURE;
            }

            if (status != CY_U3P_SUCCESS)
            {
                /* If there was an error, cleanup and retry. */
                if (CyU3PI2cErrorRecovery (CyTrue))
                {
                    /* There is some serious error. Do not retry. */
                    status = CY_U3P_ERROR_BLOCK_FAILURE;
                }
                break;
            }

            /* Copy the data from the fifo. */
            data[index] = I2C->lpp_i2c_ingress_data;
            index++;
        }

    } while ((status != CY_U3P_SUCCESS) && (retryCount-- != 0));

    /* Return correct error code. */
    if (status == CY_U3P_ERROR_FAILURE)
    {
        temp = I2C->lpp_i2c_status;
        if (temp & CY_U3P_LPP_I2C_TIMEOUT)
        {
            status = CY_U3P_ERROR_TIMEOUT;
        }
        if (temp & CY_U3P_LPP_I2C_LOST_ARBITRATION)
        {
            status = CY_U3P_ERROR_LOST_ARBITRATION;
        }
    }

    /* Release the lock. */
    CyU3PI2cReleaseLock();
    return status;
}

/* Wait for I2C to ACK on preamble. */
CyU3PReturnStatus_t
CyU3PI2cWaitForAck (
        CyU3PI2cPreamble_t *preamble,
        uint32_t retryCount)
{
    uint32_t timeout, temp, status;

    if (preamble == NULL)
        return CY_U3P_ERROR_NULL_POINTER;
    if ((preamble->length == 0) || (preamble->length > 8))
        return CY_U3P_ERROR_BAD_ARGUMENT;

    status = CyU3PI2cGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    do 
    {
        /* Send the preamble with no data phase. */
        status = MyI2cSendCommand (preamble, 0, CyFalse);
        if (status != CY_U3P_SUCCESS)
        {
            break;
        }

        /* Wait for the TX_DONE or error bits. */
        timeout = CY_U3P_I2C_TIMEOUT;
        while (timeout-- != 0)
        {
            temp = I2C->lpp_i2c_status;
            if (temp & (CY_U3P_LPP_I2C_ERROR | CY_U3P_LPP_I2C_TIMEOUT
                | CY_U3P_LPP_I2C_LOST_ARBITRATION))
            {
                /* An error has been encountered. Since this is a
                * poll function any error is similar to a timeout. Retry. */
                status = CY_U3P_ERROR_FAILURE;
                break;
            }

            if (((temp & CY_U3P_LPP_I2C_BUSY) == 0) &&
                ((I2C->lpp_i2c_command & CY_U3P_LPP_I2C_PREAMBLE_VALID) == 0))
            {
                /* Completed transmission successfully. */
                break;
            }
        }

        if (timeout == 0)
        {
            status = CY_U3P_ERROR_LOST_ARBITRATION;
        }

        /* In case of failure, cleanup and retry. */
        if (status != CY_U3P_SUCCESS)
        {
            if (CyU3PI2cErrorRecovery (CyFalse))
            {
                status = CY_U3P_ERROR_BLOCK_FAILURE;
                /* Do not retry when there is fatal failure. */
                break;
            }
        }

    } while ((status != CY_U3P_SUCCESS) && (retryCount-- != 0));

    /* Return correct error code. */
    if (status == CY_U3P_ERROR_FAILURE)
    {
        temp = I2C->lpp_i2c_status;
        if (temp & CY_U3P_LPP_I2C_TIMEOUT)
        {
            status = CY_U3P_ERROR_TIMEOUT;
        }
        if (temp & CY_U3P_LPP_I2C_LOST_ARBITRATION)
        {
            status = CY_U3P_ERROR_LOST_ARBITRATION;
        }
    }

    /* Release the lock. */
    CyU3PI2cReleaseLock();

    return status;
}

/* Wait for data transfer.
 */
CyU3PReturnStatus_t
CyU3PI2cWaitForBlockXfer (
                          CyBool_t isRead)
{
    uint32_t temp;
    uint32_t timeout, status;
    uint32_t mask, error_mask;

    if (!glIsI2cDma)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    status = CyU3PI2cGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    if (isRead)
    {
        mask = CY_U3P_LPP_I2C_RX_DONE;
    }
    else
    {
        mask = CY_U3P_LPP_I2C_TX_DONE;
    }

    error_mask = (CY_U3P_LPP_I2C_TIMEOUT | CY_U3P_LPP_I2C_ERROR |
        CY_U3P_LPP_I2C_LOST_ARBITRATION);

    timeout = CY_U3P_I2C_TIMEOUT;
    while (timeout != 0)
    {
        /* Check for the error bits */
        if (I2C->lpp_i2c_intr & error_mask)
        {
            /* Error has happened */
            status = CY_U3P_ERROR_FAILURE;
            break;
        }

        /* Check for transfer completion. */
        if (I2C->lpp_i2c_intr & mask)
        {
            /* Data transfer is done */
            break;
        }

        timeout--;
    }

    if (timeout == 0)
    {
        status = CY_U3P_ERROR_TIMEOUT;
    }
    else if (status != CY_U3P_SUCCESS)
    {        
        if (CyU3PI2cErrorRecovery (CyFalse))
        {
            status = CY_U3P_ERROR_BLOCK_FAILURE;
        }
    }
    else
    {
        /* Do nothing. */
    }

    /* Return correct error code. */
    if (status == CY_U3P_ERROR_FAILURE)
    {
        temp = I2C->lpp_i2c_status;
        if (temp & CY_U3P_LPP_I2C_TIMEOUT)
        {
            status = CY_U3P_ERROR_TIMEOUT;
        }
        if (temp & CY_U3P_LPP_I2C_LOST_ARBITRATION)
        {
            status = CY_U3P_ERROR_LOST_ARBITRATION;
        }
    }

    /* Release the lock. */
    CyU3PI2cReleaseLock();

    return status;
}

CyU3PReturnStatus_t
CyU3PI2cGetErrorCode (CyU3PI2cError_t *error_p)
{
    uint32_t temp;
    CyU3PReturnStatus_t status;

    if (error_p == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    status = CyU3PI2cGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* use the stored I2C Status reg value */
    temp = glI2cStatus;

    if (temp & CY_U3P_LPP_I2C_ERROR)
    {
        *error_p = (CyU3PI2cError_t)((temp & CY_U3P_LPP_I2C_ERROR_CODE_MASK)
            >> CY_U3P_LPP_I2C_ERROR_CODE_POS);
    }
    else
    {
        status = CY_U3P_ERROR_NOT_STARTED;
        *error_p = (CyU3PI2cError_t)0;
    }

    /* Release the lock. */
    CyU3PI2cReleaseLock();

    return status;
}

/* [] */

