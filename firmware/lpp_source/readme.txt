
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

LPP Library 
-----------

  This project build the low performance peripheral (LPP) library for FX3.

  This library contains the FX3 firmware driver and API source for GPIO, I2S, I2C, SPI
  and UART. The corresponding register set is documented in the FX3 Programmer's Manual.
  
  Files:

    * cyu3gpio.c           : Firmware driver and API source for the FX3 GPIO block.

    * cyu3gpiocomplex.c    : APIs for complex GPIO setup and access. These have been moved to a separate source
                             file to reduce memory footprint for applications that do not use complex GPIOs.

    * cyu3i2c.c		   : Firmware driver and API source for the FX3 I2C block.

    * cyu3i2s.c		   : Firmware driver and API source for the FX3 I2S block.

    * cyu3uart.c	   : Firmware driver and API source for the FX3 UART block.

    * cyu3spi.c		   : Firmware driver and API source for the FX3 SPI block.

    * makefile             : GNU make compliant script for building the API library.

[]

