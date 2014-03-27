/*
 ## Cypress USB 3.0 Platform source file (cyfxgpiocomplexapp.c)
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

/*
 * This example illustrates the use of the FX3 firmware APIs to implement
 * a complex GPIO application example.
 *
 * The example does the following:
 * 
 * 1. A PWM output generated on GPIO 50. The time period of the PWM is set to
 *    generate 1KHz 25% duty cycle for 1s and the to generate 75% duty cycle
 *    for the next 1s. This is repeated in a loop.
 *
 * 2. GPIO 51 is used to measure the low time period for the signal generated
 *    by GPIO 50. GPIO 50 needs to be connected to GPIO 51. This is done by using
 *    the MEASURE_LOW_ONCE feature.
 *    
 * 3. GPIO 52 is used as a counter input. The line is internally pulled to high
 *    with the weak pull-up feature. When a low is applied to the IO line, the
 *    negative edge is used to increment the counter. The counter can be sampled
 *    to identify the current count.
 *
 * GPIO 50 can be accessed at J20.4 on the DVK.
 * GPIO 51 can be accessed at J20.5 on the DVK.
 * GPIO 52 can be accessed at J20.6 on the DVK.
 */

#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3error.h>
#include <cyu3gpio.h>
#include <cyu3uart.h>

#define CY_FX_GPIOAPP_THREAD_STACK       (0x0400)       /* GPIO application thread stack size */
#define CY_FX_GPIOAPP_THREAD_PRIORITY    (8)            /* GPIO application thread priority */

#define CY_FX_PWM_PERIOD                 (201600 - 1)   /* PWM time period. */
#define CY_FX_PWM_25P_THRESHOLD          (50400  - 1)   /* PWM threshold value for 25% duty cycle. */
#define CY_FX_PWM_75P_THRESHOLD          (151200 - 1)   /* PWM threshold value for 75% duty cycle. */

CyU3PThread gpioPWMThread;      /* GPIO thread structure */
CyU3PThread gpioMeasureThread;  /* GPIO thread structure */
CyU3PThread gpioCounterThread;  /* GPIO thread structure */

/* Application error handler. */
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

/* Initialize the debug module with UART. */
CyU3PReturnStatus_t
CyFxDebugInit (
        void)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    CyU3PUartConfig_t uartConfig;

    /* Initialize and configure the UART for logging. */
    status = CyU3PUartInit ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof (uartConfig));
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit  = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity   = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyFalse;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma    = CyTrue;
    status = CyU3PUartSetConfig (&uartConfig, NULL);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Set the dma for an inifinity transfer */
    status = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Start the debug module for printing log messages. */
    status = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);

    return status;
}

