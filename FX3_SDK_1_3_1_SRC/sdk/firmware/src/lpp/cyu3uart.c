/*
 ## Cypress EZ-USB FX3 Source file (cyu3uart.c)
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

#include <cyu3uart.h>
#include <cyu3error.h>
#include <cyu3dma.h>
#include <cyu3lpp.h>
#include <cyu3system.h>
#include <cyu3utils.h>
#include <uart_regs.h>

/* @@UART
 * Summary
 * UART driver and convenience API for the EZ-USB FX3 device.
 */

/* Default baud rate used by the UART driver. */
#define CY_U3P_UART_DEFAULT_BAUD_RATE           (9600)

/* Default timeout value in waiting for a response from the UART registers. */
#define CY_U3P_UART_TIMEOUT                     (0xFFFFF)

static CyBool_t glIsUartDma = CyFalse;          /* Whether UART is in DMA mode. */
static CyBool_t glIsUartConfigured = CyFalse;   /* Whether the UART module has been configured. */
static CyBool_t glIsUartActive = CyFalse; 
static CyU3PUartIntrCb_t glUartIntrCb = NULL;   /* Callback for UART event notifications. */
static CyU3PMutex glUartLock;                   /* Mutex lock for UART access APIs. */

/* Get the mutex lock for the Uart */ 
static CyU3PReturnStatus_t
CyU3PUartGetLock (
                  void)
{
    uint32_t waitOption = 0;

    if (!glIsUartActive)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (CyU3PThreadIdentify () != NULL)
    {
        waitOption = CY_U3P_UART_DEFAULT_LOCK_TIMEOUT;
    }

    if (CyU3PMutexGet (&glUartLock, waitOption) != CY_U3P_SUCCESS)
    {
        return CY_U3P_ERROR_MUTEX_FAILURE;
    }

    return CY_U3P_SUCCESS;
} 

/* Release the mutex lock for the Uart */ 
static CyU3PReturnStatus_t 
CyU3PUartReleaseLock(
                     void)
{
    if (!glIsUartActive)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (CyU3PMutexPut (&glUartLock) != CY_U3P_SUCCESS)
    {
        return CY_U3P_ERROR_MUTEX_FAILURE;
    }
    return CY_U3P_SUCCESS;		
}

/* Register Call back function for UART */
void 
CyU3PRegisterUartCallBack (
                           CyU3PUartIntrCb_t uartIntrCb)
{
    glUartIntrCb = uartIntrCb;
}

void
CyU3PUartInt_ThreadHandler (
                            void)
{
    uint32_t mask, status;

    status = CyU3PUartGetLock ();
    if (status != CY_U3P_SUCCESS)
    {    
        return;
    }

    /* Read out the interrupts and clear them. */
    mask = UART->lpp_uart_intr & UART->lpp_uart_intr_mask;
    UART->lpp_uart_intr = mask;

    if (glUartIntrCb == NULL)
    {     
        CyU3PUartReleaseLock ();
        return;
    }
    if (mask & CY_U3P_LPP_UART_RX_DONE)
    {
        glUartIntrCb (CY_U3P_UART_EVENT_RX_DONE, (CyU3PUartError_t)0);
    }

    if (mask & CY_U3P_LPP_UART_TX_DONE)
    {
        glUartIntrCb (CY_U3P_UART_EVENT_TX_DONE, (CyU3PUartError_t)0);
    }

    if (mask & CY_U3P_LPP_UART_ERROR)
    {
        glUartIntrCb (CY_U3P_UART_EVENT_ERROR, (CyU3PUartError_t)((UART->lpp_uart_status & 
            CY_U3P_LPP_UART_ERROR_CODE_MASK) >> CY_U3P_LPP_UART_ERROR_CODE_POS));
    }
    CyU3PUartReleaseLock ();
}

/*
 * This function initializes the UART 
 */
