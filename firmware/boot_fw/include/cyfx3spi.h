/*
 ## Cypress USB 3.0 Platform header file (cyfx3spi.h)
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

#ifndef _INCLUDED_CYFX3SPI_H_
#define _INCLUDED_CYFX3SPI_H_

#include <cyu3types.h>
#include <cyfx3error.h>
#include <cyu3externcstart.h>

/** \file cyfx3spi.h
    \brief SPI (Serial Peripheral Interface) is a serial interface defined for
    inter-device communication. The FX3 device includes a SPI master that can
    connect to a variety of SPI slave devices and function at various speeds
    and modes. This file defines the data structures and APIs that enable SPI
    slave access from the FX3 boot API library.
 */

/**************************************************************************
 ******************************* Data Types *******************************
 **************************************************************************/

/** \brief Enumeration defining SSN lead and lag times with respect to SCK.

    **Description**\n
    The Slave Select (SSN) signal needs to lead the SCK at the beginning of a
    SPI transaction and lag the SCK at the end of the transfer. When the default
    SSN signal on the FX3 is used, the hardware provides various modes of
    automatically controlling the SSN assertion. This enumerated type lists the
    various SSN timings that are supported by the FX3 hardware.

    **\see
    *\see CyFx3BootSpiConfig_t
    *\see CyFx3BootSpiSetConfig
 */
typedef enum CyFx3BootSpiSsnLagLead_t
{
    CY_FX3_BOOT_SPI_SSN_LAG_LEAD_ZERO_CLK = 0,  /**< SSN will be asserted in sync with SCK. */
    CY_FX3_BOOT_SPI_SSN_LAG_LEAD_HALF_CLK,      /**< SSN leads / lags SCK by a half clock cycle. */
    CY_FX3_BOOT_SPI_SSN_LAG_LEAD_ONE_CLK,       /**< SSN leads / lags SCK by one clock cycle. */
    CY_FX3_BOOT_SPI_SSN_LAG_LEAD_ONE_HALF_CLK,  /**< SSN leads / lags SCK by one and half clock cycles. */
    CY_FX3_BOOT_SPI_NUM_SSN_LAG_LEAD            /**< Number of enumerations. Should not be used. */
} CyFx3BootSpiSsnLagLead_t;

/** \brief List of ways in which the SPI Slave Select (SSN) line can be controlled.

    **Description**\n
    The SPI Slave Select (SSN) signal is a per-slave signal; and multiple SSN
    signals may be needed in the case where FX3 needs to talk to multiple SPI
    slaves. To satisfy this need, the FX3 device supports multiple ways in which
    the SSN signal can be controlled during a SPI data transfer.

    The default SSN signal on FX3 can be automatically controlled by the FX3
    hardware in sync with the SPI clock (SCK). Another possibility is to have
    the SSN signal controlled as a GPIO by the SPI transfer APIs. This functionality
    is also supported only for the default SSN signal.

    A third option is for the firmware application to directly assert/de-assert
    the SSN signal through the GPIO APIs. This mode of operation can be used with
    any FX3 GPIO.

    **\see
    *\see CyFx3BootSpiConfig_t
    *\see CyFx3BootSpiSetConfig
 */
typedef enum CyFx3BootSpiSsnCtrl_t
{
    CY_FX3_BOOT_SPI_SSN_CTRL_FW = 0,            /**< SSN is controlled by the API and is not synchronized to clock
                                                     boundaries. It is asserted at the beginning of a transfer, and
                                                     is de-asserted at the end of transfer. */
    CY_FX3_BOOT_SPI_SSN_CTRL_HW_END_OF_XFER,    /**< SSN is controlled by hardware and is toggled in sync with clock.
                                                     The SSN is asserted at the beginning of a transfer, and is
                                                     de-asserted at the end of a transfer or when no data is available
                                                     to transmit (underrun). */
    CY_FX3_BOOT_SPI_SSN_CTRL_HW_EACH_WORD,      /**< SSN is controlled by hardware and is toggled in sync with clock.
                                                     The SSN is asserted at the beginning of transfer of every word,
                                                     and de-asserted at the end of the transfer of that word. */
    CY_FX3_BOOT_SPI_SSN_CTRL_HW_CPHA_BASED,     /**< If CPHA is 0, the SSN control is per word; and if CPHA is 1,
                                                     then the SSN control is per transfer. */
    CY_FX3_BOOT_SPI_SSN_CTRL_NONE,              /**< SSN control is done externally. The SSN lines are selected by
                                                     the application and are ignored by the hardware / API. */
    CY_FX3_BOOT_SPI_NUM_SSN_CTRL                /**< Number of enumerations. Should not be used. */
} CyFx3BootSpiSsnCtrl_t;

