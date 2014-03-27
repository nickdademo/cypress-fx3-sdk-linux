## Copyright Cypress Semiconductor Corporation, 2010-2012,
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

FX3FWROOT=..

include $(FX3FWROOT)/common/fx3_build_config.mak

ifeq ($(CYFXOPT),fx3sfs)
    EXSUBDIRS=cyfx3s_fatfs
endif

ifeq ($(CYFXOPT),fx3ssdio)
    EXSUBDIRS=cyfx3s_sdiouart
endif

ifeq ($(CYFXOPT),)
    EXSUBDIRS=cyfx3s_fatfs \
	      cyfx3s_sdiouart
endif

all:compile

clean:
	rm -f $(FX3FWROOT)/common/*.o
	for subdir in $(EXSUBDIRS); do \
		$(MAKE) -C $$subdir clean; \
		done

compile:
	for subdir in $(EXSUBDIRS); do \
		$(MAKE) -C $$subdir $@; \
		done

eclipse_build:
	for subdir in $(EXSUBDIRS); do \
		cp $(FX3FWROOT)/common/cyfx_gcc_startup.S $$subdir/. ;\
		cp $(FX3FWROOT)/common/cyfxtx.c $$subdir/. ;\
		done

eclipse_clean:
	for subdir in $(EXSUBDIRS); do \
		rm -f $$subdir/cyfx_gcc_startup.S ;\
		rm -f $$subdir/cyfxtx.c ;\
		rm -f $$subdir/Debug/* ;\
		rmdir  $$subdir/Debug ;\
		rm -f $$subdir/Release/* ;\
		rmdir  $$subdir/Release ;\
		done

#[]#

