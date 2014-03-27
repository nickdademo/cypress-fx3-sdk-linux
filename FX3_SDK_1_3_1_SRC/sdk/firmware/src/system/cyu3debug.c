/*
 ## Cypress USB 3.0 Platform source file (cyu3debug.c)
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

#include <cyu3types.h>
#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3uart.h>
#include <cyu3error.h>
#include <cyu3dma.h>
#include <cyu3socket.h>
#include <cyu3utils.h>
#include <cyu3mmu.h>
#include <stdarg.h>
#include <string.h>

/* Forward Declaration */
uint8_t *
CyU3PDebugIntToStr (
        uint8_t  *convertedString,
        uint32_t  num,
        uint8_t   base);
void
CyU3PDebugThreadEntry (
        uint32_t threadInput);

/* Global variables. */
uint8_t glDebugTraceLevel;                      /* Trace level threshold */
uint16_t glDebugLogMask;                        /* Debug enable for threads */

/* Variables static to this module. */
static CyU3PThread      glDebugThread;                  /* Debug Thread */
static CyU3PQueue       glDebugQueue;                   /* Debug Queue */
static CyU3PMutex       glDebugLock;                    /* Debug lock */
static CyU3PDmaBuffer_t glDebugBuf_p;                   /* Pointer to the current buffer in use */
static uint16_t         glDebugBufOffset;               /* Offset for next log message in the buffer */ 
static CyU3PDmaChannel  glDebugChanHandle;              /* The debug channel handle */  
static CyBool_t         glDebugInit = CyFalse;          /* Debug Init has been called or not */
static CyBool_t         glSysMemLogInit = CyFalse;      /* Whether sys_mem logging is enabled or not. */
static CyBool_t         glDebugSendPreamble = CyTrue;   /* Send preamble data before actual message */

#define CY_U3P_DEBUG_STACK_SIZE	        (0x200)                 /* Debug Thread stack size in Byte */
#define CY_U3P_DEBUG_THREAD_PRIORITY    (6)                     /* Debug Thread Priority */
#define CY_U3P_DEBUG_MSG_SIZE           (3)                     /* Size in words of messages for the debug thread. */
#define CY_U3P_DEBUG_QUEUE_SIZE         (0x84)                  /* Debug Queue size in Byte */
#define CY_U3P_DEBUG_QUEUE_WAIT_OPTION  (CYU3P_WAIT_FOREVER)    /* Waiting option till room in queue is available. */

#define CY_U3P_DEBUG_LOG_MASK_DEFAULT   (0x0000)                /* Default logging capability. */
#define CY_U3P_DEBUG_WRAP               (0x01)                  /* Enable wrap around when logging to SYS_MEM. */

/*
 * Summary
 * This function defines the applivation define function for the debug thread
 */
void 
CyU3PDebugApplicationDefine (void)
{
    uint8_t *pointer;

    /* Variable initialization. */
    glDebugInit = CyFalse;
    glDebugSendPreamble = CyTrue;

    /* Create the thread and message queue */
    pointer = CyU3PMemAlloc (CY_U3P_DEBUG_STACK_SIZE);
    CyU3PThreadCreate (&glDebugThread, "Debug Thread", CyU3PDebugThreadEntry, 0, pointer, 
        CY_U3P_DEBUG_STACK_SIZE, CY_U3P_DEBUG_THREAD_PRIORITY, CY_U3P_DEBUG_THREAD_PRIORITY, 
        CYU3P_NO_TIME_SLICE, CYU3P_AUTO_START);

    pointer = CyU3PMemAlloc (CY_U3P_DEBUG_QUEUE_SIZE);
    CyU3PQueueCreate (&glDebugQueue, CY_U3P_DEBUG_MSG_SIZE, pointer, 
        CY_U3P_DEBUG_QUEUE_SIZE);

    CyU3PMutexCreate (&glDebugLock, CYU3P_NO_INHERIT);
}