/** \brief Structure defining the configuration of SPI interface.

    **Description**\n
    This structure encapsulates all of the configurable parameters that can be
    selected for the SPI interface. The CyFx3BootSpiSetConfig() function accepts a
    pointer to this structure, and updates all of the interface parameters.

    **\see
    *\see CyFx3BootSpiSetConfig
    *\see CyFx3BootSpiSsnCtrl_t
    *\see CyFx3BootSpiSsnLagLead_t
 */
typedef struct CyFx3BootSpiConfig_t
{
    CyBool_t                 isLsbFirst;        /**< Data shift mode - CyFalse: MSB first; CyTrue: LSB first */
    CyBool_t                 cpol;              /**< Clock polarity - CyFalse: SCK idles low; CyTrue: SCK idles high */
    CyBool_t                 cpha;              /**< Clock phase - CyFalse: Slave samples at idle-active edge;
                                                     CyTrue: Slave samples at active-idle edge */
    CyBool_t                 ssnPol;            /**< Polarity of SSN line. CyFalse: SSN is active low;
                                                     CyTrue: SSN is active high. */
    CyFx3BootSpiSsnCtrl_t    ssnCtrl;           /**< Mode of SSN control. */
    CyFx3BootSpiSsnLagLead_t leadTime;          /**< Time between SSN assertion and first SCLK edge. This is at the
                                                     beginning of a transfer and is valid only for hardware
                                                     controlled SSN. Zero lead time is not supported. */
    CyFx3BootSpiSsnLagLead_t lagTime;           /**< Time between the last SCK edge to SSN de-assertion. This is at
                                                     the end of a transfer and is valid only for hardware
                                                     controlled SSN. */
    uint32_t                 clock;             /**< SPI interface clock frequency in Hz. */
    uint8_t                  wordLen;           /**< Word length in bits. Valid values are 4 - 32. */
} CyFx3BootSpiConfig_t;

/**************************************************************************
 *************************** FUNCTION PROTOTYPE ***************************
 **************************************************************************/

/** \brief Starts the SPI interface block on the device.

    **Description**\n
    This function powers up the SPI interface block on the device and is expected
    to be the first SPI API function that is called by the application.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS - if the SPI block was successfully initialized.\n

    **\see
    *\see CyFx3BootSpiDeInit
    *\see CyFx3BootSpiSetConfig
 */
extern CyFx3BootErrorCode_t
CyFx3BootSpiInit (
        void);

/** \brief Stops the SPI block.

    **Description**\n
    This function disables and powers off the SPI interface. This function can
    be used to shut off the interface to save power when it is not in use.

    **Return value**\n
    * None

    **\see
    *\see CyFx3BootSpiInit
 */
extern void
CyFx3BootSpiDeInit (
        void);

/** \brief Configures SPI interface parameters.

    **Description**\n
    This function is used to configure the SPI master interface based on the
    desired settings to talk to the desired slave. This function can be called
    repeatedly to change the settings if different settings are to be used to
    communicate with different slave devices.
 
    This API resets the TX/RX FIFOs associated with the SPI block, and will
    result in the loss of any data that is in the FIFOs.
 
    **Return value**\n
    * CY_FX3_BOOT_SUCCESS            - when the SetConfig is successful.\n
    * CY_FX3_BOOT_ERROR_NOT_STARTED  - if the SPI block has not been initialized.\n
    * CY_FX3_BOOT_ERROR_NULL_POINTER - when the config pointer is NULL.\n
    * CY_FX3_BOOT_ERROR_TIMEOUT      - when the operation times out.

    **\see
    *\see CyFx3BootSpiConfig_t
 */
extern CyFx3BootErrorCode_t
CyFx3BootSpiSetConfig (
        CyFx3BootSpiConfig_t *config    /**< Pointer to the SPI config structure */
        );

/** \brief Assert / Deassert the SSN Line.

    **Description**\n
    Asserts/de-asserts the SSN Line for the default slave device. This is possible
    only if the SSN line control is configured for FW controlled mode.

    **Return value**\n
    * None

    **\see
    *\see CyFx3BootSpiSsnCtrl_t
 */
extern void
CyFx3BootSpiSetSsnLine (
        CyBool_t isHigh         /**< CyFalse: Pull down the SSN line, CyTrue: Pull up the SSN line. */
        );

/** \brief Transmits data word by word over the SPI interface

    **Description**\n
    This function is used to transmit data word by word. The function can be called
    only if there is no active DMA transfer on the bus. If CyFx3BootSpiSetBlockXfer has
    been called, the CyFx3BootSpiDisableBlockXfer API needs to be called before this
    API can be used.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS            - when the TransmitWords is successful.\n
    * CY_FX3_BOOT_ERROR_NULL_POINTER - when the data pointer is NULL.\n
    * CY_FX3_BOOT_ERROR_XFER_FAILURE - when the SPI block encounters an error during the transfer.\n
    * CY_FX3_BOOT_ERROR_TIMEOUT      - when the TransmitWords times out.

    **\see
    *\see CyFx3BootSpiReceiveWords
 */
