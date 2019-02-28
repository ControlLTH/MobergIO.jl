LIBRARIES=libmoberg_comedi.so
CCFLAGS+=-Wall -Werror -I../.. -g
MODULES=$(wildcard modules/*)

all:	$(LIBRARIES:%=../../build/%) 

../../build/libmoberg_comedi.so: comedi.c Makefile
	$(CC) -o $@ $(CCFLAGS) -shared -fPIC -lcomedi $<

../../build/libmoberg_comedi.so: ../../moberg_module.h
