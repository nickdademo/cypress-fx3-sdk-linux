
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

SYNCHRONOUS SLAVE FIFO 5BIT PROTOCOL EXAMPLE
--------------------------------------------

  This example illustrates the configuration and usage of the GPIF II
  interface on the FX3 device to implement the Synchronous Slave FIFO
  5 Bit protocol.

  A master device that implements the Cypress defined Synchronous Slave FIFO
  5 Bit protocol is required to perform data transfers with this application.

  This application example does the following:

  1. Configures the GPIF II interface to implement the Synchronous Slave FIFO
     5 Bit protocol.

  2. Enumerates as a vendor specific USB device with maximum of 30 bulk endpoints
     (15-OUT and 15-IN) configurable with CY_FX_NUMBER_OF_ADDR_LINES.

  3. The CY_FX_NUMBER_OF_ADDR_LINES defines the number of socket and the
     number of bulk endpoints. The CY_FX_NUMBER_OF_ADDR_LINES can have a
     value from 2 to 5. The number of socket and bulk endpoints are defined
     using the equation:
        Number of sockets = Number of bulk endpoints
                          = ((2 ^ CY_FX_NUMBER_OF_ADDR_LINES) - 2)

  4. Half of the sockets are used for receiving the data from master device
     and other half is used for sending the data. Similarly the half of the
     bulk endpoints is for sending the data through USB and other half is used for
     receiving the data from USB.

     Following is the table of parameters that varies with respect to the
     number of address lines selected:

     /-------------------------------------------------------------------------\
     |   CY_FX_NUMBER_OF_ADDR_LINES  |   2  |     3     |     4     |     5    |
     |   Receive Data Socket         |   1  |    1-3    |    1-7    |    1-15  |
     |   Send Data socket            |   3  |    5-7    |    9-15   |   17-31  |
     |   OUT Bulk Endpoints          | 0x01 | 0x01-0x03 | 0x01-0x07 | 0x01-0x0F|
     |   IN Bulk Endpoints           | 0x81 | 0x81-0x83 | 0x81-0x87 | 0x81-0x8F|
     \-------------------------------------------------------------------------/

  5. Create AUTO DMA channels to enable the following data paths:
     a. All data received from the USB host through the X-OUT endpoint is
        forwarded to the master device on the slave port through socket
        (2 ^ (CY_FX_NUMBER_OF_ADDR_LINES - 1) + X).

        e.g.: If CY_FX_NUMBER_OF_ADDR_LINES = 5, the data received on 1-OUT endpoint
        will be sent through socket 17 to the master device. Similarly, data received
        through the 15-OUT endpoint, will be sent to the master through socket 31.

     b. All data received from the master device on the slave port through
        socket X is forwarded to the USB host through the X-IN endpoint.

        e.g.: Data received on socket 1 will be sent to the USB host on the
        1-IN endpoint. Similarly, data received on socket 15 will be sent
        to the USB host on the 15-IN endpoint.

  Files:

    * cyfx_gcc_startup.S   : Start-up code for the ARM-9 core on the FX3
                             device.  This assembly source file follows the
                             syntax for the GNU assembler.

    * cyfxslfifosync5bit.c : Main source file that implements the example.
                             This initializes all of the relevant FX3 blocks
                             and creates the necessary DMA channels.

    * cyfxslfifosync.h     : C header file that defines constants used by
                             this example implementation. Can be modified
                             to select USB connection speed, endpoint numbers
                             and properties etc.

    * cyfxslfifousbdscr.c  : C source file that contains USB descriptors used
                             by this example.

    * cyfxgpif_syncsf.h    : C header file that contains the data required
                             to configure the GPIF interface to implement the
                             Synchronous Slave FIFO protocol.

    * cyfxtx.c             : C source file that provides ThreadX RTOS wrapper
                             functions and other utilites required by the FX3
                             firmware library.

    * makefile             : GNU make compliant build script for compiling this
                             example.

[]

