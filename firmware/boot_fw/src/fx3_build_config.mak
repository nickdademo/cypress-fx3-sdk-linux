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

# the common include path
Include	=-I. -I../include 

# the common compiler options
ifeq ($(CYFXBUILD), arm)
CCFLAGS	= -g -Ospace $(Include)
else
CCFLAGS	= -g -Os $(Include)
endif

ifeq ($(CYDEVICE), CYUSB3011)
    CCFLAGS += -DCYMEM_256K
endif

# the common linker options
LDFLAGS	= --entry Reset_Handler $(LDLIBS)

# the required assembly files

LDLIBS = ../lib/cyfx3_boot.a

# now include the compile specific build options
# gcc is the default compiler
ifeq ($(CYFXBUILD), arm)
	include fx3_armrvds_config.mak
else
	include fx3_armgcc_config.mak
endif	

#[]