/*
* Summary
* This function initializes the debug. 
* It creates the channel and initilizes the debug port.
*/
CyU3PReturnStatus_t
CyU3PDebugInit (CyU3PDmaSocketId_t destSckId, uint8_t traceLevel)
{
    CyU3PDmaChannelConfig_t dmaConfig;
    CyU3PReturnStatus_t stat;

    if ((glDebugInit) || (glSysMemLogInit))
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    stat = CyU3PDmaSocketIsValidConsumer (destSckId);
    if (!stat)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    CyU3PMutexGet (&glDebugLock, CYU3P_WAIT_FOREVER);

    /* Create a MANUAL_OUT DMA channel to push the data out through. */
    dmaConfig.size = CY_U3P_DEBUG_DMA_BUFFER_SIZE;
    dmaConfig.count = CY_U3P_DEBUG_DMA_BUFFER_COUNT;
    dmaConfig.prodAvailCount = 0;
    dmaConfig.prodSckId = CY_U3P_CPU_SOCKET_PROD;
    dmaConfig.consSckId = destSckId;
    dmaConfig.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dmaConfig.prodHeader = 0;
    dmaConfig.prodFooter = 0;
    dmaConfig.consHeader = 0;
    dmaConfig.cb = NULL;
    stat = CyU3PDmaChannelCreate (&glDebugChanHandle,
            CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaConfig);
    if (stat == CY_U3P_SUCCESS)
    {
        /* The cache control for this channel has to be done internally. */
        if (glIsDCacheEnabled)
        {
            stat = CyU3PDmaChannelCacheControl (&glDebugChanHandle, CyTrue);
        }
    }

    if (stat == CY_U3P_SUCCESS)
    {
        stat = CyU3PDmaChannelSetXfer (&glDebugChanHandle, 0);
    }

    if (stat == CY_U3P_SUCCESS)
    {
        stat = CyU3PDmaChannelGetBuffer (&glDebugChanHandle,
                &glDebugBuf_p, CYU3P_WAIT_FOREVER);
    }

    if (stat != CY_U3P_SUCCESS)
    {
        CyU3PDmaChannelDestroy (&glDebugChanHandle);
        CyU3PMutexPut (&glDebugLock);
        return stat;
    }

    /* Initialize the global variables */
    glDebugLogMask = CY_U3P_DEBUG_LOG_MASK_DEFAULT;
    glDebugBufOffset = 0;
    glDebugTraceLevel = traceLevel;

    glDebugInit = CyTrue;

    CyU3PMutexPut (&glDebugLock);

    return stat;
}

