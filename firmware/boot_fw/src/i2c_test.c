/*
## Cypress FX3 Boot Firmware Example Source file (i2c_test.c)
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

#include <cyfx3i2c.h>
#include <cyfx3device.h>
#include <cyfx3utils.h>

static uint32_t i2c_bitrate[] = {400000,350000,300000,250000,200000,150000,100000, 1000000};
extern void myMemSet (uint8_t *d, uint8_t c, int32_t cnt);

#define I2C_REG_XFER_SIZE       (32)    /*8,16,32,64 sizes tested*/
#define I2C_PAGE_SIZE           (64)
#define I2C_DMA_XFER_SIZE       (16)

/* Using global buffers for DMA transfers, as stack variables cannot be used for DMA transfers. */
static uint8_t I2cWriteBuffer[I2C_DMA_XFER_SIZE];
static uint8_t I2cReadBuffer[I2C_DMA_XFER_SIZE];

CyFx3BootErrorCode_t 
testI2cRegMode (
        void )
{
    CyFx3BootI2cConfig_t i2cConfig;
    CyFx3BootI2cPreamble_t preamble;

    uint8_t size,i;
    uint16_t j = 0;
    uint8_t wr_buf[I2C_REG_XFER_SIZE];
    uint8_t rd_buf[I2C_REG_XFER_SIZE];
    CyFx3BootErrorCode_t status;

    uint8_t divider = (sizeof(i2c_bitrate)/sizeof(i2c_bitrate[0]));

    size = (uint8_t)I2C_REG_XFER_SIZE;
	
    status = CyFx3BootI2cInit();
    if (status != CY_FX3_BOOT_SUCCESS)
        return status;

    i2cConfig.busTimeout = 0xFFFFFFFF;
    i2cConfig.dmaTimeout = 0xFFFF;
    i2cConfig.isDma = CyFalse;

    for (j = 0; j < 250; j++)
    {
        /* Run at various bit rates. */
        i2cConfig.bitRate = i2c_bitrate[j % divider];

        status = CyFx3BootI2cSetConfig (&i2cConfig);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }
        
        /* The address at which read/write is to be performed */
        preamble.buffer[1] = j;
        preamble.buffer[2] = 0;

        /*For EEPROM 64 byte page allignment....*/
        if((preamble.buffer[2] % I2C_PAGE_SIZE) != 0)
        {
            preamble.buffer[2] = (preamble.buffer[2] / I2C_PAGE_SIZE) *
                I2C_PAGE_SIZE;
        }
 
        preamble.buffer[0] = 0xA0; /* Write to I2C */
        preamble.length = 3;
        preamble.ctrlMask = 0x0000;

        myMemSet (wr_buf, j, I2C_REG_XFER_SIZE);

        status = CyFx3BootI2cTransmitBytes (&preamble, wr_buf, size, 200);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        preamble.length = 1;
        status = CyFx3BootI2cWaitForAck (&preamble, 200);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        /* Read the written location */
        preamble.buffer[3] = 0xA1;
        preamble.buffer[0] = 0xA0;
        preamble.length = 4;
        preamble.ctrlMask = 0x0004;

        status = CyFx3BootI2cReceiveBytes (&preamble, rd_buf, size, 200);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        /* Compare the received data */
        for (i = 0; i < size; i++)
        {
            if(rd_buf[i] != wr_buf[i])
            {
                /* Compare Error. Return apt error code. */
                return CY_FX3_BOOT_ERROR_FAILURE;
            }
        }

        CyFx3BootBusyWait (10);
    }

    CyFx3BootI2cDeInit();
    return CY_FX3_BOOT_SUCCESS;
}

CyFx3BootErrorCode_t
testI2cDmaMode (
        void)
{
    CyFx3BootErrorCode_t status;
    CyFx3BootI2cConfig_t i2cConfig;
    CyFx3BootI2cPreamble_t preamble;

    uint32_t byte_count;
    uint16_t i = 0;
    uint16_t j = 0;

    uint8_t divider = (sizeof(i2c_bitrate)/sizeof(i2c_bitrate[0]));

    byte_count = I2C_DMA_XFER_SIZE;

    /* Fill up the write buffer */
    for (i = 0; i < I2C_DMA_XFER_SIZE; i++)
    {
        I2cWriteBuffer[i] = 0xA5;
    }

    /* Initialize the I2C */
    status = CyFx3BootI2cInit();
    if (status != CY_FX3_BOOT_SUCCESS)
        return status;

    i2cConfig.busTimeout = 0xFFFFFFFF;
    i2cConfig.dmaTimeout = 0xFFFF;
    i2cConfig.isDma = CyTrue;

    for (j = 0; j < 250; j++)
    {
        i2cConfig.bitRate = i2c_bitrate[j % divider];

        status = CyFx3BootI2cSetConfig (&i2cConfig);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        /* The address at which read/write is to be performed */
        (*(uint16_t *)&preamble.buffer[1]) = j;

        /*For EEPROM 64 byte page allignment....*/
        if((preamble.buffer[2] % I2C_PAGE_SIZE) != 0)
        {
            preamble.buffer[2] = (preamble.buffer[2] / I2C_PAGE_SIZE) *
                I2C_PAGE_SIZE;
        }

        preamble.buffer[0] = 0xA0; /* Write to I2C */
        preamble.length = 3;
        preamble.ctrlMask = 0x0000;

        /*For EEPROM 64 byte page allignment....*/
        if((preamble.buffer[2] % I2C_DMA_XFER_SIZE) != 0)
        {
            preamble.buffer[2] = (preamble.buffer[2]/I2C_DMA_XFER_SIZE)*I2C_DMA_XFER_SIZE;
        }

        status = CyFx3BootI2cSendCommand (&preamble, byte_count, CyFalse);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        status = CyFx3BootI2cDmaXferData (CyFalse, (uint32_t)I2cWriteBuffer, I2C_DMA_XFER_SIZE, 200);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        /*Send the command to i2c eeprom for read*/
        preamble.buffer[3] = 0xA1;
        preamble.buffer[0] = 0xA0;
        preamble.length = 4;
        preamble.ctrlMask = 0x0004;

        /* Have to wait for approx. 5 ms to ensure that the slave has completed 
         * writing the data. */
        CyFx3BootBusyWait (3500);

        status = CyFx3BootI2cSendCommand (&preamble, byte_count, CyTrue);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        status = CyFx3BootI2cDmaXferData (CyTrue, (uint32_t)I2cReadBuffer, I2C_DMA_XFER_SIZE, 200);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        /* Wait for the read dma transfer to complete */

        for (i = 0; i < I2C_DMA_XFER_SIZE; i++)
        {
            if (I2cReadBuffer[i] != I2cWriteBuffer[i])
            {
                return CY_FX3_BOOT_ERROR_FAILURE;  
            }
        }
    }

    CyFx3BootI2cDeInit();
    return CY_FX3_BOOT_SUCCESS;
}


