
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK
			---------------------------------

USB BULK STREAMS EXAMPLE
------------------------

  This example illustrates the use of the FX3 firmware APIs to illustrate
  data source and data sink over a pair of USB Bulk endpoints using streams.

  The device enumerates as a vendor specific USB device with a pair of bulk
  endpoints (1-OUT and 1-IN). The OUT endpoint acts as data sink and the IN
  endpoint acts as data source to the PC host.

  Each endpoint defines fixed number of bulk streams. The source is achieved
  with the help of DMA MANUAL_OUT channels and sink is acheived with DMA MANUAL_OUT
  channels per individual stream. Each stream id of the endpoint is mapped to a 
  DMA socket. 

  Any data received from the host through the DMA MANUAL_IN channels is discarded.
  A constant data pattern is continuously loaded into the DMA MANUAL_OUT channel
  and sent to the host whenever requested.

  Streams are valid only for super speed. For other speeds the example implements
  a simple data source and data sink without streams.

  When in stream mode, for EP1 IN, socket CY_U3P_UIB_PROD_1 will be the default
  stream (1). Unallocated sockets (those EP numbers which are not used by the 
  device, can be used to represent other streams. In this example, we are using
  CY_U3P_UIB_PROD_2 for EP1 IN stream 2 and so on. The mapping is done similarly
  for the consumer sockets as well.

  Files:

    * cyfx_gcc_startup.S   : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxbulksreams.h     : Constant definitions for the Bulk streams 
      application. The USB connection speed, numbers and properties of the 
      endpoints etc. can be selected through definitions in this file.

    * cyfxbulkdscr.  c     : C source file containing the USB descriptors that
      are used by this firmware example. VID and PID is defined in this file.

    * cyfxtx.c             : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxbulkstreams.c    : Main C source file that implements the Bulk streams
      example.

    * makefile             : GNU make compliant build script for compiling this
      example.

[]

