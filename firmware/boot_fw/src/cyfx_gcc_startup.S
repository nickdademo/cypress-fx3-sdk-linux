#  Copyright Cypress Semiconductor Corporation, 2010-2011,
#  All Rights Reserved
#  UNPUBLISHED, LICENSED SOFTWARE.
#
#  CONFIDENTIAL AND PROPRIETARY INFORMATION
#  WHICH IS THE PROPERTY OF CYPRESS.
#
#  Use of this file is governed
#  by the license agreement included in the file
#
#     <install>/license/license.txt
#
#  where <install> is the Cypress software
#  installation root directory path.
#

# Cypress FX3 Firmware Startup code


.section .text
.code 32

.global __main

__main:
	mov	R0, #0
	ldr	R1, =_bss_start
	ldr	R2, =_bss_end
1:	cmp	R1, R2
	strlo	R0, [R1], #4
	blo	1b

	b	main

.end

# []
