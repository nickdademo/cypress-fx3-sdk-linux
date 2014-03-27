/*
 ## Cypress FX3S firmware example (cyfx3s_sdiouart.c)
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

/* This file implements a USB to SDIO-UART application */

#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3error.h"
#include "cyu3uart.h"
#include "cyu3dma.h"
#include "cyu3usb.h"
#include "cyu3gpio.h"
#include "cyu3cardmgr_fx3s.h"
#include "cyfx3s_sdiouart.h"
#include "cyu3utils.h"

static CyU3PThread SDIOUARTAppThread;
static CyU3PDmaChannel glChHandleUsbOut;           /* DMA MANUAL (USB TO CPU for UART Tx) channel handle.*/
static CyU3PDmaChannel glChHandleUsbIn;            /* DMA MANUAL (CPU TO USB for UART Rx) channel handle.*/
static CyU3PDmaChannel glChHandleCputoUsb;         /* DMA MANUAL OUT CHANNEL (CPU TO USB) for Int EP Xfers. */

static CyU3PSibDevInfo_t   glDevInfo[2];
static CyFxSdioCardData_t glSdioCardInfo[2];
static UARTSettings_t glUartSettings;

static CyBool_t glIsApplnActive = CyFalse;       /* Whether the application is active or not. */
static CyU3PUartConfig_t glUartConfig = {0};     /* Global UART Configuration structure. */
static uint8_t serialstate_info[32];             /* Array to handle SERIAL_STATE notification. */

/* Application Error Handler */
void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus  /* API return status */
        )
{
    /* Application failed with the error code apiRetStatus */

    /* Add custom debug or recovery actions here */

    /* Loop Indefinitely */
    for (;;)
    {
        /* Thread sleep : 100 ms */
        CyU3PThreadSleep (100);
    }
}

/* -------------------------------------------------------------------------
 * ------------------ SDIO Function Configuration routines -----------------
 * -------------------------------------------------------------------------
 */

/* De-Initialize an SDIO function*/
static CyU3PReturnStatus_t
CyFxSdioFunctionDeInit (
        uint8_t portId,
        uint8_t funcNo)
{
    uint8_t data;
    CyU3PReturnStatus_t status;

    if ((funcNo > 7) || (funcNo == 0))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Check if the Function is Initialized */
    if (!glSdioCardInfo[portId].fbr[funcNo - 1].isInitialized)
    {
        return CY_U3P_SUCCESS;
    }

    /* Read the IEn register. */
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
            CY_U3P_SDIO_REG_IO_ENABLE, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Write back to IEn register disabling the selected function. */
    data &= ~(1 << funcNo);
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_WRITE, 0,
            CY_U3P_SDIO_REG_IO_ENABLE, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    glSdioCardInfo[portId].fbr[funcNo-1].isInitialized = 0;
    return status;
}

/* Get FBR register data and information from the Function's CIS Tuple */
static CyU3PReturnStatus_t
CyFxSdioGetFbrData (
        uint8_t portId,
        uint8_t funcNo)
{
    CyU3PReturnStatus_t status;
    uint8_t data, tupleSize;
    uint32_t baseAddress = (funcNo << 8);
    uint8_t buff[256];
    CyFxSdioFbr_t * fbr;

    fbr = &glSdioCardInfo[portId].fbr[funcNo - 1];

    /* Read Standard Interface Code */
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
            baseAddress | CY_U3P_SDIO_REG_FBR_INTERFACE_CODE, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    fbr->interfaceCode = (data & 0xF);
    fbr->supportCSA    = (data >> 6) & 0x1;

     /* Read Extended Interface Code*/
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION , CY_U3P_SDIO_READ, 0,
            baseAddress | CY_U3P_SDIO_REG_FBR_EXT_INTERFACE_CODE, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    fbr->extInterfaceCode = data;

    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION , CY_U3P_SDIO_READ, 0,
            baseAddress | CY_U3P_SDIO_REG_FBR_CIS_PTR_DO, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    fbr->addrCIS = (uint32_t)data;

    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION , CY_U3P_SDIO_READ, 0,
            baseAddress | CY_U3P_SDIO_REG_FBR_CIS_PTR_D1, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    fbr->addrCIS |= (data << 8);

    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION , CY_U3P_SDIO_READ, 0,
            baseAddress | CY_U3P_SDIO_REG_FBR_CIS_PTR_D2, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    fbr->addrCIS |= (data << 16);
    fbr->addrCSA  = 0;
    if (fbr->supportCSA)
    {
        status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION , CY_U3P_SDIO_READ, 0,
                baseAddress|CY_U3P_SDIO_REG_FBR_CSA_PTR_DO, &data);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        fbr->addrCSA = (uint32_t)data;

        status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION , CY_U3P_SDIO_READ, 0,
                baseAddress | CY_U3P_SDIO_REG_FBR_CSA_PTR_D1, &data);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        fbr->addrCSA |= ((uint32_t)data) << 8;

        status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION , CY_U3P_SDIO_READ, 0,
                baseAddress | CY_U3P_SDIO_REG_FBR_CSA_PTR_D2, &data);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        fbr->addrCSA |= ((uint32_t)data) << 16;
    }

    status = CyU3PSdioGetTuples (portId, funcNo, CY_U3P_SDIO_CISTPL_FUNCE, fbr->addrCIS, buff, &tupleSize);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Get Tuple Information */
    fbr->supportWakeUp = buff[1];
    fbr->cardPSN       = CY_U3P_MAKEDWORD (buff[6], buff[5], buff[4], buff[3]);
    fbr->csaProperty   = buff[0xB];
    fbr->csaSize       = CY_U3P_MAKEDWORD (buff[10], buff[9], buff[8], buff[7]);

    if (glSdioCardInfo[portId].cardInfo.cardCapability & CY_U3P_SDIO_CARD_CAPABLITY_SMB)
    {
        fbr->maxIoBlockSize = CY_U3P_MAKEWORD (buff[0xD], buff[0xC]);

        /* Set the IO Block Size for function */
        if (fbr->maxIoBlockSize)
        {
            status = CyU3PSdioSetBlockSize (portId, funcNo, fbr->maxIoBlockSize);

            if (status != CY_U3P_SUCCESS)
            {
                return status;
            }
        }
    }

    return status;
}

