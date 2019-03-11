LIBRARIES=libmoberg4simulink.so
CCFLAGS+=-Wall -Werror -O3 -I. -I../.. -g

all:	$(LIBRARIES:%=../../build/%)

clean:

../../build/libmoberg4simulink.so: moberg4simulink.c Makefile
	$(CC) -o $@ $(CCFLAGS) -L../../build -shared -fPIC -lmoberg $<

../../build/libmoberg4simulink.so: ../../moberg.h