extern CyFx3BootErrorCode_t
CyFx3BootSpiTransmitWords (
        uint8_t *data,          /**< Source data pointer. This needs to be padded to nearest
                                     byte if the word length is not byte aligned. */
        uint32_t byteCount      /**< This needs to be a multiple of the word length aligned to the next
                                     byte value. */
        );

/** \brief Receives data word by word over the SPI interface.

    **Description**\n
    Receives data from the SPI slave in word-by-word register mode of transfer.
    This can be used only when a DMA transfer on the SPI block has not been started
    using the CyFx3BootSpiSetBlockXfer API.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS            - when the ReceiveWords is successful.\n
    * CY_FX3_BOOT_ERROR_NULL_POINTER - when the data pointer is NULL.\n
    * CY_FX3_BOOT_ERROR_XFER_FAILURE - when the SPI block encounters an error during the transfer.\n
    * CY_FX3_BOOT_ERROR_TIMEOUT      - when the ReceiveWords times out.

    **\see
    *\see CyFx3BootSpiTransmitWords
 */
extern CyFx3BootErrorCode_t
CyFx3BootSpiReceiveWords (
        uint8_t *data,          /**< Destination buffer pointer. */
        uint32_t byteCount      /**< Amount of data to be received (in bytes). This needs to be a multiple
                                     of the word length after alignment to the next byte value. */
        );

/** \brief Enables DMA data transfers through the SPI interface.

    **Description**\n
    This function switches the SPI block to DMA mode, and initiates the desired
    read/write transfers.

    If the txSize parameter is non-zero, then TX is enabled; and if rxSize
    parameter is non-zero, then RX is enabled. If both TX and RX are enabled,
    transfers can only happen while both the SPI producer and SPI consumer
    sockets are ready for data transfer.

    If the receive count is less than the transmit count, the data transmit
    continues after the scheduled reception is complete. However, if the
    transmit count is lesser, data reception is stalled at the end of the
    transmit operation. The SPI transmit transfer needs to disabled before the
    receive can proceed.

    The CyFx3BootSpiDmaXferData API needs to be used to set up the DMA data
    path to do the actual data transfer. A call to SetBlockXfer has to be
    followed by a call to DisableBlockXfer, before the SPI block can be used
    in Register mode.

    **Return value**\n
    * None

    **\see
    *\see CyFx3BootSpiDisableBlockXfer
    *\see CyFx3BootSpiDmaXferData
 */
extern void
CyFx3BootSpiSetBlockXfer (
        uint32_t txSize,        /**< Number of words to be transmitted. */
        uint32_t rxSize         /**< Number of words to be received. */
        );

/** \brief Disable DMA data transfers through the SPI interface.

    **Description**\n
    This function disables DMA data transfers through the SPI interface, so that
    register mode transfers can be performed again.

    **Return value**\n
    * None

    **\see
    *\see CyFx3BootSpiSetBlockXfer
 */
extern void
CyFx3BootSpiDisableBlockXfer (
        void );

/** \brief This function is used to setup a DMA transfer from CPU to SPI or vice versa.

    **Description**\n
    This function is used to read/write data from/to a SPI slave in DMA mode. The
    contents of a single data buffer can be transferred through a one-shot DMA pipe
    that is configured by this API call.

    **Note**\n
    The CyFx3BootSpiSetBlockXfer API needs to be called to configure the SPI block
    for DMA bound data transfers.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS                - if the data transfer is successful.\n
    * CY_FX3_BOOT_ERROR_INVALID_DMA_ADDR - if the address specified is not in the SYSMEM area.\n
    * CY_FX3_BOOT_ERROR_XFER_FAILURE     - if the data transfer encountered any error.\n
    * CY_FX3_BOOT_ERROR_TIMEOUT          - if the data transfer times out.

    **\see
    *\see CyFx3BootSpiDisableBlockXfer
    *\see CyFx3BootSpiSetBlockXfer
 */
extern CyFx3BootErrorCode_t
CyFx3BootSpiDmaXferData (
        CyBool_t isRead,        /**< Transfer direction - isRead=CyTrue for read operations;
                                     and isRead=CyFalse for write operations. */
        uint32_t address,       /**< Address of the buffer from/to which data is to be transferred. */
        uint32_t length,        /**< Length of the data to be transferred. */
        uint32_t timeout        /**< Timeout duration in multiples of 10 us. Can be set to CY_FX3_BOOT_WAIT_FOREVER
                                     to compulsorily wait for end of transfer. */
        );


#include <cyu3externcend.h>

#endif /* _INCLUDED_CYFX3SPI_H_ */

/*[]*/
