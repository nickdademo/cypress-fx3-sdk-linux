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

MODULE=cyu3lpp

Include=-I.\
        -I$(CYFXROOT)/firmware/include

SOURCE= cyu3i2s.c \
	cyu3gpio.c \
	cyu3gpiocomplex.c \
	cyu3uart.c \
	cyu3i2c.c \
	cyu3spi.c


##[]

