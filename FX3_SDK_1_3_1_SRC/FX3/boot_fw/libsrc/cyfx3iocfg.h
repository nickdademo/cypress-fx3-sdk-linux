/*
 ## Cypress USB 3.0 Platform header file (cyfx3iocfg.h)
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

#ifndef __CYFX3IOCFG_H__
#define __CYFX3IOCFG_H__

#include <cyu3externcstart.h>
/**************************************************************************
 ******************************* Macros ***********************************
 **************************************************************************/

/* GPIOs 32:0 are used for the 16 DQ lines, the necessary control signals on
   the GPIF side and the PMODE pins. The DQ[15:0], CTL[3:0] and PMODE pins
   are reserved for their default usage. CTL[12:4] can be overridden as
   GPIO pins.  Bits 63:61 do not map to valid pins.
 */
#define CY_FX3_BOOT_GPIO_AVAIL0_DEFAULT_MASK         (0x3FE00000)
#define CY_FX3_BOOT_GPIO_AVAIL1_DEFAULT_MASK         (0x1FFFFFFE)

/* Pins 44:33 form the S0 port.
   MMC mode - Pins 44:33.
   SD  mode - Pins 36:33 and Pins 44:41.
   DQ  mode - Pins 44:33.
   */
#define CY_FX3_BOOT_GPIO_AVAIL1_S0_MMC_MASK          (0xFFFFE001)
#define CY_FX3_BOOT_GPIO_AVAIL1_S0_SD_MASK           (0xFFFFE1E1)
#define CY_FX3_BOOT_GPIO_AVAIL1_S0_DQ_MASK           (0xFFFFE001)

/* Pins 56:46 form the S1 port.
   MMC mode - Pins 56:46.
   SD  mode - Pins 52:46.
   UART     - Pins 56:53.
   I2S      - Pins 57:54.
   SPI      - Pins 56:53.
 */
#define CY_FX3_BOOT_GPIO_AVAIL1_S1_MMC_MASK          (0xFE003FFF)
#define CY_FX3_BOOT_GPIO_AVAIL1_S1_SD_MASK           (0xFFE03FFF)
#define CY_FX3_BOOT_GPIO_AVAIL1_S1_UART_MASK         (0xFE1FFFFF)
#define CY_FX3_BOOT_GPIO_AVAIL1_S1_I2S_MASK          (0xFC3FFFFF)
#define CY_FX3_BOOT_GPIO_AVAIL1_S1_SPI_MASK          (0xFE1FFFFF)

/* In DQ mode, the S1 port cannot have storage.
   Instead it can have DQ, UART and I2S.
   DQ mode - Pins 49:46.
   UART    - Pins 56:53.
   I2S     - Pins 52:50 and Pin 57.
 */
#define CY_FX3_BOOT_GPIO_AVAIL1_S1_DQ_MASK           (0xFFFC3FFF)
#define CY_FX3_BOOT_GPIO_AVAIL1_DQ_UART_MASK         (0xFE1FFFFF)
#define CY_FX3_BOOT_GPIO_AVAIL1_DQ_I2S_MASK          (0xFDE3FFFF)

/* Pins 57:46 form the LPP port. This is muxed with S1 port.
   UART - Pins 49:46.
   I2S  - Pins 52:50 and Pin 57.
   SPI  - Pins 56:53.
 */
#define CY_FX3_BOOT_GPIO_AVAIL1_LPP_UART_MASK        (0xFFFC3FFF)
#define CY_FX3_BOOT_GPIO_AVAIL1_LPP_I2S_MASK         (0xFDE3FFFF)
#define CY_FX3_BOOT_GPIO_AVAIL1_LPP_SPI_MASK         (0xFE1FFFFF)

/* Pins 59:58 form the I2C lines. */
#define CY_FX3_BOOT_GPIO_AVAIL1_I2C_MASK             (0xF3FFFFFF)

/* Number of GPIO pins in the device. */
#define CY_FX3_BOOT_GPIO_MAX             		(61)

/* Summary
   IO configuration for the S port pins.

   Description
   The two S ports on the USB 3.0 device can be configured in different
   ways.
   This type lists the different configurations that are possible for
   these ports.

   If the corresponding pins are being used to implement one or
   more of the serial
   protocols for communication with peripherals, they should be
   marked as GPIO.
 */
typedef enum CyFx3BootStorIfMode_t
{
    CY_FX3_BOOT_STORMODE_MMC = 0,    /* Configured as 8-bit wide
                                   MMC interface. */
    CY_FX3_BOOT_STORMODE_SD,         /* Configured as 4-bit
                                   wide SD interface. */
    CY_FX3_BOOT_STORMODE_DQ,         /* Configured as
                                   part of the 32-bit wide DQ bus on P-port. */
    CY_FX3_BOOT_STORMODE_GPIO        /* Configured as
                                   GPIO or GPIF data interface. */
} CyFx3BootStorIfMode_t;


#include <cyu3externcend.h>
#endif /* __CYFX3IOCFG_H__ */

