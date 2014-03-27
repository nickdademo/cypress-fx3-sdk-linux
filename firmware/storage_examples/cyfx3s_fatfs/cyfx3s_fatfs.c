/*
## Cypress USB 3.0 Platform source file (cyfx3s_fatfs.c)
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



/*
This example implements a File System Management Module (FATFS) between the UART TX and 
Storage Module using DMA MANUAL channels and UART register mode. 3 DMA MANUAL channels are
created namely,
Manual IN =  Storage --> CPU
Manual OUT = CPU --> Storage
Manual OUT = CPU --> UART

From UART --> CPU data is transferred in register mode. Upon every reception of data from the
UART, CPU parses command and argument from data and then call appropriate FATFS functions. These
FATFS functions in turn call Block Drivers and performs required operation.
*/

#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3error.h>
#include <cyu3usb.h>
#include <cyu3usbconst.h>
#include <cyu3uart.h>
#include <cyu3sib.h>
#include <cyu3gpio.h>

#include "ff.h"
#include "cyfx3s_fatfs.h"
#include "diskio.h"


CyU3PThread     FatFsAppThread;                         /* Application thread structure */
CyU3PEvent      glStorDriverEvent;                      /* Application event group */

CyU3PDmaChannel glChHandleStorOut;                      /* DMA channel for CPU -> Storage */
CyU3PDmaChannel glChHandleStorIn;                       /* DMA channel for Storage -> CPU */
CyU3PDmaChannel glChHandleUARTIN;                       /* DMA channel for CPU -> UART */

CyU3PDmaBuffer_t Buf_t;                    /* DMA Buffer for UART -> CPU */
CyU3PUartConfig_t uartConfig;                /* UART configuration Attributes Holder*/

UINT n_read = 0;

uint8_t cmnd[128] = {0}, arg1[40] = {0}, arg2[100] = {0}, cmd_start[20] = {0};
uint8_t *ptr_cmnd=(uint8_t *)cmnd;
uint8_t *ptr_arg1=(uint8_t *)arg1;
uint8_t *ptr_arg2=(uint8_t *)arg2;
uint8_t *cmd = (uint8_t *)cmd_start;
uint8_t Drive = 0, n_bytes = 0;

static FATFS Fatfs[_VOLUMES];        /* File system object */
static FIL Fil[2];            /* File object */
BYTE Buff[128];        /* File read buffer */

/* Commands Syntax List*/
uint8_t Help[1006] =  "ls -> Lists current directory\
                      \r\nls, ls 0:, ls 1:, ls Myfolder, ls 1:/Myfolder\r\n\
                      \r\ntouch -> Create an empty file\
                      \r\ntouch file.txt, touch 0:/Myfolder/file.txt, touch 1:file.txt, touch 1:Myfolder/file.txt\r\n\
                      \r\nrm -> Removes File or Directory\
                      \r\nrm file.txt, rm Myfolder, rm 0:/Myfolder/file.txt, rm 1:file.txt, rm 1:/Myfolder1/Myfolder2\r\n\
                      \r\nmkdir -> Create new Directory\
                      \r\nmkdir Myfolder, mkdir 0:/Myfolder1/Myfolder2, mkdir 1:Myfolder, mkdir 1:/Myfolder1/Myfolder2\r\n\
                      \r\nread -> Read file content\
                      \r\nread file.txt, read Myfolder/file.txt, read 1:file.txt,read 1:/Myfolder/file.txt\r\n\
                      \r\nwrite -> Write Data in file\
                      \r\nwrite file.txt TEXT, write 0:/ Myfolder/file.txt TEXT, write 1:file.txt TEXT, write 1:/Myfolder/file.txt TEXT\r\n\
                      \r\nexit -> Exit from Shell\r\nexit\r\n";

