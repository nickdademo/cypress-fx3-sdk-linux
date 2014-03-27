/*
 ## Cypress USB 3.0 Platform header file (cyfx3i2c.h)
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

#ifndef _INCLUDED_CYFX3I2C_H_
#define _INCLUDED_CYFX3I2C_H_

#include <cyfx3error.h>
#include <cyu3types.h>
#include <cyu3externcstart.h>

/** \file cyfx3i2c.h
    \brief The I2C interface driver in the FX3 Boot Firmware provides a set of
    APIs that allow the configuration of the I2C interface properties and data
    exchange with one or more I2C slave devices.
 */

/**************************************************************************
 ******************************* Data Types *******************************
 **************************************************************************/

/** \brief Structure defining the configuration of the I2C interface.

    **Description**\n
    This structure encapsulates all of the configurable parameters that can be
    selected for the I2C interface. The CyFx3BootI2cSetConfig() function accepts a
    pointer to this structure, and updates all of the interface parameters.

    The I2C block can function in the bit rate range of 100 KHz to 1MHz. In default
    mode of operation, the timeouts need to be kept disabled.

    **\see
    *\see CyFx3BootI2cSetConfig
 */
typedef struct CyFx3BootI2cConfig_t
{
    uint32_t bitRate;                   /**< Bit rate for the interface (e.g.: 100000 for 100KHz). */
    CyBool_t isDma;                     /**< CyFalse: Register transfer mode, CyTrue: DMA transfer mode */
    uint32_t busTimeout;                /**< Number of core clocks that SCK can be held low for by the slave byte
                                             transmission before triggering a timeout error. 0xFFFFFFFFU means no
                                             timeout. */
    uint16_t dmaTimeout;                /**< Number of core clocks DMA can remain not ready before flagging an error.
                                             0xFFFF means no timeout. */
} CyFx3BootI2cConfig_t;

/** \brief Structure defining the preamble to be sent on the I2C interface.

    **Description**\n
    All I2C data transfers start with a preamble where the first byte contains
    the slave address and the direction of transfer. The preamble can optionally
    contain other bytes where device specific address values or other commands
    are sent to the slave device.

    The FX3 device supports associating a preamble with a maximum length of
    8 bytes to any I2C data transfer. This allows the user to specify a multi-byte
    preamble which covers the slave address, device specific address fields and then
    initiate the data transfer.

    The ctrlMask indicate the start / stop bit conditions after each byte of
    the preamble.

    For example, an I2C EEPROM read requires the byte address for the read to
    be written first. These two I2C operations can be combined into one I2C API
    call using the parameters of the structure.

    Typical I2C EEPROM page Read operation:

    Byte 0:\n
         Bit 7 - 1: Slave address.\n
         Bit 0    : 0 - Indicating this is a write from master.\n
    Byte 1, 2: Address to which the data has to be written.\n
    Byte 3:\n
         Bit 7 - 1: Slave address.\n
         Bit 0    : 1 - Indicating this is a read operation.\n
    The buffer field shall hold the above four bytes, the length field shall be
    four; and ctrlMask field will be 0x0004 as a start bit is required after the
    third byte (third bit is set).

    Typical I2C EEPROM page Write operation:

    Byte 0:\n
         Bit 7 - 1: Slave address.\n
         Bit 0    : 0 - Indicating this is a write from master.\n
    Byte 1, 2: Address to which the data has to be written.\n
    The buffer field shall hold the above three bytes, the length field shall be
    three, and the ctrlMask field is zero as no additional start/stop conditions are
    needed.
 
    **\see
   * CyFx3BootI2cSetConfig
 */
typedef struct CyFx3BootI2cPreamble_t
{
    uint8_t  buffer[8];                 /**< The extended preamble information. */
    uint8_t  length;                    /**< The length of the preamble to be sent. Should be between 1 and 8. */
    uint16_t ctrlMask;                  /**< This field controls the start stop condition after every byte of
                                             preamble data. Bits 0 - 7 represent a bit mask for start condition
                                             and Bits 8 - 15 represent a bit mask for stop condition. If both bits
                                             are set for an index, then the stop condition takes priority. */
} CyFx3BootI2cPreamble_t;

