
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3S SDK

FX3S SDIO-UART Example
------------------------------------

  This example illustrates the use of the FX3S SDIO APIs by implementing a USB-UART device over
  an SDIO card with a UART Function. The example is expected to work with any SDIO device with a 
  standard UART interface compliant function. It has been developed and tested on the 
  ARASAN AC2200 SDIO HDK platform.

  This example enumerates with a UART Baud rate of 9600 Baud and it is the only baud rate supported
  for this example.

  The application uses the VID/PID for the Cypress USB-UART device and uses the Cypress  
  Virtual Com driver for the Cypress USB-UART device to interact with PC side `applications.

  This example works on USB2.0 and USB3.0 hosts.

  Files:

    * cyfx_gcc_startup.S : Start-up code for the ARM-9 core on the FX3S device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxtx.c           : ThreadX RTOS wrappers and utility functions required
      by the FX3S API library.

    * cyfx3s_sdiouart.h  : Constant definitions used in the application.

    * cyfx3s_sdiouart.c  : C source file implementing the USB<->SDIO-UART application.

    * cyfx3s_sdiodscr.c  : C source file containing the USB descriptors for the application.

    * makefile           : GNU make compliant build script for compiling this example.

[]

