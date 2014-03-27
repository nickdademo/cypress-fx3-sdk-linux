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
##

FX3FWROOT=../../..
FX3PFWROOT=../../../u3p_firmware
ELF2IMGPATH=./../../../../../sdk/bin/util/elf2img/debug/

all:compile

include $(FX3FWROOT)/common/fx3_build_config.mak

MODULE = cyfxmasterxilinx

SOURCE += $(MODULE).c

C_OBJECT=$(SOURCE:%.c=./%.o)
A_OBJECT=$(SOURCE_ASM:%.S=./%.o)

EXES = $(MODULE).$(EXEEXT)

$(MODULE).$(EXEEXT): $(A_OBJECT) $(C_OBJECT)
	$(LINK)
	$(ELF2IMGPATH)/elf2img -i $(MODULE).axf -o cywbgpif.img

$(C_OBJECT) : %.o : %.c
	$(COMPILE)

$(A_OBJECT) : %.o : %.S
	$(ASSEMBLE)

clean:
	rm -f ./$(MODULE).$(EXEEXT)
	rm -f ./$(MODULE).map
	rm -f ./*.o
	rm -f ./cywbgpif.img

compile: $(C_OBJECT) $(A_OBJECT) $(EXES)

#[]#
