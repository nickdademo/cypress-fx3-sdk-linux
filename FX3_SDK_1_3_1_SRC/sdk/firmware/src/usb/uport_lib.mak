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

MODULE=cyu3uport

Include=-I.\
        -I$(CYFXROOT)/firmware/include

SOURCE= \
	cyu3usb.c \
	cyu3usbpp.c \
	cyu3usbhost.c \
	cyu3usbotg.c \
	cyu3usbutil.c

##[]
