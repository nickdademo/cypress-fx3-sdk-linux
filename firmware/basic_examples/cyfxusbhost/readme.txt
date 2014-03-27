
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB HOST EXAMPLE
-------------------------

   This example illustrates the use of the FX3 firmware APIs to implement
   USB host mode operation. The example supports simple HID mouse class
   and simple MSC class devices.

   VBus Control:

   This example code uses the CTL4 pin (GPIO21) of the FX3/FX3S device to
   control the VBus power output to a USB peripheral.
   
   The CY_FX_HOST_VBUS_ENABLE_VALUE constant in the cyfxusbhost.h file controls
   the polarity of this GPIO control. The default setting is Active High;
   i.e., driving CTL4 high will turn on the VBus output, and driving CTL4
   low will turn off the VBus output. This matches the power switch
   configuration on the FX3 Rev7 DVK board. Please change the definition if
   the board you are using requires a different polarity.

   USB HID MOUSE DRIVER:

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
   in cyfxusbhost.h file.

   USB MSC DRIVER:

   A simple single interface USB BOT MSC device will be successfully 
   enumerated and storage parameters will be queried. The example shall perform
   read / write tests to fixed sectors which is repeated at an interval of 1 minute.
   The write operation is disabled by default. It can be enabled by enabling
   the macro definition CY_FX_MSC_ENABLE_WRITE_TEST, in the cyfxusbhost.h file.

   NOTE: If write tests are enabled, then the data on the drive might be
   lost / corrupted. The drive has to be formatted again on a PC.

  Files:

    * cyfx_gcc_startup.S : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxusbhost.h   : Constant definitions for the host application.

    * cyfxtx.c           : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxusbhost.c   : Main C source file that implements the host mode
      example.

    * makefile        : GNU make compliant build script for compiling this
      example.

[]