/* Initiailize an SDIO Function */
static CyU3PReturnStatus_t
CyFxSdioFunctionInit (
        uint8_t portId,
        uint8_t funcNo
        )
{
    uint8_t data, bitFnNo;
    uint8_t funcIndex;
    CyU3PReturnStatus_t status;
    uint32_t trials=0;

    if ((funcNo > 7) || (funcNo == 0))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Check if the function is already initialized. */
    funcIndex = funcNo - 1;
    if (glSdioCardInfo[portId].fbr[funcIndex].isInitialized)
    {
        CyFxSdioFunctionDeInit (portId, funcNo);
    }

    /* Read the IO Enable Register */
    data = 0;
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
            CY_U3P_SDIO_REG_IO_ENABLE, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Initialize the Function */
    bitFnNo  = (1 << funcNo);
    data    |= bitFnNo;
    status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_WRITE, 0,
            CY_U3P_SDIO_REG_IO_ENABLE, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Wait till IO Ready is set or 100 attempts */
    trials = 100;
    data   = 0;
    while ((!(data & bitFnNo)) && (trials > 0))
    {
        status = CyU3PSdioByteReadWrite (portId, CY_U3P_SDIO_CIA_FUNCTION, CY_U3P_SDIO_READ, 0,
                CY_U3P_SDIO_REG_IO_READY, &data);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        trials--;
    }

    if (!(data & bitFnNo))
    {
        return CY_U3P_ERROR_TIMEOUT;
    }

    glSdioCardInfo[portId].fbr[funcIndex].isInitialized = 1;
    status = CyFxSdioGetFbrData (portId, funcNo);
    if (status != CY_U3P_SUCCESS)
    {
        CyFxSdioFunctionDeInit (portId, funcNo);
    }

    return status;
}

/* ------------------------------------------------------------------- */
/* ---- End of SDIO Function Access and Configuration routines ------- */
/* ------------------------------------------------------------------- */


/* -------------------------------------------------------------------------
 * --------- UART Data Transfer and Configuration routines -----------------
 * -------------------------------------------------------------------------
 */

/* Function to Send Data out of SDIO UART Function (USB->UART transfer)*/
CyU3PReturnStatus_t
CySdioUartTxData (
        uint8_t  *buffer,
        uint16_t  count)
{
    uint16_t i;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint8_t portId, funcNo;
    uint8_t statusReg;

    portId = glUartSettings.sdioPort;
    funcNo = glUartSettings.sdioFunction;

    /* Transmit the bytes one at a time into the Transmit and Hold register */
    for (i = 0; i < count; i++)
    {
        status = CyU3PSdioByteReadWrite (portId, funcNo, CY_U3P_SDIO_WRITE, 0, CY_SDIOUART_THR, &buffer[i]);
        if (status != CY_U3P_SUCCESS)
            return status;

        do {
            status = CyU3PSdioByteReadWrite (portId, funcNo, CY_U3P_SDIO_READ, 0,  CY_SDIOUART_LSR, &statusReg);
            if (status != CY_U3P_SUCCESS)
            {
                return status;
            }
        } while ((statusReg & CY_SDIOUART_TX_HLD_REG_EMPTY) == 0);
    }

    return status;
}

/* Function to Recv Data from SDIO UART Function (UART->USB Transfer) */
CyU3PReturnStatus_t
CySdioUartRxData (
        uint8_t *buffer,
        uint8_t *count)
{
    uint8_t portId, funcNo, data, morebytes;
    CyU3PReturnStatus_t status;

    portId = glUartSettings.sdioPort;
    funcNo = glUartSettings.sdioFunction;

    *count = 0;
    morebytes = 0xFF;
    while (morebytes & CY_SDIOUART_DATA_READY)
    {
        /* Read the data in the Receive Buffer into the CPU buffer to send back over USB */
        status = CyU3PSdioByteReadWrite (portId, funcNo, CY_U3P_SDIO_READ, 0, CY_SDIOUART_RBR, &data);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }

        buffer[(*count)++] = data;
        status = CyU3PSdioByteReadWrite (portId, funcNo, CY_U3P_SDIO_READ, 0,  CY_SDIOUART_LSR, &morebytes);
        if (status != CY_U3P_SUCCESS)
        {
            return status;
        }
    }

    return status;
}

