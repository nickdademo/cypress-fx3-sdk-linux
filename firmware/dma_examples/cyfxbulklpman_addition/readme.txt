
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB BULKLOOP-MANUAL HEADER / FOOTER ADDITION EXAMPLE
---------------------------

  This example illustrates the use of the FX3 firmware APIs to implement a
  data loopback application over a pair of USB Bulk endpoints.  This version
  of the data loopback makes use of a MANUAL DMA channel, where a fixed 4 byte
  header and footer gets added to all received data.

  This application enumerates as a vendor specific USB device and receives
  data on the 1-OUT endpoint from the USB host.  The application uses a DMA
  callback to add header and footer in the received the data packets and sends
  the data back to the host through the 1-IN endpoint.

  Files:

    * cyfx_gcc_startup.S : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxbulklpmanualadd.h : Constant definitions for the bulk loop application.
      The USB connection speed, numbers and properties of the endpoints etc.
      can be selected through definitions in this file.

    * cyfxbulklpdscr.c   : C source file containing the USB descriptors that
      are used by this firmware example. VID and PID is defined in this file.

    * cyfxtx.c           : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxbulklpmanualadd.c : Main C source file that implements the bulk loopback
      example.

    * makefile           : GNU make compliant build script for compiling this
      example.

[]

