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

MODULE = cyfx_boot_app

ifeq ($(CYFXBUILD), arm)
APP_ASM_SOURCE = 
else
APP_ASM_SOURCE = ./cyfx_gcc_startup.S
endif

APP_SOURCE = 			\
	     main.c		\
	     spi_test.c 	\
	     gpio_test.c	\
	     usb_boot.c		\
	     usb_descriptors.c 	\
	     i2c_test.c 	\
	     test_uart.c

APP_OBJECT=$(APP_SOURCE:%.c=./%.o)
APP_ASM_OBJECT=$(APP_ASM_SOURCE:%.S=./%.o)

EXES = $(MODULE).$(EXEEXT)

$(APP_ASM_OBJECT) : %.o : %.S 
	$(ASSEMBLE)

$(APP_OBJECT) : %.o : %.c 
	$(COMPILE)

$(MODULE).$(EXEEXT): $(APP_OBJECT) $(APP_ASM_OBJECT) 
	$(LINK)

clean:
	rm -f ./$(MODULE).$(EXEEXT)
	rm -f ./$(MODULE).map
	rm -f ./*.o
ifeq ($(CYFXBUILD), gcc)
	rm -f ./gcceclipse_files/*.o
endif


compile: $(APP_OBJECT) $(APP_ASM_OBJECT) $(EXES)

#[]#
