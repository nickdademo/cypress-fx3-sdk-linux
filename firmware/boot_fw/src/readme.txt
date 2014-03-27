README:
-------

This directory contains an example application that shows the usage of the FX3 2-stage booter APIs.
The application provides the following functionality:
1. USB Boot
2. SPI Boot
3. SPI Register/DMA Mode Access
4. I2C Register/DMA Mode Access
5. UART Register/DMA Mode Access
6. GPIO set/get value

1. USB Boot
-----------

By default in the main () USB Boot functionality is enabled. The device enumerates as a vendor 
specific USB device with a pair of bulk endpoints (1-OUT and 1-IN). The data paths for these 
endpoints have not been setup. The user needs to setup the datapath before making use of these
endpoints.

The standard and the vendor requests are handled in the application. The relevant code is in 
the file usb_boot.c and the usb_descriptors.c file contains the descriptors that are used by 
the application.

The application by default connects in the SuperSpeed mode and falls back to lower speeds if 
connected to a different speed port.

The application lets the user to download the final application over control endpoint. After 
the application has been downloaded to the SYSMEM the control is transferred to the application.

By default the application enables the no re-enumeration feature. Corresponding changes to the 
final application are required. The final application is just expected not to set the descriptors
and issue the connect call if the no re-enumeration feature is to be made use of. When this feature
is used the descriptors across the booter and the final application are expected to be same. The 
noReEnum parameter of the function CyFx3BootUsbStart () is to be set to CyFalse if the no 
re-enumeration feature is to be disabled. In this case, no changes to the final application are required.

2. SPI Boot
-----------

The SPI related code has to be enabled in the main () and the Usb related code has to be disabled. 
Comments have been placed in the main () as to where these changes are to be made.

The SPI Boot functionality boots the final application that is stored on the SPI flash.

3. SPI Register/DMA Mode Access
-------------------------------

The file spi_test.c consists of test code that shows how to access the SPI in register/DMA mode.
The function testSpiRegMode () shows how to setup the data transfer to/from SPI using register
mode.
The function testSpiDmaMode () shows how to setup the data transfer to/from SPI using the DMA mode.

These functions are to be invoked from the main (). SPI has to be enabled in the IO Matrix
configuration done as part of the initialization.

4. I2C Register/DMA Mode Access
-------------------------------

The file i2c_test.c consists of test code that shows how to access the I2C in register/DMA mode.
The function testI2cRegMode () shows how to setup the data transfer to/from I2C using register
mode.
The function testI2cDmaMode () shows how to setup the data transfer to/from I2C using the DMA mode.

These functions are to be invoked from the main (). I2C has to be enabled in the IO Matrix
configuration done as part of the initialization.

5. UART Register/DMA Mode Access
--------------------------------

The file test_uart.c consists of test code that shows how to access the UART in register/DMA mode.
The function testUartRegMode () shows how to setup the data transfer to/from UART using register
mode.
The function testUartDmaMode () shows how to setup the data transfer to/from UART using the DMA mode.

These functions are to be invoked from the main (). UART has to be enabled in the IO Matrix
configuration done as part of the initialization.

6. GPIO set/get value
---------------------

The sample application makes use of the GPIOs 45 and 57 to show the usage of the GPIO APIs.
GPIO 45 is configured as an output pin while GPIO 57 is configured in input mode. The example shows
how to set and get values on the GPIOs.

The testGpio () function is to be invoked from the main (). The GPIOs 45 and 57 are to be configured
as simple GPIOs in the IO Matrix.
