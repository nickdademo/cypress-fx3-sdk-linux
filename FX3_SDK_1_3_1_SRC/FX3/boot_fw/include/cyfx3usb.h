/*
 ## Cypress USB 3.0 Platform header file (cyfx3usb.h)
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

#ifndef __CYFX3USB_H__
#define __CYFX3USB_H__

#include <cyu3types.h>
#include <cyfx3error.h>
#include <cyu3externcstart.h>

/** \file cyfx3usb.h
    \brief The FX3 boot library implements a USB driver that supports the device
    mode of USB operation (no host or OTG support). This file defines the data
    structures and APIs provided by the USB driver to control the USB operation.
 */

/** \brief List of supported USB connection speeds.
 */
typedef enum CyFx3BootUsbSpeed_t
{
    CY_FX3_BOOT_NOT_CONNECTED = 0x00,           /**< USB device not connected. */
    CY_FX3_BOOT_FULL_SPEED,                     /**< USB full speed. */
    CY_FX3_BOOT_HIGH_SPEED,                     /**< High speed. */
    CY_FX3_BOOT_SUPER_SPEED                     /**< Super speed. */
} CyFx3BootUsbSpeed_t;

/** \brief Enumeration of USB event types.
 */
typedef enum CyFx3BootUsbEventType_t
{
    CY_FX3_BOOT_USB_CONNECT = 0x00,             /**< USB Connect Event */
    CY_FX3_BOOT_USB_DISCONNECT,                 /**< USB DisConnect Event */
    CY_FX3_BOOT_USB_RESET,                      /**< USB Reset Event */
    CY_FX3_BOOT_USB_SUSPEND,                    /**< USB Suspend Event */
    CY_FX3_BOOT_USB_RESUME,                     /**< USB Resume Event */
    CY_FX3_BOOT_USB_IN_SS_DISCONNECT,           /**< Event to check if the device is in SS Disconnect State */
    CY_FX3_BOOT_USB_COMPLIANCE                  /**< USB Compliance Event */
} CyFx3BootUsbEventType_t;

/** \brief Enumeration of the endpoint types.
 */
typedef enum CyFx3BootUsbEpType_t
{
    CY_FX3_BOOT_USB_EP_CONTROL = 0,             /**< Control Endpoint Type */
    CY_FX3_BOOT_USB_EP_ISO,                     /**< Isochronous Endpoint Type */
    CY_FX3_BOOT_USB_EP_BULK,                    /**< Bulk Endpoint Type */
    CY_FX3_BOOT_USB_EP_INTR                     /**< Interrupt Endpoint Type */
} CyFx3BootUsbEpType_t;

/** \brief Virtual descriptor types to be used to set descriptors.

    **Description**\n
    The user application can register different device and configuration descriptors with the
    USB driver, to be used at various USB connection speeds. To support this, the CyU3PUsbSetDesc
    call accepts a descriptor type parameter that is different from the USB standard
    descriptor type value that is embedded in the descriptor itself. This enumerated type
    is to be used by the application to register various descriptors with the USB driver.
 */
typedef enum CyU3PUSBSetDescType_t
{
    CY_U3P_USB_SET_SS_DEVICE_DESCR,     /**< SuperSpeed (USB 3.0) device descriptor. */
    CY_U3P_USB_SET_HS_DEVICE_DESCR,     /**< USB 2.0 device descriptor. */
    CY_U3P_USB_SET_DEVQUAL_DESCR,       /**< Device qualifier descriptor */
    CY_U3P_USB_SET_FS_CONFIG_DESCR,     /**< Full Speed configuration descriptor. */
    CY_U3P_USB_SET_HS_CONFIG_DESCR,     /**< High Speed configuration descriptor. */
    CY_U3P_USB_SET_STRING_DESCR,        /**< String descriptor. */
    CY_U3P_USB_SET_SS_CONFIG_DESCR,     /**< SuperSpeed configuration descriptor. */
    CY_U3P_USB_SET_SS_BOS_DESCR         /**< BOS descriptor. */
} CyU3PUSBSetDescType_t;

/** \brief Enumeration of USB descriptor types.

    **Description**\n
    Descriptor types as defined in the USB Specification.
 */
