/*
 ## Cypress USB 3.0 Platform source file (cyfx3i2c.c)
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
#include <cyfx3i2c.h>
#include <cyfx3device.h>
#include <cyfx3utils.h>
#include <gctl_regs.h>
#include <lpp_regs.h>
#include <i2c_regs.h>
#include <cyfx3bootloader.h>

/*
 * @@I2C 
 * Summary
 * This file contains the I2C functionalities.
 */

#define CY_FX3_BOOT_LPP_SOCKET_I2C_CONS  (0x02)         /* Outgoing data to I2C slave. */
#define CY_FX3_BOOT_LPP_SOCKET_I2C_PROD  (0x05)         /* Incoming data from I2C slave. */

/* Default bit rate to be used on initialization. */
#define CY_FX3_BOOT_I2C_DEFAULT_BIT_RATE             (100000)
/* Default timeout value in waiting for a response from the
 * I2C registers. */
#define CY_FX3_BOOT_I2C_TIMEOUT                      (0xFFF)

extern uint32_t glSysClkFreq;
extern uint8_t glLppBlockOn;

/*
 * memcpy
 *  Parameters:  d=dest pointer, s=src pointer, cnt in byte
 *  Return none
 */
static void 
memcopy (
        uint8_t *d, 
        uint8_t *s, 
        int cnt
        )
{
    int i;
    for (i=0; i<cnt; i++)
    {
        *d++ = *s++;
    }
}
/* 
 * Internal function which sets the
 * frequency depending on the bitRate.
 */
