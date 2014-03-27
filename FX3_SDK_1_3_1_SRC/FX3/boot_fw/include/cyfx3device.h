/*
 ## Cypress USB 3.0 Platform header file (cyfx3device.h)
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

#ifndef __CYFX3DEVICE_H__
#define __CYFX3DEVICE_H__

#include <cyu3types.h>
#include <cyfx3error.h>
#include <cyfx3_api.h>

#include <cyu3externcstart.h>

/** \file cyfx3device.h
    \brief The Boot APIs for FX3 provide a low footprint API library that can be used
    to put together simple FX3 applications, primarily for the purpose of building
    custom boot-loaders.

    **Description**\n
    The regular FX3 API library is designed for maximum flexibility and performance, and
    makes use of an embedded OS. This results in a higher memory footprint for simple
    applications. This is not always desirable, particularly in cases where a small
    boot-loader type application is required to improve the flexibility and robustness
    of the boot process.

    The FX3 boot API library is provided to address this problem, and provides a low
    footprint set of APIs without any RTOS dependency. This library only supports a
    small set of features, all of which are targeted at building booting applications.

    The software boot supports the following interfaces on the FX3 device:\n
    1. USB device mode\n
    2. SPI\n
    3. I2C\n
    4. UART\n
    5. GPIO\n

    This library does not make use of an RTOS and minimal use of interrupts. All of the
    serial peripheral APIs operate on a polling model, and it is expected that these
    will be called in the appropriate sequence from the firmware main.

    The only interrupt that is enabled is for the USB block. This interrupt handler
    ensures that all of the USB 2.0 and USB 3.0 link and protocol requests are handled
    appropriately, and provides hooks in the form of callback functions.

    \section Fx3Memories FX3 Memory Regions
    \brief ITCM and SYSMEM memory address ranges on the FX3 device.

    **Description**\n
    The I-TCM is a 16 KB memory region which is tightly coupled to the ARM9 CPU
    and can be used to locate interrupt service routines. The first 256 bytes of
    the I-TCM are reserved for setting up the ARM exception vectors.

    The SYSMEM region is the main code/data RAM on the FX3 device, and can be
    512 KB or 256 KB in size depending on the part being used. The first 8 KB of
    the SYSMEM is reserved for holding DMA related data structures (descriptors).
*/

/** \def CY_FX3_BOOT_ITCM_BASE
    \brief Base address of the I-TCM region.
 */
#define CY_FX3_BOOT_ITCM_BASE           (0x00000000)

/** \def CY_FX3_BOOT_ITCM_END
    \brief End address of the I-TCM region.
 */
#define CY_FX3_BOOT_ITCM_END            (CY_FX3_BOOT_ITCM_BASE + 0x4000)

/** \def CY_FX3_BOOT_SYSMEM_BASE
    \brief Base address of the SYSMEM region.
 */
#define CY_FX3_BOOT_SYSMEM_BASE         (0x40000000)

/** \def CY_FX3_BOOT_SYSMEM_END
    \brief End address of the SYSMEM region. This value is valid for the CYUSB3014 part. Some other
    FX3 parts could have smaller SYSMEM regions.
 */
#define CY_FX3_BOOT_SYSMEM_END          (CY_FX3_BOOT_SYSMEM_BASE + 0x80000)

/** \def CY_FX3_BOOT_SYSMEM_BASE1
    \brief Start address of the usable range of SYSMEM.
 */
#define CY_FX3_BOOT_SYSMEM_BASE1        (CY_FX3_BOOT_SYSMEM_BASE + 0x2000)

/** \def CY_FX3_BOOT_NO_WAIT
    \brief Option that specifies that a polling API should return immediately after checking
    the status of the transfer.
 */
#define CY_FX3_BOOT_NO_WAIT                     (0x00)

/** \def CY_FX3_BOOT_WAIT_FOREVER
    \brief Option that specifies that a polling API should wait as long as required for a
    transfer to complete.
 */
#define CY_FX3_BOOT_WAIT_FOREVER                (0xFFFFFFFF)

/** \brief Clock source for a peripheral block.

    **Description**\n
    The clocks for various hardware blocks on the FX3 device are derived using
    frequency dividers from the master system clock. The source clock for these
    frequency dividers can be the master system clock itself or some other clocks
    derived from it.

    This type lists the various clocks that can be used as the source clock for
    deriving the peripheral clocks.
 */