typedef enum CyU3PUsbDescType
{
    CY_U3P_USB_DEVICE_DESCR = 0x01,             /**< Super Speed Device descr  */
    CY_U3P_USB_CONFIG_DESCR,                    /**< Configuration */
    CY_U3P_USB_STRING_DESCR,                    /**< String */
    CY_U3P_USB_INTRFC_DESCR,                    /**< Interface */
    CY_U3P_USB_ENDPNT_DESCR,                    /**< End Point */
    CY_U3P_USB_DEVQUAL_DESCR,                   /**< Device Qualifier */
    CY_U3P_USB_OTHERSPEED_DESCR,                /**< Other Speed Configuration */
    CY_U3P_USB_INTRFC_POWER_DESCR,              /**< Interface  power descriptor */
    CY_U3P_BOS_DESCR = 0x0F,                    /**< BOS descriptor*/
    CY_U3P_DEVICE_CAPB_DESCR,                   /**< Device Capability descriptor*/
    CY_U3P_USB_HID_DESCR = 0x21,                /**< HID descriptor */
    CY_U3P_USB_REPORT_DESCR,                    /**< Report descriptor */
    CY_U3P_SS_EP_COMPN_DESCR = 0x30             /**< End Point companion descriptor*/
} CyU3PUsbDescType;

/** \brief Number of string descriptors that are supported by the FX3 booter.

    **Description**\n
    The FX3 booter is capable of storing pointers to and handling a number
    of string descriptors. This constant represents the number of strings
    that the booter is capable of handling and is currently fixed to 16.
 */
#define CY_FX3_USB_MAX_STRING_DESC_INDEX    (16)

/** \brief Pointer to the various descriptors.

    **Description**\n
    This data structure stores pointers to the various USB descriptors. These pointers are
    set as part of the CyFx3BootUsbSetDesc() function.
*/
typedef struct CyU3PUsbDescrPtrs
{
    uint8_t             *usbDevDesc_p;              /**< Pointer to device desc of device */
    uint8_t             *usbSSDevDesc_p;            /**< Pointer to SS device desc of device */
    uint8_t             *usbDevQualDesc_p;          /**< Pointer to device qualifier desc of device */
    uint8_t             *usbConfigDesc_p;           /**< Pointer to config desc of device */
    uint8_t             *usbOtherSpeedConfigDesc_p; /**< Pointer to other speed configuration desc of device */
    uint8_t             *usbHSConfigDesc_p;         /**< Pointer to HIGH SPEED speed configuration desc of device */
    uint8_t             *usbFSConfigDesc_p;         /**< Pointer to FULL SPEED speed configuration desc of device */
    uint8_t             *usbSSConfigDesc_p;         /**< Pointer to SUPER SPEED speed configuration desc of device */
    uint8_t             *usbStringDesc_p[CY_FX3_USB_MAX_STRING_DESC_INDEX]; /**< Array of pointers to string
                                                                                 descriptors. */
    uint8_t             *usbSSBOSDesc_p;            /**< Pointer to Super speed BOS descriptor */
} CyU3PUsbDescrPtrs;

/** \brief USB Setup Packet data structure.

    **Description**\n
    Control Endpoint setup packet data structure.
*/
typedef struct CyFx3BootUsbEp0Pkt_t
{
    uint8_t   bmReqType;                /**< Direction, type of request and intended recipient */
    uint8_t   bReq;                     /**< Request being made */
    uint8_t   bVal0;                    /**< wValue field, LSB. */
    uint8_t   bVal1;                    /**< wValue field, MSB. */
    uint8_t   bIdx0;                    /**< wIndex field, LSB. */
    uint8_t   bIdx1;                    /**< wIndex field, MSB. */
    uint16_t  wLen;                     /**< wLength field. */
    uint8_t  *pData;                    /**< Pointer to the data */
} CyFx3BootUsbEp0Pkt_t;

/** \brief Endpoint configuration information.

    **Description**\n
    This structure holds all the properties of a USB endpoint. This structure
    is used to provide information about the desired configuration of various
    endpoints in the system.
 */
typedef struct CyFx3BootUsbEpConfig_t
{
    CyBool_t             enable;        /**< Endpoint status - enabled or not. */
    CyFx3BootUsbEpType_t epType;        /**< The endpoint type */
    uint16_t             streams;       /**< Number of bulk streams used by the endpoint. */
    uint16_t             pcktSize;      /**< Maximum packet size for the endpoint. Valid range <1 - 1024> */
    uint8_t              burstLen;      /**< Maximum burst length in packets. */
    uint8_t              isoPkts;       /**< Number of packets per micro-frame for ISO endpoints. */
} CyFx3BootUsbEpConfig_t;

