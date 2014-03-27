
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB BULKLOOP-LOW LEVEL EXAMPLE
-------------------------------

  This is another variant of the USB bulk loopback application that shows
  the use of FX3 to receive data from USB and to loop it back to USB host
  using the low level DMA API.

  Though the functionality implemented is the same as the other loopback
  examples, the implementation is different. The low level socket and
  descriptor APIs are used to receive a buffer of data which is then sent
  back to the USB host.

  NOTE: This example is made simple and does transfers at single buffer
  basis. This means that USB host can only send out a single USB packet
  in one transfer and has to read back before sending the next packet.

  Files:

    * cyfx_gcc_startup.S   : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxbulklplowlevel.h : Constant definitions for the bulk loop application.
      The USB connection speed, numbers and properties of the endpoints etc.
      can be selected through definitions in this file.

    * cyfxbulklpdscr.c     : C source file containing the USB descriptors that
      are used by this firmware example. VID and PID is defined in this file.

    * cyfxtx.c             : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxbulklplowlevel.c : Main C source file that implements the bulk loopback
      example.

    * makefile             : GNU make compliant build script for compiling this
      example.

[]

