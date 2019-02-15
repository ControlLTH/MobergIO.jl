LIBRARIES=libmoberg_comedi.so
CCFLAGS+=-Wall -Werror -I../.. -g
LDFLAGS+=-Lbuild/ -lmoberg
MODULES=$(wildcard modules/*)

all:	$(LIBRARIES:%=../../build/%) 

../../build/libmoberg_comedi.so: comedi.c 
	$(CC) -o $@ $(CCFLAGS) -shared -fPIC $<

../../build/libmoberg_comedi.so: ../../moberg_config_parser.h
