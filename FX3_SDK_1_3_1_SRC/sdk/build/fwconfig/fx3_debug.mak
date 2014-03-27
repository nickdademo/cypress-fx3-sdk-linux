##
## Copyright Cypress Semiconductor Corporation, 2010-2013,
## All Rights Reserved
## UNPUBLISHED, LICENSED SOFTWARE.
##
## CONFIDENTIAL AND PROPRIETARY INFORMATION
## WHICH IS THE PROPERTY OF CYPRESS.
##
## Use of this file is governed
## by the license agreement included in the file
##
##	<install>/license/license.txt
##
## where <install> is the Cypress software
## installation root directory path.
##

###
### FX3 firmware library - debug configuration
###

# Tools
ifeq ($(CYTOOLCHAIN), armcc)

CY.ASM      = armasm
CY.CC       = armcc
CY.AR       = armar
CY.LD	    = armlink $(LDFLAGS)

else

CY.ASM      = arm-none-eabi-gcc
CY.CC       = arm-none-eabi-gcc
CY.AR       = arm-none-eabi-ar
CY.LD	    = arm-none-eabi-ld $(LDFLAGS)

endif

# Command flags.
ifeq ($(CYTOOLCHAIN), armcc)

ASMFLAGS = --cpreproc -g --cpu ARM926EJ-S --apcs /interwork	\
	   --pd "CYU3P_FX3 SETA (1)" 				\
	   --pd "CYU3P_SILICON SETA (1)"			\
	   --pd "CYU3P_DEBUG SETA (1)"				\
	   --pd "CY_USE_ARMCC SETA (1)"

CCFLAGS  = -g --cpu ARM926EJ-S --apcs /interwork -O0		\
	   --strict_warnings --diag_suppress 667		\
	   --split-sections					\
	   -DCYU3P_FX3=1 -DCYU3P_SILICON=1                	\
	   -DTX_ENABLE_STACK_CHECKING=1				\
	   -DCYU3P_STORAGE_SDIO_SUPPORT=1			\
	   -DDEBUG -DCYU3P_DEBUG -DCY_USE_ARMCC=1		\
	   -D__CYU3P_TX__=1 $(Include)

LIBEXT	= a
EXEEXT	= axf

LDFLAGS	= -debug --elf --remove					\
	  --scatter $(CYFXROOT)/build/fx3.scat			\
	  --entry CyU3PFirmwareEntry

## All required libraries are provided by the linker.
DEFLIBS = 

else

ASMFLAGS = -c -g -mcpu=arm926ej-s -mthumb-interwork		\
	   -DDEBUG -DCYU3P_DEBUG				\
	   -DCYU3P_FX3=1 -DCYU3P_SILICON=1			\
	   $(Include)

CCFLAGS  = -g -mcpu=arm926ej-s -mthumb-interwork -O0		\
	   -ffunction-sections -fdata-sections			\
	   -DCYU3P_FX3=1 -DCYU3P_SILICON=1                	\
	   -DTX_ENABLE_STACK_CHECKING=1				\
	   -DCYU3P_STORAGE_SDIO_SUPPORT=1			\
	   -DDEBUG -DCYU3P_DEBUG				\
	   -D__CYU3P_TX__=1 $(Include)

LIBEXT		= a
EXEEXT		= elf
LDFLAGS		= --gc-sections	-d --no-wchar-size-warning 	\
		  -T $(CYFXROOT)/build/fx3.ld 			\
		  --entry CyU3PFirmwareEntry

## Default libraries provided by the compiler tool-chain.
DEFLIBS = -L"$$ARMGCC_INSTALL_PATH"/arm-none-eabi/lib				\
	  -L"$$ARMGCC_INSTALL_PATH"/lib/gcc/arm-none-eabi/$(ARMGCC_VERSION) 	\
	  -lgcc -lc

endif

CYDEVICE	= fx3
CYPPORTIF	= gpif

# Command shortcuts
ifeq ($(CYTOOLCHAIN), armcc)

MAKEDEPEND      = $(CY.CC) $(CCFLAGS) --depend=$(df).d -o $@ -E $<
BuildLibrary	= echo "Creating library $@" ; rm -f $@ ; $(CY.AR) --create $@ $+
LinkProgram	= $(CY.LD) -o $@ $+ ; echo $(df)

else

MAKEDEPEND      = $(CY.CC) -M $(CCFLAGS) -o $(df).d $<
BuildLibrary	= echo "Creating library $@" ; rm -f $@ ; $(CY.AR) -cr $@ $+
LinkProgram	= $(CY.LD) -o $@ $+ $(DEFLIBS)

endif

#[]