/* GPIO application initialization function. */
void
CyFxGpioInit (void)
{
    CyU3PGpioClock_t gpioClock;
    CyU3PGpioComplexConfig_t gpioConfig;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Init the GPIO module. The GPIO block will be running 
     * with a fast clock at SYS_CLK / 2 and slow clock is not
     * used. For the DVK, the SYS_CLK is running at 403 MHz.*/
    gpioClock.fastClkDiv = 2;
    gpioClock.slowClkDiv = 0;
    gpioClock.simpleDiv = CY_U3P_GPIO_SIMPLE_DIV_BY_2;
    gpioClock.clkSrc = CY_U3P_SYS_CLK;
    gpioClock.halfDiv = 0;

    apiRetStatus = CyU3PGpioInit(&gpioClock, NULL);
    if (apiRetStatus != 0)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "CyU3PGpioInit failed, error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Configure GPIO 50 as PWM output */
    gpioConfig.outValue = CyFalse;
    gpioConfig.inputEn = CyFalse;
    gpioConfig.driveLowEn = CyTrue;
    gpioConfig.driveHighEn = CyTrue;
    gpioConfig.pinMode = CY_U3P_GPIO_MODE_PWM;
    gpioConfig.intrMode = CY_U3P_GPIO_NO_INTR;
    gpioConfig.timerMode = CY_U3P_GPIO_TIMER_HIGH_FREQ;
    gpioConfig.timer = 0;
    gpioConfig.period = CY_FX_PWM_PERIOD;
    gpioConfig.threshold = CY_FX_PWM_25P_THRESHOLD;
    apiRetStatus = CyU3PGpioSetComplexConfig(50, &gpioConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PGpioSetComplexConfig failed, error code = %d\n",
                apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Configure GPIO 51 as input for single time low measurement. */
    gpioConfig.outValue = CyTrue;
    gpioConfig.inputEn = CyTrue;
    gpioConfig.driveLowEn = CyFalse;
    gpioConfig.driveHighEn = CyFalse;
    /* For doing one time measurements, this value should be
     * set to static. For doing contiunous measurement, this
     * value should be set to the required mode. */
    gpioConfig.pinMode = CY_U3P_GPIO_MODE_STATIC;
    gpioConfig.intrMode = CY_U3P_GPIO_NO_INTR;
    gpioConfig.timerMode = CY_U3P_GPIO_TIMER_HIGH_FREQ;
    gpioConfig.timer = 0;
    /* Load the period to the maximum value
     * so that the count is not reset. */
    gpioConfig.period = 0xFFFFFFFF;
    gpioConfig.threshold = 0;
    apiRetStatus = CyU3PGpioSetComplexConfig(51, &gpioConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PGpioSetComplexConfig failed, error code = %d\n",
                apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Enable a weak pullup on GPIO 52. */
    apiRetStatus = CyU3PGpioSetIoMode (52, CY_U3P_GPIO_IO_MODE_WPU);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PGpioSetIoMode failed, error code = %d\n",
                apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Configure GPIO 52 as input for counter mode. */
    gpioConfig.outValue = CyTrue;
    gpioConfig.inputEn = CyTrue;
    gpioConfig.driveLowEn = CyFalse;
    gpioConfig.driveHighEn = CyFalse;
    gpioConfig.pinMode = CY_U3P_GPIO_MODE_STATIC;
    gpioConfig.intrMode = CY_U3P_GPIO_NO_INTR;
    gpioConfig.timerMode = CY_U3P_GPIO_TIMER_NEG_EDGE;
    gpioConfig.timer = 0;
    /* Load the period to the maximum value
     * so that the count is not reset. */
    gpioConfig.period = 0xFFFFFFFF;
    gpioConfig.threshold = 0;
    apiRetStatus = CyU3PGpioSetComplexConfig(52, &gpioConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PGpioSetComplexConfig failed, error code = %d\n",
                apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }
}

/* Entry function for the gpioPWMThread */
void
GpioPWMThread_Entry (
        uint32_t input)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize Debug module */
    apiRetStatus = CyFxDebugInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Initialize GPIO module. */
    CyFxGpioInit ();

    /* Now resume the other two threads. */
    CyU3PThreadResume (&gpioMeasureThread);
    CyU3PThreadResume (&gpioCounterThread);

    for (;;)
    {
        /* Wait for 1s. */
        CyU3PThreadSleep (1000);

        /* Change the PWM duty cycle to 75%. */
        apiRetStatus = CyU3PGpioComplexUpdate (50,
                CY_FX_PWM_75P_THRESHOLD, CY_FX_PWM_PERIOD);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PGpioComplexUpdate failed, error code = %d\n",
                    apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }

        /* Wait for 1s. */
        CyU3PThreadSleep (1000);

        /* Change the PWM duty cycle to 25%. */
        apiRetStatus = CyU3PGpioComplexUpdate (50,
                CY_FX_PWM_25P_THRESHOLD, CY_FX_PWM_PERIOD);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PGpioComplexUpdate failed, error code = %d\n",
                    apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }
    }
}

/* Entry function for the gpioMeasureThread. */
void
GpioMeasureThread_Entry (
        uint32_t input)
{
    uint32_t threshold = 0;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    for (;;)
    {
        /* Invoke the measure API. This will measure the low period for the PWM */
        apiRetStatus = CyU3PGpioComplexMeasureOnce (51,
                CY_U3P_GPIO_MODE_MEASURE_LOW_ONCE);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PGpioComplexMeasureOnce failed, error code = %d\n",
                    apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }

        /* Since this thread should not block other threads, do not wait. */
        do
        {
            CyU3PThreadSleep (1);
            apiRetStatus = CyU3PGpioComplexWaitForCompletion (51, &threshold, CyFalse);

        } while (apiRetStatus == CY_U3P_ERROR_TIMEOUT);

        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PGpioComplexWaitForCompletion failed, error code = %d\n",
                    apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }

        /* Print out the threshold value. */
        CyU3PDebugPrint (4, "GPIO 51 low threshold = %d.\n", threshold);

        /* Sample every 0.5s. */
        CyU3PThreadSleep (500);
    }
}

/* Entry function for the gpioCounterThread */
void
GpioCounterThread_Entry (
        uint32_t input)
{
    uint32_t threshold = 0;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    for (;;)
    {
        /* This will retreive the current counter value. */
        apiRetStatus = CyU3PGpioComplexSampleNow (52, &threshold);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PGpioComplexSampleNow failed, error code = %d\n",
                    apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }

        /* Print out the counter value. */
        CyU3PDebugPrint (4, "GPIO 52 counter reading = %d.\n", threshold);

        /* Sample every 2s. */
        CyU3PThreadSleep (2000);
    }
}

