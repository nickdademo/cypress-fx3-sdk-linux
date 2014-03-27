
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    CX3 SDK

CX3 16-Bit RGB565 Video Stream Example using Aptina A0260 Sensor
--------------------------------------------------

  This example implements a Bulk-only RGB-565 video streaming example over the UVC protocol which illustrates 
  the usage of the  CX3 APIs using an Aptina AS0260 Sensor. 

  This example streams Uncompressed 16-Bit RGB-565 video from the image sensor over the CX3 to the host PC
  which can be viewed using a viewer such as AmCap or the Windows 8 Camera App.  

  This example has been tested against the Microsoft Windows UVC 1.1 host drivers available on 
  Windows 7 and Windows 8. 
  
  The following resolutions are implemented:
    A) Super Speed USB 3.0: 
       1) 1920 x 1080 @ 30 Frames per second.
       2) 1280 x 720  @ 55 Frames per second.
    B) High Speed USB 2.0:
       1) 640 x 480   @ 60 Frames per second.
       2) 320 x 240   @ 90 Frames per second.
    C) Full Speed USB:
       1) 320 x 240   @ 05 Frames per second.

  The example enumerates with the VID/PID 04B4/00C3.

  This example does not currently implement any UVC functions (Brightness, Contrast, Exposure etc.)
  
  NOTE: This example will only correctly stream if built in Release configuration.
  This example requires the following CX3 library and header files over and above the standard FX3 
  libraries (available under u3p_firmware\lib and u3p_firmware\inc):
  1) cyu3mipicsi.a      :       Library providing the APIs for the CX3 Mipi-Csi2 interface.
  2) cy_as0260.a        :       Library providing APIs to configure the Aptina AS0260 sensor.
                                In case a different sensor needs to be used, this library (and the calls 
                                made to it) should be replaced with the customer defined sensor library.
  3) cyu3mipicsi.h      :       Header file defining the CX3 Mipi-CSI2 interface APIs.
  4) cyu3imagesensor.h  :       Header file defining the Image sensor APIs.
  
  Application Files:
    * cyfx_gcc_startup.S:       Start-up code for the ARM-9 core on the CX3 device.
                                This assembly source file follows the syntax for the GNU assembler.

    * cyfxtx.c          :       ThreadX RTOS wrappers and utility functions required
                                by the CX3 API library.

    * cycx3_rgb16.h     :       Constant definitions used in the application.

    * cycx3_rgb16.c     :       C source file implementing the 16-Bit RGB 565 example.

    * cycx3_dscr.c      :       C source file containing the USB descriptors and Mipi interface 
                                configuration structure objects for the application.

    * makefile          :       GNU make compliant build script for compiling this example.

[]

