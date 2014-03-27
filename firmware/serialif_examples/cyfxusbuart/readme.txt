
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB - UART BRIDGE EXAMPLE
-------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a Virtual COM Port or USB-UART Bridge. The implementation adheres to the
  CDC-ACM class and allows data communication from USB through to the UART
  port on the FX3 device.

  Files:

    * cyfx_gcc_startup.S   : Start-up code for the ARM-9 core on the FX3 device.
                             This assembly source file follows the syntax for the
                             GNU assembler.

    * cyfxtx.c             : ThreadX RTOS wrappers and utility functions required
                             by the FX3 API library.

    * cyfxusbuartdscr.c    : USB descriptors for this example.

    * cyfxusbuart.h        : Constants and definitions for the USB-UART bridge
                             example.

    * cyfxusbuart.c        : Main source file that implements the USB-UART
                             bridge logic.

    * makefile             : GNU make compliant build script for compiling this
                             example.

[]