/**************************************************************************
 *************************** Function prototypes **************************
 **************************************************************************/

/** \brief Starts the I2C interface block.

    **Description**\n
    This function powers up the I2C interface block on the FX3 device and is
    expected to be the first I2C API function that is called by the application.
    IO configuration function is expected to have been called prior to this
    call.

    This function also sets up the I2C interface at a default bit rate of
    100KHz.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS             - If the block is initialized successfully.

    **\see
    *\see CyFx3BootI2cSetConfig
    *\see CyFx3BootI2cDeinit
 */
extern CyFx3BootErrorCode_t
CyFx3BootI2cInit (
        void);

/** \brief Stops the I2C module.

    **Description**\n
    This function disables and powers off the I2C interface. This function can
    be used to shut off the interface to save power when it is not in use.

    **Return value**\n
    * None

    **\see
    *\see CyFx3BootI2cInit
 */
extern void
CyFx3BootI2cDeInit(
        void);

/** \brief Sets the I2C interface parameters.

    **Description**\n
    This function is used to configure the I2C master interface based on the
    desired baud rate and address length settings to talk to the desired slave.
    This function should be called repeatedly to change the settings if
    different settings are to be used to communicate with different slave
    devices.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS            - When the SetConfig is successful.\n
    * CY_FX3_BOOT_ERROR_NOT_STARTED  - If the I2C block has not been initialized.\n
    * CY_FX3_BOOT_ERROR_NULL_POINTER - When the config parameter is NULL.\n
    * CY_FX3_BOOT_ERROR_TIMEOUT      - If the driver timed out while trying to clear the RX and TX FIFOs.

    **\see
    *\see CyFx3BootI2cConfig_t
 */
extern CyFx3BootErrorCode_t
CyFx3BootI2cSetConfig (
        CyFx3BootI2cConfig_t *config            /**< I2C configuration settings. */
        );

/** \brief Perform a read or write operation to the I2C slave.

    **Description**\n
    This function is used to send the extended preamble over the I2C bus, and is
    used to initiate a transfer when the I2C block is configured for DMA mode of
    transfer.

    This function is used internally by the CyFx3BootI2cReceiveBytes and
    CyFx3BootI2cTransmitBytes APIs, and should not be called directly when the
    register mode of transfer is selected.
 
    **Return value**\n
    * CY_FX3_BOOT_SUCCESS            - When the SendCommand is successful.\n
    * CY_FX3_BOOT_ERROR_NULL_POINTER - When the preamble is NULL.\n
    * CY_FX3_BOOT_ERROR_TIMEOUT      - When the transfer times out.

    **\see
    *\see CyFx3BootI2cReceiveBytes
    *\see CyFx3BootI2cTransmitBytes
 */
extern CyFx3BootErrorCode_t
CyFx3BootI2cSendCommand (
        CyFx3BootI2cPreamble_t *preamble,       /**< Preamble information to be sent out. */
        uint32_t byteCount,                     /**< Size of the transfer in bytes. */
        CyBool_t isRead                         /**< Direction of transfer; CyTrue: Read, CyFalse: Write. */
        );

/** \brief Writes a small number of bytes to an I2C slave.

    **Description**\n
    This function is used to write data one byte at a time to an I2C slave.
    This function requires that the I2C interface be configured in register
    (non-DMA) mode.
  
    If a non-zero retryCount is specified, the API will retry a failing
    transfer until it succeeds or the specified count has been reached.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS            - if the TransmitBytes is successful.\n
    * CY_FX3_BOOT_ERROR_NULL_POINTER - if the preamble or data is NULL.\n
    * CY_FX3_BOOT_ERROR_XFER_FAILURE - if the transfer fails due to a bus error.\n
    * CY_FX3_BOOT_ERROR_TIMEOUT      - if the transfer times out.

    **\see
    *\see CyFx3BootI2cSetConfig
    *\see CyFx3BootI2cReceiveBytes
 */
