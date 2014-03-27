
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    CX3 SDK

CX3 UVC 1.1/1.0 Example using Omnivision OV5640 Sensor
------------------------------------------------------

  This example implements a Bulk-only UVC 1.1 compliant example which illustrates the use of the 
  CX3 APIs using an Omnivision OV5640 Sensor. 
  This example streams Uncompressed 16-Bit YUV video from the image sensor over the CX3 to the host PC
  which can be viewed using a viewer such as AmCap or the Windows 8 Camera App.  

  This example has been tested against the Microsoft Windows UVC 1.1 host drivers available on 
  Windows 7 and Windows 8. 
  
  The following resolutions are implemented:
    A) Super Speed USB 3.0: 
       1) 2952 x 1944 @ 15 Frames per second.
       1) 1920 x 1080 @ 30 Frames per second.
       2) 1280 x 720  @ 60 Frames per second.
    B) High Speed USB 2.0:
       1) 640 x 480   @ 60 Frames per second.
    C) Full Speed USB:
       1) Not supported.

  The example enumerates with the VID/PID 04B4/00C3.

  This example does not currently implement any UVC functions (Brightness, Contrast, Exposure etc.)
  This example has autofocus enabled.
  
  NOTE: This example will only correctly stream if built in Release configuration.

  This example requires the following CX3 library and header files over and above the standard FX3 
  libraries (available under u3p_firmware\lib and u3p_firmware\inc):
  1) cyu3mipicsi.a      :       Library providing the APIs for the CX3 Mipi-Csi2 interface.
  2) cy_ov5640.a        :       Library providing APIs to configure the  Omnivision OV5640 sensor.
                                In case a different sensor needs to be used, this library (and the calls 
                                made to it) should be replaced with the customer defined sensor library.
  3) cyu3mipicsi.h      :       Header file defining the CX3 Mipi-CSI2 interface APIs.
  4) cyu3imagesensor.h  :       Header file defining the Image sensor APIs.
  
  Application Files:
    * cyfx_gcc_startup.S:       Start-up code for the ARM-9 core on the CX3 device.
                                This assembly source file follows the syntax for the GNU assembler.

    * cyfxtx.c          :       ThreadX RTOS wrappers and utility functions required
                                by the CX3 API library.

    * cycx3_uvc.h       :       Constant definitions used in the application.

    * cycx3_uvc.c       :       C source file implementing UVC 1.1 example.

    * cycx3_uvcdscr.c   :       C source file containing the USB descriptors and Mipi interface 
                                configuration structure objects for the application.

    * makefile          :       GNU make compliant build script for compiling this example.

[]

