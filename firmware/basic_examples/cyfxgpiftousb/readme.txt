
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

GPIF to USB Transfer Example
----------------------------

  This example illustrates the use of a DMA AUTO channel to continuously
  transfer data from the GPIF port to the USB host.

  A stub GPIF state machine which does not require any external devices
  is used to continuously commit data into the DMA buffers. This state
  machine continues to push data into the DMA channel whenever the thread
  is in the ready state.

  The device enumerates as a vendor specific USB device with one Bulk-IN
  endpoint. The data committed by the GPIF state machine is continuously
  streamed to this endpoint without any firmware intervention.

  Files:

    * cyfx_gcc_startup.S    : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxgpiftousb.h       : Default constant definitions for this firmware
      examples including endpoint number, burst size, number and size of the
      buffers.

    * cyfxgpiftousb.c       : Source file implementing the GPIF to USB transfer
      example.
      
    * cyfxbulkdscr.c        : C source file containing the USB descriptors that
      are used by this firmware example. VID and PID is defined in this file.

    * cyfxgpif2config.h     : GPIF II Designer generated header file that implements
      the state machine used by this example.

    * cyfxtx.c              : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * makefile              : GNU make compliant build script for compiling this
      example.

    * continuous_read.cydsn : Folder containing the GPIF II Designer project used
      in this example.

  PERFORMANCE OPTIMIZATIONS

  As the GPIF state machine used here continues to commit data to the DMA channel
  whenever possible, the transfer performance depends on the capabilities of the
  USB host and on transfer parameters such as endpoint burst length; and the amount
  of data buffering available.

  The following constants defined in the cyfxgpiftousb.h header file are used to
  vary these parameters and study their impact on transfer rate. It is also possible
  to vary these parameters on the compiler command line by adding them to the list
  of pre-processor definitions.

  1. CY_FX_EP_BURST_LENGTH  : Burst length for the endpoint in terms of 1 KB packets.
                              This is set to 8 KB by default.
  2. CY_FX_DMA_BUF_SIZE     : Size of each DMA buffer used on the DMA channel.
                              This is set to 16 KB by default.
  3. CY_FX_DMA_BUF_COUNT    : Number of DMA buffers used for the DMA channel.
                              This is set to 4 buffers by default.

[]

