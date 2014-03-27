
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

GPIO APPLICATION EXAMPLE
----------------------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a simple GPIO application example.

  The example illustrates the usage of simple GPIO API to set and get 
  the status of the pin and the usage of GPIO interrupts.

  The example uses GPIO 21 as output. It toggles this pin ON and OFF 
  at an interval of 2s.

  GPIO 45 is used as the input GPIO. Interrupts are enabled and a 
  callback is registered for the GPIO edge interrupts both positive 
  and negative edges.

  GPIO 21 is located at pin 2 of the jumper J100 on the DVK board.

  GPIO 45 is located on the DVK board at the test point TP13.

  Files:

    * cyfx_gcc_startup.S   : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxtx.c             : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxgpioapp.c        : Main C source file that implements the application
      example.

    * makefile             : GNU make compliant build script for compiling this
      example.

[]

