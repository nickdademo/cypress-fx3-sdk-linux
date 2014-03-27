/*
 ## Cypress FX3 Boot Firmware Example Source file (spi_test.c)
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

#include <cyfx3spi.h>
#include <cyfx3device.h>
#include <cyfx3utils.h>

/*
 * Note: Address of 4 KB DMA scratch buffer used for SPI data transfers. This is located outside of the
 * 32 KB region allocated for the boot firmware code and data, and is expected to overlap the DMA buffer
 * region used by the full FX3 firmware image.
 *
 * Turn on the CYMEM_256K pre-processor definition to build this binary for the CYUSB3011/CYUSB3012 devices
 * that only have 256 KB of System RAM.
 */
#ifdef CYMEM_256K
#define SPI_DMA_BUF_ADDRESS     (0x40037000)
#else
#define SPI_DMA_BUF_ADDRESS     (0x40077000)
#endif

#define SPI_DMA_XFER_SIZE (64)
#define SPI_REG_XFER_SIZE (256)

/*
   Summary
   Creates a word from four eight bit numbers.

   Description
   The macro combines four eight bit unsigned numbers
   to form a single 32 bit unsigned number.
*/
#define CY_U3P_MAKEDWORD(b3, b2, b1, b0) ((uint32_t)((((uint32_t)(b3)) << 24) | (((uint32_t)(b2)) << 16) | \
                                         (((uint32_t)(b1)) << 8) | ((uint32_t)(b0))))


/* Note:
   The following test code has been tested against the SPI part # M25P40

*/
static uint32_t spiDmaBitrate[] = {
    10000000,
    8000000,
    6000000,
    4000000,
    2000000,
    1000000
    };

static uint32_t spiBitrate[] = {
    10000000,
    8000000,
    6000000,
    5000000,
    4000000,
    3000000,
    2000000,
    1000000
    };

extern void myMemCopy(uint8_t *d, uint8_t *s, int cnt);

CyFx3BootErrorCode_t 
spiWriteEnable()
{
    uint8_t buf[SPI_DMA_XFER_SIZE];
    CyFx3BootErrorCode_t status;

    buf[0] = 0x06;/*WR_ENABLE*/

    CyFx3BootSpiSetSsnLine(CyFalse);
    
    status = CyFx3BootSpiTransmitWords(buf,1);
    if(status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }
    
    /* set chip enable line high */
    CyFx3BootSpiSetSsnLine(CyTrue);

    return CY_FX3_BOOT_SUCCESS;
}

/* Function to check if the SPI is ready for data transfers */
CyFx3BootErrorCode_t 
waitForSpiStatus (void)
{
    uint8_t buf[2];
    CyFx3BootErrorCode_t status = CY_FX3_BOOT_SUCCESS;

    do 
    {
        spiWriteEnable();
        buf[0] = 0x05;

        CyFx3BootSpiSetSsnLine(CyFalse);

        status = CyFx3BootSpiTransmitWords(buf,1);
        if(status != CY_FX3_BOOT_SUCCESS)
        {
            break;
        }

        status = CyFx3BootSpiReceiveWords(buf,2);
        if(status != CY_FX3_BOOT_SUCCESS)
        {
            break;
        }

        CyFx3BootSpiSetSsnLine(CyTrue);

    } while ((buf[0] & 1)|| (!(buf[0] & 0x2))); 

    return status;
}

CyFx3BootErrorCode_t
spiReadBytes (
        uint32_t address,
        uint16_t length,
        uint8_t *buffer)
{
    CyFx3BootErrorCode_t status = CY_FX3_BOOT_SUCCESS;
    uint8_t addr[5];

    /* TODO: Modify this according to the SPI devices */
    addr[0] = 0x03;
    addr[1] = (address >> 16) & 0xFF;

    /* */
    addr[2] = (address >> 8) & 0xFF;
    addr[3] = (address) & 0xFF;

    CyFx3BootSpiSetSsnLine(CyFalse);

    /* TODO: Modify the length according to the SPI devices */
    status = CyFx3BootSpiTransmitWords(addr, 4);
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    status = CyFx3BootSpiReceiveWords(buffer, length);
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    CyFx3BootSpiSetSsnLine(CyTrue);

    return CY_FX3_BOOT_SUCCESS;
}

CyFx3BootErrorCode_t 
bulkErase (void)
{
    CyFx3BootErrorCode_t status = CY_FX3_BOOT_SUCCESS;
    uint8_t addr[5];

    addr[0] = 0xC7;
    status = spiWriteEnable();

    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    CyFx3BootSpiSetSsnLine(CyFalse);

    status = CyFx3BootSpiTransmitWords(addr, 1);
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    CyFx3BootSpiSetSsnLine(CyTrue);
    return CY_FX3_BOOT_SUCCESS;
}

