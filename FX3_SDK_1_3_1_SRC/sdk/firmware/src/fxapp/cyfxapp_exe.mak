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
##      <install>/license/license.txt
##
## where <install> is the Cypress software
## installation root directory path.
## 

MODULE=cyfxapp

Include+=-I. \
	-I$(CYFXROOT)/firmware/include

ifeq ($(CYTOOLCHAIN), armcc)
SOURCE_ASM= \
	cyfx_armcc_startup.S
else
SOURCE_ASM= \
	cyfx_gcc_startup.S
endif
	    
SOURCE= \
	cyfx_threadx.c \
	cyfxmain.c

##[]

