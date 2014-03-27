## Copyright Cypress Semiconductor Corporation, 2010-2011,
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
#

FX3FWROOT=..

include $(FX3FWROOT)/common/cx3_build_config.mak

all:compile

ifeq ($(CYFXEXOPT),cycx3_uvc)
    EXSUBDIRS=cycx3_uvc_as0260\
	      cycx3_uvc_ov5640
endif

ifeq ($(CYFXEXOPT),cycx3_rgb24)
    EXSUBDIRS=cycx3_rgb24_as0260
endif

ifeq ($(CYFXEXOPT),cycx3_rgb16)
    EXSUBDIRS=cycx3_rgb16_as0260
endif

ifeq ($(CYFXEXOPT),)
    EXSUBDIRS= cycx3_uvc_as0260 \
	       cycx3_uvc_ov5640 \
	       cycx3_rgb24_as0260 \
	       cycx3_rgb16_as0260
endif

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

##[]

