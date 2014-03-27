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

# Copy Platform library and ThreadX library files and corresponding
# header files for FX3  

all: compile

CYUSBROOT=../../..
CYPFFXROOT=$(CYUSBROOT)/sdk/bin/fw/$(CYCONFOPT)
CYPFFXINCROOT=$(CYUSBROOT)/sdk/firmware/include
U3PLIBDIR = lib
U3PINCDIR = inc
#This file provides the list of Platform header and ThreadX header files
FX3INCLST= fx3_header_files.txt
FX3LIBLST= fx3_library_files.txt
FX3FWROOT=..
include $(FX3FWROOT)/common/fx3_build_config.mak

clean:	
	
	if (test -d $(U3PINCDIR)); then \
		cd $(U3PINCDIR);\
		rm -f *.h;\
		cd ..;\
		rmdir $(U3PINCDIR);\
	fi;
	if (test -d $(U3PLIBDIR)); then \
		cd $(U3PLIBDIR);\
		rm -rf *;\
		cd ..;\
		rmdir $(U3PLIBDIR);\
	fi;
	
compile:
	#Copy Platform Library files
	if !(test -d $(U3PLIBDIR)); then \
		mkdir $(U3PLIBDIR);\
	fi;
	if !(test -d $(U3PLIBDIR)/$(CYCONFOPT)); then \
        cd $(U3PLIBDIR);\
		mkdir $(CYCONFOPT);\
	fi;
	cat $(CYPFFXINCROOT)/$(FX3LIBLST) | grep .a > ./tmp.inc
	chmod 777 ./tmp.inc
	dos2unix ./tmp.inc
	for i in `cat ./tmp.inc`; do\
		cp -f $(CYPFFXROOT)/$$i ./$(U3PLIBDIR)/$(CYCONFOPT) ;\
	done
	rm -f ./tmp.inc
	#Copy Platform header files	
	if !(test -d $(U3PINCDIR)); then \
		mkdir $(U3PINCDIR);\
	fi;		
	cat $(CYPFFXINCROOT)/$(FX3INCLST) | grep .h > ./tmp.inc
	chmod 777 ./tmp.inc
	dos2unix ./tmp.inc
	for i in `cat ./tmp.inc`; do\
		cp -f $(CYPFFXINCROOT)/$$i ./$(U3PINCDIR) ;\
	done
	rm -f ./tmp.inc

#[]#