/* Application error handler */
void
CyFxAppErrorHandler (
                     CyU3PReturnStatus_t apiRetStatus    /* API return status */
                     )
{
    /* Application failed with the error code apiRetStatus */

    /* Add custom debug or recovery actions here */

    /* Loop indefinitely */
    for (;;)
    {
        /* Thread sleep : 100 ms */
        CyU3PThreadSleep (100);
    }
}

/* Returns a valid time */
DWORD get_fattime (void)
{
    return ((DWORD)(2012 - 1980) << 25)                /* Year = 2012 */
        | ((DWORD)1 << 21)                /* Month = 1 */
        | ((DWORD)1 << 16)                /* Day_m = 1*/
        | ((DWORD)0 << 11)                /* Hour = 0 */
        | ((DWORD)0 << 5)                /* Min = 0 */
        | ((DWORD)0 >> 1);                /* Sec = 0 */
}

/* This function de-initializes the UART module*/
void 
CyFxUartLpAppLnDeInit(void)
{

    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    apiRetStatus = CyU3PUartDeInit(); 
    if ((apiRetStatus != CY_U3P_SUCCESS) && (apiRetStatus != CY_U3P_ERROR_NOT_STARTED))
    {
        CyFxAppErrorHandler (apiRetStatus);
    }
}

/* This function initializes the UART module */ 
void
CyFxUartApplnInit (void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PDmaChannelConfig_t dmaUartConfig;

    /* Initialize the UART module */
    apiRetStatus = CyU3PUartInit ();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Configure the UART:
    Baud-rate = 115200, One stop bit, No parity, Flow control enabled.
    */
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity = CY_U3P_UART_NO_PARITY;
    uartConfig.flowCtrl = CyTrue;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyTrue;

    if (uartConfig.isDma != CyTrue)  /* Register Mode */
    {
        /*Set the UART configuration */
        apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
        if (apiRetStatus != CY_U3P_SUCCESS )
        {
            /* Error handling */ 
            CyFxAppErrorHandler(apiRetStatus);
        }

    }

    else    /* DMA Mode*/
    {
        /*Set the UART configuration */
        apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
        if (apiRetStatus != CY_U3P_SUCCESS )
        {
            /* Error handling */ 
            CyFxAppErrorHandler(apiRetStatus);
        }    

        /* Set UART Tx and Rx transfer Size to infinite */
        apiRetStatus = CyU3PUartTxSetBlockXfer(0xFFFFFFFF);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            /* Error handling */
            CyFxAppErrorHandler(apiRetStatus);
        }

        CyU3PMemSet ((uint8_t *)&dmaUartConfig, 0, sizeof(dmaUartConfig));
        dmaUartConfig.size           = CY_FX_FATFS_DMA_BUF_SIZE;
        dmaUartConfig.count          = CY_FX_FATFS_DMA_BUF_COUNT; 
        dmaUartConfig.dmaMode        = CY_U3P_DMA_MODE_BYTE;
        dmaUartConfig.prodHeader     = 0;
        dmaUartConfig.prodFooter     = 0;
        dmaUartConfig.consHeader     = 0;
        dmaUartConfig.prodAvailCount = 0;
        dmaUartConfig.prodSckId      = CY_U3P_CPU_SOCKET_PROD;
        dmaUartConfig.consSckId      = CY_U3P_LPP_SOCKET_UART_CONS;
        dmaUartConfig.notification   = 0;
        dmaUartConfig.cb             = NULL;

        apiRetStatus = CyU3PDmaChannelCreate (&glChHandleUARTIN, CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaUartConfig);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyFxAppErrorHandler (apiRetStatus);
        }
    }
}


/* This function is used to destroy DMA channel between UART and CPU*/
void
CyFxUartDmaApplnDeInit (void)
{
    CyU3PDmaChannelDestroy (&glChHandleUARTIN);
}

