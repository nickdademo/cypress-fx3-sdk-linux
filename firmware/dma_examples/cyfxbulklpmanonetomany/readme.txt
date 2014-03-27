
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB BULKLOOP-MANUAL_ONE_TO_MANY EXAMPLE
-------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a data loopback application over three USB bulk endpoints. This version
  of the data loopback makes use of a MANUAL DMA multichannel, and 
  demonstrates the use of the ARM9 core to make changes if necessary 
  to the data streaming through the FX3 device.

  The device enumerates as a vendor specific USB device with three Bulk
  endpoints (1-OUT, 1-IN and 2-IN).  The application loops back any data 
  that it receives on the Bulk OUT endpoint to the two Bulk IN endpoints in 
  an interleaved fashion.

  The loopback is achieved with the help of a multichannel DMA 
  MANUAL_ONE_TO_MANY which is created connecting the three endpoints. This
  application receives data on the 1-OUT endpoint from the USB host. CPU is
  signalled of the incoming data via a DMA callback. CPU then sends the same
  data back to the host through the 1-IN and 2-IN endpoint in an interleaved
  fashion.

  Files:

    * cyfx_gcc_startup.S : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxbulklpmanonetomany.h : Constant definitions for the bulk loop 
      application. The USB connection speed, numbers and properties of the 
      endpoints etc. can be selected through definitions in this file.

    * cyfxbulklpdscr.c   : C source file containing the USB descriptors that
      are used by this firmware example. VID and PID is defined in this file.

    * cyfxtx.c           : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxbulklpmanonetomany.c : Main C source file that implements the bulk 
      loopback example.

    * makefile           : GNU make compliant build script for compiling this
      example.

[]

