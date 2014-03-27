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

include $(FX3FWROOT)/common/fx3_build_config.mak

all:compile

ifeq ($(CYFXEXOPT),bulklpautoenum)
    EXSUBDIRS=cyfxbulklpautoenum
endif

ifeq ($(CYFXEXOPT),isolpmaninout)
    EXSUBDIRS=cyfxisolpmaninout
endif

ifeq ($(CYFXEXOPT),isolpauto)
    EXSUBDIRS=cyfxisolpauto
endif

ifeq ($(CYFXEXOPT),bulksrcsink)
    EXSUBDIRS=cyfxbulksrcsink
endif

ifeq ($(CYFXEXOPT),bulkstreams)
    EXSUBDIRS=bulkstreams
endif

ifeq ($(CYFXEXOPT),isosrcsink)
    EXSUBDIRS=cyfxisosrcsink
endif

ifeq ($(CYFXEXOPT),isosrc)
    EXSUBDIRS=cyfxisosrc
endif

ifeq ($(CYFXEXOPT),flashprog)
    EXSUBDIRS=cyfxflashprog
endif

ifeq ($(CYFXEXOPT),cyfxbulklpotg)
    EXSUBDIRS=cyfxbulklpotg
endif

ifeq ($(CYFXEXOPT),cyfxusbdebug)
    EXSUBDIRS=cyfxusbdebug
endif
ifeq ($(CYFXEXOPT),cyfxusbhost)
    EXSUBDIRS=cyfxusbhost
endif
ifeq ($(CYFXEXOPT),cyfxusbotg)
    EXSUBDIRS=cyfxusbotg
endif

ifeq ($(CYFXEXOPT),) 
    EXSUBDIRS= cyfxbulklpautoenum \
	       cyfxbulksrcsink \
	       cyfxbulkstreams \
	       cyfxisolpauto \
	       cyfxisolpmaninout \
	       cyfxisosrcsink \
	       cyfxgpiftousb \
	       cyfxisosrc \
	       cyfxflashprog \
	       cyfxbulklpotg \
	       cyfxusbdebug \
	       cyfxusbhost \
	       cyfxusbotg
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