static void
CyFx3BootI2cSetClock (
    uint32_t bitRate)
{
    int32_t  clkdiv;
    uint32_t temp = 0;

    if (bitRate == 0)
    {
        bitRate = CY_FX3_BOOT_I2C_DEFAULT_BIT_RATE;
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
    /* I2C core clock needs to be 10X the bus clock. */
    /* The I2C clock is set to SYS clock/16.
     * Hence (SYS clock / 16) * 4 = SYS clock / 4 */
    clkdiv = (glSysClkFreq >> 2) / (bitRate * 10);

    if ((clkdiv & 0x03) == 0)
    {
        /*  (x - floor(x)) < 0.25. */
        clkdiv >>= 2;
    }
    else if (((clkdiv & 0x03) == 1) || ((clkdiv & 0x03) == 2))
    {
        /* (((x - floor(x)) >= 0.25) && (x - floor(x)) < 0.5) */
        clkdiv >>= 2;
        temp |= CY_U3P_GCTL_I2CCLK_HALFDIV;
    }
    else /* ((clkdiv & 0x03) == 3) */
    {
        /* (x - floor(x)) >= 0.75. */
        clkdiv = ((clkdiv >> 2)  + 1);
    }

    clkdiv--;
    if ((clkdiv < 1) || (clkdiv > CY_U3P_GCTL_I2CCLK_DIV_MASK))
    {
        /* The max clock rate allowed
         * is half the source clock. and min
         * clock allowed is 2 ^ 10 divider. */
        clkdiv = 1;
    }

    temp |= (clkdiv | (CY_FX3_BOOT_SYS_CLK_BY_16 << CY_U3P_GCTL_I2CCLK_SRC_POS) |
            CY_U3P_GCTL_I2CCLK_CLK_EN);

    GCTL->i2c_core_clk = temp;
}

/* 
 * This function initializes the I2C module
 */
CyFx3BootErrorCode_t
CyFx3BootI2cInit (
      void)
{
    /* Set the clock frequency. This should preceed
     * the I2C power up */
    CyFx3BootI2cSetClock (CY_FX3_BOOT_I2C_DEFAULT_BIT_RATE);

    if (!glLppBlockOn)
    {
        /* Reset and enable the LPP block */
        LPP->power &= ~CY_U3P_LPP_RESETN;
        LPP->power |= CY_U3P_LPP_RESETN;
        while (!(LPP->power & CY_U3P_LPP_ACTIVE));
    }

    /* Set the variable indicating that the I2C Block has been turned on. */
    glLppBlockOn |= 0x01;

    /* Power on the I2C module */
    I2C->lpp_i2c_power |= CY_U3P_LPP_I2C_RESETN;
    while (!(I2C->lpp_i2c_power & CY_U3P_LPP_I2C_ACTIVE));

    return CY_FX3_BOOT_SUCCESS;
}

/*
 * This function de-initializes the I2C 
 */
void
CyFx3BootI2cDeInit (
        void)
{
    /* Reset the reset bit */
    I2C->lpp_i2c_power &= ~(CY_U3P_LPP_I2C_RESETN);

    if (glLppBlockOn == 0x01)
    {
        /* Reset and disable the LPP block */
        LPP->power &= ~CY_U3P_LPP_RESETN;
        while (LPP->power & CY_U3P_LPP_ACTIVE);
        glLppBlockOn = 0;
    }

    /* Disable the I2c clock */
    GCTL->i2c_core_clk &= (~CY_U3P_GCTL_I2CCLK_CLK_EN);

    glLppBlockOn &= ~(0x01);
}

/* 
 * configures and opens the I2C.
 */
CyFx3BootErrorCode_t 
CyFx3BootI2cSetConfig (
        CyFx3BootI2cConfig_t *config )
{
    uint32_t temp = 0, timeout;

    if ((glLppBlockOn & 0x01) == 0)
        return CY_FX3_BOOT_ERROR_NOT_STARTED;

    /* Check for parameter validity. */
    if (!config)
    {
        return CY_FX3_BOOT_ERROR_NULL_POINTER;
    }

    /* Setup clock for the UART block according
     * to the baud rate. */
    CyFx3BootI2cSetClock (config->bitRate);

    /* Disable and Clear the TX and RX_FIFO. */
    I2C->lpp_i2c_config = (CY_U3P_LPP_I2C_TX_CLEAR |
            CY_U3P_LPP_I2C_RX_CLEAR);

    /* Wait till the FIFOs becomes empty */
    timeout = CY_FX3_BOOT_I2C_TIMEOUT;
    while ((I2C->lpp_i2c_intr & CY_U3P_LPP_I2C_TX_DONE) == 0)
    {
        if (timeout-- == 0)
        {
            return  CY_FX3_BOOT_ERROR_TIMEOUT;
        }
    }

    timeout = CY_FX3_BOOT_I2C_TIMEOUT;
    while ((I2C->lpp_i2c_status & CY_U3P_LPP_I2C_RX_DATA) != 0)
    {
        if (timeout-- == 0)
        {
            return CY_FX3_BOOT_ERROR_TIMEOUT;
        }
    }

    I2C->lpp_i2c_config = 0;

    /* Update the configuration. */
    if (config->bitRate <= CY_FX3_BOOT_I2C_DEFAULT_BIT_RATE)
    {
        /* We have to use clock with duty cycle of 50% */
        temp = CY_U3P_LPP_I2C_I2C_100KHZ;
    }

    /* Setting the transfer mode and the timeout register */
    if (config->isDma)
    {
        temp |= CY_U3P_LPP_I2C_DMA_MODE;
        I2C->lpp_i2c_dma_timeout = config->dmaTimeout;
    }
    else
    {
        I2C->lpp_i2c_dma_timeout = 0xFFFFU;
    }

    I2C->lpp_i2c_config = temp;

    I2C->lpp_i2c_socket = ((CY_FX3_BOOT_LPP_SOCKET_I2C_CONS & CY_U3P_LPP_I2C_EGRESS_SOCKET_MASK) |
            ((CY_FX3_BOOT_LPP_SOCKET_I2C_PROD << CY_U3P_LPP_I2C_INGRESS_SOCKET_POS) 
                & CY_U3P_LPP_I2C_INGRESS_SOCKET_MASK));

    I2C->lpp_i2c_timeout = config->busTimeout;
    /* Clear all stale interrupts. */
    I2C->lpp_i2c_intr = 0xFFFFFFFFU;

    /* Disable the interrupts */
    I2C->lpp_i2c_intr_mask = 0;

    /* Finally enable the block. */
    I2C->lpp_i2c_config |= CY_U3P_LPP_I2C_ENABLE;
    while ((I2C->lpp_i2c_config & CY_U3P_LPP_I2C_ENABLE) == 0);

    return CY_FX3_BOOT_SUCCESS;
}

/* Send the command for read and write */
CyFx3BootErrorCode_t
CyFx3BootI2cSendCommand (
        CyFx3BootI2cPreamble_t *preamble,
        uint32_t byteCount,
        CyBool_t isRead)
{
    uint32_t timeout;
    uint32_t value[2], retry;

    CyFx3BootErrorCode_t status;

    /* Check the parameters. */
    if (!preamble)
    {
        return CY_FX3_BOOT_ERROR_NULL_POINTER;
    }

    for (retry = 0; retry < 2; retry++)
    {
        /* Check if the bus and block are not busy. */
        timeout = CY_FX3_BOOT_I2C_TIMEOUT;
        status = CY_FX3_BOOT_SUCCESS;
        while (I2C->lpp_i2c_status & (CY_U3P_LPP_I2C_BUSY | CY_U3P_LPP_I2C_BUS_BUSY))
        {
            if (timeout-- == 0)
            {
                /* The I2c device is busy or our block has
                 * gotten stuck. Reset the block and retry. */
                I2C->lpp_i2c_config &= ~CY_U3P_LPP_I2C_ENABLE;
                CyFx3BootBusyWait (1);
                I2C->lpp_i2c_config |= CY_U3P_LPP_I2C_ENABLE;
                status = CY_FX3_BOOT_ERROR_TIMEOUT;
                break;
            }
        }
    }

    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    I2C->lpp_i2c_preamble_rpt = 0;  /* Disable repeat */

    /* Copy the preamble into a local array and then copy
     * to the register to prevent unaligned writes in register
     * area. */
    memcopy ((uint8_t *)&value[0], preamble->buffer, preamble->length);

    /* Update the register. */
    I2C->lpp_i2c_preamble_data0 = value[0];
    I2C->lpp_i2c_preamble_data1 = value[1];

    /* Clear all conditions. */
    I2C->lpp_i2c_command = 0;
    /* Clear the interrupts */
    I2C->lpp_i2c_intr = ~0;
    /* Clear all sticky bits in status register. */
    I2C->lpp_i2c_status = CY_U3P_LPP_I2C_TIMEOUT |
        CY_U3P_LPP_I2C_LOST_ARBITRATION | CY_U3P_LPP_I2C_ERROR;

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

    return CY_FX3_BOOT_SUCCESS;
}

/*
 * Transmits data byte by byte over the I2C interface.
 */
CyFx3BootErrorCode_t 
CyFx3BootI2cTransmitBytes (
        CyFx3BootI2cPreamble_t *preamble,
        uint8_t *data, 
        uint32_t byteCount,
        uint32_t retryCount)
{
    uint32_t index, timeout, flag;
    CyFx3BootErrorCode_t status;

    if ((!data) || (!preamble))
    {   
        return CY_FX3_BOOT_ERROR_NULL_POINTER;
    }

    if (byteCount == 0)
    {
        return CY_FX3_BOOT_SUCCESS;
    }

    flag = CY_U3P_LPP_I2C_ERROR | CY_U3P_LPP_I2C_TIMEOUT | CY_U3P_LPP_I2C_LOST_ARBITRATION;

    do 
    {
        /* Copy the first data byte to the fifo before sending the command. */
        I2C->lpp_i2c_egress_data = data[0];
 
        /* Send the command to write in to the device */
        status = CyFx3BootI2cSendCommand (preamble, byteCount, CyFalse);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            break;
        }

        index = 1;

        while (index < byteCount)
        {
            timeout = CY_FX3_BOOT_I2C_TIMEOUT;

            /* Wait for the tx_space or error bits. */
            while ((I2C->lpp_i2c_status & (flag | CY_U3P_LPP_I2C_TX_SPACE)) == 0)
            {
                if (timeout-- == 0)
                {
                    /* Do not retry in case of a timeout. */
                    status = CY_FX3_BOOT_ERROR_TIMEOUT;
                    break;
                }
            }

            if (I2C->lpp_i2c_status & (CY_U3P_LPP_I2C_ERROR | CY_U3P_LPP_I2C_TIMEOUT
                        | CY_U3P_LPP_I2C_LOST_ARBITRATION))
            {
                /* An error has been encountered. Retry from the beginning. */
                status = CY_FX3_BOOT_ERROR_XFER_FAILURE;
                break;
            }

            /* Add the next data byte */
            I2C->lpp_i2c_egress_data = data[index];
            index++;

            if (byteCount == index)
            {
                /* If this is the last data, wait for the
                 * FIFO to be drained. */
                timeout = CY_FX3_BOOT_I2C_TIMEOUT;
                while ((I2C->lpp_i2c_intr & (flag | CY_U3P_LPP_I2C_TX_DONE)) == 0)
                {
                    if (timeout-- == 0)
                    {
                        /* Do not retry in case of a timeout. */
                        status = CY_FX3_BOOT_ERROR_TIMEOUT;
                        break;
                    }
                }

                if (I2C->lpp_i2c_status & (CY_U3P_LPP_I2C_ERROR | CY_U3P_LPP_I2C_TIMEOUT
                        | CY_U3P_LPP_I2C_LOST_ARBITRATION))
                {
                    /* An error has been encountered. Retry from the beginning. */
                    status = CY_FX3_BOOT_ERROR_XFER_FAILURE;
                    break;
                }
            }
        }

    } while ((status == CY_FX3_BOOT_ERROR_XFER_FAILURE) && (retryCount-- != 0));

    return status;
}

