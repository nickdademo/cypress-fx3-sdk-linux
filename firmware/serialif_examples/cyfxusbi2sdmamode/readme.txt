
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB I2S (DMA MODE) EXAMPLE
----------------------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a USB to I2S example.

  The device enumerates as a vendor specific USB device with two bulk endpoints
  EP1 OUT and EP2 OUT. The data received on EP1 OUT is streamed to I2S left
  channel and data received on EP2 OUT is streamed to I2S right channel.

  The I2S master is configured to run in stereo I2S mode with sample width
  as 16 bits. If a valid WAV file is streamed over the interface with left and
  right channel, the audio can be listened to by connecting an I2S slave device
  on I2S header on the DVK.

  Files:

    * cyfx_gcc_startup.S   : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxusbi2sdmamode.h  : Constant definitions for the USB I2S application.
      The USB connection speed, numbers and properties of the endpoints etc.
      can be selected through definitions in this file.

    * cyfxusbenumdscr.c    : C source file containing the USB descriptors that
      are used by this firmware example. VID and PID is defined in this file.

    * cyfxtx.c             : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxusbi2sdmamode.c  : Main C source file that implements the USB I2S application
      example.

    * makefile             : GNU make compliant build script for compiling this
      example.

[]

