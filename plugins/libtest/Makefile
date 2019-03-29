LIBRARIES=libmoberg_libtest.so
CCFLAGS+=-Wall -Werror -I../.. -I. -O3 -g -fPIC
LDFLAGS+=-Lbuild/ -lmoberg
LDFLAGS_libtest=-shared -fPIC -L../../build -lmoberg  

all:	$(LIBRARIES:%=build/%)

build/libmoberg_%.so: build/%.o Makefile | build
	$(CC) $(LDFLAGS) $(LDFLAGS_$(*)) -o $@ $(filter %.o,$^)

.PRECIOUS: build/%.o
build/%.o:      %.c Makefile | build
	$(CC) $(CCFLAGS) -c -o $@ $<

build:
	mkdir -p $@

clean:
	rm -f build/*

build/libtest.o: ../../moberg_module.h
