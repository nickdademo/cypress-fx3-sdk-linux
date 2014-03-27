
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3S SDK

SD/MMC BACKED MASS STORAGE CLASS EXAMPLE
----------------------------------------

  This example illustrates the use of the FX3S firmware APIs to implement
  a mass storage class device that allows access to SD/MMC devices connected
  to FX3S.

  The application partitions the storage device found on each of the storage
  ports into two volumes, and then enumerates them as separate logical units
  on the USB side. The application also demonstrates the use of the delayed
  write commit option to optimize write performance to the storage device.

  Files:

    * cyfx_gcc_startup.S : Start-up code for the ARM-9 core on the FX3S device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfx3s_msc.h       : Constant definitions for the storage access and mass
      storage class implementation.

    * cymsc_dscr.c       : C source file containing the USB descriptors that are
      used by this firmware example.

    * cyfxtx.c           : ThreadX RTOS wrappers and utility functions required
      by the FX3S API library.

    * cyfx3s_msc.c       : Main C source file that implements the mass storage
      function.

    * makefile           : GNU make compliant build script for compiling this
      example.

[]