CyFx3BootErrorCode_t 
sectorErase (
        uint32_t address )
{
    CyFx3BootErrorCode_t status = CY_FX3_BOOT_SUCCESS;
    uint8_t addr[5];

    addr[0] = 0xD8;
    addr[1] = (address >> 16) & 0xFF;

    /* */
    addr[2] = (address >> 8) & 0xFF;
    addr[3] = (address) & 0xFF;

    status = spiWriteEnable();

    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    CyFx3BootSpiSetSsnLine(CyFalse);

    status = CyFx3BootSpiTransmitWords(addr, 4);
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    CyFx3BootSpiSetSsnLine(CyTrue);
    return CY_FX3_BOOT_SUCCESS;
}

CyFx3BootErrorCode_t 
spiWriteBytes (
        uint32_t address, 
        uint16_t length,
        uint8_t *data)
{
    CyFx3BootErrorCode_t status;
    uint8_t addr[5];

    /* TODO: Modify this according to the SPI devices */
    addr[0] = 0x02;
    addr[1] = (address >> 16) & 0xFF;
   
    /* */
    addr[2] = (address >> 8) & 0xFF;
    addr[3] = (address) & 0xFF;


    /* Wait for SPI to be ready */
    status = waitForSpiStatus();
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    CyFx3BootSpiSetSsnLine(CyFalse);

    /* TODO: Modify the length according to the SPI devices */
    status = CyFx3BootSpiTransmitWords(addr, 4);
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    status = CyFx3BootSpiTransmitWords(data, length);
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }
    
    CyFx3BootSpiSetSsnLine(CyTrue);
    return CY_FX3_BOOT_SUCCESS;
}

CyBool_t
bootFromSpi()
{
    uint8_t buffer[64];

    uint32_t spiAddress = 0;
    int32_t firmwareImagePtr = 0;
    int32_t sectionLength;
    int32_t sectionAddress;
    int32_t downloadAddress;
    int32_t bytesLeftToDownload;
    CyBool_t isTrue = CyTrue;
    CyFx3BootErrorCode_t status;
    uint8_t addr[4];

    /* Read 4 bytes from the SPI
       Check "CY" signature (0x43,0x59)
       */
    spiAddress = 0;
    status = spiReadBytes (spiAddress, 4, buffer);
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return CyFalse;
    }
    status = waitForSpiStatus ();
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    /* validate the signature */
    if ((buffer[firmwareImagePtr] != 0x43) || (buffer[firmwareImagePtr + 1] != 0x59))
    {	
        return CyFalse;
    }

    /* Skip the next two bytes. */

    /* Download one section at a time to the device.
       Optional: Checksum can be computed on the section being downloaded,
       after downloading the section, it can be read back and the checksum 
       can be verified.
     */
    spiAddress += 4;

    while (isTrue)
    {
        /* Read 8 bytes from the SPI */
        status = spiReadBytes (spiAddress, 8, buffer);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return CyFalse;
        }

        status = waitForSpiStatus ();
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        /* Get section length (4 bytes) and convert it from 32-bit word count to byte count */

        sectionLength = (CY_U3P_MAKEDWORD(buffer[3], buffer[2], buffer[1], buffer[0]));
        sectionLength = sectionLength << 2;

        /* If length = 0, the transfer is complete */
        if (sectionLength == 0)
        {
            break;
        }

        /* Get section address (4 bytes) */
        sectionAddress = (*(uint32_t *)&buffer[4]);

        bytesLeftToDownload = sectionLength;
        downloadAddress = sectionAddress;

        spiAddress += 8;

        while (bytesLeftToDownload > 0)
        {
            int32_t bytesToTransfer = SPI_DMA_XFER_SIZE;
            if (bytesLeftToDownload < SPI_DMA_XFER_SIZE)
            {
                bytesToTransfer = bytesLeftToDownload;
            }

            bytesToTransfer = bytesToTransfer - (spiAddress % SPI_DMA_XFER_SIZE);

            /* Configure the SPI for the dma transfer by sending down the
             * address of the data to be transferred. 
             * */
            status = spiWriteEnable();
            if (status != CY_FX3_BOOT_SUCCESS)
            {
                return status;
            }

            CyFx3BootSpiSetSsnLine(CyFalse);

            addr[0] = 0x03;
            addr[1] = (spiAddress >> 16) & 0xFF;
            addr[2] = (spiAddress >> 8) & 0xFF;
            addr[3] = (spiAddress) & 0xFF;

            status = CyFx3BootSpiTransmitWords (addr, 4);
            if (status != CY_FX3_BOOT_SUCCESS)
            {
                return status;
            }

            /* Invoke the SPI function to do the DMA transfer */
            CyFx3BootSpiSetBlockXfer (0, SPI_DMA_XFER_SIZE);

            /* Validate ITCM Memory */
            if ((downloadAddress + bytesToTransfer) < CY_FX3_BOOT_ITCM_END)
            {
                /* Set up the SPI DMA transfer to System Memory */
                status = CyFx3BootSpiDmaXferData (CyTrue, SPI_DMA_BUF_ADDRESS, SPI_DMA_XFER_SIZE, 100);
            }
            else
            {
                /* Set up the SPI DMA transfer to System Memory */
                status = CyFx3BootSpiDmaXferData (CyTrue, downloadAddress, SPI_DMA_XFER_SIZE, 100);
            }

            if (status != CY_FX3_BOOT_SUCCESS)
            {
                return CyFalse;
            }

            /* Validate ITCM Memory */
            if ((downloadAddress + bytesToTransfer) < CY_FX3_BOOT_ITCM_END)
            {
                /* Waiting for about 1 ms for the DMA transfer to be completed before copying the data to the ITCM. */
                myMemCopy((uint8_t*)downloadAddress, (uint8_t *)SPI_DMA_BUF_ADDRESS, bytesToTransfer); 
            }

            CyFx3BootSpiSetSsnLine(CyTrue);

            CyFx3BootSpiDisableBlockXfer ();

            downloadAddress += bytesToTransfer;
            bytesLeftToDownload -= bytesToTransfer;
            spiAddress += bytesToTransfer;
        }
    }

    /* Read 4 bytes from the SPI */
    status = spiReadBytes (spiAddress, 8, buffer);
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return CyFalse;
    }

    status = waitForSpiStatus ();
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    if (CY_U3P_MAKEDWORD(buffer[3], buffer[2], buffer[1], buffer[0]) == 0)
    {
        /* Jump to the program entry */
        CyFx3BootJumpToProgramEntry ((*(uint32_t *)&buffer[4]));
    }

    return CyTrue;
}

