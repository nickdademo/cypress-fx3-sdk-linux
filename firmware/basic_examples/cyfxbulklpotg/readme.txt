
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB BULKLOOP OTG EXAMPLE
-------------------------

   This example illustrates the use of the FX3 firmware APIs to implement
   USB OTG mode operation. Here as a device FX3 enumerates as an OTG
   bulkloop device and as a host FX3 will respond only to a bulkloop device
   with fixed VID and PID. If the attached device is an OTG bulkloop device,
   then after 30 seconds the host shall initiate an HNP. Once the HNP is
   completed, the current host will act as a bulk loopback device. If FX3 OTG
   device is connected to a PC, it shall do a normal bulkloop back function.

   Care should be taken so that both sides do not drive VBUS. This means that
   the ID pin should be configured correctly.

   Host mode:

   Please refer to the FX3 DVK user manual for enabling VBUS control.
   The current example controls the VBUS supply via GPIO 21. The example assumes
   that a low on this line will turn on VBUS and allow remotely connected B-device
   to enumerate. A high / tri-state  on this line will turn off VBUS supply from
   the board. Also this requires that FX3 DVK is powered using external power supply.
   VBATT should also be enabled and connected. Depending upon the VBUS control,
   update the CY_FX_OTG_VBUS_ENABLE_VALUE and CY_FX_OTG_VBUS_DISABLE_VALUE definitions
   in cyfxbulklpotg.h file.

  Files:

    * cyfx_gcc_startup.S : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxbulklpotg.h    : Constant definitions for the OTG application.

    * cyfxtx.c           : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxbulklpotg.c    : Main C source file that implements the OTG mode
      example.

    * cyfxusbhost.c      : USB host stack implementation.

    * cyfxusbdev.c       : USB bulk loop device stack implementation.

    * cyfxusbdscr.c      : USB device mode descriptor definitions.

    * makefile           : GNU make compliant build script for compiling this
      example.

[]

