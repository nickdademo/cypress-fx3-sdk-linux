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
##	<install>/license/license.txt
##
## where <install> is the Cypress software
## installation root directory path.
## 

# Location for the .d dependency files.
DEPDIR = $(CYCONFIG)/.deps
df = $(DEPDIR)/$*

# Get list of C and Assembly output files.
OBJECT=$(SOURCE:%.c=$(CYCONFIG)/%.o)
OBJECT+=$(SOURCE_ASM:%.S=$(CYCONFIG)/%.o)

CYU3POUTDIR=$(CYFXROOT)/../../firmware/u3p_firmware/lib/$(CYCONFIG)

ifndef CYVERBOSE
AT := @
COMPILE_MSG = @echo "Compiling $<"
endif

ifndef CYDCACHE
CYDCACHE = on
endif

ifndef CYPREFETCH
CYPREFETCH = on
endif

checkconfig :: checkplatform

##
## Check that a valid configuration has been specified and
## exit if it has not.  Print out valid CYCONFIG.
##
checkplatform ::
	@if [ ! -f $(CYFXROOT)/build/fwconfig/$(CYCONFIG).mak ]; then          		\
	  echo " ";                                                                	\
	  echo "Configuration '$(CYCONFIG)' is not valid." ;                       	\
	  echo " ";                                                                	\
	  echo "Valid configurations are:" ;                                       	\
	  echo "*****************************************************************";	\
	  cd $(CYFXROOT)/build/fwconfig; ls -C *.mak | sed -e "s-.mak-    -g"; 		\
	  echo "*****************************************************************";	\
	  echo " " ;                                                               	\
	  echo " Example:  make CYCONFIG=fx3_debug   " ;                  	      	\
	  echo " " ;                                                               	\
	  exit 1 ;                                                                 	\
	fi

NOW::;

## Common Clean Targets

cleanall: cleanallconfigs

cleanallconfigs:
	@ ls $(CYFXROOT)/build/fwconfig | sed -n s/.mak$$//p | xargs -i rm -rf {}

cleanconfig: checkconfig
	@ echo "Cleaning for config $(CYCONFIG) ...."
	@ rm -rf $(CYCONFIG)
	@ echo "... done"

##  -------------------------------------------------------------------
## NB: Here's where the automagic dependency generation occurs.
##  -------------------------------------------------------------------
## How it works:
##     Use the compiler to generate the primary dependencies into $(df).d,
##         for example, "X : Y Z".
##     Postprocess $(df).d into $(df).P to add an "empty" dependency
##         for each depended-upon target, eg, add "Y Z :", 
##         so that if Y or Z is removed, gmake does not complain.
##     Use gmake -include (note the dash) to include the dependencies,
##         so that if they do not exist, there is no complaint.
##     See also http://sources.redhat.com/automake/dependencies.html
##     See also http://make.paulandlesley.org/autodep.html
##  -------------------------------------------------------------------
$(CYCONFIG)/%.o : %.c
	@ mkdir -p $(CYCONFIG)
	@ mkdir -p  $(CYCONFIG)/$(<D)
	@ test -d $(DEPDIR) || mkdir -p $(DEPDIR)
	@ test -d $(DEPDIR)/$(<D) || mkdir -p $(DEPDIR)/$(<D)
	@$(MAKEDEPEND); \
	    dos2unix $(df).d 2>/dev/null ; \
	    grep -v -E "Program Files" $(df).d | grep -v -E "program.*files" > $(df).P ; \
	    echo >> $(df).P ; \
	    cp $(df).P $(df).d ; \
	    sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	        -e '/^$$/ d' -e 's/$$/ :/' < $(df).d >> $(df).P; \
	    rm -f $(df).d
	$(COMPILE_MSG)
	$(AT) $(CY.CC) $(CCFLAGS) $(CYCFLAGS) -c $< -o $@

$(CYCONFIG)/%.o : %.S
	@ mkdir -p $(CYCONFIG)
	@ mkdir -p  $(CYCONFIG)/$(<D)
	$(COMPILE_MSG)
	$(AT) $(CY.ASM) $(ASMFLAGS) $(CYASMFLAGS) $< -o $@

# Unit-tests are not implemented as yet.
unittest: compile

# Before compiling, verify that the CYCONFIG specified is valid.
compile: checkconfig

## Include the generated dependencies.
-include $(SOURCE:%.c=$(DEPDIR)/%.P)

##[]