/*
* Summary
* This function de-initializes the debug. 
* It de-initializes the debug port and destroys the channel.
*/
CyU3PReturnStatus_t
CyU3PDebugDeInit (void)
{
    if (!glDebugInit)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }
    CyU3PMutexGet (&glDebugLock, CYU3P_WAIT_FOREVER);
    glDebugInit = CyFalse;
    CyU3PDmaChannelDestroy (&glDebugChanHandle);
    glDebugLogMask = CY_U3P_DEBUG_LOG_MASK_DEFAULT;
    CyU3PMutexPut (&glDebugLock);

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDebugSysMemInit (uint8_t *buffer, uint16_t size, CyBool_t isWrapAround, uint8_t traceLevel)
{
    if ((glDebugInit) || (glSysMemLogInit))
    {
        return CY_U3P_ERROR_ALREADY_STARTED;
    }

    if (buffer == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    if ((size == 0) || (size % sizeof (CyU3PDebugLog_t)))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    if ((buffer < CY_U3P_DMA_BUFFER_AREA_BASE) ||
            ((buffer + size) >= CY_U3P_DMA_BUFFER_AREA_LIMIT))
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    CyU3PMutexGet (&glDebugLock, CYU3P_WAIT_FOREVER);
    glDebugBuf_p.buffer = buffer;
    glDebugBuf_p.size   = size;
    glDebugBuf_p.count  = 0;
    glDebugBuf_p.status = (isWrapAround) ? CY_U3P_DEBUG_WRAP : 0;
    glDebugTraceLevel   = traceLevel;
    glDebugLogMask = CY_U3P_DEBUG_LOG_MASK_DEFAULT;

    glSysMemLogInit = CyTrue;
    CyU3PMutexPut (&glDebugLock);

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDebugSysMemDeInit (void)
{
    if (!glSysMemLogInit)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    CyU3PMutexGet (&glDebugLock, CYU3P_WAIT_FOREVER);
    glSysMemLogInit = CyFalse;
    glDebugLogMask = CY_U3P_DEBUG_LOG_MASK_DEFAULT;
    CyU3PMutexPut (&glDebugLock);

    return CY_U3P_SUCCESS;
}

/* Summary
 * Reset the debug DMA channel and initialize it for transfer again.
 */
static CyU3PReturnStatus_t
CyU3PDebugChannelReset (void)
{
    CyU3PReturnStatus_t stat;

    CyU3PDmaChannelReset (&glDebugChanHandle);
    stat = CyU3PDmaChannelSetXfer (&glDebugChanHandle, 0);
    if (stat == CY_U3P_SUCCESS)
    {
        stat = CyU3PDmaChannelGetBuffer (&glDebugChanHandle, &glDebugBuf_p, CYU3P_WAIT_FOREVER);
    }

    return stat;
}

/* 
* Summary
* Debug Thread Entry function. This thread is a low priority thread which prints the debug message sent by
* other threads or ISRs. It prints Priority of the message, module which has called send debug function  
* followed by the debug message.
*/
void
CyU3PDebugThreadEntry (uint32_t threadInput)
{
    CyU3PDebugLog_t msg;
    uint32_t status;
    CyU3PReturnStatus_t ret = CY_U3P_SUCCESS;

    for (;;)
    {
        status = CyU3PQueueReceive (&glDebugQueue, &msg, CYU3P_WAIT_FOREVER);

        if (status == CY_U3P_SUCCESS)
        {
            CyU3PMutexGet (&glDebugLock, CYU3P_WAIT_FOREVER);
            if (glDebugBufOffset != CY_U3P_DEBUG_DMA_BUFFER_SIZE)
            {
                CyU3PMemCopy (glDebugBuf_p.buffer + glDebugBufOffset, (uint8_t *)&msg, 8);
            }
             
            if (glDebugBufOffset == CY_U3P_DEBUG_DMA_BUFFER_SIZE - 8)
            {
                /* Now the buffer is full */
                ret = CyU3PDmaChannelCommitBuffer (&glDebugChanHandle, CY_U3P_DEBUG_DMA_BUFFER_SIZE, 0);
                if (ret == CY_U3P_SUCCESS)
                {
                    ret = CyU3PDmaChannelGetBuffer (&glDebugChanHandle, &glDebugBuf_p, CYU3P_WAIT_FOREVER);
                }

                if (ret != CY_U3P_SUCCESS)
                {
                    ret = CyU3PDebugChannelReset ();
                    if (ret != CY_U3P_SUCCESS)
                    {
                        /* Unrecoverable error in debug module. Stop functioning. */
                        break;
                    }
                }

                glDebugBufOffset = 0;
            }

            CyU3PMutexPut (&glDebugLock);
        }
    }
}

/*
 * This function enables/ disables sending preamble data depending on the parameter.
 * This is useful when the tool is not used to decode the debug log and prints.
 */
void 
CyU3PDebugPreamble (CyBool_t sendPreamble)
{
    glDebugSendPreamble = sendPreamble;
}

static MyDebugSNPrint (
        uint8_t  *debugMsg,
        uint16_t *length,
        char     *message,
        va_list   argp)
{
    uint8_t  *string_p;
    uint8_t  *argStr = NULL;
    CyBool_t  copyReqd = CyFalse;
    uint16_t  i = 0, j, maxLength = *length;
    int32_t   intArg;
    uint32_t  uintArg;
    uint8_t   convertedString[11];

    if (debugMsg == 0)
        return CY_U3P_ERROR_BAD_ARGUMENT;

    /* Parse the string and copy into the buffer for sending out. */
    for (string_p = (uint8_t *)message; (*string_p != '\0'); string_p++)
    {
        if (i >= (maxLength - 2))
            return CY_U3P_ERROR_BAD_ARGUMENT;

        if (*string_p != '%')
        {
            debugMsg[i++] = *string_p;
            continue;
        }

        string_p++;
        switch (*string_p)
        {
        case '%' :
            {
                debugMsg[i++] = '%';
            }
            break;

        case 'c' : 
            {
                debugMsg[i++] = (uint8_t)va_arg (argp, int32_t);
            }
            break;

        case 'd' : 
            {
                intArg = va_arg (argp, int32_t);
                if (intArg < 0)
                {
                    debugMsg[i++] = '-';
                    intArg = -intArg;
                }

                argStr = CyU3PDebugIntToStr (convertedString, intArg, 10);
                copyReqd = CyTrue;
            }
            break;

        case 's': 
            {
                argStr = va_arg (argp, uint8_t *); 
                copyReqd = CyTrue;
            }
            break;

        case 'u': 
            {
                uintArg = va_arg (argp, uint32_t); 
                argStr = CyU3PDebugIntToStr (convertedString, uintArg, 10);
                copyReqd = CyTrue;
            }
            break;

        case 'X':
        case 'x': 
            {
                uintArg = va_arg (argp, uint32_t); 
                argStr = CyU3PDebugIntToStr (convertedString, uintArg, 16);
                copyReqd = CyTrue;
            }
            break;

        default:
            return CY_U3P_ERROR_BAD_ARGUMENT;
        }

        if (copyReqd)
        {
            j = (uint16_t)strlen ((char *)argStr);
            if (i >= (maxLength - j - 1))
                return CY_U3P_ERROR_BAD_ARGUMENT;
            strcpy ((char *)(debugMsg + i), (char *)argStr);
            i += j;
            copyReqd = CyFalse;
        }
    }

    /* NULL-terminate the string. There will always be space for this. */
    debugMsg[i] = '\0';
    *length     = i;

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDebugStringPrint (
        uint8_t *buffer,
        uint16_t maxLength,
        char    *fmt,
        ...)
{
    CyU3PReturnStatus_t stat;
    uint16_t len = maxLength;
    va_list  argp;
   
    va_start (argp, fmt);
    stat = MyDebugSNPrint (buffer, &len, fmt, argp);
    va_end (argp);

    return stat;
}

/* 
 * Summary
 * This function is called by other threads. This calculates the size of the debug message,
 * allocates that many byte and then copies the message after converting all the arguments to strings.
 * Then commits the buffer.
 */
CyU3PReturnStatus_t 
CyU3PDebugPrint (
        uint8_t priority,
        char *message,
        ...)
{
    CyU3PThread        *currentThread;
    CyU3PDebugLog_t     log;
    CyU3PReturnStatus_t stat;

    va_list   argp;
    uint8_t  *debugMsg;
    uint8_t  *threadName;
    uint8_t   threadId;
    uint16_t  limit;

    if (!glDebugInit)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if (priority > glDebugTraceLevel)
    {
        return CY_U3P_SUCCESS;
    }

    currentThread =  CyU3PThreadIdentify ();
    if (currentThread == NULL)
    {
        /* This function can only be called from a thread. */
        return CY_U3P_ERROR_INVALID_CALLER;
    }

    /* Check if the thread ID has been masked. */
    CyU3PThreadInfoGet (currentThread, &threadName, NULL, NULL, NULL);
    threadId = (10 * (threadName[0] - '0') + (threadName[1] - '0'));
    
    CyU3PMutexGet (&glDebugLock, CYU3P_WAIT_FOREVER);

    /* Commit the buffer if there is any pending debug logs to be sent. */
    if (glDebugBufOffset)
    {
        stat = CyU3PDmaChannelCommitBuffer (&glDebugChanHandle, CY_U3P_DEBUG_DMA_BUFFER_SIZE, 0);
        if (stat == CY_U3P_SUCCESS)
        {
            stat = CyU3PDmaChannelGetBuffer (&glDebugChanHandle, &glDebugBuf_p, CYU3P_WAIT_FOREVER);
        }

        if (stat != CY_U3P_SUCCESS)
        {
            CyU3PDebugChannelReset ();
        }

        glDebugBufOffset = 0;
    }

    debugMsg = glDebugBuf_p.buffer;
    limit    = CY_U3P_DEBUG_DMA_BUFFER_SIZE;
    if (glDebugSendPreamble)
    {
        debugMsg += 8;
        limit    -= 8;
    }

    va_start (argp, message);
    stat = MyDebugSNPrint (debugMsg, &limit, message, argp);
    va_end (argp);

    if (glDebugSendPreamble)
    {
        debugMsg = glDebugBuf_p.buffer;

        /* Prepend the header for the string. */
        log.priority = priority;
        log.threadId = threadId;
        log.msg      = 0xffff;
        log.param    = limit;
        CyU3PMemCopy (debugMsg, (uint8_t *)&log, 8);

        limit += 8;
    }

    stat = CyU3PDmaChannelCommitBuffer (&glDebugChanHandle, limit, 0);
    if (stat == CY_U3P_SUCCESS)
    {
        stat = CyU3PDmaChannelGetBuffer (&glDebugChanHandle, &glDebugBuf_p, CYU3P_WAIT_FOREVER);
    }
    if (stat != CY_U3P_SUCCESS)
    {
        CyU3PDebugChannelReset ();
        return stat;
    }

    glDebugBufOffset = 0;
    CyU3PMutexPut (&glDebugLock);
    return CY_U3P_SUCCESS;
}

/* 
* Summary
* This function is called by other threads and ISRs. This adds logs to the buffer. It doesnot
* commit the buffer immediately. 
*/
CyU3PReturnStatus_t 
CyU3PDebugLog (uint8_t priority, uint16_t message, uint32_t parameter)
{
    uint8_t threadId;
    CyU3PDebugLog_t logMsg;
    uint32_t wait, status, size;
    CyU3PThread *currentThread = CyU3PThreadIdentify ();
    uint8_t *threadName;

    if ((!glDebugInit) && (!glSysMemLogInit))
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if (priority > glDebugTraceLevel)
    {
        return CY_U3P_SUCCESS;
    }
    /* Check if the thread ID has been masked. */
    if (currentThread == NULL)
    {
        wait = 0;
        threadId = 0;
        threadName = NULL;
    }
    else
    {
        CyU3PThreadInfoGet (currentThread, &threadName, NULL, NULL, NULL);
        threadId = (10 * (threadName[0] - 48) + (threadName[1] - 48));
        wait = CY_U3P_DEBUG_QUEUE_WAIT_OPTION;
    }

    logMsg.threadId = threadId;    /* Thread Id. */
    logMsg.priority = priority;    /* Fix the priority */
    logMsg.msg =  message;         /* Fix the message */
    logMsg.param = parameter;      /* Fix the parameter */

    status = CyU3PMutexGet (&glDebugLock, wait);
    if (status != CY_U3P_SUCCESS)
    {
        return CY_U3P_ERROR_FAILURE;
    }

    if (glSysMemLogInit)
    {
        /* Check if we have space in the buffer to add this log. */
        if (glDebugBuf_p.count < glDebugBuf_p.size)
        {
            size = CY_U3P_MIN (glDebugBuf_p.size - glDebugBuf_p.count, sizeof (CyU3PDebugLog_t));
            CyU3PMemCopy ((glDebugBuf_p.buffer + glDebugBuf_p.count), (uint8_t *)&logMsg, size);
            glDebugBuf_p.count += size;

            if (glDebugBuf_p.count == glDebugBuf_p.size)
            {
                if (glDebugBuf_p.status & CY_U3P_DEBUG_WRAP)
                {
                    if (size < sizeof (CyU3PDebugLog_t))
                    {
                        CyU3PMemCopy (glDebugBuf_p.buffer, ((uint8_t *)&logMsg) + size,
                                sizeof (CyU3PDebugLog_t) - size);
                        glDebugBuf_p.count = sizeof (CyU3PDebugLog_t) - size;
                    }
                    else
                        glDebugBuf_p.count = 0;
                }
            }

            /* Make sure the log area is cleaned from the D-cache. */
            CyU3PSysCleanDRegion ((uint32_t *)glDebugBuf_p.buffer, glDebugBuf_p.size);
        }
    }
    else
    {
        if (glDebugBufOffset == CY_U3P_DEBUG_DMA_BUFFER_SIZE - 8)
        {
            /* msg.size = CY_U3P_DEBUG_DMA_BUFFER_SIZE; */
            /* Memory for only one log or none is available.
             * Only thread can do commit and get buffer */
            CyU3PQueueSend(&glDebugQueue, &logMsg, wait);
        }
        else
        {
            /* After writing this the memory will have memory
             * for atleast one log */ 
            CyU3PMemCopy ((glDebugBuf_p.buffer + glDebugBufOffset),
                    (uint8_t *)&logMsg, 8);
            glDebugBufOffset += 8;
        }
    }
    CyU3PMutexPut (&glDebugLock);

    return CY_U3P_SUCCESS;
}

/* 
 * Debug flush function. It commits the buffer and gets a new buffer 
 */
CyU3PReturnStatus_t 
CyU3PDebugLogFlush (void)
{
    CyU3PThread *currentThread = CyU3PThreadIdentify ();
    CyU3PReturnStatus_t stat;

    if (!glDebugInit)
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if (currentThread == NULL)
    {
        return CY_U3P_ERROR_INVALID_CALLER;
    }

    CyU3PMutexGet (&glDebugLock, CYU3P_WAIT_FOREVER);
    stat = CyU3PDmaChannelCommitBuffer (&glDebugChanHandle, glDebugBufOffset, 0);
    if (stat == CY_U3P_SUCCESS)
    {
        stat = CyU3PDmaChannelGetBuffer (&glDebugChanHandle, &glDebugBuf_p, CYU3P_WAIT_FOREVER);
    }
    if (stat != CY_U3P_SUCCESS)
    {
        CyU3PDebugChannelReset ();
    }

    glDebugBufOffset = 0;
    CyU3PMutexPut (&glDebugLock);
    return CY_U3P_SUCCESS;
}


/* 
 * Debug Clear function. It clears the buffer without commiting it. 
 */
CyU3PReturnStatus_t 
CyU3PDebugLogClear (void)
{
    CyU3PThread *currentThread = CyU3PThreadIdentify ();

    if ((!glDebugInit) || (!glSysMemLogInit))
    {
        return CY_U3P_ERROR_NOT_STARTED;
    }

    if (currentThread == NULL)
    {
        return CY_U3P_ERROR_INVALID_CALLER;
    }

    CyU3PMutexGet (&glDebugLock, CYU3P_WAIT_FOREVER);
    if (glDebugInit)
    {
        glDebugBufOffset = 0;
    }
    else
    {
        glDebugBuf_p.count = 0;
    }
    CyU3PMutexPut (&glDebugLock);

    return CY_U3P_SUCCESS;
}

/*
 * Summary
 * Converts a number to a string depending on base (hex or int) e.g. 10 to "A" or "10"
 */
uint8_t *
CyU3PDebugIntToStr(uint8_t *convertedString, uint32_t num, uint8_t base)
{
    uint8_t *str_p, i = 10;
    str_p = convertedString;
    str_p[i] = '\0';
    do
    {
        str_p[--i] = "0123456789ABCDEF"[num%base];
        num /= base;
    } while (num != 0);
    return (&str_p[i]);
}

/* Set the trace level. It can be updated during runtime also. */
void
CyU3PDebugSetTraceLevel (uint8_t level)
{
    if (!glDebugInit)
    {
        return;
    }

    CyU3PMutexGet (&glDebugLock, CYU3P_WAIT_FOREVER);
    glDebugTraceLevel = level;
    CyU3PMutexPut (&glDebugLock);
}

/* Enable the debug logging. */
void
CyU3PDebugEnable (uint16_t threadMask)
{
    if (!glDebugInit)
    {
        return;
    }

    CyU3PMutexGet (&glDebugLock, CYU3P_WAIT_FOREVER);
    glDebugLogMask = threadMask;
    CyU3PMutexPut (&glDebugLock);
}

/* Disable the debug logging. */
uint16_t
CyU3PDebugDisable ()
{
    uint16_t threadMask;

    if (!glDebugInit)
    {
        return CY_U3P_DEBUG_LOG_MASK_DEFAULT;
    }

    CyU3PMutexGet (&glDebugLock, CYU3P_WAIT_FOREVER);
    threadMask = glDebugLogMask;
    glDebugLogMask = 0;
    CyU3PMutexPut (&glDebugLock);

    return threadMask;
}

/* [] */

