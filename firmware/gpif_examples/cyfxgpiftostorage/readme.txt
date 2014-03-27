
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3S SDK

GPIF to SD/MMC Storage Access Example
-------------------------------------

  This example illustrates the use of the FX3S firmware APIs to allow an
  external processor to access SD/MMC devices through the GPIF port.
  
  A 16-bit Async. ADMux interface is used on the GPIF side, and the P-port
  mailbox registers are used to receive control requests from the external
  processor. The firmware interprets the various requests received, and
  initiates data transfers that are requested by the processor.

  Files:

    * cyfx_gcc_startup.S    : Start-up code for the ARM-9 core on the FX3S
      device. This assembly source file follows the syntax for the GNU
      assembler.

    * cyfxgpiftostorage.h   : Type and constant definitions for this example.

    * cyfxgpiftostorage.c   : Source file implementing the GPIF to SD/MMC
      accesses.

    * cyfxgpif_asyncadmux.h : GPIF II Designer generated configuration for the
      16-bit Async ADMux interface.

    * cyfxtx.c              : ThreadX RTOS wrappers and utility functions
      required by the FX3S API library.

    * makefile              : GNU make compliant build script for compiling
      this example.

[]

