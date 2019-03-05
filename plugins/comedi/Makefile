LIBRARIES=libmoberg_comedi.so
CCFLAGS+=-Wall -Werror -I../.. -g

all:	$(LIBRARIES:%=../../build/%) 

../../build/libmoberg_comedi.so: comedi.c Makefile
	$(CC) -o $@ $(CCFLAGS) -shared -fPIC -lcomedi -L../../build -lmoberg $<

../../build/libmoberg_comedi.so: ../../moberg_module.h