/** \brief USB Events notification callback

    **Description**\n
    Type of callback function that is invoked to notify the USB events.
    The FX3 booter does the necessary handling of the USB events and
    the application should NOT block on this function.
 */
typedef void (*CyFx3BootUSBEventCb_t) (
        CyFx3BootUsbEventType_t event           /**< USB event type. */
        );

/** \brief USB setup request handler type.

    **Description**\n
    Type of callback function that is invoked to handle USB setup requests.
    The booter doesn't handle any of the standard/vendor requests and this handler
    is expected to handle all such requests.
 */
typedef void (*CyFx3BootUSBSetupCb_t) (
        uint32_t setupDat0,             /**< Lower 4 bytes of the setup packet. */
        uint32_t setupDat1              /**< Upper 4 bytes of the setup packet. */
        );

/** \brief Start the USB driver.

    **Description**\n
    This function is used to start the USB driver of the booter. This function powers
    on the USB block on the FX3 device, if it is not already on. The application can
    register for the USB events of type CyFx3BootUSBEventCb_t.

    **Return value**\n
    * CY_FX3_BOOT_ERROR_NO_REENUM_REQUIRED - indicates that the USB block has been left on
      when transferring control from the main FX3 application to the boot firmware.
    * CY_FX3_BOOT_SUCCESS                  - USB block and driver have been started up.
 */
extern CyFx3BootErrorCode_t
CyFx3BootUsbStart (
        CyBool_t noReEnum,              /**< CyTrue: Do not re-enumerate if the USB connection is active;
                                             CyFalse: Force re-enumeration by resetting the USB block. */
        CyFx3BootUSBEventCb_t cb        /**< USB Event handler. */
        );

/** \brief Register a USB descriptor with the booter.

    **Description**\n
    This function is used to register a USB descriptor with the booter.
    The booter is capable of remembering one descriptor each of the various
    supported types as well as upto 16 different string descriptors.
 
    The booter only stores the descriptor pointers that are passed in to this
    function, and does not make copies of the descriptors. The caller therefore
    should not free up these descriptor buffers while the USB booter is active.

    **Note**\n
    If the noReEnum option to the CyFx3BootUsbStart API is used, this API makes
    a copy of the descriptor to pass to the full firmware application. Since a
    finite memory space (total space of 3 KB) is available for these copied
    descriptors; this API will return an error when all of the available memory
    has been used up.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS - On Success.\n
    * CY_FX3_BOOT_ERROR_BAD_ARGUMENT -  Bad descriptor index.\n
    * CY_FX3_BOOT_ERROR_BAD_DESCRIPTOR_TYPE - Bad descriptor type.\n
    * CY_FX3_BOOT_ERROR_MEMORY_ERROR - out of memory when copying descriptors.
 */
extern CyFx3BootErrorCode_t
CyFx3BootUsbSetDesc (
        CyU3PUSBSetDescType_t descType,         /**< Type of the descriptor */
        uint8_t desc_index,                     /**< Descriptor index for string descriptors only. */
        uint8_t *desc                           /**< Pointer to the descriptor. */
        );

/** \brief Enable/Disable USB connection.

    **Description**\n
    This function is used to enable the USB PHYs and to control the connection
    to the USB host in that manner.
 
    The connect parameter enables/disables the USB connection.

    The ssEnable parameter controls the SS or HS/FS enumeration. If SS enumeration
    is tried and fails, the device automatically falls back to HS/FS mode.
 
    **Return value**\n
    * None
*/
extern void
CyFx3BootUsbConnect (
        CyBool_t connect,       /**< CyFalse - Disable USB Connection; CyTrue - Enable USB Connection */
        CyBool_t ssEnable       /**< CyFalse - HS/FS Enumeration; CyTrue - SS Enumeration */
        );

/** \brief Function to register a USB setup request handler.

    **Description**\n
    This function is used to register a USB setup request handler with the booter.
    This setup request handler is expected to handle all the USB standard/vendor requests.

    **Return value**\n
    * None

    **\see
    *\see CyFx3BootUSBSetupCb_t
 */
extern void
CyFx3BootRegisterSetupCallback (
        CyFx3BootUSBSetupCb_t callback  /**< Setup request handler */
        );

