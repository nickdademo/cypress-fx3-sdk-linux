/*
 ## Cypress FX3 Boot Firmware Example Source file (main.c)
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

#include "cyfx3usb.h"
#include "cyfx3device.h"
#include "cyfx3utils.h"
#include "cyfx3gpio.h"

/* GPIO to be used for testing. */
#define FX3_GPIO_TEST_OUT               (50)
#define FX3_GPIO_TO_LOFLAG(gpio)        (1 << (gpio))
#define FX3_GPIO_TO_HIFLAG(gpio)        (1 << ((gpio) - 32))

/* Enable this for booting off the USB */
#define USB_BOOT
#ifdef USB_BOOT
extern void
myUsbBoot (
        void);
extern uint8_t glCheckForDisconnect;
extern uint8_t glInCompliance;
#endif


/* Enable this for booting off the SPI Flash */
/* #define SPI_BOOT */
#ifdef SPI_BOOT
extern CyFx3BootErrorCode_t initSpi (void);
extern CyBool_t bootFromSpi (void);
#endif

void 
myMemCopy (
        uint8_t *d, 
        uint8_t *s, 
        int32_t cnt
        )
{
    int32_t i;
    for (i = 0; i < cnt; i++)
    {
        *d++ = *s++;
    }
}

void 
myMemSet (
	uint8_t *d, 
	uint8_t c, 
	int32_t cnt
	)
{
    int32_t i;
    for (i = 0; i < cnt; i++)
    {
        *d++ = c;
    }
}

/****************************************************************************
 * main:
 ****************************************************************************/
int main (
        void)
{
    int temp = 0;

    CyFx3BootErrorCode_t status;
    CyFx3BootIoMatrixConfig_t  ioCfg;
    CyFx3BootGpioSimpleConfig_t gpioCfg;

    /* HW and SW initialization code  */
    CyFx3BootDeviceInit (CyTrue);

    ioCfg.isDQ32Bit = CyFalse;
    ioCfg.useUart   = CyFalse;
    ioCfg.useI2C    = CyFalse;
    ioCfg.useI2S    = CyFalse;
    ioCfg.useSpi    = CyTrue;
    ioCfg.gpioSimpleEn[0] = 0;
    ioCfg.gpioSimpleEn[1] = FX3_GPIO_TO_HIFLAG(FX3_GPIO_TEST_OUT);

    status = CyFx3BootDeviceConfigureIOMatrix (&ioCfg);

    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

#ifdef SPI_BOOT 
    /* Enable this for booting off the SPI */
    status = initSpi ();

    if(status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    bootFromSpi ();
#endif    

    /* Initialize the GPIO module. Force GPIO[21]/CTL4 low. Then request to retain GPIO state across boot. */
    CyFx3BootGpioInit ();

    gpioCfg.outValue    = CyFalse;
    gpioCfg.driveLowEn  = CyTrue;
    gpioCfg.driveHighEn = CyTrue;
    gpioCfg.inputEn     = CyFalse;
    gpioCfg.intrMode    = CY_FX3_BOOT_GPIO_NO_INTR;
    status = CyFx3BootGpioSetSimpleConfig (FX3_GPIO_TEST_OUT, &gpioCfg);
    if (status != CY_FX3_BOOT_SUCCESS)
        return status;

    CyFx3BootRetainGpioState ();

#ifdef USB_BOOT
    /* Enable this for booting off the USB */
    myUsbBoot ();
#endif

    while (1)
    {
        temp = temp;

/* Enable this piece of code when using the USB module. */
#ifdef USB_BOOT
        
        if (glCheckForDisconnect)
        {
            CyFx3BootUsbCheckUsb3Disconnect ();
            glCheckForDisconnect = 0;
        }

        CyFx3BootUsbEp0StatusCheck ();

        if (glInCompliance)
        {
            CyFx3BootUsbSendCompliancePatterns ();
            glInCompliance = 0;
        }
#endif
    }

    return 0;
}

