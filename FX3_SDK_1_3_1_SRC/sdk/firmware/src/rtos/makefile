## Copyright Cypress Semiconductor Corporation, 2010-13,
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

all:copy
compile:copy
everything:copy

include $(CYFXROOT)/build/fwconfig/$(CYCONFIG).mak
include $(CYFXROOT)/build/cyu3fw_common.mak

# Just copy the library to the target folder.
copy: $(CYCONFIG)/cyu3threadx.a
	$(AT) mkdir -p $(CYU3POUTDIR)
	cp $(CYCONFIG)/cyu3threadx.a $(CYU3POUTDIR)
	chmod +w $(CYU3POUTDIR)/cyu3threadx.a

# Nothing to do for clean or unit test.
clean:
myclean:
unittest:
	true

##[]

