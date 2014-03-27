
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB BULKLOOP-AUTO_MANY_TO_ONE EXAMPLE
-------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a data loopback application over three USB bulk endpoints.

  The device enumerates as a vendor specific USB device with three bulk
  endpoints (1-OUT, 2-OUT and 1-IN).  The application loops back any data that 
  it receives on the two bulk OUT endpoints to the bulk IN endpoint in an 
  interleaved fashion.

  The loopback is achieved with the help of a multichannel DMA AUTO_MANY_TO_ONE
  which is created connecting the three endpoints.  There is no FX3 CPU involvement
  in the data transfer, and the loopback is performed automatically by the
  FX3 device hardware.

  Files:

    * cyfx_gcc_startup.S : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxbulklpautomanytoone.h : Constant definitions for the bulk loop application.
      The USB connection speed, numbers and properties of the endpoints etc.
      can be selected through definitions in this file.

    * cyfxbulklpdscr.c   : C source file containing the USB descriptors that
      are used by this firmware example. VID and PID is defined in this file.

    * cyfxtx.c           : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxbulklpautomanytoone.c : Main C source file that implements the bulk 
      loopback example.

    * makefile           : GNU make compliant build script for compiling this
      example.

[]

