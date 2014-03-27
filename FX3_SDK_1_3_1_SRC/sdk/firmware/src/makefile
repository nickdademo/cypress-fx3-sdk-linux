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

CYFXROOT=../..

all: compile

-include $(CYFXROOT)/build/fwconfig/$(CYCONFIG).mak
include $(CYFXROOT)/build/cyu3fw_common.mak

SUBDIRS=		\
	rtos		\
	dma		\
	lpp		\
	pport		\
	storage		\
	usb		\
	system		\
	fxapp

clean:
	@echo === $@ ... ===
	@date
	@echo ""
	for subdir in $(SUBDIRS); do \
		$(MAKE) -C $$subdir clean;\
	done
	@echo ""
	@date
	@echo === ... $@ ===

compile:
	@echo === $@ ... ===
	@date
	@echo ""
	for subdir in $(SUBDIRS); do \
		$(MAKE) -C $$subdir $@  ; \
	done
	@echo ""
	@date
	@echo === ... $@ ===

unittest: compile
	@echo === $@ ... ===
	@date
	@echo ""
	for subdir in $(SUBDIRS); do \
		$(MAKE) -C $$subdir $@  ; \
	done
	@echo ""
	@date
	@echo === ... $@ ===

#[]#

