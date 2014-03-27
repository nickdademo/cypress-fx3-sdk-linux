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

MODULE=cyfxapi

Include+=-I. \
	-I$(CYFXROOT)/firmware/include

ifeq ($(CYTOOLCHAIN), armcc)
SOURCE_ASM= \
	cyu3entry_armcc.S \
	cyu3isr_armcc.S
else
SOURCE_ASM= \
	cyu3entry_gcc.S \
	cyu3isr_gcc.S
endif

SOURCE= \
	cyu3vic.c \
	cyu3device.c \
	cyu3mmu.c \
	cyu3iocfg.c \
	cyu3debug.c \
	cyu3utils.c \
	cyu3system.c \
	cyfx3stor.c \
	cyu3sibint.c \
	cyu3lpp.c

##[]

