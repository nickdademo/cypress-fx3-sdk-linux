all:
	g++ -fPIC -o lib/libcyusb.o -c lib/libcyusb.c
	g++ -shared -Wl,-soname,libcyusb.so -o lib/libcyusb.so.1 lib/libcyusb.o -l usb-1.0 -l rt
	cd lib; ln -sf libcyusb.so.1 libcyusb.so
	rm -f lib/libcyusb.o
clean:
	rm -f lib/libcyusb.so lib/libcyusb.so.1
help:
	@echo	'make		would compile and create the library and create a link'
	@echo	'make clean	would remove the library and the soft link to the library (soname)'
