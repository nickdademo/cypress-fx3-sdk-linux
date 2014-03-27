
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB BULKLOOP-AUTO_SIGNAL EXAMPLE
-----------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a data loopback application over a pair of USB bulk endpoints.

  This is a variant of the loopback implementation in the cyfxbulklpauto
  folder. The actual data loopback from the bulk OUT (1-OUT) endpoint
  to the Bulk IN (1-IN) endpoint is performed automatically by the FX3
  hardware without any CPU intervention.
  
  The difference from the cyfxbulklpauto case is that an event callback
  is provided to the application running on FX3 whenever a data packet
  is received on the Bulk OUT endpoint by the device.  The loopback as
  well as the callback events are achieved by the use of a DMA AUTO_SIGNAL
  channel.

  Files:

    * cyfx_gcc_startup.S  : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxbulklpautosig.h : Constant definitions for the bulk loop application.
      The USB connection speed, numbers and properties of the endpoints etc.
      can be selected through definitions in this file.

    * cyfxbulklpdscr.c    : C source file containing the USB descriptors that
      are used by this firmware example. VID and PID is defined in this file.

    * cyfxtx.c            : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxbulklpautosig.c : Main C source file that implements the bulk loopback
      example.

    * makefile            : GNU make compliant build script for compiling this
      example.

[]