/* Application define function which creates the threads. */
void
CyFxApplicationDefine (
        void)
{
    void *ptr = NULL;
    uint32_t retThrdCreate = CY_U3P_SUCCESS;

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_GPIOAPP_THREAD_STACK);

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (&gpioPWMThread,           /* GPIO Example App Thread structure */
                          "21:GPIO_PWM",                         /* Thread ID and Thread name */
                          GpioPWMThread_Entry,                   /* GPIO Example App Thread Entry function */
                          0,                                     /* No input parameter to thread */
                          ptr,                                   /* Pointer to the allocated thread stack */
                          CY_FX_GPIOAPP_THREAD_STACK,            /* Thread stack size */
                          CY_FX_GPIOAPP_THREAD_PRIORITY,         /* Thread priority */
                          CY_FX_GPIOAPP_THREAD_PRIORITY,         /* Pre-emption threshold for the thread. */
                          CYU3P_NO_TIME_SLICE,                   /* No time slice for the application thread */
                          CYU3P_AUTO_START                       /* Start the Thread immediately */
                          );

    /* Check the return code */
    if (retThrdCreate != 0)
    {
        /* Thread creation failed with the error code retThrdCreate */

        /* Add custom recovery or debug actions here */

        /* Application cannot continue */
        /* Loop indefinitely */
        while(1);
    }

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_GPIOAPP_THREAD_STACK);

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (&gpioMeasureThread,        /* GPIO Example App Thread structure */
                          "22:GPIO_measure",                      /* Thread ID and Thread name */
                          GpioMeasureThread_Entry,                /* GPIO Example App Thread entry function */
                          0,                                      /* No input parameter to thread */
                          ptr,                                    /* Pointer to the allocated thread stack */
                          CY_FX_GPIOAPP_THREAD_STACK,             /* Thread stack size */
                          CY_FX_GPIOAPP_THREAD_PRIORITY,          /* Thread priority */
                          CY_FX_GPIOAPP_THREAD_PRIORITY,          /* Pre-emption threshold for the thread */
                          CYU3P_NO_TIME_SLICE,                    /* No time slice for the application thread */
                          CYU3P_AUTO_START                        /* Start the Thread immediately */
                          );

    /* Check the return code */
    if (retThrdCreate != 0)
    {
        /* Thread creation failed with the error code retThrdCreate */

        /* Add custom recovery or debug actions here */

        /* Application cannot continue */
        /* Loop indefinitely */
        while(1);
    }

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_GPIOAPP_THREAD_STACK);

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (&gpioCounterThread,        /* GPIO Example App Thread structure */
                          "23:GPIO_counter",                      /* Thread ID and Thread name */
                          GpioCounterThread_Entry,                /* GPIO Example App Thread entry function */
                          0,                                      /* No input parameter to thread */
                          ptr,                                    /* Pointer to the allocated thread stack */
                          CY_FX_GPIOAPP_THREAD_STACK,             /* Thread stack size */
                          CY_FX_GPIOAPP_THREAD_PRIORITY,          /* Thread priority */
                          CY_FX_GPIOAPP_THREAD_PRIORITY,          /* Pre-emption threshold for the thread */
                          CYU3P_NO_TIME_SLICE,                    /* No time slice for the application thread */
                          CYU3P_AUTO_START                        /* Start the Thread immediately */
                          );

    /* Check the return code */
    if (retThrdCreate != 0)
    {
        /* Thread creation failed with the error code retThrdCreate */

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

    /* Initialize the caches. Enable both Instruction and Data Caches. */
    status = CyU3PDeviceCacheControl (CyTrue, CyTrue, CyTrue);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Configure the IO matrix for the device. On the FX3 DVK board,
     * the COM port is connected to the IO(53:56). This means that
     * either DQ32 mode should be selected or lppMode should be set
     * to UART_ONLY. Here we are choosing UART_ONLY configuration. */
    CyU3PMemSet ((uint8_t *)&io_cfg, 0, sizeof(io_cfg));
    io_cfg.isDQ32Bit = CyFalse;
    io_cfg.s0Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.s1Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.useUart   = CyTrue;
    io_cfg.useI2C    = CyFalse;
    io_cfg.useI2S    = CyFalse;
    io_cfg.useSpi    = CyFalse;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_UART_ONLY;
    io_cfg.gpioSimpleEn[0]  = 0;
    io_cfg.gpioSimpleEn[1]  = 0;
    /* GPIOs 50, 51 and 52 are used as complex GPIO. */
    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0x001C0000;
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

