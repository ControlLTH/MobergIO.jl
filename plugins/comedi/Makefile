LIBRARIES=libmoberg_comedi.so
CCFLAGS+=-Wall -Werror -I../.. -I. -O3 -g -fPIC
LDFLAGS+=-Lbuild/ -lmoberg
LDFLAGS_comedi=-shared -fPIC -L../../build -lmoberg -lcomedi -lm  

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

build/comedi.c: ../../moberg.h
build/comedi.o: ../../moberg_config.h
build/comedi.o: ../../moberg_device.h
build/comedi.o: ../../moberg_inline.h
build/comedi.o: ../../moberg_module.h
build/comedi.o: ../../moberg_parser.h
