/*
 ## Cypress USB 3.0 Platform source file (cyfx3spi.c)
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
#include <cyfx3bootloader.h>
#include <cyfx3device.h>
#include <cyfx3spi.h>
#include <cyfx3utils.h>
#include <cyfx3error.h>
#include <gctl_regs.h>
#include <lpp_regs.h>
#include <spi_regs.h>
#include <gctlaon_regs.h>
/*
 * @@SPI 
 * Summary
 * This file contains the SPI functionalities.
 */

#define CY_FX3_LPP_SOCKET_SPI_CONS (0x0004)
#define CY_FX3_LPP_SOCKET_SPI_PROD (0x0007)

        /* Default SPI clock in Hz */
#define CY_FX3_BOOT_SPI_DEFAULT_CLK              (1000000)

#define CY_FX3_BOOT_SPI_TIMEOUT                  (0xFFFFF)

extern CyFx3BootIoMatrixConfig_t glBootIoCfg;
extern uint32_t glSysClkFreq;
extern uint8_t glLppBlockOn;
/* 
 * Internal function which sets the frequency depending on the clock
 */
static void
CyFx3BootSpiSetClock (
    uint32_t clock)
{
    int32_t  clkdiv;
    uint32_t temp = 0;

    /* Calculate the clk division value. */
    if (clock == 0)
    {
        clock = CY_FX3_BOOT_SPI_DEFAULT_CLK;
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
    clkdiv = (glSysClkFreq << 2) / (clock * 2);
    if ((clkdiv & 0x03) == 0)
    {
        /*  (x - floor(x)) < 0.25. */
        clkdiv >>= 2;
    }
    else if (((clkdiv & 0x03) == 1) || ((clkdiv & 0x03) == 2))
    {
        /* (((x - floor(x)) >= 0.25) && (x - floor(x)) < 0.5) */
        clkdiv >>= 2;
        temp |= CY_U3P_GCTL_SPICLK_HALFDIV;
    }
    else /* ((clkdiv & 0x03) == 3) */
    {
        /* (x - floor(x)) >= 0.75. */
        clkdiv = ((clkdiv >> 2)  + 1);
    }

    clkdiv--;
    if ((clkdiv < 1) || (clkdiv > CY_U3P_GCTL_SPICLK_DIV_MASK))
    {
        clkdiv = 1;
    }
 
    temp |= (clkdiv | (3 << CY_U3P_GCTL_SPICLK_SRC_POS)
            | CY_U3P_GCTL_SPICLK_CLK_EN);

    GCTL->spi_core_clk = temp;
}

/* 
 * This function initializes the SPI module
 */
CyFx3BootErrorCode_t
CyFx3BootSpiInit (
        void)
{
    /* Set the clock frequency. This should precede the SPI power up */
    CyFx3BootSpiSetClock (CY_FX3_BOOT_SPI_DEFAULT_CLK);

    if (!glLppBlockOn)
    {
        /* Reset and enable the LPP block */
        LPP->power &= ~CY_U3P_LPP_RESETN;
        LPP->power |= CY_U3P_LPP_RESETN;
        while (!(LPP->power & CY_U3P_LPP_ACTIVE));
    }

    /* Set the variable indicating that the SPI Block has been turned on. */
    glLppBlockOn |= 0x02;

    /* Power on the SPI module */
    SPI->lpp_spi_power &= ~(CY_U3P_LPP_SPI_RESETN);
    SPI->lpp_spi_power |= CY_U3P_LPP_SPI_RESETN;

    /* Wait till the active bit is set */
    while (!(SPI->lpp_spi_power & CY_U3P_LPP_SPI_ACTIVE));
    return CY_FX3_BOOT_SUCCESS;
}

/*
 * This function de-initializes the SPI 
 */
void
CyFx3BootSpiDeInit(
        void)
{
    SPI->lpp_spi_power &= ~(CY_U3P_LPP_SPI_RESETN);

    if (glLppBlockOn == 0x02)
    {
        /* Reset and disable the LPP block */
        LPP->power &= ~CY_U3P_LPP_RESETN;
        while (LPP->power & CY_U3P_LPP_ACTIVE);
        glLppBlockOn = 0;
    }

    /* Disable the spi clock */
    GCTL->spi_core_clk &= (~CY_U3P_GCTL_SPICLK_CLK_EN);

    glLppBlockOn &= ~(0x02);
}

/* 
 * configures and opens the SPI
 */
CyFx3BootErrorCode_t 
CyFx3BootSpiSetConfig (
        CyFx3BootSpiConfig_t *config)
{
    uint32_t temp = 0, timeout;

    if ((glLppBlockOn & 0x02) == 0)
        return CY_FX3_BOOT_ERROR_NOT_STARTED;

    if (!config)
    {
        return CY_FX3_BOOT_ERROR_NULL_POINTER;
    }
    /* Set the clock for Spi */
    CyFx3BootSpiSetClock (config->clock);

    /* Clear the TX and RX_FIFO */
    SPI->lpp_spi_config = (CY_U3P_LPP_SPI_TX_CLEAR | CY_U3P_LPP_SPI_RX_CLEAR);

    /* Wait until the SPI block is disabled and the RX_DATA bit is clear and TX_DONE bit is set. */
    timeout = CY_FX3_BOOT_SPI_TIMEOUT;
    while ((SPI->lpp_spi_status & CY_U3P_LPP_SPI_RX_DATA) != 0)
    {
        if (timeout-- == 0)
        {
            return CY_FX3_BOOT_ERROR_TIMEOUT;
        }
    }

    timeout = CY_FX3_BOOT_SPI_TIMEOUT;
    while ((SPI->lpp_spi_status & CY_U3P_LPP_SPI_TX_DONE) == 0)
    {
        if (timeout-- == 0)
        {
            return CY_FX3_BOOT_ERROR_TIMEOUT;
        }
    }

    SPI->lpp_spi_config = 0;

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
    if (config->ssnCtrl == CY_FX3_BOOT_SPI_SSN_CTRL_NONE)
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
        ((CY_FX3_LPP_SOCKET_SPI_CONS & CY_U3P_LPP_SPI_EGRESS_SOCKET_MASK) |
         ((CY_FX3_LPP_SOCKET_SPI_PROD << CY_U3P_LPP_SPI_INGRESS_SOCKET_POS) & 
        CY_U3P_LPP_SPI_INGRESS_SOCKET_MASK));

    /* Read back to flush */
    temp = SPI->lpp_spi_config;

    SPI->lpp_spi_intr_mask = 0;

    return CY_FX3_BOOT_SUCCESS;
}

/* 
 * Resets the FIFO.
 * Leaves SPI block disabled at the end.
 */
static void
CyFx3BootSpiResetFifo (
        CyBool_t isTx,
        CyBool_t isRx
        )
{
    uint32_t ctrlMask = 0;

    SPI->lpp_spi_intr_mask = 0;

    if (isTx)
    {
        ctrlMask = CY_U3P_LPP_SPI_TX_CLEAR;
    }
    if (isRx)
    {
        ctrlMask |= CY_U3P_LPP_SPI_RX_CLEAR;
    }

    /* Disable the SPI */
    SPI->lpp_spi_config &= ~CY_U3P_LPP_SPI_ENABLE;
    while ((SPI->lpp_spi_config & CY_U3P_LPP_SPI_ENABLE) != 0);

    /* Disable Rx, Tx and DMA Mode. */
    SPI->lpp_spi_config &= ~(CY_U3P_LPP_SPI_RX_ENABLE | CY_U3P_LPP_SPI_TX_ENABLE |
            CY_U3P_LPP_SPI_DMA_MODE);

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

    /* Clear all interrupts. */
    SPI->lpp_spi_intr |= CY_U3P_LPP_SPI_TX_DONE;
}

/* 
 * Aserts / deasserts the SSN line.
 */
void
CyFx3BootSpiSetSsnLine (CyBool_t isHigh)
{
    if (isHigh)
    {
        SPI->lpp_spi_config |= CY_U3P_LPP_SPI_SSN_BIT;
    }
    else
    {
        SPI->lpp_spi_config &= ~CY_U3P_LPP_SPI_SSN_BIT;
    }
}

static CyFx3BootErrorCode_t
CyFx3BootSpiWaitForBlkXfer(
        uint32_t intr,
        uint32_t timeout
        )
{
    CyFx3BootErrorCode_t status = CY_FX3_BOOT_ERROR_TIMEOUT;
    uint32_t errorIntr = (1 << 6);
    uint32_t intstat;

    do 
    {
        intstat = SPI->lpp_spi_intr;
        if ((intstat & intr) == intr)
        {
            status = CY_FX3_BOOT_SUCCESS;
            break;
        }

        if ((intstat & errorIntr) != 0)
        {
            status = CY_FX3_BOOT_ERROR_XFER_FAILURE;
            break;
        }

        if ((timeout != CY_FX3_BOOT_NO_WAIT) && (timeout != CY_FX3_BOOT_WAIT_FOREVER))
        {
            if ((SPI->lpp_spi_config & CY_U3P_LPP_SPI_DMA_MODE) != 0)
                CyFx3BootBusyWait (10);
            timeout --;
        }
    } while (timeout > 0);

    /* Clear the interrupts. */
    SPI->lpp_spi_intr = (intr | errorIntr);
    return status;
}

/*
 * Transmits data word by word over
 * the SPI interface.
 */
CyFx3BootErrorCode_t 
CyFx3BootSpiTransmitWords (
        uint8_t *data, 
        uint32_t byteCount)
{
    uint8_t  wordLen;
    uint32_t i;
    uint32_t temp, timeout;
    CyFx3BootErrorCode_t status = CY_FX3_BOOT_SUCCESS;

    if (byteCount == 0)
    {
        return CY_FX3_BOOT_SUCCESS;
    }
    if (!data)
    {   
        return CY_FX3_BOOT_ERROR_NULL_POINTER;
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
        byteCount = wordLen;
    }

    CyFx3BootSpiResetFifo (CyTrue, CyFalse);

    /* Ensure that the interrupt lines are disabled. */
    SPI->lpp_spi_intr_mask = 0;

    /* Enable the TX. */
    SPI->lpp_spi_config |= CY_U3P_LPP_SPI_TX_ENABLE;

    /* Re-enable SPI block. */
    SPI->lpp_spi_config |= CY_U3P_LPP_SPI_ENABLE;

    for (i = 0; i < byteCount; i += wordLen)
    {
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

        /* Wait for the tx_space bit in status register */
        timeout = CY_FX3_BOOT_SPI_TIMEOUT;
        while (!(SPI->lpp_spi_status & CY_U3P_LPP_SPI_TX_SPACE))
        {
            if (timeout-- == 0)
            {
                status = CY_FX3_BOOT_ERROR_TIMEOUT;
                break;
            }
        }

        if (status != CY_FX3_BOOT_SUCCESS)
        {
            break;
        }

        SPI->lpp_spi_egress_data = temp;

        /* wait for the TX_DONE */
        status = CyFx3BootSpiWaitForBlkXfer (CY_U3P_LPP_SPI_TX_DONE, 0xFFFF);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            break;
        }

        /* Clear the TX_DONE interrupt. */
        SPI->lpp_spi_intr = CY_U3P_LPP_SPI_TX_DONE;
    }

    /* Disable the TX. */
    SPI->lpp_spi_config &= ~CY_U3P_LPP_SPI_TX_ENABLE;

    /* Clear all interrupts. */
    SPI->lpp_spi_intr |= CY_U3P_LPP_SPI_TX_DONE;

    /* Wait until the SPI block is no longer busy and disable. */
    while ((SPI->lpp_spi_status & CY_U3P_LPP_SPI_BUSY) != 0);
    SPI->lpp_spi_config &= ~CY_U3P_LPP_SPI_ENABLE;

    return status;
}