CyFx3BootErrorCode_t 
testSpiDmaMode ()
{
    CyFx3BootErrorCode_t status = CY_FX3_BOOT_SUCCESS;
    CyFx3BootSpiConfig_t spiConfig;

    uint8_t wrBuf[SPI_DMA_XFER_SIZE],rdBuf[SPI_DMA_XFER_SIZE];

    uint8_t location[4],i,j;
    uint32_t address = 0x70000;
	static uint16_t spiBitrateIndex = 0;

    spiConfig.isLsbFirst = CyFalse;
    spiConfig.cpol       = CyFalse;
    spiConfig.cpha       = CyFalse;
    spiConfig.leadTime   = CY_FX3_BOOT_SPI_SSN_LAG_LEAD_HALF_CLK;
    spiConfig.lagTime    = CY_FX3_BOOT_SPI_SSN_LAG_LEAD_HALF_CLK;
    spiConfig.ssnCtrl    = CY_FX3_BOOT_SPI_SSN_CTRL_FW;
    spiConfig.ssnPol     = CyFalse;
    spiConfig.wordLen    = 8;

    status = sectorErase (address);
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    /* Wait for SPI to be ready */
    status = waitForSpiStatus();
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    for (j = 0; j < 6; j++)
    {
        for (i = 0; i < SPI_DMA_XFER_SIZE; i++)
        {
            wrBuf[i] = j + i;
        }

        CyFx3BootSpiSetSsnLine(CyFalse);

        /* send command and addr for PP */
        location[0] = 0x02;
        location[1] = (address >> 16) & 0xFF;
        location[2] = (address >> 8) & 0xFF;
        location[3] = (address) & 0xFF;

        status = CyFx3BootSpiTransmitWords (location, 4);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        /* Invoke the SPI function to do the DMA transfer */
        CyFx3BootSpiSetBlockXfer(SPI_DMA_XFER_SIZE, 0);

        status = CyFx3BootSpiDmaXferData (CyFalse, (uint32_t)wrBuf, SPI_DMA_XFER_SIZE, 100);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        CyFx3BootSpiSetSsnLine(CyTrue);

        CyFx3BootSpiDisableBlockXfer ();

        /*****Read data************/
        status = waitForSpiStatus();
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        status = spiWriteEnable();
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        CyFx3BootSpiSetSsnLine(CyFalse);

        location[0] = 0x03;
        location[1] = (address >> 16) & 0xFF;
        location[2] = (address >> 8) & 0xFF;
        location[3] = (address) & 0xFF;

        status = CyFx3BootSpiTransmitWords(location, 4);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        CyFx3BootSpiSetBlockXfer(0, SPI_DMA_XFER_SIZE);

        status = CyFx3BootSpiDmaXferData (CyTrue, (uint32_t)rdBuf, SPI_DMA_XFER_SIZE, 100);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        CyFx3BootSpiSetSsnLine(CyTrue);

        CyFx3BootSpiDisableBlockXfer ();
        /* Compare data */
        for(i=0;i<SPI_DMA_XFER_SIZE;i++)
        {
            if(rdBuf[i] != wrBuf[i])
            {
                return status;
            }
        }

        address += SPI_DMA_XFER_SIZE;

        /*reconfigure spi with different clock freq.*/
        spiBitrateIndex++;
        spiBitrateIndex %= (sizeof (spiDmaBitrate))/4;

        spiConfig.clock = spiDmaBitrate[spiBitrateIndex];

        status = CyFx3BootSpiSetConfig(&spiConfig);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }
    }

    /*spi deinit*/
    CyFx3BootSpiDeInit();
    return status;
}