/*
 * Receive data byte by byte over the I2C interface
 */
CyFx3BootErrorCode_t 
CyFx3BootI2cReceiveBytes (
        CyFx3BootI2cPreamble_t *preamble,
        uint8_t *data,
        uint32_t byteCount,
        uint32_t retryCount)
{
    uint32_t index, flag, timeout;
    CyFx3BootErrorCode_t status;

    if ((!data) || (!preamble))
    {   
        return CY_FX3_BOOT_ERROR_NULL_POINTER;
    }
    if (byteCount == 0)
    {
        return CY_FX3_BOOT_SUCCESS;
    }

    flag = CY_U3P_LPP_I2C_RX_DATA | CY_U3P_LPP_I2C_ERROR |
        CY_U3P_LPP_I2C_TIMEOUT | CY_U3P_LPP_I2C_LOST_ARBITRATION;

    do 
    {
        /* Send the command to read */
        status = CyFx3BootI2cSendCommand (preamble, byteCount, CyTrue);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            break;
        }

        index = 0;

        while (index < byteCount)
        {
            /* Wait for the RX_DATA or error bits. */
            timeout = CY_FX3_BOOT_I2C_TIMEOUT;
            while ((I2C->lpp_i2c_status & flag) == 0)
            {
                if (timeout-- == 0)
                {
                    /* Do not retry in case of a timeout. */
                    status = CY_FX3_BOOT_ERROR_TIMEOUT;
                    break;
                }
            }

            if (I2C->lpp_i2c_status & (CY_U3P_LPP_I2C_ERROR | CY_U3P_LPP_I2C_TIMEOUT
                        | CY_U3P_LPP_I2C_LOST_ARBITRATION))
            {
                /* An error has been encountered. Retry from the beginning. */
                status = CY_FX3_BOOT_ERROR_XFER_FAILURE;
                break;
            }

            /* Copy the data from the fifo. */
            data[index] = I2C->lpp_i2c_ingress_data;
            index++;
        }
    } while ((status == CY_FX3_BOOT_ERROR_XFER_FAILURE) && (retryCount-- != 0));

    return status;
}