/*
 * Receive data word by word over the SPI interface.
 */
CyFx3BootErrorCode_t 
CyFx3BootSpiReceiveWords (
        uint8_t *data,
        uint32_t byteCount)
{
    uint8_t  wordLen;
    uint32_t i, temp, timeout;
    CyFx3BootErrorCode_t status = CY_FX3_BOOT_SUCCESS;

    if (byteCount == 0)
    {
        return CY_FX3_BOOT_SUCCESS;
    }
    if (!data)
    {   
        return CY_FX3_BOOT_ERROR_NULL_POINTER;
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
        byteCount = wordLen;
    }

    CyFx3BootSpiResetFifo (CyTrue, CyTrue);

    /* Disable interrupts. */
    SPI->lpp_spi_intr_mask = 0;

    /* Enable TX and RX. */
    SPI->lpp_spi_config |= (CY_U3P_LPP_SPI_TX_ENABLE | CY_U3P_LPP_SPI_RX_ENABLE);

    /* Re-enable SPI block. */
    SPI->lpp_spi_config |= CY_U3P_LPP_SPI_ENABLE;

    for (i = 0; i < byteCount; i += wordLen)
    {
        /* Wait for the tx_space bit in status register */
        timeout = CY_FX3_BOOT_SPI_TIMEOUT;
        while (!(SPI->lpp_spi_status & CY_U3P_LPP_SPI_TX_SPACE))
        {
            if (timeout-- == 0)
            {
                status = CY_FX3_BOOT_ERROR_TIMEOUT;
                break;
            }
        }

        if (status != CY_FX3_BOOT_SUCCESS)
        {
            break;
        }

        /* Transmit zero. */
        SPI->lpp_spi_egress_data = 0;

        /* wait for the TX_DONE and RX_DATA. */
        status = CyFx3BootSpiWaitForBlkXfer (CY_U3P_LPP_SPI_RX_DATA | CY_U3P_LPP_SPI_TX_DONE, 0xFFFF);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            break;
        }

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

    /* Clear all interrupts. */
    SPI->lpp_spi_intr |= CY_U3P_LPP_SPI_TX_DONE;

    /* Wait until the SPI block is no longer busy and disable. */
    while ((SPI->lpp_spi_status & CY_U3P_LPP_SPI_BUSY) != 0);
    SPI->lpp_spi_config &= ~CY_U3P_LPP_SPI_ENABLE;

    return status;
}