void
CyFxFatFsAppSibInit (
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
        CyFxAppErrorHandler (status);
    }

    intfParams.resetGpio       = 0xFF;                          /* No GPIO control on SD/MMC power. */
    intfParams.rstActHigh      = CyTrue;                        /* Don't care as no GPIO is selected. */
    intfParams.cardDetType     = CY_U3P_SIB_DETECT_DAT_3;       /* Card detect based on SD_DAT[3]. */
    intfParams.writeProtEnable = CyTrue;                        /* Write protect handling enabled. */
    intfParams.lowVoltage      = CyFalse;                       /* Low voltage operation not enabled. */
    intfParams.voltageSwGpio   = 0xFF;                          /* Low voltage operation not enabled. */
    intfParams.lvGpioState     = CyFalse;                       /* Driving GPIO low selects 1.8 V on SxVDDQ. */
    intfParams.useDdr          = CyTrue;                        /* DDR clocking enabled. */
    intfParams.maxFreq         = CY_U3P_SIB_FREQ_104MHZ;        /* No S port clock limitation. */
    intfParams.cardInitDelay   = 0;                             /* No SD/MMC initialization delay required. */

    status = CyU3PSibSetIntfParams (0, &intfParams);
    if (status == CY_U3P_SUCCESS)
        status = CyU3PSibSetIntfParams (1, &intfParams);

    if (status != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (status);
    }

    status = CyU3PSibStart ();
    if (status != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (status);
    }

    /* Register a callback for SIB events. */
    CyU3PSibRegisterCbk (CyFxFatfsAppSibCallback);
}


/* Create the DMA channels required for the FATFS application */
void
CyFxFatFsAppDmaInit (
                     void)
{
    CyU3PDmaChannelConfig_t dmaConfig;
    CyU3PReturnStatus_t status;

    /* Both DMA channels are created with SuperSpeed parameters. The CyU3PSetEpPacketSize () API is used
    to reconfigure the endpoints to work with DMA channels with large buffers. */
    dmaConfig.size           = CY_FX_FATFS_DMA_BUF_SIZE;
    dmaConfig.count          = CY_FX_FATFS_DMA_BUF_COUNT;
    dmaConfig.prodSckId      = CY_U3P_CPU_SOCKET_PROD;
    dmaConfig.consSckId      = CY_U3P_SIB_SOCKET_0;
    dmaConfig.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    dmaConfig.notification   = 0;
    dmaConfig.cb             = NULL;
    dmaConfig.prodHeader     = 0;
    dmaConfig.prodFooter     = 0;
    dmaConfig.consHeader     = 0;
    dmaConfig.prodAvailCount = 0;

    status = CyU3PDmaChannelCreate (&glChHandleStorOut, CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaConfig);
    if (status != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (status);
    }

    dmaConfig.prodSckId      = CY_U3P_SIB_SOCKET_1;
    dmaConfig.consSckId      = CY_U3P_CPU_SOCKET_CONS;

    status = CyU3PDmaChannelCreate (&glChHandleStorIn, CY_U3P_DMA_TYPE_MANUAL_IN, &dmaConfig);
    if (status != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (status);
    }
}

void
CyFxFatFsAppInit (
                  void)
{
    uint32_t txApiRetStatus;

    /* Initialize SIB and get device information. */
    CyFxFatFsAppSibInit ();

    txApiRetStatus = CyU3PEventCreate (&glStorDriverEvent);
    if (txApiRetStatus != 0)
    {
        CyFxAppErrorHandler (txApiRetStatus);
    } 


}

/* Function used to print on UART in register mode */
void
CyFXUartPrint (
               CyBool_t addline,
               char    *str)
{
    uint8_t local[64];
    uint8_t len = 0;

    local[0] = '\r';
    local[1] = '\n';
    CyU3PUartTransmitBytes (local, 2, 0);

    while (*str != '\r')
    {
        local[len++] = (uint8_t)*str;
        str++;
    }

    if (addline)
    {
        local[len++] = '\r';
        local[len]   = '\n';
    }

    CyU3PUartTransmitBytes (local, len, 0);
}