CyFx3BootErrorCode_t 
initSpi (void)
{
    CyFx3BootSpiConfig_t spiConfig;
    CyFx3BootErrorCode_t stat;

    static uint16_t spiBitrateIndex = 0;
       
    stat = CyFx3BootSpiInit();
    if (stat != CY_FX3_BOOT_SUCCESS)
        return stat;

    spiConfig.isLsbFirst = CyFalse;
    spiConfig.cpol       = CyFalse;
    spiConfig.cpha       = CyFalse;
    spiConfig.leadTime   = CY_FX3_BOOT_SPI_SSN_LAG_LEAD_HALF_CLK;
    spiConfig.lagTime    = CY_FX3_BOOT_SPI_SSN_LAG_LEAD_HALF_CLK;
    spiConfig.ssnCtrl    = CY_FX3_BOOT_SPI_SSN_CTRL_FW;
    spiConfig.ssnPol     = CyFalse;
    spiConfig.clock      = spiBitrate[spiBitrateIndex];
    spiConfig.wordLen    = 8;
    
    return CyFx3BootSpiSetConfig(&spiConfig);
}

CyFx3BootErrorCode_t 
eraseFlash ()
{
    CyFx3BootErrorCode_t status = CY_FX3_BOOT_SUCCESS;
    status = bulkErase ();
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    /* Wait for SPI to be ready */
    return waitForSpiStatus();
}

CyFx3BootErrorCode_t 
testSpiRegMode (void)
{
    CyFx3BootErrorCode_t status = CY_FX3_BOOT_SUCCESS;
    CyFx3BootSpiConfig_t spiConfig;

    uint8_t writeBuffer[SPI_REG_XFER_SIZE],readBuffer[SPI_REG_XFER_SIZE];
    uint16_t i,j;
    static uint16_t spiBitrateIndex = 0;
    uint32_t address = 0x70000;

    spiConfig.isLsbFirst = CyFalse;
    spiConfig.cpol       = CyFalse;
    spiConfig.cpha       = CyFalse;
    spiConfig.leadTime   = CY_FX3_BOOT_SPI_SSN_LAG_LEAD_HALF_CLK;
    spiConfig.lagTime    = CY_FX3_BOOT_SPI_SSN_LAG_LEAD_HALF_CLK;
    spiConfig.ssnCtrl    = CY_FX3_BOOT_SPI_SSN_CTRL_FW;
    spiConfig.ssnPol     = CyFalse;
    spiConfig.wordLen    = 8;

    status = sectorErase (address);

    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }
    status = waitForSpiStatus ();
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0; i < SPI_REG_XFER_SIZE; i++)
        {
            writeBuffer[i] = 0xA5;
            readBuffer[i] = 0;
        }

        /*Wait for SPI to be ready*/
        status = waitForSpiStatus();
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        status = spiWriteBytes (address, SPI_REG_XFER_SIZE, writeBuffer);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        /*Wait for SPI to be ready*/
        status = waitForSpiStatus();
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        status = spiReadBytes (address, SPI_REG_XFER_SIZE, readBuffer);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        /*Wait for SPI to be ready*/
        status = waitForSpiStatus();
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        /*Compare the received data*/
        for (i = 0; i < SPI_REG_XFER_SIZE; i++)
        {
            if(readBuffer[i] != writeBuffer[i])
            {
                return CY_FX3_BOOT_ERROR_FAILURE;
            }
        }
        address += SPI_REG_XFER_SIZE;

        spiBitrateIndex++;
        spiBitrateIndex %= (sizeof (spiBitrate))/4;
        /* Reconfigure spi with different clock/bitrate */
        spiConfig.clock = spiBitrate[spiBitrateIndex];

        status = CyFx3BootSpiSetConfig(&spiConfig);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }
    }

    return status;
}