typedef enum CyFx3BootSysClockSrc_t
{
    CY_FX3_BOOT_SYS_CLK_BY_16 = 0,      /**< SYS_CLK divided by 16. */
    CY_FX3_BOOT_SYS_CLK_BY_4,           /**< SYS_CLK divided by 4. */
    CY_FX3_BOOT_SYS_CLK_BY_2,           /**< SYS_CLK divided by 2. */
    CY_FX3_BOOT_SYS_CLK,                /**< SYS_CLK itself. */
    CY_FX3_BOOT_NUM_CLK_SRC             /**< Number of clock source enumerations. */
} CyFx3BootSysClockSrc_t;

/** \brief Enumeration of EZ-USB FX3 part numbers.

    **Description**\n
    There are multiple EZ-USB FX3 parts which support varying feature sets. Please refer
    to the device data sheets or the Cypress device catalogue for information on the
    features supported by each FX3 part.

    This enumerated type lists the various valid part numbers in the EZ-USB FX3 family.

    **\see
    *\see CyFx3BootGetPartNumber
 */
typedef CyU3PPartNumber_t CyFx3PartNumber_t;

/** \brief Defines the IO matrix configuration parameters.

    **Description**\n
    Most of the IOs on the FX3 device are multi-purpose; and the specific function that
    each pin should serve need to be selected during device initialization. This structures
    defines all of the parameters that are required to configure the FX3 IOs.

    Please note that the DQ[15:0] pins, CTL[3:0] pins and the PMODE[2:0] pins
    are reserved and cannot be enabled as GPIOs through this structure. The
    GPIO Override APIs need to be used for this purpose.

    **\see
    *\see CyFx3BootDeviceConfigureIOMatrix
    *\see CyFx3BootGpioOverride
 */
typedef struct CyFx3BootIoMatrixConfig_t
{
    CyBool_t isDQ32Bit;         /**< CyTrue: The GPIF bus width is 32 bit; CyFalse: The GPIF bus width is 16 bit */
    CyBool_t useUart;           /**< CyTrue: The UART interface is to be used; CyFalse: The UART interface is not
                                     to be used */
    CyBool_t useI2C;            /**< CyTrue: The I2C interface is to be used; CyFalse: The I2C interface is not to
                                     be used */
    CyBool_t useI2S;            /**< CyTrue: The I2S interface is to be used; CyFalse: The I2S interface is not to
                                     be used. */
    CyBool_t useSpi;            /**< CyTrue: The SPI interface is to be used; CyFalse: The SPI interface is not to
                                     be used. */
    uint32_t gpioSimpleEn[2];   /**< Bitmap variable that identifies pins that should be configured as simple GPIOs. */
} CyFx3BootIoMatrixConfig_t;

/** \brief This function initializes the FX3 device.

    **Description**\n
    The function is expected to be invoked as the first call from the main ()
    function. This function should be called only once.

    The setFastSysClk parameter is equivalent to the setSysClk400 parameter used
    in the main FX3 API library; and specifies that the SYSCLK frequency should
    be set to a value greater than 400 MHz.
*/
extern void
CyFx3BootDeviceInit (
        CyBool_t setFastSysClk          /**< Indicates whether the FX3 system clock should be set to faster than
                                             400 MHz or not. Should be set to CyTrue if the GPIF will be used in
                                             Synchronous 32-bit mode at 100 MHz. */
        );

/** \brief Transfer control to the specified address.

    **Description**\n
    This function is used to transfer the control to the next stage's program entry.
    All the Serial IOs except GPIO (I2C, SPI, UART) that have been initialized must be
    de-initialized prior to calling this function.
*/
extern void
CyFx3BootJumpToProgramEntry (
        uint32_t address                /**< The program entry address */
        );

