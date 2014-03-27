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
##	<install>/license/license.txt
##
## where <install> is the Cypress software
## installation root directory path.
##

CYUSBROOT=../..
CYTHXROOT=$(CYUSBROOT)/sdk/firmware/src/rtos/threadx
CYPFFWROOT=$(CYUSBROOT)/sdk/firmware/src
FX3FWROOT=.
FX3LIBROOT=u3p_firmware
CYFXBUILD=gcc
# What needs to be built
export CYFXEXAMPLE
export CYFXEXOPT

export CYCONFOPT

# the selected toolchain for build (used by the file included below)
export CYFXBUILD
include $(FX3FWROOT)/common/fx3_build_config.mak

all: compile

ifeq ($(CYFXEXAMPLE),uvc)
    FX3SUBDIRS=		\
        uvc_examples
endif

ifeq ($(CYFXEXAMPLE),slavefifo)
    FX3SUBDIRS=		\
        slavefifo_examples
endif

ifeq ($(CYFXEXAMPLE),msc)
    FX3SUBDIRS=		\
	MSC
endif

ifeq ($(CYFXEXAMPLE),basicex)
    FX3SUBDIRS=		\
	basic_examples
endif

ifeq ($(CYFXEXAMPLE),serialex)
    FX3SUBDIRS=		\
	serialif_examples
endif

ifeq ($(CYFXEXAMPLE),dmaex)
    FX3SUBDIRS=		\
	dma_examples
endif

ifeq ($(CYFXEXAMPLE),gpifex)
    FX3SUBDIRS=		\
	gpif_examples
endif

ifeq ($(CYFXEXAMPLE),storageex)
    FX3SUBDIRS=		\
	storage_examples
endif

ifeq ($(CYFXEXAMPLE),cx3ex)
    FX3SUBDIRS=         \
	cx3_examples
endif

ifeq ($(CYFXEXAMPLE),)
    FX3SUBDIRS=			\
	uvc_examples		\
	basic_examples  	\
	dma_examples		\
	slavefifo_examples	\
        serialif_examples 	\
	uac_examples		\
	gpif_examples		\
	storage_examples	\
	MSC			\
	cx3_examples
endif

ifeq ($(CYFXEXAMPLE),all)
    FX3SUBDIRS=			\
	uvc_examples		\
	basic_examples  	\
	dma_examples		\
	slavefifo_examples	\
        serialif_examples 	\
	uac_examples		\
	gpif_examples		\
	storage_examples	\
	MSC			\
	cx3_examples
endif

clean:
	@echo === $@ ... ===
	@date
	@echo ""
	for subdir in $(FX3SUBDIRS); do \
		$(MAKE) -C $$subdir clean;\
	done
	@echo ""
	@date
	@echo === ... $@ ===

apiclean:
	@echo === $@ ... ===
	@date
	@echo ""
	make CYCONFIG=$(CYCONFOPT) -C $(CYPFFWROOT) myclean;
	make -C $(FX3LIBROOT) clean;
	@echo ""
	@date
	@echo === ... $@ ===

apitxclean:
	@echo === $@ ... ===
	@date
	@echo ""
	make CYCONFIG=$(CYCONFOPT) -C $(CYPFFWROOT) clean;
	make -C $(FX3LIBROOT) clean;
	@echo ""
	@date
	@echo === ... $@ ===



eclipse_build:
	for subdir in $(FX3SUBDIRS); do \
		$(MAKE) -C $$subdir $@  ; \
	done

eclipse_clean:
	for subdir in $(FX3SUBDIRS); do \
		$(MAKE) -C $$subdir $@  ; \
	done

compile:
	@echo === $@ ... ===
	@date
	@echo ""
	for subdir in $(FX3SUBDIRS); do \
		$(MAKE) -C $$subdir $@  ; \
	done
	@echo "$$subdir"
	@date
	@echo === ... $@ ===

api:
	@echo === $@ ... ===
	@date
	@echo ""
	make CYCONFIG=$(CYCONFOPT) -C $(CYPFFWROOT);
	make -C $(FX3LIBROOT);
	@echo ""
	@date
	@echo === ... $@ ===

unittest: compile
	@echo === $@ ... ===
	@date	make -C $(FX3LIBROOT) $@;
	@echo ""
	make CYCONFIG=$(CYCONFOPT) -C $(CYPFFWROOT) $@;
	make -C $(FX3LIBROOT) $@;
	for subdir in $(FX3SUBDIRS); do \
		$(MAKE) -C $$subdir $@  ; \
	done
	@echo ""
	@date
	@echo === ... $@ ===

#[]#