/* 
 * Enables the transferring of data.
 */
void
CyFx3BootSpiSetBlockXfer (
        uint32_t txSize,
        uint32_t rxSize)
{
    uint32_t temp = 0;

    CyFx3BootSpiResetFifo (CyTrue, CyTrue);

    /* Enable DMA mode. */
    SPI->lpp_spi_config |= CY_U3P_LPP_SPI_DMA_MODE;

    /* Update the counters. */
    SPI->lpp_spi_tx_byte_count = txSize;
    SPI->lpp_spi_rx_byte_count = rxSize;

    /* Enable Tx and Rx as required. */
    if (txSize)
    {
        temp = CY_U3P_LPP_SPI_TX_ENABLE;
    }

    if (rxSize)
    {
        temp |= CY_U3P_LPP_SPI_RX_ENABLE;
    }

    SPI->lpp_spi_config |= temp;

    /* Enable the SPI block. */
    SPI->lpp_spi_config |= CY_U3P_LPP_SPI_ENABLE;
    while ((SPI->lpp_spi_config & CY_U3P_LPP_SPI_ENABLE) == 0);
}

/* 
 * Disable the transferring data.
 */
void
CyFx3BootSpiDisableBlockXfer ( void )
{
    /* Wait until the SPI has come out of busy state */
    while ((SPI->lpp_spi_status & CY_U3P_LPP_SPI_BUSY) != 0);

    /* Disable the SPI Block */
    SPI->lpp_spi_config &= ~CY_U3P_LPP_SPI_ENABLE;
    while ((SPI->lpp_spi_config & CY_U3P_LPP_SPI_ENABLE) != 0);

    /* Disable DMA mode. */
    SPI->lpp_spi_config &= ~(CY_U3P_LPP_SPI_TX_ENABLE | CY_U3P_LPP_SPI_RX_ENABLE | CY_U3P_LPP_SPI_DMA_MODE);
}

CyFx3BootErrorCode_t
CyFx3BootSpiDmaXferData (
        CyBool_t isRead,
        uint32_t address,
        uint32_t length,
        uint32_t timeout
        )
{
    uint16_t socket = CY_FX3_LPP_SOCKET_SPI_CONS;

    if ((address < CY_FX3_BOOT_SYSMEM_BASE) || (address >= CY_FX3_BOOT_SYSMEM_END))
        return CY_FX3_BOOT_ERROR_INVALID_DMA_ADDR;

    if (isRead)
    {
        socket = CY_FX3_LPP_SOCKET_SPI_PROD;
    }

    CyFx3BootDmaXferData (isRead, socket, address, length);

    if (isRead)
    {
        return CyFx3BootSpiWaitForBlkXfer (CY_U3P_LPP_SPI_RX_DONE, timeout);
    }
    else
    {
        return CyFx3BootSpiWaitForBlkXfer (CY_U3P_LPP_SPI_TX_DONE, timeout);
    }
}

/*[]*/

