
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3S SDK

SD/MMC BACKED RAID-0 EXAMPLE
----------------------------

  This example illustrates the use of the FX3S firmware APIs to implement
  a RAID-0 system based on SD/MMC storage devices.
  
  The RAID-0 implementation allows two storage devices (similar SD cards
  or eMMC devices) to be used as a higher performance single storage volume.
  The performance improvement is achieved by striping the user data across
  the two devices.
  
  A USB Mass Storage Class (MSC) application that provides access to this
  RAID-0 disk is provided.

  As the RAID-0 system is only viable when two good storage devices are
  available, this application will only function when both devices are
  present. The application does not support any hotplug operation either,
  as replacing one of the devices in a striped volume will cause data
  corruption.

  This application uses a logical block size of 4 KB for the RAID-0 disk.
  As FX3S is not capable of splitting data from a single data buffer into
  two output pipes, the DMA buffer size used for this application has to
  match the desired logical block size. This value can be controlled using
  the CY_FX_RAID_BLOCK_SIZE parameter in the cyfx3s_raid0.h. The minimum
  supported value for this parameter is 1 KB.

  Files:

    * cyfx_gcc_startup.S : Start-up code for the ARM-9 core on the FX3S
                           device. This assembly source file follows the
                           syntax for the GNU assembler.

    * cyfxtx.c           : ThreadX RTOS wrappers and utility functions
                           required by the FX3S API library.

    * cymsc_dscr.c       : C source file containing the USB descriptors
                           that are used by this firmware example.

    * cyfx3s_raid0.h     : Constant definitions for the RAID-0 firmware
                           application.

    * cyfx3s_msc.c       : Main C source file that implements RAID-0 disk
                           and the USB mass storage class function.

    * makefile           : GNU make compliant build script for compiling this
                           example.

[]