/** \brief Configure the IO matrix for the device

    **Description**\n
    This function configures the functionality for each of the FX3 IO pins.
    If the GPIF data bus is 32-bits wide, only one from among the SPI, UART
    and I2S peripherals can be used. If the GPIF data bus is not 32-bits wide;
    all of the SPI, UART and I2S interfaces are available for simultaneous use.
    The specific pins mapped to these serial interfaces can also be changed.

    This API completes the IO configuration based on the user specified parameters.
    Any pin that is not used as part of the GPIF or serial peripheral interfaces
    can be used as a GPIO.

    The IO configuration is not expected to be changed dynamically, and it is
    recommended that this be setup as soon as the CyFx3BootDeviceInit API has
    been called.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS - When the IO configuration is successful.\n
    * CY_FX3_BOOT_ERROR_NOT_SUPPORTED - the FX3 part in use does not support the desired configuration.\n
    * CY_FX3_BOOT_ERROR_BAD_ARGUMENT - If some configuration value is invalid.

    **\see
    *\see CyFx3BootIoMatrixConfig_t
 */
extern CyFx3BootErrorCode_t
CyFx3BootDeviceConfigureIOMatrix (
        CyFx3BootIoMatrixConfig_t *cfg_p        /**< Pointer to configuration structure. */
        );

/** \brief Enable/disable the watchdog timer.

    **Description**\n
    The FX3 device implements a watchdog timer that can be used to reset the device
    when the CPU is not responsive. This function is used to enable the watchdog feature
    and to set the period for the timer.

    **Return value**\n
    * None

    **\see
    *\see CyFx3BootWatchdogClear
 */
extern void
CyFx3BootWatchdogConfigure (
        CyBool_t enable,                /**< Whether the watchdog timer is to be enabled or disabled. */
        uint32_t period                 /**< Period for the timer in milliseconds. Used only for enable calls. */
        );

/** \brief Clear the watchdog timer to prevent device reset.

    **Description**\n
    This function is used to clear the watchdog timer so as to prevent the timer from
    resetting the device. This function needs to be called more often than the period of
    the watchdog timer.

    **Return value**\n
    * None

    **\see
    *\see CyFx3BootWatchdogConfigure
 */
extern void
CyFx3BootWatchdogClear (
        void);

/** \brief Get the part number of the FX3 device in use.

    **Description**\n
    The EZ-USB FX3 family has multiple parts which support various sets of features.
    This function can be used to query the part number of the current device so as to
    check whether specific functionality is supported or not.

    **Return value**\n
    * Part number of the FX3 device in use.

    **\see
    *\see CyFx3PartNumber_t
 */
extern CyFx3PartNumber_t
CyFx3BootGetPartNumber (
        void);

/** \brief Request to keep the GPIO block powered ON across control transfer to the full firmware.

    **Description**\n
    All serial peripheral blocks on the FX3 device are normally reset when control of execution
    is transferred to the full firmware. This API is used to specify that the GPIO block should
    be left ON while jumping to the full firmware.

    **Return value**\n
    * None
 */
extern void
CyFx3BootRetainGpioState (
        void);

/** \brief Override a specific FX3 pin as a GPIO.

    **Description**\n
    Some of the FX3 device pins are reserved for standard functions and not allowed to be
    configured as GPIOs at the time of configuring the IO Matrix. It is possible that the
    customer design does not make use of the standard functionality of these pins, and needs
    to use them as GPIOs. This function is used to override a specified IO pin as a GPIO.

    Please note that this API does not check whether the pin being overridden is currently
    in use by any of the other interfaces. Therefore, this API should be used with caution.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS if the pin override is successful.\n
    * CY_FX3_BOOT_ERROR_BAD_ARGUMENT if the pin specified is not valid.

    **\see\n
    *\see CyFx3BootDeviceConfigureIOMatrix
    *\see CyFx3BootGpioRestore
 */
extern CyFx3BootErrorCode_t
CyFx3BootGpioOverride (
        uint8_t pinNumber               /**< Pin number to be over-ridden as a simple GPIO. */
        );

/** \brief Restore the standard function of a pin.

    **Description**\n
    This function restores the standard function of a pin that was previously over-ridden
    as a simple GPIO.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS if the pin restore is successful.\n
    * CY_FX3_BOOT_ERROR_BAD_ARGUMENT if the pin specified is not valid.

    **\see\n
    *\see CyFx3BootGpioOverride
 */
extern CyFx3BootErrorCode_t
CyFx3BootGpioRestore (
        uint8_t pinNumber               /**< Pin number to be restored to default function. */
        );

#include <cyu3externcend.h>

#endif /* __CYFX3DEVICE_H__ */

