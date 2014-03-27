/*
 ## Cypress USB 3.0 Platform source file (cyu3iocfg.c)
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

/* This file defines the IO configuration specific APIs. */

#include <cyu3system.h>
#include <cyu3error.h>
#include <cyu3regs.h>
#include <cyu3utils.h>
#include <cyfx3_api.h>

#define CY_U3P_GPIO_COUNT (61)
CyBool_t 
CyU3PIsLppIOConfigured(
        CyU3PLppModule_t lppModule)
{
    CyBool_t ioConfigValue = CyFalse;

    switch(lppModule)
    {
        case CY_U3P_LPP_I2S:
        ioConfigValue = CyFx3DevIOIsI2sConfigured ();
            break;
        case CY_U3P_LPP_I2C:
        ioConfigValue = CyFx3DevIOIsI2cConfigured ();
            break;
        case CY_U3P_LPP_UART:
        ioConfigValue = CyFx3DevIOIsUartConfigured ();
            break;
        case CY_U3P_LPP_SPI:
        ioConfigValue = CyFx3DevIOIsSpiConfigured ();
            break;
        default:
            break;
    }

    return ioConfigValue;
}

CyBool_t 
CyU3PIsGpioValid (
        uint8_t gpioId)
{
    return (gpioId < CY_U3P_GPIO_COUNT);
}

CyBool_t 
CyU3PIsGpioSimpleIOConfigured(
                              uint32_t gpioId)
{
    return (CyFx3DevIOIsGpio (gpioId, CyTrue));
}

CyBool_t 
CyU3PIsGpioComplexIOConfigured(
        uint32_t gpioId)
{    
    return (CyFx3DevIOIsGpio (gpioId, CyFalse));
}

CyU3PReturnStatus_t
CyU3PDeviceGpioOverride (
        uint8_t gpioId,
        CyBool_t isSimple)
{
    if (gpioId >= CY_U3P_GPIO_COUNT)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    CyFx3DevIOSelectGpio (gpioId, CyTrue, isSimple);
    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t
CyU3PDeviceGpioRestore (
        uint8_t gpioId)
{
    if (gpioId >= CY_U3P_GPIO_COUNT)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    CyFx3DevIOSelectGpio (gpioId, CyFalse, CyTrue);
    return CY_U3P_SUCCESS;
}

/* This function is only for the use of FX3 device */
CyU3PReturnStatus_t
CyU3PDeviceConfigureIOMatrix (
        CyU3PIoMatrixConfig_t *cfg_p)
{
    /* Check for parameter validity. */
    if (cfg_p == NULL)
    {
        return CY_U3P_ERROR_NULL_POINTER;
    }

    if (CyFx3DevIOConfigure (cfg_p) == CyFalse)
        return CY_U3P_ERROR_BAD_ARGUMENT;

    return CY_U3P_SUCCESS;
}

/*
 * This function sets the Drive Strength for the Serial Interfaces (UART,SPI and I2S).
 */
CyU3PReturnStatus_t
CyU3PSetSerialIoDriveStrength (
            CyU3PDriveStrengthState_t serialIoDriveStrength
            )
{
    /* Check for parameter validity. */
    if (serialIoDriveStrength > CY_U3P_DS_FULL_STRENGTH)
    {
        return CY_U3P_ERROR_BAD_ARGUMENT;
    }

    /* Reset the current I2C Drive strength */
    GCTL->ds &= ~(CY_U3P_S1LDS_MASK);

    /* Set the requested Drive strength */
    GCTL->ds |= (((uint32_t)serialIoDriveStrength << CY_U3P_S1LDS_POS) & CY_U3P_S1LDS_MASK);

    return CY_U3P_SUCCESS;

}

/* [] */

