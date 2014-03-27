/*
 ## Cypress USB 3.0 Platform source file (cyfx3uart.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2011-2012,
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

#include <cyfx3uart.h>
#include <cyfx3error.h>
#include <cyfx3device.h>
#include <gctl_regs.h>
#include <uart_regs.h>
#include <lpp_regs.h>
#include <cyfx3bootloader.h>

/* @@UART
 * Summary
 * This file contains UART functionalities which can
 * be called from Fx3 applications.
 */

#define CY_FX3_BOOT_LPP_SOCKET_UART_CONS   (0x0003)        /* Outgoing data to UART peer. */
#define CY_FX3_BOOT_LPP_SOCKET_UART_PROD   (0x0006)        /* Incoming data from UART peer. */

#define CY_FX3_BOOT_UART_DEFAULT_BAUD_RATE           (9600)
/* Default timeout value in waiting for a response from the UART registers. */
#define CY_FX3_BOOT_UART_TIMEOUT                     (0xFFFFF)

extern uint32_t glSysClkFreq;
extern CyFx3BootIoMatrixConfig_t glBootIoCfg;
extern uint8_t glLppBlockOn;

static void
CyFx3BootUartSetClock (
        uint32_t baudRate
        )
{
    int32_t  clkdiv;
    uint32_t regVal = 0;

    if (baudRate == 0)
    {
        baudRate = CY_FX3_BOOT_UART_DEFAULT_BAUD_RATE;
    }

    /* Clock approximation needs to be done. Since we have
     * a half divider, we will use the following algorithm:
     * if x is the actual divider and n is the required
     * integral divider to be used, then following conditions
     * are used to evaluate:
     * if (x - floor(x)) < 0.25 ==> n = floor(x);
     * if (((x - floor(x)) >= 0.25) &&
     *     (x - floor(x)) < 0.5) ==> n = floor(x) + half divider;
     * if (x - floor(x)) >= 0.75 ==> n = floor(x) + 1;
     *
     * For this the source frequency is multiplied 4 times
     * and two least significant bits are used as decimal places
     * for evaluation.
     *
     * 00     ==> n = floor(x);
     * 01, 10 ==> n = floor(x) + half divider;
     * 11     ==> n = floor(x) + 1;
     */
    /* UART core clock must be 16X the baud rate. */
    clkdiv = (glSysClkFreq << 2) / (baudRate * 16);

    switch (clkdiv & 0x03)
    {
        case 0:
            /*  (x - floor(x)) < 0.25. */
            clkdiv >>= 2;
            break;
        case 1:
        case 2:
            /* (((x - floor(x)) >= 0.25) && (x - floor(x)) < 0.5) */
            clkdiv >>= 2;
            regVal |= CY_U3P_GCTL_UARTCLK_HALFDIV;
            break;
        default:
            /* (x - floor(x)) >= 0.75. */
            clkdiv = ((clkdiv >> 2)  + 1);
            break;
    }

    clkdiv--;
    if ((clkdiv < 1) || (clkdiv > CY_U3P_GCTL_UARTCLK_DIV_MASK))
    {
        clkdiv = 1;
    }

    regVal |= (clkdiv | (CY_FX3_BOOT_SYS_CLK << CY_U3P_GCTL_UARTCLK_SRC_POS)
            | CY_U3P_GCTL_UARTCLK_CLK_EN);

    /* Update the register. */
    GCTL->uart_core_clk = regVal;
}

CyFx3BootErrorCode_t
CyFx3BootUartInit (
        void)
{
    /* Set the clock to a default value.
     * This should prcede the UART power up*/
    CyFx3BootUartSetClock (CY_FX3_BOOT_UART_DEFAULT_BAUD_RATE);

    if (!glLppBlockOn)
    {
        /* Reset and enable the LPP block */
        LPP->power &= ~CY_U3P_LPP_RESETN;
        LPP->power |= CY_U3P_LPP_RESETN;
        while (!(LPP->power & CY_U3P_LPP_ACTIVE));        
    }

    /* Set the variable indicating that the UART Block has been turned on. */
    glLppBlockOn |= 0x04;

    /* Hold the UART block in reset. */
    UART->lpp_uart_power &= ~(CY_U3P_LPP_UART_RESETN);
    UART->lpp_uart_power |= (CY_U3P_LPP_UART_RESETN);

    /* Wait for the active bit to be asserted by the hardware */
    while (!(UART->lpp_uart_power & CY_U3P_LPP_UART_ACTIVE));
    return CY_FX3_BOOT_SUCCESS;
}

/*
 * This function de-initializes the UART 
 */
void
CyFx3BootUartDeInit(
        void)
{
    /* Power off UART block. */
    UART->lpp_uart_power &= ~(CY_U3P_LPP_UART_RESETN);

    if (glLppBlockOn == 0x04)
    {
        /* Reset and disable the LPP block */
        LPP->power &= ~CY_U3P_LPP_RESETN;
        while (LPP->power & CY_U3P_LPP_ACTIVE);
        glLppBlockOn = 0;
    }

    /* Disable the UART clock */
    GCTL->uart_core_clk &= (~CY_U3P_GCTL_UARTCLK_CLK_EN);

    glLppBlockOn &= ~(0x04);
}

/* 
 * configures and opens the UART  
 */
