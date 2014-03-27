
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB AUDIO CLASS EXAMPLE
-----------------------

  This example implements a USB audio class (microphone) device that streams
  audio data to the USB host. 

  This example application demonstrates the following:

  1. The usage of the FX3 USB APIs to implement a standard USB audio class
     device.  The application shows how class specific USB control requests
     can be processed at the application level.

  2. The implementation of a USB device with multiple alternate interfaces,
     and protocol handling based on the interface setting selected by the
     host.

  3. Configuration and data transfer setting through an isochronous endpoint. 

  Files:

    * cyfx_gcc_startup.S : Start-up code for the ARM-9 core on the FX3
      device.  This assembly source file follows the syntax for the GNU
      assembler.

    * cyfxuac.h     : C header file that defines constants used by
      this example implementation.  Can be modified to select USB connection
      speed, endpoint numbers and properties etc.

    * cyfxuacdscr.c : C source file that contains USB descriptors
      used by this example. VID and PID is defined in this file.

    * cyfxtx.c      : C source file that provides ThreadX RTOS wrapper
      functions and other utilites required by the FX3 firmware library.

    * cyfxuac.c     : Main C source file that implements this example.

    * makefile      : GNU make compliant build script for compiling
      this example.

[]

