
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

UART (DMA MODE) LOOPBACK EXAMPLE
----------------------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a simple UART loopback application.

  The loopback is achieved with the help of a DMA MANUAL Channel. DMA MANUAL 
  Channel is created between the UART producer socket and the UART consumer 
  socket. Upon every reception of data in the DMA buffer from the host, the 
  CPU is signalled using DMA callbacks. The CPU then commits the DMA buffer 
  received so that the data is transferred to the consumer socket.

  The above loopback mechanism is implemented using UART DMA mode of 
  operation. Since the UART RX will commit the data to UART TX only after the
  DMA buffer is filled, the remote UART receiver will only get the loopback
  data after receiving CY_FX_UART_DMA_BUF_SIZE bytes. The example supports
  32 byte data loopback.
 
  Files:

    * cyfx_gcc_startup.S   : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxtx.c             : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxuartlpdmamode.c  : Main C source file that implements the UART application
      example.

    * makefile             : GNU make compliant build script for compiling this
      example.

[]

