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

MODULE=cyu3dma

Include=-I.				\
        -I$(CYFXROOT)/firmware/include


SOURCE_ASM=

SOURCE= 			\
	cyu3dscrpool.c		\
	cyu3descriptor.c	\
	cyu3socket.c		\
	cyu3channel.c		\
	cyu3multichannel.c	\
	cyu3multichannelutils.c \
	cyu3multicast.c		\
	cyu3dma.c		\
	cyu3dmaint.c

#[]#