/* Function to parse caommand and argument from UART Terminal */
void
CyFxUartParseBuffer()
{    

    uint8_t cnt = 0;
    Drive = '0';

    ptr_cmnd = (uint8_t *)cmnd;
    while (1)
    {
        *cmd = *ptr_cmnd;
        ptr_cmnd++;
        cmd++;
        if ((*ptr_cmnd == ' ') || (*ptr_cmnd == '\r') || (*ptr_cmnd == '/'))
        {
            if (*ptr_cmnd == ' ') ptr_cmnd++;
            break;
        }
    }   

    cmd = cmd_start;

    if (*ptr_cmnd == (uint8_t)'\r') 
    {
        Drive = '0';            
    }

    while (*ptr_cmnd != (uint8_t)'\r')
    {
        *ptr_arg1 = *ptr_cmnd;

        if(cnt == 0 && (*ptr_arg1 == '0' || *ptr_arg1 == '1'))
        {

            Drive = *ptr_arg1; 
            cnt++;        
        }

        ptr_arg1++;
        ptr_cmnd++;

        if (*ptr_cmnd == ' ') 
        {
            ptr_cmnd++;    
            break;
        }
    }    

    while ((*ptr_cmnd != '\r'))
    {

        if(*ptr_cmnd == '\r')
            break;

        *ptr_arg2 = *ptr_cmnd;
        ptr_arg2++;
        ptr_cmnd++;

        n_bytes++;
    }

    ptr_arg1 = arg1;
    ptr_arg2 = arg2;


}
/* Function to print Command Syntax on UART terminal in DMA Mode */
void
CyFxUartSendHelpBuffer(uint8_t *Buffer_Print, int32_t cnt_read)
{

    uint8_t count = 0;
    CyU3PReturnStatus_t Txstatus;

    count = cnt_read >> 2; /* Buffer divided into 4 parts */
    /* Buffer Part 1*/
    Buf_t.buffer = Buffer_Print;
    Buf_t.count  = count;
    Buf_t.size   = 1024;
    Buf_t.status = 0;

    Txstatus = CyU3PDmaChannelSetupSendBuffer (&glChHandleUARTIN,  &Buf_t);
    if (Txstatus != CY_U3P_SUCCESS)
        CyFxAppErrorHandler (Txstatus);

    Txstatus = CyU3PDmaChannelWaitForCompletion(&glChHandleUARTIN, CYU3P_WAIT_FOREVER);

    if (Txstatus != CY_U3P_SUCCESS)
        CyFxAppErrorHandler (Txstatus);

    /* Buffer Part 2*/ 
    CyU3PMemSet (Buf_t.buffer, 0, sizeof (Buf_t.buffer));
    Buf_t.buffer = (Buffer_Print + count);
    Buf_t.count  = count;
    Buf_t.size   = 1024;
    Buf_t.status = 0;

    Txstatus = CyU3PDmaChannelSetupSendBuffer (&glChHandleUARTIN,  &Buf_t);
    if (Txstatus != CY_U3P_SUCCESS)
        CyFxAppErrorHandler (Txstatus);

    Txstatus = CyU3PDmaChannelWaitForCompletion(&glChHandleUARTIN, CYU3P_WAIT_FOREVER);

    if (Txstatus != CY_U3P_SUCCESS)
        CyFxAppErrorHandler (Txstatus);

    /* Buffer Part 3*/ 
    CyU3PMemSet (Buf_t.buffer, 0, sizeof (Buf_t.buffer));
    Buf_t.buffer = (Buffer_Print + (count << 1));
    Buf_t.count  = count;
    Buf_t.size   = 1024;
    Buf_t.status = 0;

    Txstatus = CyU3PDmaChannelSetupSendBuffer (&glChHandleUARTIN,  &Buf_t);
    if (Txstatus != CY_U3P_SUCCESS)
        CyFxAppErrorHandler (Txstatus);

    Txstatus = CyU3PDmaChannelWaitForCompletion(&glChHandleUARTIN, CYU3P_WAIT_FOREVER);

    if (Txstatus != CY_U3P_SUCCESS)
        CyFxAppErrorHandler (Txstatus);

    /* Buffer Part 4*/ 
    CyU3PMemSet (Buf_t.buffer, 0, sizeof (Buf_t.buffer));
    Buf_t.buffer = (Buffer_Print + (count << 1) + count);
    Buf_t.count  = count + 1;
    Buf_t.size   = 1024;
    Buf_t.status = 0;

    Txstatus = CyU3PDmaChannelSetupSendBuffer (&glChHandleUARTIN,  &Buf_t);
    if (Txstatus != CY_U3P_SUCCESS)
        CyFxAppErrorHandler (Txstatus);

    Txstatus = CyU3PDmaChannelWaitForCompletion(&glChHandleUARTIN, CYU3P_WAIT_FOREVER);

    if (Txstatus != CY_U3P_SUCCESS)
        CyFxAppErrorHandler (Txstatus);
}

