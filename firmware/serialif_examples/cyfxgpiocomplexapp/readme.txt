
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

COMPLEX GPIO APPLICATION EXAMPLE
----------------------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a complex GPIO application example.

  The example does the following:

  1. A PWM output generated on GPIO 50. The time period of the PWM is set to
     generate 1KHz 25% duty cycle for 1s and the to generate 75% duty cycle
     for the next 1s. This is repeated in a loop.

  2. GPIO 51 is used to measure the low time period for the signal generated
     by GPIO 50. GPIO 50 needs to be connected to GPIO 51. This is done by using
     the MEASURE_LOW_ONCE feature.

  3. GPIO 52 is used as a counter input. The line is internally pulled to high
     with the weak pull-up feature. When a low is applied to the IO line, the
     negative edge is used to increment the counter. The counter can be sampled
     to identify the current count.

  GPIO 50 can be accessed at J20.4 on the DVK.
  GPIO 51 can be accessed at J20.5 on the DVK.
  GPIO 52 can be accessed at J20.6 on the DVK.

  Files:

    * cyfx_gcc_startup.S   : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxtx.c             : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxgpiocomplexapp.c : Main C source file that implements the application
      example.

    * makefile             : GNU make compliant build script for compiling this
      example.

[]

