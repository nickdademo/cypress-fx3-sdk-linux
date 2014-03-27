
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB I2C (DMA MODE) EXAMPLE
----------------------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a USB I2C EEPROM programming example.

  The device enumerates as a vendor specific USB device with only the control
  endpoint and provides a set of vendor commands to read/write the data on
  I2C EEPROM devices.

  Files:

    * cyfx_gcc_startup.S   : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxusbi2cdmamode.h  : Constant definitions for the USB I2C application.
      The USB connection speed, numbers and properties of the endpoints etc.
      can be selected through definitions in this file.

    * cyfxusbenumdscr.c    : C source file containing the USB descriptors that
      are used by this firmware example. VID and PID is defined in this file.

    * cyfxtx.c             : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxusbi2cdmamode.c  : Main C source file that implements the USB I2C application
      example.

    * makefile             : GNU make compliant build script for compiling this
      example.

   1. Read Firmware ID
      bmRequestType = 0xC0
      bRequest      = 0xB0
      wValue        = 0x0000
      wIndex        = 0x0000
      wLength       = 0x0008

      Data response = "fx3 i2c"

   2. Write to I2C EEPROM
      bmRequestType = 0x40
      bRequest      = 0xBA
      wValue        = I2C EEPROM Slave Address (Can be in the 0 to 7 range, must be set according to the 
                      EEPROM address switch SW40)
      wIndex        = EEPROM byte address (can vary from 0x0000 to 0xFFFF. The max address is capped by 
                      the EEPROM max size)
      wLength       = Length of data to be written (Should be a multiple of 64 and less than or equal to 4096)

      Data phase should contain the actual data to be written.

   3. Read from I2C EEPROM
      bmRequestType = 0xC0
      bRequest      = 0xBB
      wValue        = I2C EEPROM Slave Address (Can be in the 0 to 7 range, must be set according to the 
                      EEPROM address switch SW40)
      wIndex        = EEPROM byte address (can vary from 0x0000 to 0xFFFF. The max address is capped by 
                      the EEPROM max size)
      wLength       = Length of data to be read (Should be a multiple of 64 and less than or equal to 4096)

      Data phase will contain the data read from the EEPROM

Note:- bmRequestType, bRequest, wValue, wIndex, wLength are fields of setup packet. Refer USB 
       specification for understanding format of setup data. 

[]