/** \brief Retrieve the current state of the specified endpoint.

    **Description**\n
    This function retrieves the current NAK and STALL status of the specified
    endpoint. The isNak return value will be CyTrue if the endpoint is forced
    to NAK all requests. The isStall return value will be CyTrue if the endpoint
    is currently stalled.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS - On Success.\n
    * CY_FX3_BOOT_ERROR_NULL_POINTER - If isNak or the isStall parameter is NULL.
 */
extern CyFx3BootErrorCode_t
CyFx3BootUsbGetEpCfg (
        uint8_t ep,             /**< Endpoint number to query. */
        CyBool_t *isNak,        /**< Return parameter which will be filled with the NAK status. */
        CyBool_t *isStall       /**< Return parameter which will be filled with the STALL status. */
        );

/** \brief Set/Clear Feature USB Request handler.

    **Description**\n
    This function handles standard SET_FEATURE and CLEAR_FEATURE commands from the USB host.
    The requests handled include EP_HALT, FUNCTION_SUSPEND, Ux_ENABLE, LTM_ENABLE and TEST_MODE
    features.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS - on success.\n
    * CY_FX3_BOOT_ERROR_FAILURE - on failure.
*/
extern CyFx3BootErrorCode_t
CyFx3BootUsbSetClrFeature (
        uint32_t              sc,               /**< 1 - Set feature, 0 - Clear feature. */
        CyBool_t              isConfigured,     /**< Whether the device is in configured (SET_CONFIG) state. */
        CyFx3BootUsbEp0Pkt_t *pEp0              /**< Pointer to the control request structure. */
        );

/** \brief Function to transfer USB endpoint data using DMA.

    **Description**\n
    This function can be used to transfer data on a USB endpoint. This works for the
    control endpoint as well as for other endpoints. The function does not validate
    the parameters, and can cause a fatal lock-up if called with the wrong parameters.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS                - in case of succesful DMA transfer.\n
    * CY_FX3_BOOT_ERROR_INVALID_DMA_ADDR - if the address specified is not in the SYSMEM area.\n
    * CY_FX3_BOOT_ERROR_XFER_FAILURE     - in case of DMA transfer failure.\n
    * CY_FX3_BOOT_ERROR_TIMEOUT          - in case of DMA transfer times out.
 */
extern CyFx3BootErrorCode_t
CyFx3BootUsbDmaXferData (
        uint8_t epNum,          /**< Endpoint number to/from which to transfer data. The MSB is
                                     used to identify whether this is a read or a write transfer. */
        uint32_t address,       /**< SYSMEM address from/to which data is to be transferred. */
        uint32_t length,        /**< Length of the transfer in bytes. */
        uint32_t timeout        /**< Timeout in multiples of 100 us. Also refer the macros CY_FX3_BOOT_NO_WAIT
                                     and CY_FX3_BOOT_WAIT_FOREVER */
        );

/** \brief Get the connection speed at which USB is operating.

    **Description**\n
    This function is used to get the operating speed of the active USB connection.

    **Return value**\n
    * Current USB connection speed.
 */
extern CyFx3BootUsbSpeed_t
CyFx3BootUsbGetSpeed (
        void);

/** \brief Set or clear the stall status of an endpoint.

    **Description**\n
    This function is to set or clear the stall status of a given endpoint.
    This function is to be used in response to SET_FEATURE and CLEAR_FEATURE
    requests from the host as well as for interface specific error handling.
    When the stall condition is being cleared, the data toggles for the
    endpoint can also be cleared. While an option is provided to leave the
    data toggles unmodified, this should only be used under specific conditions
    as recommended by Cypress.

    **Return value**\n
    * None
 */
extern void
CyFx3BootUsbStall (
        uint8_t ep,                     /**< Endpoint number to be modified. */
        CyBool_t stall,                 /**< CyTrue: Set the stall condition, CyFalse: Clear the stall */
        CyBool_t toggle                 /**< CyTrue: Clear the data toggles in a Clear Stall call */
        );

/** \brief Complete the status handshake of a USB control request.

    **Description**\n
    This function is used to complete the status handshake of a USB control
    request that does not involve any data transfer.
*/
extern void
CyFx3BootUsbAckSetup (
        void);

/** \brief Retrieves the pointer to the USB descriptors.

    **Description**\n
    The booter stores the descriptor pointers that are passed in as part of the
    CyFx3BootUsbSetDesc() function. This function is used to retrieve pointer
    of type CyU3PUsbDescrPtrs. This pointer can be used to retrieve the relevant
    data while handling the standard USB requests.

    **Return value**\n
    * Pointer of type CyU3PUsbDescrPtrs on Success.\n
    * NULL on Failure

    **\see
    *\see CyFx3BootUsbSetDesc
*/
extern CyU3PUsbDescrPtrs *
CyFx3BootUsbGetDesc (
        void);