extern CyFx3BootErrorCode_t
CyFx3BootI2cTransmitBytes (
        CyFx3BootI2cPreamble_t *preamble,       /**< Preamble to be sent out before the data transfer. */
        uint8_t *data,                          /**< Pointer to buffer containing data to be written. */
        uint32_t byteCount,                     /**< Size of the transfer in bytes. */
        uint32_t retryCount                     /**< Number of times to retry request if a byte is NAKed
                                                     by the slave. */
        );

/** \brief Reads a small number of bytes from an I2C slave.

    **Description**\n
    This function reads a few bytes one at a time from the I2C slave selected
    through the preamble parameter. The I2C interface should be configured in
    register (non-DMA) mode to make use of this function.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS            - if the TransmitBytes is successful.\n
    * CY_FX3_BOOT_ERROR_NULL_POINTER - if the preamble or data is NULL.\n
    * CY_FX3_BOOT_ERROR_XFER_FAILURE - if the transfer fails due to a bus error.\n
    * CY_FX3_BOOT_ERROR_TIMEOUT      - if the transfer times out.

    **\see
    *\see CyFx3BootI2cSetConfig
    *\see CyFx3BootI2cTransmitBytes
 */
extern CyFx3BootErrorCode_t
CyFx3BootI2cReceiveBytes (
        CyFx3BootI2cPreamble_t *preamble,       /**< Preamble to be sent out before the data transfer. */
        uint8_t *data,                          /**< Pointer to buffer where the data is to be placed. */
        uint32_t byteCount,                     /**< Size of the transfer in bytes. */
        uint32_t retryCount                     /**< Number of times to retry request if preamble is NAKed
                                                     or an error is encountered. */
        );

/** \brief Poll an I2C slave until all of the preamble is ACKed.

    **Description**\n
    This function waits for a ACK handshake from the slave, and can be used
    to ensure that the slave device has reached a desired state before issuing
    the next transaction or shutting the interface down. This function call
    returns when the specified handshake has been received or when the wait has
    timed out. The function call can be repeated on CY_FX3_BOOT_ERROR_TIMEOUT
    without any error recovery.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS            - if the device ACKs the preamble.\n
    * CY_FX3_BOOT_ERROR_NULL_POINTER - if the preamble is NULL.\n
    * CY_FX3_BOOT_ERROR_TIMEOUT      - if a timeout occurs.
 */
extern CyFx3BootErrorCode_t
CyFx3BootI2cWaitForAck (
        CyFx3BootI2cPreamble_t *preamble,       /**< Preamble to be sent out for polling the slave. */
        uint32_t retryCount                     /**< Number of times to retry request if preamble is NAKed
                                                     or an error is encountered. */
        );

/** \brief This function is used to setup a dma from CPU to I2C or vice versa.

    **Description**\n
    This function is used to read/write data from/to an I2C slave when the I2C
    interface is configured in DMA mode. The CyFx3BootI2cSendCommand API should
    be used to address the slave and initiate the data transfer on the slave side.

    This function is a blocking call which waits until the desired transfer is
    complete or the specified timeout duration has elapsed.

    It is expected that the length of data being transferred is a multiple of
    16 bytes.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS                - if the data transfer is successful.\n
    * CY_FX3_BOOT_ERROR_INVALID_DMA_ADDR - if the address specified is not in the SYSMEM area.\n
    * CY_FX3_BOOT_ERROR_XFER_FAILURE     - if the data transfer encountered any error.\n
    * CY_FX3_BOOT_ERROR_TIMEOUT          - if the data transfer times out.

    **\see
    *\see CyFx3BootI2cSetConfig
    *\see CyFx3BootI2cSendCommand
 */
extern
CyFx3BootErrorCode_t
CyFx3BootI2cDmaXferData (
        CyBool_t isRead,        /**< isRead=CyTrue for read transfers, and isRead=CyFalse for write transfers. */
        uint32_t address,       /**< Address of the buffer from/to which data is to be transferred */
        uint32_t length,        /**< Length of the data to be transferred. Should be a multiple of
                                     16 bytes and is limited by the amount of data that the slave can
                                     transfer without delays. */
        uint32_t timeout        /**< Timeout duration in multiples of 10 us. Can be set to CY_FX3_BOOT_WAIT_FOREVER
                                     to wait until the transfer is complete. */
        );

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYFX3I2C_H_ */

/*[]*/
