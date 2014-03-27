/*
 ## Cypress FX3 Boot Firmware Example Source file (test_uart.c)
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

/* UART Register mode and DMA mode example file. */
#include <cyfx3uart.h>
#include <cyfx3device.h>
#include <cyfx3utils.h>

CyFx3BootErrorCode_t
testUartRegMode ( 
        void )
{
    CyFx3BootUartConfig_t uartConfig;
    CyFx3BootErrorCode_t status;
    uint8_t RxByte[64], repeat;
    uint32_t count = 0;
    uint16_t j = 0;

    uint8_t bytesToRecv = 64;

    status = CyFx3BootUartInit ();
    if (status != CY_FX3_BOOT_SUCCESS)
        return status;

    uartConfig.stopBit = CY_FX3_BOOT_UART_ONE_STOP_BIT;
    uartConfig.parity = CY_FX3_BOOT_UART_NO_PARITY;
    uartConfig.flowCtrl = CyFalse;

    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyTrue; 

    /* Reg mode */
    uartConfig.isDma = CyFalse;

    uartConfig.baudRate = CY_FX3_BOOT_UART_BAUDRATE_115200;
    status = CyFx3BootUartSetConfig (&uartConfig);
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    for (j = 0; j < 32; j++)
    {
        repeat = 0;
        do
        {
            count = CyFx3BootUartReceiveBytes(&RxByte[0], bytesToRecv);
            if (count == 0)
            {
                return CY_FX3_BOOT_ERROR_FAILURE;
            }
        } while ((count != bytesToRecv) && (repeat++ < 2));

        /*send byte*/
        repeat = 0;
        do
        {
            count = CyFx3BootUartTransmitBytes (&RxByte[0], bytesToRecv);
            if (count == 0)
            {
                return CY_FX3_BOOT_ERROR_FAILURE;
            }
        } while ((count != bytesToRecv) && (repeat++ < 2));
    }

    CyFx3BootUartDeInit();
    return CY_FX3_BOOT_SUCCESS;
}

static uint8_t UartXferBuffer[64];

CyFx3BootErrorCode_t
testUartDmaMode ( 
        void )
{
    CyFx3BootUartConfig_t uartConfig;
    CyFx3BootErrorCode_t status;
    int i = 0;
    uint8_t bytesToRecv = 64;

    status = CyFx3BootUartInit ();
    if (status != CY_FX3_BOOT_SUCCESS)
        return status;

    uartConfig.baudRate = CY_FX3_BOOT_UART_BAUDRATE_9600;
    uartConfig.stopBit = CY_FX3_BOOT_UART_ONE_STOP_BIT;
    uartConfig.parity = CY_FX3_BOOT_UART_NO_PARITY;
    uartConfig.flowCtrl = CyFalse;

    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyTrue; 

    uartConfig.isDma = CyTrue;
    status = CyFx3BootUartSetConfig (&uartConfig);
    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    for (i = 0; i < 32; i++)
    {
        CyFx3BootUartRxSetBlockXfer(bytesToRecv);

        status = CyFx3BootUartDmaXferData (CyTrue, (uint32_t)UartXferBuffer, bytesToRecv, CY_FX3_BOOT_WAIT_FOREVER);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }

        CyFx3BootUartTxSetBlockXfer(bytesToRecv);

        status = CyFx3BootUartDmaXferData (CyFalse, (uint32_t)UartXferBuffer, bytesToRecv, CY_FX3_BOOT_WAIT_FOREVER);
        if (status != CY_FX3_BOOT_SUCCESS)
        {
            return status;
        }
    }
    CyFx3BootUartDeInit();
    return CY_FX3_BOOT_SUCCESS;
}

/*[]*/

