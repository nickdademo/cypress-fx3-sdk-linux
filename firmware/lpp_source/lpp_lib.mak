## Copyright Cypress Semiconductor Corporation, 2008-2009,
## All Rights Reserved
## UNPUBLISHED, LICENSED SOFTWARE.
##
## CONFIDENTIAL AND PROPRIETARY INFORMATION 
## WHICH IS THE PROPERTY OF CYPRESS.
##
## Use of this file is governed 
## by the license agreement included in the file 
##
##      <install>/license/license.txt
##
## where <install> is the Cypress software
## installation root directory path.
## 

Include	=-I. \
		-I$(FX3PFWROOT)/inc

# Default option for Config is fx3_debug
ifeq ($(CYCONFOPT),)
        CYCONFOPT=fx3_debug
endif

CYFXBUILD ?= gcc
CCFLAGS	= 

ifeq ($(CYFXBUILD), g++)
CCFLAGS	+= -g -O0 -DTX_ENABLE_EVENT_TRACE -DDEBUG -DCYU3P_FX3=1	\
			-D__CYU3P_TX__=1 $(Include)
endif
ifeq ($(CYFXBUILD), gcc)
CCFLAGS	+= -g -O0 -DTX_ENABLE_EVENT_TRACE -DDEBUG -DCYU3P_FX3=1	\
		-D__CYU3P_TX__=1 $(Include)
endif
ifeq ($(CYFXBUILD), arm)
# the common compiler options
CCFLAGS	+= -g --cpu ARM926EJ-S --apcs /interwork -O0		\
	   --strict_warnings --diag_suppress 667		\
		-DTX_ENABLE_EVENT_TRACE -DDEBUG -DCYU3P_FX3=1	\
		-D__CYU3P_TX__=1 $(Include)
endif

# now include the compile specific build options
# arm compiler is default
ifeq ($(CYFXBUILD), gcc)
	include $(FX3FWROOT)/common/fx3_armgcc_config.mak
endif
ifeq ($(CYFXBUILD), g++)
	include $(FX3FWROOT)/common/fx3_armg++_config.mak
endif
ifeq ($(CYFXBUILD), arm)
	include $(FX3FWROOT)/common/fx3_armrvds_config.mak
endif	
ifeq ($(CYFXBUILD),)
	include $(FX3FWROOT)/common/fx3_armgcc_config.mak
endif

##[]

