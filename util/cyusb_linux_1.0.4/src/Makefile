all:
	mkdir -p ../bin
	g++ -o ../bin/00_fwload          00_fwload.c          -L ../lib -l cyusb
	g++ -o ../bin/01_getdesc         01_getdesc.c         -L ../lib -l cyusb
	g++ -o ../bin/03_getconfig       03_getconfig.c       -L ../lib -l cyusb
	g++ -o ../bin/04_kerneldriver    04_kerneldriver.c    -L ../lib -l cyusb
	g++ -o ../bin/05_claiminterface  05_claiminterface.c  -L ../lib -l cyusb
	g++ -o ../bin/06_setalternate    06_setalternate.c    -L ../lib -l cyusb
	g++ -o ../bin/08_cybulk          08_cybulk.c          -L ../lib -l cyusb -l pthread
	g++ -o ../bin/config_parser      config_parser.c      -L ../lib -l cyusb
	g++ -o ../bin/cyusbd             cyusbd.c             -L ../lib -l cyusb
	g++ -o ../bin/getconfig	  	 getconfig.c          -L ../lib -l cyusb
	g++ -o ../bin/download_fx2       download_fx2.c       -L ../lib -l cyusb
	g++ -o ../bin/download_fx3       download_fx3.c       -L ../lib -l cyusb
clean:
	rm -f ../bin/00_fwload ../bin/01_getdesc ../bin/03_getconfig ../bin/04_kerneldriver ../bin/05_claiminterface ../bin/06_setalternate ../bin/07_bulkreader ../bin/07_bulkwriter
	rm -f ../bin/08_cybulk ../bin/config_parser ../bin/cyusbd ../bin/getconfig ../bin/download_fx2 ../bin/download_fx3

help:
	@echo	'make		would compile all source programs in this directory
	@echo	'make clean	would remove all object code in this directory