/* Wait for I2C to ACK on preamble. */
CyFx3BootErrorCode_t
CyFx3BootI2cWaitForAck (
        CyFx3BootI2cPreamble_t *preamble,
        uint32_t retryCount)
{
    uint32_t timeout, temp;
    CyFx3BootErrorCode_t status;

    if (!preamble)
    {
        return CY_FX3_BOOT_ERROR_NULL_POINTER;
    }

    do 
    {
        /* Send the preamble with no data phase. */
        status = CyFx3BootI2cSendCommand (preamble, 0, CyFalse);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            break;
        }

        /* Wait for the TX_DONE or error bits. */
        timeout = CY_FX3_BOOT_I2C_TIMEOUT;
        while (timeout-- != 0)
        {
            temp = I2C->lpp_i2c_status;
            if (temp & (CY_U3P_LPP_I2C_ERROR | CY_U3P_LPP_I2C_TIMEOUT
                    | CY_U3P_LPP_I2C_LOST_ARBITRATION))
            {
                /* An error has been encountered. Since this is a
                 * poll function any error is similar to a timeout. Retry. */
                status = CY_FX3_BOOT_ERROR_TIMEOUT;
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
            /* Do not retry in case of a timeout. */
            retryCount = 0;
            status = CY_FX3_BOOT_ERROR_TIMEOUT;
        }

    } while ((status != CY_FX3_BOOT_SUCCESS) && (retryCount-- != 0));

    return status;
}