/* Function to print error commands*/
void
CyFxUartSendBuffer(uint8_t *Buffer_Print,uint8_t cnt_read)
{

    CyU3PReturnStatus_t Txstatus;

    Buf_t.buffer = Buffer_Print;
    Buf_t.count  = (cnt_read+2);
    Buf_t.size   = 128;
    Buf_t.status = 0;

    Txstatus = CyU3PDmaChannelSetupSendBuffer (&glChHandleUARTIN,  &Buf_t);
    if (Txstatus != CY_U3P_SUCCESS)
        CyFxAppErrorHandler (Txstatus);

    Txstatus = CyU3PDmaChannelWaitForCompletion(&glChHandleUARTIN, CYU3P_WAIT_FOREVER);

    if (Txstatus != CY_U3P_SUCCESS)
        CyFxAppErrorHandler (Txstatus);

}

/* String Comparison */
uint8_t CyFxstrcomp (
                 char *first,
                 char *second)
{
    while ((*second == *first) && (*first != '\x00'))
    {
        first++;
        second++;
    }

    if ((*first == '\x00') && (*second == NULL))
        return 0;
    else
        return 1;
}


/* Entry function for the FatFsAppThread */
void
FatFsAppThread_Entry (
                      uint32_t input)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    FRESULT rc;                                /* Result code in FatFs file - ff.h*/
    DIR dir;                                /* Directory object */
    FILINFO fno;                            /* File information object */
    UINT bw, br;
    BYTE b = 0;
    uint8_t actualCount = 0, count_cmnd = 0, len_cmnd = 0, offset = 0;
    uint8_t Port = 0, mov = 0, count = 0;
    uint8_t error[11] = "ERROR CODE:";
    uint8_t *err;
    uint8_t size = 0;
    uint8_t buffer[256];
    uint16_t err_code = 0;

    /* Initialize the UART for receiving commands from the user. */
    /*CyFxUartLpApplnInit();*/
    uartConfig.isDma = CyFalse;
    CyFxUartApplnInit();

    /* Initialize the Storage ports. */
    CyFxFatFsAppInit();

    /* Create the required DMA channels. */
    CyFxFatFsAppDmaInit ();

    /* Pre-mount all storage volumes. */
    for (b = 0; b < _VOLUMES; b++)
    {
        rc = f_mount(b, &Fatfs[b]);
        if (rc)
            CyFXUartPrint (0, "MOUNT ERROR\r");
    }        

    for (;;)
    {

        CyU3PMemSet (cmnd, 0, sizeof (cmnd));
	CyU3PMemSet (buffer, 0, sizeof (buffer));
        CyU3PMemSet (arg1, 0, sizeof (arg1));
        CyU3PMemSet (arg2, 0, sizeof (arg2));
        CyU3PMemSet (cmd_start, 0, sizeof (cmd_start));
        ptr_cmnd = (uint8_t *)cmnd;
        ptr_arg1 = (uint8_t *)arg1;
        ptr_arg2 = (uint8_t *)arg2;
        cmd      = (uint8_t *)cmd_start;

        CyFXUartPrint (0, "\r\n");
        CyFXUartPrint (0, "FX3S $ \r");

        while(1)
        {

		/* Receive 1 byte */
                actualCount = CyU3PUartReceiveBytes(ptr_cmnd, 1, &apiRetStatus);
                if (actualCount)
                {
                    count_cmnd++;
                    if(*ptr_cmnd == '\r')
                    {
                        len_cmnd = 0;
                        break;
                    }
                    ptr_cmnd++;
                    len_cmnd++;
                }
                if(len_cmnd > 127)
                {  
                    CyFXUartPrint (0, "Command Array Overflow\r\n");
                    CyFXUartPrint (0, "Wrong Command\r\n");
                    break;
                }

            }

            if(len_cmnd > 127)
            {
                break;
            }

            CyFXUartPrint (1, (char *)cmnd);
            CyFXUartPrint (0, "\r\n");
            CyFxUartParseBuffer();

            Port = (uint8_t) Drive - '0';

            if (actualCount != 0)
            {       
                CyFxUartLpAppLnDeInit();
                uartConfig.isDma = CyTrue;
                CyFxUartApplnInit();    


                /* Creates a 0kb file */
                if (!CyFxstrcomp ((char *)cmd, "touch"))
                {
                    rc = f_open (&Fil[Port], (const TCHAR *)ptr_arg1 , FA_CREATE_ALWAYS);
                    if (rc)
                    {        
                        err_code = (uint16_t)rc + 48;
                        if (err_code > 57)
                        {
                            err_code = err_code + 7;
                        }
                        err = (uint8_t *) &err_code;    
                        CyFxUartSendBuffer(error, 10);
                        CyFxUartSendBuffer(err, 1);
                    }

                    else
                    {
                        rc = f_close (&Fil[Port]);
                        if (rc)
                        {    
                            err_code = (uint16_t)rc + 48;
                            if (err_code > 57)
                            {
                                err_code = err_code + 7;
                            }
                            err = (uint8_t *) &err_code;    
                            CyFxUartSendBuffer(error, 10);
                            CyFxUartSendBuffer(err, 1);
                        }
                    }    
                }


                /* Deletes a file */
                else if(!CyFxstrcomp ((char *)cmd, "rm"))
                {
                    rc = f_unlink ((const TCHAR *)ptr_arg1);
                    if (rc)
                    {    
                        err_code = (uint16_t)rc + 48;
                        if (err_code > 57)
                        {
                            err_code = err_code + 7;
                        }
                        err = (uint8_t *) &err_code;    
                        CyFxUartSendBuffer(error, 10);
                        CyFxUartSendBuffer(err, 1);
                    }
                }


                /* Create Directory */
                else if(!CyFxstrcomp ((char *)cmd, "mkdir"))
                {
                    rc = f_mkdir ((const TCHAR *)ptr_arg1);
                    if (rc)
                    {    

                        err_code = (uint16_t)rc + 48;
                        if (err_code > 57)
                        {
                            err_code = err_code + 7;
                        }
                        err = (uint8_t *) &err_code;
                        CyFxUartSendBuffer(error, 10);
                        CyFxUartSendBuffer((uint8_t *)err, 1);
                    }
                }

                /* List Directory */
                else if(!CyFxstrcomp ((char *)cmd, "ls"))
                {
                    rc = f_opendir(&dir, (const TCHAR *)ptr_arg1);
		    offset = 0;
		    count = 0;
                    if (rc)
                    {        
                        err_code = (uint16_t)rc + 48;
                        if (err_code > 57)
                        {
                            err_code = err_code + 7;
                        }
                        err = (uint8_t *) &err_code;
                        CyFxUartSendBuffer(error, 10);
                        CyFxUartSendBuffer((uint8_t *)err, 1);
                    }
                    else
                    {
                        rc = f_readdir(&dir, &fno);
                        while ((rc == FR_OK) || fno.fname[0])
                        {
			    mov = 0;
                            while (fno.fname[mov] != '\x00')
                            {
                                buffer[offset++] = (uint8_t)fno.fname[mov];
				mov++;
                            }

                            rc = f_readdir(&dir, &fno);

                            buffer[offset++] = ' '; 
			    count = offset - count;
			    if (br == 0)
			    {
				    size += offset;
				    br++;
			    }
			    
			    else
			    {
			            size += count; 
			    }
			    
                            if (((size + 13) >= 256) || (rc != FR_OK) || !fno.fname[0])
                            {
                                CyFxUartSendBuffer(buffer, size);
                                size = 0;
                                break;
                            }
                        }
                    }
                }


    /* Writing in a file */
        else if(!CyFxstrcomp ((char *)cmd, "write"))
        {
            rc = f_open(&Fil[Port], (const TCHAR *)ptr_arg1, FA_CREATE_ALWAYS | FA_WRITE);
            if (rc)
            {        
                err_code = (uint16_t)rc + 48;
                if (err_code > 57)
                {
                    err_code = err_code + 7;
                }
                err = (uint8_t *) &err_code;    
                CyFxUartSendBuffer(error, 10);
                CyFxUartSendBuffer(err, 1);
            }
            else
            {
                rc = f_write(&Fil[Port], ptr_arg2, n_bytes, &bw);
                if (rc)
                {        
                    err_code = (uint16_t)rc + 48;
                    if (err_code > 57)
                    {
                        err_code = err_code + 7;
                    }
                    err = (uint8_t *) &err_code;    
                    CyFxUartSendBuffer(error, 10);
                    CyFxUartSendBuffer(err, 1);
                }

                rc = f_close(&Fil[Port]);
                if (rc)
                {        
                    err_code = (uint16_t)rc + 48;
                    if (err_code > 57)
                    {
                        err_code = err_code + 7;
                    }
                    err = (uint8_t *) &err_code;    
                    CyFxUartSendBuffer(error, 10);
                    CyFxUartSendBuffer(err, 1);
                }
            }
            }

            /* Read from file */
        else if(!CyFxstrcomp ((char *)cmd, "read"))
        {
            rc = f_open(&Fil[Port], (const TCHAR *)ptr_arg1, FA_OPEN_EXISTING | FA_READ);
            if (rc)
            {        
                err_code = (uint16_t)rc + 48;
                if (err_code > 57)
                {
                    err_code = err_code + 7;
                }
                err = (uint8_t *) &err_code;    
                CyFxUartSendBuffer(error, 10);
                CyFxUartSendBuffer(err, 1);
            }
            else
            {
		br = 0;
                /* Read from File */
                for (;;) 
                {
                    rc = f_read(&Fil[Port], Buff, sizeof Buff, &br);     /* Read a chunk of src file */
                    if (rc)
                    {        
                        err_code = (uint16_t)rc + 48;
                	if (err_code > 57)
                	{
                    	err_code = err_code + 7;
                	}
                	err = (uint8_t *) &err_code;    
					CyFxUartSendBuffer(error, 10);
                    CyFxUartSendBuffer(err, 1);
                    }
                    if (br != 0)
                    {
                        n_read = n_read + br;
                    }
                    if (rc || br == 0) break;                  /* error or eof */
                    CyFxUartSendBuffer((uint8_t *)Buff, br);
                }

                rc = f_close(&Fil[Port]);
                if (rc)
                {        
                    err_code = (uint16_t)rc + 48;
                	if (err_code > 57)
                	{
                    	err_code = err_code + 7;
               		}
                	err = (uint8_t *) &err_code;    
                    CyFxUartSendBuffer(error, 10);
                    CyFxUartSendBuffer(err, 1);
                }

                
            }
        }

        /* Exit */
        else if(!CyFxstrcomp ((char *)cmd, "exit"))
        {
            break;
        }

        /* Help: Lists Command Syntax*/
        else if(!CyFxstrcomp ((char *)cmd, "help"))
        {
           
            CyFxUartSendHelpBuffer(Help, (int32_t) 1007);
        } 

        /* Illegal Command Handler*/
        else
        {
            uint8_t Error[14] = "WRONG COMMAND\n";            
            CyFxUartSendBuffer(Error, 14);
        }

        ptr_cmnd = (uint8_t *)cmnd;
        ptr_arg1 = (uint8_t *)arg1;
        ptr_arg2 = (uint8_t *)arg2;
        CyFxUartDmaApplnDeInit();
        CyFxUartLpAppLnDeInit();
        uartConfig.isDma = CyFalse;
        CyFxUartApplnInit();
}
    }

    CyFxUartLpAppLnDeInit();
    uartConfig.isDma = CyFalse;
    CyFxUartApplnInit();
    CyFXUartPrint (0, "---RESTART AGAIN---- \r");
    f_mount(0, NULL);
    f_mount(1, NULL);

}

