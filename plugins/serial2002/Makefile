LIBRARIES=libmoberg_serial2002.so
CCFLAGS+=-Wall -Werror -I../.. -g
LDFLAGS+=-Lbuild/ -lmoberg
MODULES=$(wildcard modules/*)

all:	$(LIBRARIES:%=../../build/%) 

../../build/libmoberg_serial2002.so: serial2002.c Makefile
	$(CC) -o $@ $(CCFLAGS) -shared -fPIC -L../../build -lmoberg $<

../../build/libmoberg_serial2002.so: ../../moberg_module.h