static CyFx3BootErrorCode_t
CyFx3BootI2cWaitForBlkXfer(
        uint32_t intr,
        uint32_t timeout)
{
    CyFx3BootErrorCode_t status = CY_FX3_BOOT_ERROR_TIMEOUT;
    uint32_t errorIntr = (7 << 6);

    do 
    {
        if ((I2C->lpp_i2c_intr & intr) == intr)
        {
            status = CY_FX3_BOOT_SUCCESS;
            break;
        }

        if ((I2C->lpp_i2c_intr & errorIntr) != 0)
        {
            status = CY_FX3_BOOT_ERROR_XFER_FAILURE;
            break;
        }

        CyFx3BootBusyWait (10);

        if ((timeout != CY_FX3_BOOT_NO_WAIT) && (timeout != CY_FX3_BOOT_WAIT_FOREVER))
        {
            timeout --;
        }

    } while (timeout > 0);

    /* Clear the interrupts. */
    I2C->lpp_i2c_intr = (intr | errorIntr);

    return status;
}

CyFx3BootErrorCode_t
CyFx3BootI2cDmaXferData (
        CyBool_t isRead,
        uint32_t address,
        uint32_t length,
        uint32_t timeout
        )
{
    uint16_t socket = CY_FX3_BOOT_LPP_SOCKET_I2C_CONS;

    if ((address < CY_FX3_BOOT_SYSMEM_BASE) || (address >= CY_FX3_BOOT_SYSMEM_END))
        return CY_FX3_BOOT_ERROR_INVALID_DMA_ADDR;

    if (isRead)
    {
        socket = CY_FX3_BOOT_LPP_SOCKET_I2C_PROD;
    }

    CyFx3BootDmaXferData (isRead, socket, address, length);

    if (isRead)
    {
        return CyFx3BootI2cWaitForBlkXfer (CY_U3P_LPP_I2C_RX_DONE, timeout);
    }
    else
    {
        return CyFx3BootI2cWaitForBlkXfer(CY_U3P_LPP_I2C_TX_DONE, timeout);
    }
}

/* [] */

