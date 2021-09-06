LIBRARIES=libmoberg_serial2002.so
CCFLAGS+=-Wall -Werror -I../.. -I. -O3 -g -fPIC
LDFLAGS+=-Lbuild/ -lmoberg
LDFLAGS_serial2002=-shared -fPIC -L../../build -lmoberg  

all:	$(LIBRARIES:%=build/%)

build/libmoberg_%.so: build/%.o Makefile | build
	$(CC) -o $@ $(filter %.o,$^) $(LDFLAGS) $(LDFLAGS_$(*))

.PRECIOUS: build/%.o
build/%.o:      %.c Makefile | build
	$(CC) $(CCFLAGS) -c -o $@ $<

build:
	mkdir -p $@

clean:
	rm -rf build

build/serial2002.o: ../../moberg_module.h
build/serial2002.o: serial2002_lib.h
build/serial2002_lib.o: serial2002_lib.h
build/libmoberg_serial2002.so: build/serial2002.o
build/libmoberg_serial2002.so: build/serial2002_lib.o
