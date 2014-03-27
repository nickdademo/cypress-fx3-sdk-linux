
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

UART (REGISTER MODE) LOOPBACK EXAMPLE
----------------------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a simple UART loopback application.

  This example illustrates a loopback mechanism between the UART RX and UART TX
  through CPU. CPU receives a byte sent from the UART and sends the same byte 
  back to UART which results in the loopback. 

  The above loopback mechanism is implemented using UART Register mode of 
  operation.
 
  Files:

    * cyfx_gcc_startup.S   : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxtx.c             : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxuartlpregmode.c  : Main C source file that implements the UART application
      example.

    * makefile             : GNU make compliant build script for compiling this
      example.

[]

