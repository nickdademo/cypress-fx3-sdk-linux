## Copyright Cypress Semiconductor Corporation, 2011-2012,
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

# Tools
ifeq ($(CYTOOLCHAIN), armcc)
CY.ASM      = armasm
CY.CC       = armcc
CY.AR       = armar
else
CY.ASM = arm-none-eabi-gcc
CY.CC  = arm-none-eabi-gcc
CY.AR  = arm-none-eabi-ar
endif

# the common include path
Include	=-I. -I../include -I../../../sdk/firmware/include

# Command flags
ifeq ($(CYTOOLCHAIN), armcc)

ASMFLAGS	= -g --cpu ARM926EJ-S --apcs /interwork		\
	   --pd "CY_USE_ARMCC SETA 1"

CCFLAGS = --cpu ARM926EJ-S --apcs /interwork 		\
	  --strict_warnings -c -g -Ospace -O3		\
	  --signed-chars --split-sections		\
	  -DCY_USE_ARMCC=1				\
	  $(Include)

ARFLAGS     = --create

else

ASMFLAGS = -c -g -mcpu=arm926ej-s -mthumb-interwork	\
	   $(Include)

CCFLAGS	= -c -g -mcpu=arm926ej-s -mthumb-interwork -O1	\
	  -ffunction-sections -fdata-sections		\
	  $(Include)

ARFLAGS = -cr

endif

# Command shortcuts
COMPILE		= $(CY.CC) $(CCFLAGS) -o $@ $<
ASSEMBLE	= $(CY.ASM) $(ASMFLAGS) -o $@ $<
AR  		= $(CY.AR) $(ARFLAGS) $@ $+

#[]
