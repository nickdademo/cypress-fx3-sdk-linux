
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3S SDK

FX3S File System Integration Example
------------------------------------

  This example illustrates the use of the FX3S APIs to integrate a FAT file
  system. The application creates a basic UART based shell which can be used
  to list, read and write files stored on the storage devices.

  The FatFs File System implementation by Elm Chan (http://elm-chan.org/fsw/ff/00index_e.html)
  is used in this application.

  The block driver required by the file system is implemented using the FX3S storage APIs.

  The UART port is used to implement a primitive shell through which commands are received from
  the user and file operations performed. The commands supported by this application are:

        touch: Create an empty file with the specified name.
        rm   : Remove a single file or an empty directory from the file system.
        mkdir: Create a new directory with the specified name.
        ls   : List the contents of a directory.
        write: Write strings specified on the command line into the specified file (creates a new file if needed).
        read : Read and print the contents of the specified file.
        exit : Exit the shell.
	help : Lists Command Syntax.

  Files:

    * cyfx_gcc_startup.S : Start-up code for the ARM-9 core on the FX3S device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxtx.c           : ThreadX RTOS wrappers and utility functions required
      by the FX3S API library.

    * cyfx3s_fatfs.h     : Constant definitions used in the application.

    * cyfx3s_fatfs.c     : C source file implementing the UART based shell and
      the file system integration code.

    * cyfx3s_blkdrvr.c   : Block driver implementation for SD/MMC access.

    * ff.c               : FatFs file system source by Elm Chan.

    * ff.h               : FatFs file system header file.

    * ffconf.h           : FatFs file system configuration header file.

    * integer.h          : Integer type definitions used by the FatFs file system.

    * diskio.h           : Block driver definition header for the FatFs file system.

    * makefile           : GNU make compliant build script for compiling this example.

[]