/* Function to Set UART Configuration Object to the Device*/
CyU3PReturnStatus_t
CySdioUartSetConfig (
        CyU3PUartConfig_t * uartConfig)
{
    CyU3PReturnStatus_t status;
    uint8_t portId, funcNo;
    uint8_t data, data0, data1;
    uint32_t baudrate;

    if ( !glUartSettings.uartExists)
    {
        return CY_U3P_ERROR_NOT_SUPPORTED;
    }

    portId = glUartSettings.sdioPort;
    funcNo = glUartSettings.sdioFunction;

    /* Read Line Control Register */
    status = CyU3PSdioByteReadWrite (portId, funcNo, CY_U3P_SDIO_READ, 0, CY_SDIOUART_LCR, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Enable access to Divisor Latch registers */
    data |= CY_SDIOUART_DLAB_MASK;
    status = CyU3PSdioByteReadWrite (portId, funcNo, CY_U3P_SDIO_WRITE, 0, CY_SDIOUART_LCR, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Configure Baud Rate */
    switch (uartConfig->baudRate)
    {
        case CY_U3P_UART_BAUDRATE_1200:
            baudrate = CY_SDIOUART_BAUD_1200;
            break;
        case CY_U3P_UART_BAUDRATE_2400:
            baudrate = CY_SDIOUART_BAUD_2400;
            break;
        case CY_U3P_UART_BAUDRATE_4800:
            baudrate = CY_SDIOUART_BAUD_4800;
            break;
        case CY_U3P_UART_BAUDRATE_9600:
            baudrate = CY_SDIOUART_BAUD_9600;
            break;
        case CY_U3P_UART_BAUDRATE_14400:
            baudrate = CY_SDIOUART_BAUD_14400;
            break;
        case CY_U3P_UART_BAUDRATE_19200:
            baudrate = CY_SDIOUART_BAUD_19200;
            break;
        case CY_U3P_UART_BAUDRATE_38400:
            baudrate = CY_SDIOUART_BAUD_38400;
            break;
        case CY_U3P_UART_BAUDRATE_57600:
            baudrate = CY_SDIOUART_BAUD_57600;
            break;
        case CY_U3P_UART_BAUDRATE_115200:
            baudrate = CY_SDIOUART_BAUD_115200;
            break;
        default:
            /* Unsupported baud rate. Set to 9600 baud. */
            uartConfig->baudRate = CY_SDIOUART_BAUD_9600;
            baudrate             = CY_SDIOUART_BAUD_9600;
            break;
    }

    /* Write the Divisor Latch registers to set baud rate*/
    data0 = (uint8_t)(baudrate  & 0x000000FF);
    data1 = (uint8_t)((baudrate & 0x0000FF00) >> 8);
    status = CyU3PSdioByteReadWrite (portId, funcNo, CY_U3P_SDIO_WRITE, 0, CY_SDIOUART_DRL, &data0);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    status = CyU3PSdioByteReadWrite (portId, funcNo, CY_U3P_SDIO_WRITE, 0, CY_SDIOUART_DRM, &data1);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Disable access to Divisor Latch Registers */
    data &= (~CY_SDIOUART_DLAB_MASK);
    status = CyU3PSdioByteReadWrite (portId, funcNo, CY_U3P_SDIO_WRITE, CY_U3P_SDIO_READ_AFTER_WRITE,
            CY_SDIOUART_LCR, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Set Parity, Stop bits, and Data Bits*/
    if (uartConfig-> stopBit == CY_U3P_UART_TWO_STOP_BIT)
        data |= 0x04;
    else
        data &= 0xFB;

    switch (uartConfig->parity)
    {
        case CY_U3P_UART_EVEN_PARITY:
            data |= 0x18;
            break;
        case CY_U3P_UART_ODD_PARITY:
            data &= 0x7F;
            data |= 0x08;
            break;
        default:
            data &= 0xE7;
            break;
    }

    data |= 0x03; /* Set 8 bit transfers*/
    status = CyU3PSdioByteReadWrite (portId, funcNo, CY_U3P_SDIO_WRITE, 0 , CY_SDIOUART_LCR, &data);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Enable Extended FIFO */
    data   = 0x01;
    status = CyU3PSdioByteReadWrite (portId, funcNo, CY_U3P_SDIO_WRITE, 0 , CY_SDIOUART_EFIFOER, &data);

    return status;
}

/* ------------------------------------------------------------------- */
/* --------------------- End of UART Specific routines --------------- */
/* ------------------------------------------------------------------- */

/* This function initializes the debug module. The debug prints
 * are routed to the UART and can be seen using a UART console
 * running at 115200 baud rate. */
static void
CyFxApplnDebugInit (
        void)
{
    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize the UART for printing debug messages */
    apiRetStatus = CyU3PUartInit ();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Set UART configuration */
    CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof (uartConfig));
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit  = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity   = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyFalse;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma    = CyTrue;

    apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Set the UART transfer to a really large value. */
    apiRetStatus = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Initialize the debug module. */
    apiRetStatus = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    CyU3PDebugPreamble (CyFalse);
}

/* This function starts the SDIOUART application.
 * Dma channels are configured. */
static void
CyFxSDIOUARTAppStart (
        void )
{
    uint16_t size = 0;
    CyU3PEpConfig_t epCfg;
    CyU3PDmaChannelConfig_t dmaCfg;
    CyU3PDmaBuffer_t uart_info;           /* DMA Buffer to handle SERIAL_STATE notification. */
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed ();

    /* Configure the endpoint packet size based on the USB connection speed. */
    switch (usbSpeed)
    {
        case CY_U3P_FULL_SPEED:
            size = 64;
            break;

        case CY_U3P_HIGH_SPEED:
            size = 512;
            break;

        case  CY_U3P_SUPER_SPEED:
            /* Turning low power mode off to avoid USB transfer delays. */
            CyU3PUsbLPMDisable ();
            size = 1024;
            break;

        default:
            CyFxAppErrorHandler (CY_U3P_ERROR_FAILURE);
            break;
    }

    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable   = CyTrue;
    epCfg.epType   = CY_U3P_USB_EP_BULK;
    epCfg.burstLen = 1;
    epCfg.streams  = 0;
    epCfg.pcktSize = size;

    /* Producer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig (CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig (CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Interrupt endpoint configuration */
    epCfg.epType = CY_U3P_USB_EP_INTR;
    epCfg.burstLen = 0;
    epCfg.pcktSize = 64;
    epCfg.isoPkts = 0;
    apiRetStatus = CyU3PSetEpConfig (CY_FX_EP_INTERRUPT, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Create a MANUAL channel between USB producer socket and CPU consumer socket */
    dmaCfg.size           = size;
    dmaCfg.count          = CY_FX_SDIOUART_DMA_BUF_COUNT;
    dmaCfg.prodSckId      = CY_FX_EP_PRODUCER1_SOCKET;
    dmaCfg.consSckId      = CY_U3P_CPU_SOCKET_CONS;
    dmaCfg.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.notification   = 0;
    dmaCfg.cb             = NULL;
    dmaCfg.prodHeader     = 0;
    dmaCfg.prodFooter     = 0;
    dmaCfg.consHeader     = 0;
    dmaCfg.prodAvailCount = 0;

    apiRetStatus = CyU3PDmaChannelCreate (&glChHandleUsbOut,
            CY_U3P_DMA_TYPE_MANUAL_IN, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Create a MANUAL channel between CPU producer socket and usb consumer socket */
    dmaCfg.prodSckId = CY_U3P_CPU_SOCKET_PROD;
    dmaCfg.consSckId = CY_FX_EP_CONSUMER2_SOCKET;

    apiRetStatus = CyU3PDmaChannelCreate (&glChHandleUsbIn,
            CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Create a DMA manual OUT channel for the interrupt endpoint */
    dmaCfg.prodSckId = CY_U3P_CPU_SOCKET_PROD;
    dmaCfg.consSckId = CY_FX_EP_INTR_CONSUMER1_SOCKET;

    apiRetStatus = CyU3PDmaChannelCreate (&glChHandleCputoUsb,
            CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Setting up buffer for serial state notification */
    serialstate_info[0] = 0xA1;         /* bmRequestType. */
    serialstate_info[1] = 0x20;         /* bNotification - SERIAL_STATE. */
    serialstate_info[2] = 0;            /* wValue. */
    serialstate_info[3] = 0;
    serialstate_info[4] = 0;            /* wIndex - Interface. */
    serialstate_info[5] = 0;
    serialstate_info[6] = 0x02;         /* wLength - 2. */
    serialstate_info[7] = 0;
    serialstate_info[8] = 0x83;         /* UART State Bitmap Values. */
    serialstate_info[9] = 0;

    uart_info.buffer = serialstate_info;
    uart_info.count  = 0x0A;
    uart_info.size   = 32;
    uart_info.status = 0;

    /* Set DMA Manual OUT channel for transfer */
    apiRetStatus = CyU3PDmaChannelSetupSendBuffer (&glChHandleCputoUsb,
            &uart_info);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Set the bulk DMA channels up for infinite transfer. */
    apiRetStatus = CyU3PDmaChannelSetXfer (&glChHandleUsbOut, 0);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    apiRetStatus = CyU3PDmaChannelSetXfer (&glChHandleUsbIn, 0);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Update the status flag. */
    glIsApplnActive = CyTrue;
}

/* This function stops the application. This shall be called whenever
 * a RESET or DISCONNECT event is received from the USB host. The endpoints are
 * disabled and the DMA pipes is destroyed by this function. */
static void
CyFxSDIOUARTAppStop (
        void)
{
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Update the flag. */
    glIsApplnActive = CyFalse;

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp (CY_FX_EP_PRODUCER);
    CyU3PUsbFlushEp (CY_FX_EP_CONSUMER);
    CyU3PUsbFlushEp (CY_FX_EP_INTERRUPT);

    /* Destroy the channel */
    CyU3PDmaChannelDestroy (&glChHandleUsbOut);
    CyU3PDmaChannelDestroy (&glChHandleUsbIn);
    CyU3PDmaChannelDestroy (&glChHandleCputoUsb);

    /* Disable endpoints. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyFalse;

    /* Producer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig (CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig (CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Interrupt endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig (CY_FX_EP_INTERRUPT, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }
}

/* This is the callback function to handle the USB events. */
static void
CyFxSDIOUARTAppUSBEventCB (
        CyU3PUsbEventType_t evtype,       /* Event type */
        uint16_t            evdata        /* Event data */
        )
{
    switch (evtype)
    {
        case CY_U3P_USB_EVENT_SETCONF:
            /* Stop the application before re-starting. */
            if (glIsApplnActive)
            {
                CyFxSDIOUARTAppStop ();
            }
            /* Start the loop back function. */
            CyFxSDIOUARTAppStart ();
            break;

        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_DISCONNECT:
            /* Stop the loop back function. */
            if (glIsApplnActive)
            {
                CyFxSDIOUARTAppStop ();
            }
            break;

        default:
            break;
    }
}

/* Callback to handle the USB Setup Requests and CDC Class events. */
static CyBool_t
CyFxSDIOUARTAppUSBSetupCB (
        uint32_t setupdat0,               /* SETUP Data 0 */
        uint32_t setupdat1                /* SETUP Data 1 */
        )
{

    uint8_t  bRequest, bReqType;
    uint8_t  bType, bTarget;
    uint16_t wValue;
    CyBool_t isHandled = CyFalse;

    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    CyU3PUartConfig_t uartConfig;         /* UART Configuration structure. */
    uint8_t  config_data[7];              /* Array to handle UART configuration parameters. */
    uint16_t readCount = 0;               /* To keep track of data read from EP0. */

    /* Fast enumeration is used. Only requests addressed to the interface, class,
     * vendor and unknown control requests are received by this function. */

    /* Decode the fields from the setup request. */
    bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bType    = (bReqType & CY_U3P_USB_TYPE_MASK);
    bTarget  = (bReqType & CY_U3P_USB_TARGET_MASK);
    bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue   = ((setupdat0 & CY_U3P_USB_VALUE_MASK)   >> CY_U3P_USB_VALUE_POS);

    if (bType == CY_U3P_USB_STANDARD_RQT)
    {
        /* Handle SET_FEATURE (FUNCTION_SUSPEND) and CLEAR_FEATURE (FUNCTION_SUSPEND)
         * requests here. It should be allowed to pass if the device is in configured
         * state and failed otherwise. */
        if ((bTarget == CY_U3P_USB_TARGET_INTF) && ((bRequest == CY_U3P_USB_SC_SET_FEATURE)
                    || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) && (wValue == 0))
        {
            if (glIsApplnActive)
                CyU3PUsbAckSetup ();
            else
                CyU3PUsbStall (0, CyTrue, CyFalse);

            isHandled = CyTrue;
        }
    }

    /* Check for CDC Class Requests */
    if (bType == CY_U3P_USB_CLASS_RQT)
    {
        isHandled = CyTrue;

        switch (bRequest)
        {
            /* SET_LINE_CODING. */
            case SET_LINE_CODING:
                {
                    status = CyU3PUsbGetEP0Data (0x07, config_data, &readCount);
                    if (status != CY_U3P_SUCCESS)
                    {
                        CyFxAppErrorHandler (status);
                    }
                    if (readCount != 0x07)
                    {
                        CyFxAppErrorHandler (CY_U3P_ERROR_BAD_SIZE);
                    }
                    else
                    {
                        CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof (uartConfig));
                        uartConfig.baudRate = (CyU3PUartBaudrate_t)(config_data[0] | (config_data[1] << 8) |
                                (config_data[2] << 16) | (config_data[3] << 24));
                        switch (config_data[4])
                        {
                            case 0:
                                uartConfig.stopBit = CY_U3P_UART_ONE_STOP_BIT;
                                break;
                            case 2:
                                uartConfig.stopBit = CY_U3P_UART_TWO_STOP_BIT;
                                break;
                            default: /* Not supported. */
                                uartConfig.stopBit = 0;
                                break;
                        }
                        switch (config_data[5])
                        {
                            case 1:
                                uartConfig.parity = CY_U3P_UART_ODD_PARITY;
                                break;
                            case 2:
                                uartConfig.parity = CY_U3P_UART_EVEN_PARITY;
                                break;
                            default:
                                uartConfig.parity = CY_U3P_UART_NO_PARITY;
                                break;
                        }

                        uartConfig.txEnable = CyTrue;
                        uartConfig.rxEnable = CyTrue;
                        uartConfig.flowCtrl = CyFalse;
                        uartConfig.isDma    = CyTrue;

                        /* Set the uart configuration */
                        status = CySdioUartSetConfig (&uartConfig);
                        if (status == CY_U3P_SUCCESS)
                        {
                            CyU3PMemCopy ((uint8_t *)&glUartConfig, (uint8_t *)&uartConfig,
                                    sizeof (CyU3PUartConfig_t));
                        }
                    }
                }
                break;

            case GET_LINE_CODING:
                {
                    /* Get current uart config */
                    config_data[0] = (glUartConfig.baudRate & (0x000000FF));
                    config_data[1] = ((glUartConfig.baudRate & (0x0000FF00)) >> 8);
                    config_data[2] = ((glUartConfig.baudRate & (0x00FF0000)) >> 16);
                    config_data[3] = ((glUartConfig.baudRate & (0xFF000000)) >> 24);

                    config_data[4] = (glUartConfig.stopBit == CY_U3P_UART_ONE_STOP_BIT) ? 0 : 2;
                    config_data[5] = (glUartConfig.parity == CY_U3P_UART_EVEN_PARITY) ? 2 : 1;
                    config_data[6] =  0x08;

                    status = CyU3PUsbSendEP0Data (0x07, config_data);
                    if (status != CY_U3P_SUCCESS)
                    {
                        CyFxAppErrorHandler (status);
                    }
                }
                break;

            case SET_CONTROL_LINE_STATE:
                {
                    if (glIsApplnActive)
                        CyU3PUsbAckSetup ();
                    else
                        CyU3PUsbStall (0, CyTrue, CyFalse);
                }
                break;

            default:
                isHandled = CyFalse;
        }

        /* If the request handling failed, stall the control pipe. */
        if ((isHandled) && (status != CY_U3P_SUCCESS))
        {
            CyU3PUsbStall (0, CyTrue, CyFalse);
        }
    }

    return isHandled;
}

/* Callback function to handle LPM requests from the USB 3.0 host. */
static CyBool_t
CyFxSDIOUARTAppLPMRqtCB (
        CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}


/* Query and Initialize SDIO cards till a UART function is found. */
void
CyFxSDIOUARTApplnQueryDevStatus (
        void)
{
    CyU3PReturnStatus_t status;
    uint8_t i, j;

    for (i = 0; i < 2; i++)
    {
        /* Query each of the storage ports to identify whether a device has been detected. */
        status = CyU3PSibQueryDevice (i, &glDevInfo[i]);
        if (status == CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (8, "Found a device on port %d\r\n", i);
            CyU3PDebugPrint (6, "\tType=%d, numBlks=%d, eraseSize=%d, clkRate=%d\r\n",
                    glDevInfo[i].cardType, glDevInfo[i].numBlks, glDevInfo[i].eraseSize, glDevInfo[i].clkRate);
            CyU3PDebugPrint (6, "\tblkLen=%d removable=%d, writeable=%d, locked=%d\r\n",
                    glDevInfo[i].blkLen, glDevInfo[i].removable, glDevInfo[i].writeable, glDevInfo[i].locked);
            CyU3PDebugPrint (6, "\tddrMode=%d, opVoltage=%d, busWidth=%d, numUnits=%d\r\n",
                    glDevInfo[i].ddrMode, glDevInfo[i].opVoltage, glDevInfo[i].busWidth, glDevInfo[i].numUnits);

            if (glDevInfo[i].cardType == CY_U3P_SIB_DEV_SDIO)
            {
                glSdioCardInfo[i].isSDIO = 1;
                status = CyU3PSdioQueryCard (i, &glSdioCardInfo[i].cardInfo);
                if (status != CY_U3P_SUCCESS)
                {
                    CyU3PDebugPrint (2, "CyU3PSdioQueryCard (%d) failed with code %d\r\n", i, status);
                }
                else
                {
                    CyU3PDebugPrint (6, "\tNo of Functions=%d, CCCR Version=%d, SDIO Version=%d\r\n",
                            glSdioCardInfo[i].cardInfo.numberOfFunctions,  glSdioCardInfo[i].cardInfo.CCCRVersion,
                            (glSdioCardInfo[i].cardInfo.sdioVersion & 0x0F));
                    CyU3PDebugPrint (6, "\tSD Spec supported=%d, Card Cap Register=0x%x FN0_BlkSz=0x%x\r\n",
                            ((glSdioCardInfo[i].cardInfo.sdioVersion & 0xF0) >> 4),
                            glSdioCardInfo[i].cardInfo.cardCapability, glSdioCardInfo[i].cardInfo.fn0BlockSize);
                    CyU3PDebugPrint (6, "\tCIS Address=0x%x, UHS Support Byte=0x%x\r\n",
                            glSdioCardInfo[i].cardInfo.addrCIS, glSdioCardInfo[i].cardInfo.uhsSupport);
                    CyU3PDebugPrint (6, "\tManuFacturer ID=0x%x, Manufacturer Info=0x%x\r\n",
                            glSdioCardInfo[i].cardInfo.manufacturerId, glSdioCardInfo[i].cardInfo.manufacturerInfo);

                    for (j = 1; j <= glSdioCardInfo[i].cardInfo.numberOfFunctions; j++)
                    {
                        status = CyFxSdioFunctionInit (i, j);
                        if (status != CY_U3P_SUCCESS)
                        {
                            CyU3PDebugPrint (2, "CyFxSdioFunctionInit (%d, %d) failed with code %d\r\n", i, j, status);
                        }
                        else
                        {
                            /* Setup the glUartSettings field if UART function exits*/
                            if (glSdioCardInfo[i].fbr[j - 1].interfaceCode == CY_U3P_SDIO_INTFC_UART)
                            {
                                glUartSettings.uartExists   = 1;
                                glUartSettings.sdioPort     = i;
                                glUartSettings.sdioFunction = j;
                                return;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            CyU3PDebugPrint (2, "StorageQueryDev (%d) failed with code 0x%x\r\n", i, status);
        }
    }

    if (glUartSettings.uartExists == 0)
    {
        CyU3PDebugPrint (2, "CyFxSDIOUARTApplnQueryDevStatus failed with no UART device found \r\n");
    }
}

/* SIB Callback for handling SIB events */
void
CyFxSDIOUARTApplnSibCB (
        uint8_t             portId,
        CyU3PSibEventType   evt,
        CyU3PReturnStatus_t status)
{

    if (evt == CY_U3P_SIB_EVENT_INSERT)
    {
        glSdioCardInfo[portId].isSDIO = 0;
        CyFxSDIOUARTApplnQueryDevStatus ();
    }
    if (evt == CY_U3P_SIB_EVENT_REMOVE)
    {
        glSdioCardInfo[portId].isSDIO = 0;
    }
    if (evt == CY_U3P_SIB_EVENT_SDIO_INTR)
    {
        /* No SDIO interrupt handling as of now. */
    }
}

/* Initialize the device SIB block and any attached SD/MMC or SDIO cards*/
void
CyAppSIBInit (
        void)
{
    CyU3PGpioClock_t     gpioClock;
    CyU3PReturnStatus_t  status;
    CyU3PSibIntfParams_t intfParams;

    /* GPIO module needs to be initialized before SIB is initialized. This is required because
       GPIOs are used in the SIB code.
     */
    gpioClock.fastClkDiv = 2;
    gpioClock.slowClkDiv = 16;
    gpioClock.simpleDiv  = CY_U3P_GPIO_SIMPLE_DIV_BY_2;
    gpioClock.clkSrc     = CY_U3P_SYS_CLK;
    gpioClock.halfDiv    = 0;
    status = CyU3PGpioInit (&gpioClock, NULL);

    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "GPIO Init failed, code=%d\r\n", status);
        CyFxAppErrorHandler (status);
    }

    /* Setup SIB Interface parameters*/
    intfParams.resetGpio       = 0xFF;                          /* No GPIO control on SD/MMC power. Use
                                                                   this for SDIO if available as it will
                                                                   ensure card comes up in a known good state. */
    intfParams.rstActHigh      = CyTrue;                        /* Don't care as no GPIO is selected. */
    intfParams.cardDetType     = CY_U3P_SIB_DETECT_DAT_3;       /* Card detect based on SD_DAT[3]. */
    intfParams.writeProtEnable = CyFalse;                       /* Write protect handling not enabled. */
    intfParams.lowVoltage      = CyFalse;                       /* Low voltage operation not enabled. */
    intfParams.voltageSwGpio   = 0xFF;                          /* Low voltage operation not enabled. */
    intfParams.lvGpioState     = CyFalse;                       /* Driving GPIO low selects 1.8 V on SxVDDQ. */
    intfParams.useDdr          = CyFalse;                       /* DDR clocking enabled. */
    intfParams.maxFreq         = CY_U3P_SIB_FREQ_104MHZ;        /* No S port clock limitation. */
    intfParams.cardInitDelay   = 10;                            /* Card initialization delay of 10 ms*/
    status = CyU3PSibSetIntfParams (0, &intfParams);
    if (status == CY_U3P_SUCCESS)
    {
        intfParams.voltageSwGpio = 0xFF;                        /* Use GPIO_57 for voltage switch on S1 port. */
        status = CyU3PSibSetIntfParams (1, &intfParams);
    }

    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "Set SIB interface parameters failed, code=%d\r\n", status);
        CyFxAppErrorHandler (status);
    }

    /* Start the SIB and initialize any attached SD/MMC/SDIO cards */
    status = CyU3PSibStart ();
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "SIB start failed, code=%d\r\n", status);
        CyFxAppErrorHandler (status);
    }

    /* Register a callback for SIB events. */
    CyU3PSibRegisterCbk (CyFxSDIOUARTApplnSibCB);
}

/* This function initializes the USB module, UART module and sets the enumeration descriptors */
void
CyFxSDIOUARTAppInit (
        void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize the SIB and Sdio Card if Connected */
    CyAppSIBInit ();

    /* Start the USB functionality. */
    apiRetStatus = CyU3PUsbStart ();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Register the various USB related callback functions. */
    CyU3PUsbRegisterSetupCallback (CyFxSDIOUARTAppUSBSetupCB, CyTrue);
    CyU3PUsbRegisterEventCallback (CyFxSDIOUARTAppUSBEventCB);
    CyU3PUsbRegisterLPMRequestCallback (CyFxSDIOUARTAppLPMRqtCB);

    /* Set the USB enumeration descriptors */

    /* Super speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_DEVICE_DESCR, 0, (uint8_t *)CyFxUSB30DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* High speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_HS_DEVICE_DESCR, 0, (uint8_t *)CyFxUSB20DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* BOS descriptor */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_BOS_DESCR, 0, (uint8_t *)CyFxUSBBOSDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Device qualifier descriptor */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_DEVQUAL_DESCR, 0, (uint8_t *)CyFxUSBDeviceQualDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Super speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBSSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* High speed configuration descriptor*/
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_HS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBHSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Full speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_FS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBFSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* String descriptor 0 */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* String descriptor 1 */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* String descriptor 2 */
    apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Connect the USB Pins with super speed operation enabled. */
    apiRetStatus = CyU3PConnectState (CyTrue, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }
}

/* Entry function for the SDIOUARTAppThread */
void
SDIOUARTAppThread_Entry (
        uint32_t input)
{
    CyU3PDmaBuffer_t dmaBufferOut;
    CyU3PDmaBuffer_t dmaBufferIn;
    uint8_t *bufferIn;
    uint8_t *bufferOut;
    uint8_t count, data;
    CyU3PReturnStatus_t status;

    CyFxApplnDebugInit ();

    /* Initialize the SDIOUART Example Application USB and SIB */
    CyFxSDIOUARTAppInit ();

    /* Query the current device and initialize any SDIO Functions found. */
    CyFxSDIOUARTApplnQueryDevStatus ();

    /* Wait until a SDIO-UART bridge has been detected. */
    while (!glUartSettings.uartExists)
    {
        CyU3PThreadSleep (100);
    }

    /* Default configuration for the UART module. */
    CyU3PMemSet ((uint8_t *)&glUartConfig, 0, sizeof (glUartConfig));
    glUartConfig.baudRate = CY_U3P_UART_BAUDRATE_9600;
    glUartConfig.stopBit  = CY_U3P_UART_ONE_STOP_BIT;
    glUartConfig.parity   = CY_U3P_UART_NO_PARITY;
    glUartConfig.flowCtrl = CyFalse;
    glUartConfig.txEnable = CyTrue;
    glUartConfig.rxEnable = CyTrue;
    glUartConfig.isDma    = CyTrue; /* Ignored for SDIO Uart*/

    /* Set the UART configuration */
    status = CySdioUartSetConfig (&glUartConfig);
    if (status != CY_U3P_SUCCESS )
    {
        /* Error handling */
        CyFxAppErrorHandler (status);
    }

    bufferIn  = CyU3PDmaBufferAlloc (1024);
    bufferOut = CyU3PDmaBufferAlloc (1024);
    if ((bufferIn == 0) || (bufferOut == 0))
    {
        CyU3PDebugPrint (2, "Error: Failed to allocate memory\r\n");
        CyFxAppErrorHandler (CY_U3P_ERROR_MEMORY_ERROR);
    }

    CyU3PDebugPrint (4, "Startup Completed. Initiating SDIO-UART Activity\r\n");

    for (;;)
    {
        /* Check if any data is waiting on the OUT endpoint. An instantaneous check is enough because
         * we have sufficient buffering to hold a second's worth of data on the USB side. */
        status = CyU3PDmaChannelGetBuffer (&glChHandleUsbOut, &dmaBufferOut, CYU3P_NO_WAIT);
        if (status == CY_U3P_SUCCESS)
        {
            CyU3PMemCopy (bufferOut, dmaBufferOut.buffer, dmaBufferOut.count);
            status = CySdioUartTxData (bufferOut, (uint16_t)dmaBufferOut.count);
            CyU3PDmaChannelDiscardBuffer (&glChHandleUsbOut);
        }

        /* Check if the Read Buffer on SDIO Card has Data */
        data   = 0;
        status = CyU3PSdioByteReadWrite (glUartSettings.sdioPort,
                glUartSettings.sdioFunction, CY_U3P_SDIO_READ, 0, CY_SDIOUART_LSR, &data);
        if (status != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint ( 4, "Failed while checking UART LSR Reg, with error code 0x%x", status);
            CyFxAppErrorHandler (status);
        }

        if (data & CY_SDIOUART_DATA_READY)
        {
            status = CySdioUartRxData (bufferIn, &count);
            if (status == CY_U3P_SUCCESS)
            {
                status = CyU3PDmaChannelGetBuffer (&glChHandleUsbIn, &dmaBufferIn, CYU3P_WAIT_FOREVER);
                if (status == CY_U3P_SUCCESS)
                {
                    CyU3PMemCopy (dmaBufferIn.buffer, bufferIn, count);

                    dmaBufferIn.count  = count;
                    dmaBufferIn.status = 0;
                    status = CyU3PDmaChannelCommitBuffer ( &glChHandleUsbIn, dmaBufferIn.count, 0);
                }
            }
        }
    }
}

/* Application define function which creates the threads */
void
CyFxApplicationDefine (
        void)
{
    void *ptr = NULL;
    uint32_t retThrdCreate = CY_U3P_ERROR_MEMORY_ERROR;

    /* Allocate the memory for the thread*/
    ptr = CyU3PMemAlloc (CY_FX_SDIOUART_THREAD_STACK);
    if (ptr != 0)
    {
        /* Create the thread for the application */
        retThrdCreate = CyU3PThreadCreate (
                &SDIOUARTAppThread,             /* SDIOUART Example App Thread structure */
                "21:SDIOUART_DMA_mode",         /* Thread ID and Thread name */
                SDIOUARTAppThread_Entry,        /* SDIOUART Example App Thread Entry function */
                0,                              /* No input parameter to thread */
                ptr,                            /* Pointer to the allocated thread stack */
                CY_FX_SDIOUART_THREAD_STACK,    /* SDIOUART Example App Thread stack size */
                CY_FX_SDIOUART_THREAD_PRIORITY, /* SDIOUART Example App Thread priority */
                CY_FX_SDIOUART_THREAD_PRIORITY, /* SDIOUART Example App Thread priority */
                CYU3P_NO_TIME_SLICE,            /* No time slice for the application thread */
                CYU3P_AUTO_START                /* Start the Thread immediately */
                );
    }

    /* Check the return code */
    if (retThrdCreate != 0)
    {
        /* Loop indefinitely */
        while (1);
    }
}

/* Main function */
int
main (
        void)
{
    CyU3PIoMatrixConfig_t io_cfg;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Initialize the device */
    status = CyU3PDeviceInit (0);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Configure the IO matrix */
    CyU3PMemSet ((uint8_t *)&io_cfg, 0, sizeof (io_cfg));

    io_cfg.s0Mode    = CY_U3P_SPORT_8BIT;
    io_cfg.s1Mode    = CY_U3P_SPORT_4BIT;
    io_cfg.isDQ32Bit = CyFalse;
    io_cfg.useUart   = CyTrue;
    io_cfg.useI2C    = CyFalse;
    io_cfg.useI2S    = CyFalse;
    io_cfg.useSpi    = CyFalse;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_UART_ONLY;
    io_cfg.gpioSimpleEn[0]  = 0;
    io_cfg.gpioSimpleEn[1]  = 0;
    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0;
    status = CyU3PDeviceConfigureIOMatrix (&io_cfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Initialize the RTOS kernel */
    CyU3PKernelEntry ();

    return 0;

handle_fatal_error:
    while (1);
}


/*[]*/

