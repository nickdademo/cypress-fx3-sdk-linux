
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB OTG EXAMPLE
-------------------------

   This example illustrates the use of the FX3 firmware APIs to implement
   USB OTG mode operation. In host mode of operation, we expect to be
   connected to a USB mouse and in device mode operation we expected to be
   connected to a host PC which can do bulk loop operation.

   VBus Control:

   This example code uses the CTL4 pin (GPIO21) of the FX3/FX3S device to
   control the VBus power output to a USB peripheral.
   
   The CY_FX_OTG_VBUS_ENABLE_VALUE constant in the cyfxusbotg.h file controls
   the polarity of this GPIO control. The default setting is Active High;
   i.e., driving CTL4 high will turn on the VBus output, and driving CTL4
   low will turn off the VBus output. This matches the power switch
   configuration on the FX3 Rev7 DVK board. Please change the definition if
   the board you are using requires a different polarity.

   Host mode:

   A simple single interface USB HID mouse will be successfully enumerated
   and the current offset will be printed via the UART debug logs.

   We support only single interface with interface class = HID(0x03),
   interface sub class = Boot (0x01) and interface protocol = Mouse (0x02).
   This example supports only 4 byte input reports with the following format:
        BYTE0: Bitmask for each of the button present.
        BYTE1: Signed movement in X direction.
        BYTE2: Signed movement in Y direction.
        BYTE3: Signed movement in scroll wheel.
   Further types can be implemented by decoding the HID descriptor.

   Care should be taken so that an USB host is not connected to FX3 while
   host stack is active. This will mean that both sides will be driving
   the VBUS causing a hardware damage.

   Please refer to the FX3 DVK user manual for enabling VBUS control.
   The current example controls the VBUS supply via GPIO 21. The example assumes
   that a low on this line will turn on VBUS and allow remotely connected B-device
   to enumerate. A high / tri-state  on this line will turn off VBUS supply from
   the board. Also this requires that FX3 DVK is powered using external power supply.
   VBATT should also be enabled and connected. Depending upon the VBUS control,
   update the CY_FX_OTG_VBUS_ENABLE_VALUE and CY_FX_OTG_VBUS_DISABLE_VALUE definitions
   in cyfxusbotg.h file.

   Device mode:

   This example illustrates a loopback mechanism between two USB bulk endpoints.
   The example comprises of vendor class USB enumeration descriptors with two
   bulk endpoints. A bulk OUT endpoint acts as the producer of data from the host.
   A bulk IN endpint acts as the consumer of data to the host.

   The loopback is achieved with the help of a DMA AUTO channel. DMA AUTO channel
   is created between the producer USB bulk endpoint and the consumer USB bulk
   endpoint. Data is transferred from the host into the producer endpoint which
   is then directly transferred to the consumer endpoint by the DMA engine.
   CPU is not involved in the data transfer.

   The DMA buffer size is defined based on the USB speed. 64 for full speed,
   512 for high speed and 1024 for super speed. CY_FX_BULKLP_DMA_BUF_COUNT in the
   header file defines the number of DMA buffers.

  Files:

    * cyfx_gcc_startup.S : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxusbotg.h       : Constant definitions for the OTG application.

    * cyfxtx.c           : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxusbotg.c       : Main C source file that implements the OTG mode
      example.

    * cyfxusbhost.c      : USB host stack implementation.

    * cyfxusbdev.c       : USB bulk loop device stack implementation.

    * cyfxusbdscr.c      : USB device mode descriptor definitions.

    * makefile           : GNU make compliant build script for compiling this
      example.

[]

