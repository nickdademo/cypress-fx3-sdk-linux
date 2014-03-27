/*
## Cypress USB 3.0 Platform header file (cyfx3gpio.h)
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

#ifndef _INCLUDED_CYFX3GPIO_H_
#define _INCLUDED_CYFX3GPIO_H_

#include <cyu3types.h>
#include <cyfx3error.h>
#include <cyfx3device.h>
#include <cyu3externcstart.h>

/** \file cyfx3gpio.h
    \brief The file defines the data structures and APIs that are provided
    for making use of FX3 GPIOs.

    **Description**\n
    Any unused pin on the FX3 device can be used as a GPIO. The Boot API library
    only allows for these pins to be used as standard input or output pins.
*/

/**************************************************************************
 ******************************* Data Types *******************************
 **************************************************************************/

/** \brief List of GPIO interrupt modes.

    **Description**\n
    FX3 GPIOs can be configured to trigger interrupts when a desired transition
    or signal level is detected on the input signal.

    **Note**\n
    Interrupts are not supported in this release of booter. The interrupt mode
    for all GPIOs need to be set to CY_FX3_BOOT_GPIO_NO_INTR.
 */
typedef enum CyFx3BootGpioIntrMode_t
{
    CY_FX3_BOOT_GPIO_NO_INTR = 0,        /**< No Interrupt is triggered. */
    CY_FX3_BOOT_GPIO_INTR_POS_EDGE,      /**< Interrupt is triggered on positive edge of input. */
    CY_FX3_BOOT_GPIO_INTR_NEG_EDGE,      /**< Interrupt is triggered on negative edge of input. */
    CY_FX3_BOOT_GPIO_INTR_BOTH_EDGE,     /**< Interrupt is triggered on both edges of input. */
    CY_FX3_BOOT_GPIO_INTR_LOW_LEVEL,     /**< Interrupt is triggered when input is low. */
    CY_FX3_BOOT_GPIO_INTR_HIGH_LEVEL     /**< Interrupt is triggered when input is high. */
} CyFx3BootGpioIntrMode_t;

/** \brief Configuration information for simple GPIOs.

    **Description**\n
    This structure encapsulates all of the configuration information for a
    simple GPIO on the FX3 device.

    If the pin is configured as input, then the fields driveLowEn and driveHighEn
    should be CyFalse. The outValue field is a don't care in this case.

    If the pin is configured as an output, the inputEn field should be CyFalse.
    The driveLowEn and driveHighEn fields can be used to select whether the device
    should drive the pin to low/high levels or just tri-state it.

    **\see
    *\see CyFx3BootGpioIntrMode_t
 */
typedef struct CyFx3BootGpioSimpleConfig_t
{
    CyBool_t outValue;                  /**< Initial value of the GPIO if configured as output:
                                             CyFalse = 0, CyTrue = 1. */
    CyBool_t driveLowEn;                /**< When set true, the output driver is enabled for outValue = CyFalse (0),
                                             otherwise tristated. */
    CyBool_t driveHighEn;               /**< When set true, the output driver is enabled for outValue = CyTrue (1),
                                             otherwise tristated. */
    CyBool_t inputEn;                   /**< When set true, the input state is enabled. */
    CyFx3BootGpioIntrMode_t intrMode;   /**< Interrupt mode for the GPIO. Should be set to CY_FX3_BOOT_GPIO_NO_INTR. */
} CyFx3BootGpioSimpleConfig_t;

/**************************************************************************
 *************************** Function prototypes **************************
 **************************************************************************/

/** \brief Initializes the GPIO block.

    **Description**\n
    This function should be called before any other GPIO APIs can be called.
    This powers up the GPIO block and sets up the API data structures.

    **Return value**\n
    * None

    **\see
    *\see CyFx3BootGpioDeInit
 */
extern void
CyFx3BootGpioInit (
        void);

/** \brief De-initializes the GPIO block.

    **Description**\n
    This function powers off the GPIO block on the FX3 device.

    **Return value**\n
    * None

    **\see
    *\see CyFx3BootGpioInit
 */
extern void
CyFx3BootGpioDeInit (
        void);

/** \brief Configures a simple GPIO.

    **Description**\n
    This function is used to configure and enable a simple GPIO pin. This
    function needs to be called before using the simple GPIO.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS              - if the operation is successful.\n
    * CY_FX3_BOOT_ERROR_NULL_POINTER   - if cfg_p is NULL.\n
    * CY_FX3_BOOT_ERROR_NOT_CONFIGURED - if the pin has not been selected as a GPIO in the IOMATRIX.

    **\see
    *\see CyFx3BootGpioSimpleConfig_t
    *\see CyFx3BootDeviceConfigureIOMatrix
    *\see CyFx3BootGpioDisable
 */
extern CyFx3BootErrorCode_t
CyFx3BootGpioSetSimpleConfig (
       uint8_t gpioId,                          /**< GPIO id to be configured. */
       CyFx3BootGpioSimpleConfig_t *cfg_p       /**< Configuration parameters for the GPIO. */
       );

/** \brief Disables a GPIO pin.

    **Description**\n
    This function is used to disable the use of a pin as a GPIO. Both input and output
    stages on the pin will be disabled after calling this API.

    **Return value**\n
    * None

    **\see
    *\see CyFx3BootGpioSetSimpleConfig
 */
extern void
CyFx3BootGpioDisable (
        uint8_t gpioId                          /**< GPIO ID to be disabled */
        );

/** \brief Query the state of GPIO input.

    **Description**\n
    Get the current signal level on a GPIO input pin. This API can also be used
    to retrieve the current value being driven out on a output pin.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS              - if the operation is successful.\n
    * CY_FX3_BOOT_ERROR_NULL_POINTER   - if value_p is NULL.\n
    * CY_FX3_BOOT_ERROR_NOT_CONFIGURED - if the pin is not selected as a GPIO in the IOMATRIX.

    **\see
    *\see CyFx3BootGpioSetSimpleConfig
    *\see CyFx3BootGpioSetValue
 */
extern CyFx3BootErrorCode_t
CyFx3BootGpioGetValue (
        uint8_t  gpioId,                        /**< GPIO id to be queried. */
        CyBool_t *value_p                       /**< Output parameter that will be filled with the GPIO value. */
        );

/** \brief Set the state of GPIO pin output.

    **Description**\n
    This function updates the output state of a GPIO pin, and is valid only for GPIOs
    configured as output. The output value is updated by this function, but the
    driveLowEn and driveHighEn configuration parameters will determine whether the
    pin is driven actively or tri-stated.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS              - if the operation is successful.\n
    * CY_FX3_BOOT_ERROR_NOT_CONFIGURED - if the pin is not selected as a GPIO in the IOMATRIX.

    **\see
    *\see CyFx3BootGpioSetSimpleConfig
    *\see CyFx3BootGpioGetValue
 */
extern CyFx3BootErrorCode_t
CyFx3BootGpioSetValue (
        uint8_t  gpioId,                        /**< GPIO id to be modified. */
        CyBool_t  value                         /**< Value to set on the GPIO pin. */
        );

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYFX3GPIO_H_ */

/*[]*/