CyFx3BootErrorCode_t
CyFx3BootUartSetConfig (
        CyFx3BootUartConfig_t *config)
{
    uint32_t regVal = 0; 

    if ((glLppBlockOn & 0x04) == 0)
        return CY_FX3_BOOT_ERROR_NOT_STARTED;

    /* Check for parameter validity. */
    if (!config)
    {
        return CY_FX3_BOOT_ERROR_NULL_POINTER;
    }

    /* Setup clock for the UART block according
     * to the baud rate. */
    CyFx3BootUartSetClock (config->baudRate);

    /* Disable the UART before changing any register */
    UART->lpp_uart_config &= ~(CY_U3P_LPP_UART_ENABLE);
    while ((UART->lpp_uart_config & CY_U3P_LPP_UART_ENABLE) != 0);

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
    }

    regVal |= (config->stopBit << CY_U3P_LPP_UART_STOP_BITS_POS) &
        CY_U3P_LPP_UART_STOP_BITS_MASK;

    switch (config->parity)
    {
        case CY_FX3_BOOT_UART_EVEN_PARITY:
            regVal |= CY_U3P_LPP_UART_PARITY;
            break;
        case CY_FX3_BOOT_UART_ODD_PARITY:
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
    UART->lpp_uart_socket = ((CY_FX3_BOOT_LPP_SOCKET_UART_CONS & 
                CY_U3P_LPP_UART_EGRESS_SOCKET_MASK) | 
            ((CY_FX3_BOOT_LPP_SOCKET_UART_PROD & CY_U3P_LPP_UART_EGRESS_SOCKET_MASK) 
             << CY_U3P_LPP_UART_INGRESS_SOCKET_POS));

    /* Disable the interrupt */
    UART->lpp_uart_intr_mask = 0;

    /* Enable the UART only at the end. */
    UART->lpp_uart_config |= CY_U3P_LPP_UART_ENABLE;
    while((UART->lpp_uart_config & CY_U3P_LPP_UART_ENABLE) == 0);

    return CY_FX3_BOOT_SUCCESS;
}

/*
 * Sets registers for dma egress transfer 
 */
void
CyFx3BootUartTxSetBlockXfer (
        uint32_t txSize)
{
    UART->lpp_uart_tx_byte_count = txSize;
}

/*
 * Sets registers for dma ingress transfer 
 */
void
CyFx3BootUartRxSetBlockXfer (
        uint32_t rxSize)
{
    UART->lpp_uart_rx_byte_count = rxSize;
}

/*
 * Transmits data byte by byte over the UART interface
 */
uint32_t
CyFx3BootUartTransmitBytes (
        uint8_t *data_p,
        uint32_t count
        )
{
    uint32_t timeout;
    int32_t i;

    if (!data_p || (count == 0))
    {
       return 0; 
    }
    
    for (i = 0; i < count; i++)
    {
        timeout = CY_FX3_BOOT_UART_TIMEOUT;
        UART->lpp_uart_intr = CY_U3P_LPP_UART_TX_DONE;
        UART->lpp_uart_egress_data = data_p[i];

        /* Wait for the transfer to be done */
        while (!((UART->lpp_uart_intr & CY_U3P_LPP_UART_TX_DONE) || (timeout == 0)))
        {
            timeout--;
        }

        if (timeout == 0)
        {
            return i;
        }
    }

    return i;
}

/*
 * Receives data byte by byte over the UART interface
 */
uint32_t 
CyFx3BootUartReceiveBytes (
        uint8_t *data_p,
        uint32_t count
        )
{
    uint32_t timeout;
    int32_t i;

    if (!data_p || (count == 0))
    {
        return 0;
    }
    
    for (i = 0; i < count; i++)
    {
        timeout = CY_FX3_BOOT_UART_TIMEOUT;

        /* Wait for data inside the rx fifo */
        while (!((UART->lpp_uart_status & CY_U3P_LPP_UART_RX_DATA) || (timeout == 0)))
        {
            timeout--;
        }

        if (timeout == 0)
        {
            return i;
        }

        data_p[i] = ((uint8_t)(UART->lpp_uart_ingress_data));
    }

    return i;
}

static CyFx3BootErrorCode_t
CyFx3BootUartWaitForBlkXfer(
        uint32_t intr,
        uint32_t timeout)
{
    CyFx3BootErrorCode_t status = CY_FX3_BOOT_ERROR_TIMEOUT;
    uint32_t errorIntr = (1 << 9);

    do 
    {
        if ((UART->lpp_uart_intr & intr) == intr)
        {
            status = CY_FX3_BOOT_SUCCESS;
            break;
        }

        if ((UART->lpp_uart_intr & errorIntr) == errorIntr)
        {
            status = CY_FX3_BOOT_ERROR_XFER_FAILURE;
            break;
        }

        if ((timeout != CY_FX3_BOOT_NO_WAIT) && (timeout != CY_FX3_BOOT_WAIT_FOREVER))
        {
            CyFx3BootBusyWait (10);
            timeout --;
        }
    } while (timeout > 0);

    /* Clear the interrupts. */
    UART->lpp_uart_intr = (intr | errorIntr);

    return status;
}

CyFx3BootErrorCode_t
CyFx3BootUartDmaXferData (
        CyBool_t isRead,
        uint32_t address,
        uint32_t length,
        uint32_t timeout)
{
    uint16_t socket = CY_FX3_BOOT_LPP_SOCKET_UART_CONS;

    if ((address < CY_FX3_BOOT_SYSMEM_BASE) || (address >= CY_FX3_BOOT_SYSMEM_END))
        return CY_FX3_BOOT_ERROR_INVALID_DMA_ADDR;

    if (isRead)
    {
        socket = CY_FX3_BOOT_LPP_SOCKET_UART_PROD;
    }

    CyFx3BootDmaXferData (isRead, socket, address, length);

    if (isRead)
    {
        return CyFx3BootUartWaitForBlkXfer (CY_U3P_LPP_UART_RX_DONE, timeout);
    }
    else
    {
        return CyFx3BootUartWaitForBlkXfer(CY_U3P_LPP_UART_TX_DONE, timeout);
    }
}