CyU3PReturnStatus_t
CyU3PUartInit (
               void)
{
    CyU3PReturnStatus_t status;
    /* Check the IO matrix */
    if (!CyU3PIsLppIOConfigured(CY_U3P_LPP_UART))
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (glIsUartActive)
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    CyU3PMutexCreate (&glUartLock, CYU3P_NO_INHERIT);

    /* Set the clock to a default value.
    * This should prcede the UART power up*/
    status = CyU3PUartSetClock (CY_U3P_UART_DEFAULT_BAUD_RATE);
    if(status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Identify if the LPP block has been initialized. */
    status = CyU3PLppInit (CY_U3P_LPP_UART,CyU3PUartInt_ThreadHandler);	
    if(status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Hold the UART block in reset. */
    UART->lpp_uart_power &= ~(CY_U3P_LPP_UART_RESETN);
    CyU3PBusyWait (10);
    UART->lpp_uart_power |= (CY_U3P_LPP_UART_RESETN);

    /* Wait for the active bit to be asserted by the hardware */
    while (!(UART->lpp_uart_power & CY_U3P_LPP_UART_ACTIVE));

    /* Mark the module as active. */
    glIsUartActive = CyTrue;

    return CY_U3P_SUCCESS;
}

/*
 * This function de-initializes the UART 
 */
CyU3PReturnStatus_t
CyU3PUartDeInit(
                void)
{
    CyU3PReturnStatus_t status=CY_U3P_SUCCESS;
    if (!glIsUartActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Power off UART block. */
    UART->lpp_uart_power &= ~(CY_U3P_LPP_UART_RESETN);
    CyU3PBusyWait (10);

    /* Mark the block as disabled. */
    glIsUartActive = CyFalse;
    glIsUartConfigured = CyFalse;
    glIsUartDma = CyFalse;

    /* Identify if the LPP block has to be disabled. */
    status = CyU3PLppDeInit (CY_U3P_LPP_UART);   

    /* Disable the UART clock */
    CyU3PUartStopClock();

    CyU3PMutexDestroy (&glUartLock);
    return status;
}

/* 
 * configures and opens the UART  
 */
CyU3PReturnStatus_t
CyU3PUartSetConfig (
                    CyU3PUartConfig_t *config,
                    CyU3PUartIntrCb_t cb)
{
    uint32_t regVal = 0, status;

    /* Check if the UART is initiaized */
    if (!glIsUartActive)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    /* Check for parameter validity. */
    if (config == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }
    if (!(config->rxEnable | config->txEnable))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (config->parity >= CY_U3P_UART_NUM_PARITY)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((config->stopBit != CY_U3P_UART_ONE_STOP_BIT) && (config->stopBit != CY_U3P_UART_TWO_STOP_BIT))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if ((!config->isDma) && (cb != NULL))
    {
        /* Callback is allowed only in DMA mode. */
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    status = CyU3PUartGetLock ();    
    if (status != CY_U3P_SUCCESS)
    {
        return CY_U3P_ERROR_MUTEX_FAILURE;
    }

    /* Setup clock for the UART block according
    * to the baud rate. */
    status = CyU3PUartSetClock (config->baudRate);
    if (status != CY_U3P_SUCCESS)
    {
        /* Release lock here since this is the only instance
        * of error. */
        CyU3PUartReleaseLock();
        return status;
    }

    /* Disable the UART before changing any register */
    UART->lpp_uart_config &= ~(CY_U3P_LPP_UART_ENABLE);

    if (config->rxEnable)
    {
        regVal |= (CY_U3P_LPP_UART_RTS | CY_U3P_LPP_UART_RX_ENABLE);
    }
    if (config->txEnable)
    {
        regVal |= CY_U3P_LPP_UART_TX_ENABLE;
    }
    if (config->flowCtrl)
    {
        regVal |= (CY_U3P_LPP_UART_TX_FLOW_CTRL_ENBL |
            CY_U3P_LPP_UART_RX_FLOW_CTRL_ENBL);
    }
    if (config->isDma)
    {
        regVal |= CY_U3P_LPP_UART_DMA_MODE;
        glIsUartDma = CyTrue;
    }
    else
    {
        glIsUartDma = CyFalse;
    }

    regVal |= (config->stopBit << CY_U3P_LPP_UART_STOP_BITS_POS) &
        CY_U3P_LPP_UART_STOP_BITS_MASK;

    switch (config->parity)
    {
    case CY_U3P_UART_EVEN_PARITY:
        regVal |= CY_U3P_LPP_UART_PARITY;
        break;
    case CY_U3P_UART_ODD_PARITY:
        regVal |= (CY_U3P_LPP_UART_PARITY | CY_U3P_LPP_UART_PARITY_ODD);
        break;
    default:
        break;
    }

    /* Set timing when to sample for RX input */
    regVal |= 7 << CY_U3P_LPP_UART_RX_POLL_POS;
    /* Update the configuration. */
    UART->lpp_uart_config = regVal;

    /* Update the UART DMA sockets. */
    UART->lpp_uart_socket = ((CY_U3P_LPP_SOCKET_UART_CONS & 
        CY_U3P_LPP_UART_EGRESS_SOCKET_MASK) | 
        ((CY_U3P_LPP_SOCKET_UART_PROD & CY_U3P_LPP_UART_EGRESS_SOCKET_MASK) 
        << CY_U3P_LPP_UART_INGRESS_SOCKET_POS));

    CyU3PRegisterUartCallBack(cb);
    if (cb != NULL)
    {
        /* Enable the interrupts. */
        UART->lpp_uart_intr_mask = (CY_U3P_LPP_UART_RX_DONE |
            CY_U3P_LPP_UART_TX_DONE | CY_U3P_LPP_UART_BREAK |
            CY_U3P_LPP_UART_ERROR);        
    }
    else
    {
        /* Disable the interrupt */
        UART->lpp_uart_intr_mask = 0;
    }

    glIsUartConfigured = CyTrue;

    /* Enable the UART only at the end. */
    UART->lpp_uart_config |= CY_U3P_LPP_UART_ENABLE;

    CyU3PUartReleaseLock();

    return CY_U3P_SUCCESS;
}

/*
 * Sets registers for dma egress transfer 
 */
CyU3PReturnStatus_t 
CyU3PUartTxSetBlockXfer (
                         uint32_t txSize)
{
    CyU3PReturnStatus_t status;

    /* Lock is not acquired as this is only a read. */
    if (!glIsUartDma)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    status = CyU3PUartGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    UART->lpp_uart_tx_byte_count = txSize;

    CyU3PUartReleaseLock();

    return CY_U3P_SUCCESS;
}

/*
 * Sets registers for dma ingress transfer 
 */
CyU3PReturnStatus_t 
CyU3PUartRxSetBlockXfer (
                         uint32_t rxSize)
{
    CyU3PReturnStatus_t status;

    /* Lock is not acquired as this is only a read. */
    if (!glIsUartDma)
    {
        return CY_U3P_ERROR_NOT_CONFIGURED;
    }

    status = CyU3PUartGetLock ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    UART->lpp_uart_rx_byte_count = rxSize;

    CyU3PUartReleaseLock();
    return CY_U3P_SUCCESS;
}

/*
 * Transmits data byte by byte over the UART interface
 */
uint32_t
CyU3PUartTransmitBytes (
                        uint8_t *data_p,
                        uint32_t count,
                        CyU3PReturnStatus_t *status)
{
    uint32_t timeout;
    int32_t i;
    CyU3PReturnStatus_t temp = CY_U3P_SUCCESS;

    if (!glIsUartConfigured)
    {
        temp= CY_U3P_ERROR_NOT_CONFIGURED;
    }
    /* Lock is not acquired as this is only a read. */
    if (glIsUartDma)
    {
        temp = CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (data_p == NULL)
    {
        temp = CY_U3P_ERROR_NULL_POINTER;
    }
    if (count == 0)
    {
        temp = CY_U3P_ERROR_BAD_ARGUMENT;
    }
    if (temp != CY_U3P_SUCCESS)
    {
        if (status != NULL)
        {
            *status = temp;
        }
        return 0;
    }

    temp = CyU3PUartGetLock ();
    if (temp != CY_U3P_SUCCESS)
    {
        if (status != NULL)
        {
            *status = temp;
        }
        return 0;
    }

    temp =CY_U3P_SUCCESS;
    for (i = 0; i < count; i++)
    {
        timeout = CY_U3P_UART_TIMEOUT;
        UART->lpp_uart_intr = CY_U3P_LPP_UART_TX_DONE;
        UART->lpp_uart_egress_data = data_p[i];

        /* Wait for the transfer to be done */
        while (!((UART->lpp_uart_intr & CY_U3P_LPP_UART_TX_DONE) ||
            timeout == 0))
        {
            timeout--;
        }

        if (timeout == 0)
        {
            temp = CY_U3P_ERROR_TIMEOUT;
            break;
        }
    }

    CyU3PUartReleaseLock();

    if (status != NULL)
    {
        *status = temp;
    }
    return i;
}

/*
 * Receives data byte by byte over the UART interface
 */
uint32_t 
CyU3PUartReceiveBytes (
                       uint8_t *data_p,
                       uint32_t count,
                       CyU3PReturnStatus_t *status)
{
    uint32_t timeout;
    int32_t i;
    CyU3PReturnStatus_t temp = CY_U3P_SUCCESS;

    if (!glIsUartConfigured)
    {
        temp= CY_U3P_ERROR_NOT_CONFIGURED;
    }
    /* Lock is not acquired as this is only a read. */
    if (glIsUartDma)
    {
        temp = CY_U3P_ERROR_NOT_CONFIGURED;
    }
    if (data_p == NULL)
    {
        temp = CY_U3P_ERROR_NULL_POINTER;
    }
    if (count == 0)
    {
        temp = CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if (temp != CY_U3P_SUCCESS)
    {
        if (status != NULL)
        {
            *status = temp;
        }
        return 0;
    }

    temp = CyU3PUartGetLock ();
    if (temp != CY_U3P_SUCCESS)
    {
        if (status != NULL)
        {
            *status = temp;
        }
        return 0;
    }

    temp = CY_U3P_SUCCESS;
    for (i = 0; i < count; i++)
    {
        timeout = CY_U3P_UART_TIMEOUT;

        /* Wait for data inside the rx fifo */
        while (!((UART->lpp_uart_status & CY_U3P_LPP_UART_RX_DATA) ||
            timeout == 0))
        {
            timeout--;
        }

        if (timeout == 0)
        {
            temp = CY_U3P_ERROR_TIMEOUT;
            break;
        }

        data_p[i] = ((uint8_t)(UART->lpp_uart_ingress_data));
    }

    CyU3PUartReleaseLock();

    if (status != NULL)
    {
        *status = temp;
    }
    return i;
}

/* [] */

