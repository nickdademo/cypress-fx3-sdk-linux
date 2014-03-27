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
##      <install>/license/license.txt
##
## where <install> is the Cypress software
## installation root directory path.
##

all:compile

include fx3_build_config.mak

MODULE = cyfx3_boot

LIB_C_SOURCE = cyfx3usb.c cyfx3device.c cyfx3utils.c cyfx3spi.c cyfx3gpio.c cyfx3i2c.c cyfx3uart.c

ifeq ($(CYTOOLCHAIN), armcc)
LIB_ASM_SOURCE = cyfx_armcc_startup.S
else
LIB_ASM_SOURCE = cyfx_gcc_startup.S
endif

C_OBJECT=$(LIB_C_SOURCE:%.c=Objs/%.o)
A_OBJECT=$(LIB_ASM_SOURCE:%.S=Objs/%.o)

DEPLIB = ../../../sdk/firmware/src/fx3lib/source/fx3_release/cyfx3lib.a
LIBS = $(MODULE).a

$(C_OBJECT) : Objs/%.o : %.c
	@mkdir -p Objs
	$(COMPILE)

$(A_OBJECT) : Objs/%.o : %.S
	@mkdir -p Objs
	$(ASSEMBLE)

$(LIBS) : $(A_OBJECT) $(C_OBJECT)
	cd Objs && $(CY.AR) -x ../$(DEPLIB)
	$(CY.AR) $(ARFLAGS) $@ Objs/*.o
	mkdir -p ../lib
	cp -f *.a ../lib/.
	cp -f ../../../sdk/firmware/include/cyu3types.h ../include/.
	cp -f ../../../sdk/firmware/include/cyu3externcstart.h ../include/.
	cp -f ../../../sdk/firmware/include/cyu3externcend.h  ../include/.
	cp -f ../../../sdk/firmware/include/cyfx3_api.h ../include/.

clean:
	rm -rf ./*.o Objs
	rm -f ./*.a
	rm -f ../lib/*.a

compile: $(C_OBJECT) $(A_OBJECT) $(LIBS)

#[#