/* Application define function which creates the threads. */
void
CyFxApplicationDefine (
                       void)
{
    void *ptr = NULL;
    uint32_t retThrdCreate = CY_U3P_SUCCESS;

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_FATFS_THREAD_STACK);

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (&FatFsAppThread,            /* Example App Thread structure */
        "21:FatFs_Example",                      /* Thread ID and Thread name */
        FatFsAppThread_Entry,                    /* FatFs Example App Thread Entry function */
        0,                                       /* No input parameter to thread */
        ptr,                                     /* Pointer to the allocated thread stack */
        CY_FX_FATFS_THREAD_STACK,                /* Thread stack size */
        CY_FX_FATFS_THREAD_PRIORITY,             /* Thread priority */
        CY_FX_FATFS_THREAD_PRIORITY,             /* Thread priority */
        CYU3P_NO_TIME_SLICE,                     /* No time slice for the application thread */
        CYU3P_AUTO_START                         /* Start the Thread immediately */
        );

    /* Check the return code */
    if (retThrdCreate != 0)
    {
        /* Thread Creation failed with the error code retThrdCreate */

        /* Add custom recovery or debug actions here */

        /* Application cannot continue */
        /* Loop indefinitely */
        while(1);
    }
}

/*
* Main function
*/
int
main (void)
{
    CyU3PIoMatrixConfig_t io_cfg;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Initialize the device */
    status = CyU3PDeviceInit (0);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Initialize the caches. Enable only the Instruction Cache. */
    status = CyU3PDeviceCacheControl (CyTrue, CyFalse, CyFalse);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Configure the IO matrix for the device. On the FX3 DVK board, the COM port 
    * is connected to the IO(53:56). This means that either DQ32 mode should be
    * selected or lppMode should be set to UART_ONLY. Here we are choosing
    * UART_ONLY configuration. */
    CyU3PMemSet ((uint8_t *)&io_cfg, 0, sizeof(io_cfg));
    io_cfg.isDQ32Bit = CyFalse;
    io_cfg.s0Mode = CY_U3P_SPORT_8BIT;
    io_cfg.s1Mode = CY_U3P_SPORT_4BIT;
    io_cfg.useUart   = CyTrue;
    io_cfg.useI2C    = CyFalse;
    io_cfg.useI2S    = CyFalse;
    io_cfg.useSpi    = CyFalse;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_UART_ONLY;

    io_cfg.gpioSimpleEn[0]  = 0;
    io_cfg.gpioSimpleEn[1]  = 0x02102800;                       /* IOs 43, 45, 52 and 57 are chosen as GPIO. */
    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0;
    status = CyU3PDeviceConfigureIOMatrix (&io_cfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* This is a non returnable call for initializing the RTOS kernel */
    CyU3PKernelEntry ();

    /* Dummy return to make the compiler happy */
    return 0;

handle_fatal_error:
    /* Cannot recover from this error. */
    while (1);

}

/* [ ] */

