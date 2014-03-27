
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB DEBUG EXAMPLE
-------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a debug log mechanism over the USB interface.

  The device enumerates as a vendor specific USB device with a single 
  interrupt endpoint (EP1-IN). The application sends out periodic log
  information out on the USB interface. The length of log can be up to 
  128 byte. So the user should never request for data transfer of size
  less than 128 byte.

  Files:

    * cyfx_gcc_startup.S : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxusbdebug.h   : Constant definitions for the debug application.
      The USB connection speed, numbers and properties of the endpoints etc.
      can be selected through definitions in this file.

    * cyfxusbdscr.c   : C source file containing the USB descriptors that
      are used by this firmware example. VID and PID is defined in this file.

    * cyfxtx.c           : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxusbdebug.c   : Main C source file that implements the debug
      example.

    * makefile           : GNU make compliant build script for compiling this
      example.

[]