/** \brief Configure a USB endpoint's properties.

    **Description**\n
    The FX3 device has 30 user configurable endpoints (1-OUT to 15-OUT and 1-IN
    to 15-IN) which can be separately selected and configured with desired properties
    such as endpoint type and the maximum packet size.
 
    All of these endpoints are kept disabled by default. This function is used
    to enable and set the properties for a specified endpoint. Separate calls
    need to be made to enable and configure each endpoint that needs to be used.

    **Note**\n
    Only BULK endpoints are supported by the boot firmware as of now.

    **Return value**\n
    * CY_FX3_BOOT_SUCCESS - when the call is successful.\n
    * CY_FX3_BOOT_ERROR_NULL_POINTER - when the epinfo pointer is NULL.\n
    * CY_FX3_BOOT_ERROR_NOT_SUPPORTED - if eptype is non-bulk endpoint.
 */
extern CyFx3BootErrorCode_t
CyFx3BootUsbSetEpConfig (
        uint8_t ep,                             /**< Endpoint number to configured. */
        CyFx3BootUsbEpConfig_t *epinfo          /**< EP configuration information */
        );

/** \brief Configure USB block on FX3 to work off Vbatt power instead of Vbus.

    **Description**\n
    The USB block on the FX3 device can be configured to work off Vbus power or Vbatt power, with the
    Vbus power being the default setting. This function is used to enable/disable the Vbatt power input
    to the USB block.
 
    **Note**\n
    This call is expected to be done prior to the CyFx3BootUsbConnect.

    **Return value**\n
    * None
 */
extern void
CyFx3BootUsbVBattEnable (
        CyBool_t enable                         /**< CyTrue: Work off VBatt, CyFalse: Do not work off VBatt */
        );

/** \brief Function to check if the status stage of a EP0 transfer is pending.

    **Description**\n
    This function is used to check if the status stage of a EP0 transfer is pending.
    In order for the device to handle the power management appropriately the device doesn't go into
    low power modes while a EP0 request is pending.
   
    As this check cannot be done in the ISR context this has been deferred to the application context.
    This is applicable when the device is functioning in SuperSpeed only.

    **Return value**\n
    * None
 */
extern void
CyFx3BootUsbEp0StatusCheck (
        void);

/** \brief This function checks if the device state is in USB 3.0 disconnect state.

    **Description**\n
    This function checks if the device is in USB 3.0 disconnect state. If this is the case
    then a fallback to USB 2.0 is attempted. The function returns immediately if the device
    is not in the disconnect state.

    As this check cannot be done in the ISR context this has been deferred to the application
    context.

    **Return value**\n
    * None
 */
extern void
CyFx3BootUsbCheckUsb3Disconnect (
        void);

/** \brief This function is used to send the compliance patterns.

    **Description**\n
    This function is used to send the USB 3.0 compliance patterns. If the device's LTSSM state
    is currently in the compliance state, then compliance patterns are to be sent. This function
    returns when the LTSSM state is no longer in the compliance state.

    As this task cannot be done in the ISR context this has been deferred to the application context.

    **Return value**\n
    * None
 */
extern void
CyFx3BootUsbSendCompliancePatterns (
        void);

/** \brief Disable acceptance of U1/U2 entry requests from USB 3.0 host.

    **Description**\n
    In some cases, accepting U1/U2 entry requests from the USB 3.0 host continuously can limit the
    performance that can be achieved through the FX3. This API can be used to temporarily disable
    the acceptance of U1/U2 requests by the FX3 device.

    **Return value**\n
    * None

    **\see
    *\see CyFx3BootUsbLPMEnable
 */
extern void
CyFx3BootUsbLPMDisable (
        void);

/** \brief Re-enable acceptance of U1/U2 entry requests from USB 3.0 host.

    **Description**\n
    This API is used to re-enable acceptance of U1/U2 entry requests by the FX3 device. It is
    required that LPM acceptance be kept enabled whenever a new USB connection is started up.
    This is required for passing USB compliance tests.

    **Return value**\n
    * None

    **\see
    *\see CyFx3BootUsbLPMDisable
 */
extern void
CyFx3BootUsbLPMEnable (
        void);

#include <cyu3externcend.h>

#endif /* __CYFX3USB_H__ */

