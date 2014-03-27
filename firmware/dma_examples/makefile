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

ifeq ($(CYFXEXOPT),bulklpauto)
    EXSUBDIRS=cyfxbulklpauto
endif

ifeq ($(CYFXEXOPT),bulklpautosig)
    EXSUBDIRS=cyfxbulklpautosig
endif

ifeq ($(CYFXEXOPT),bulklpmanual)
    EXSUBDIRS=cyfxbulklpmanual
endif

ifeq ($(CYFXEXOPT),bulklpmaninout) 
    EXSUBDIRS=cyfxbulklpmaninout
endif

ifeq ($(CYFXEXOPT),bulklpautomanytoone)
    EXSUBDIRS=cyfxbulklpautomanytoone
endif

ifeq ($(CYFXEXOPT),bulklpautoonetomany)
    EXSUBDIRS=cyfxbulklpautoonetomany
endif

ifeq ($(CYFXEXOPT),bulklpmanmanytoone)
    EXSUBDIRS=cyfxbulklpmanmanytoone
endif

ifeq ($(CYFXEXOPT),bulklpmanonetomany)
    EXSUBDIRS=cyfxbulklpmanonetomany
endif

ifeq ($(CYFXEXOPT),bulklpmandcache)
    EXSUBDIRS=cyfxbulklpmandcache
endif

ifeq ($(CYFXEXOPT),bulklpmulticast)
    EXSUBDIRS=cyfxbulklpmulticast
endif

ifeq ($(CYFXEXOPT),cyfxbulklplowlevel)
    EXSUBDIRS=cyfxbulklplowlevel
endif

ifeq ($(CYFXEXOPT),cyfxbulklpman_addition)
    EXSUBDIRS=cyfxbulklpman_addition
endif

ifeq ($(CYFXEXOPT),cyfxbulklpman_removal)
    EXSUBDIRS=cyfxbulklpman_removal
endif

ifeq ($(CYFXEXOPT),) 
    EXSUBDIRS= cyfxbulklpauto \
	       cyfxbulklpautosig \
	       cyfxbulklpmanual \
	       cyfxbulklpmaninout \
	       cyfxbulklpautomanytoone \
	       cyfxbulklpautoonetomany \
	       cyfxbulklpmanmanytoone \
	       cyfxbulklpmanonetomany \
	       cyfxbulklpmandcache \
	       cyfxbulklpmulticast \
	       cyfxbulklplowlevel \
	       cyfxbulklpman_addition \
	       cyfxbulklpman_removal

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

