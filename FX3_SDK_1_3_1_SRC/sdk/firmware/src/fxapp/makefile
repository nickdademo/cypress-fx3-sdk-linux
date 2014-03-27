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
##      <install>/license/license.txt
##
## where <install> is the Cypress software
## installation root directory path.
## 

CYFXROOT=../../..

all:compile

-include $(CYFXROOT)/build/fwconfig/$(CYCONFIG).mak
include cyfxapp_exe.mak
include $(CYFXROOT)/build/cyu3fw_common.mak

LDLIBS= \
	$(CYU3POUTDIR)/cyu3lpp.a \
	$(CYU3POUTDIR)/cyfxapi.a \
	$(CYU3POUTDIR)/cyu3threadx.a

LIBS =
EXES = $(CYCONFIG)/$(MODULE).$(EXEEXT)

$(CYCONFIG)/$(MODULE).$(EXEEXT):$(OBJECT) $(LDLIBS)
	$(LinkProgram)

clean: cleanconfig
compile:$(LIBS) $(EXES)
unittest: compile
everything: compile unittest

##[]

