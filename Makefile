LIBRARIES=libmoberg.so
CCFLAGS+=-Wall -Werror -I$(shell pwd) -g
LDFLAGS+=-L$(shell pwd)/build/ -lmoberg
PLUGINS:=$(wildcard plugins/*)
ADAPTORS:=$(wildcard adaptors/*)
export CCFLAGS LDFLAGS
LDFLAGS_parse_config=-ldl
#-export-dynamic

all: $(LIBRARIES:%=build/%) $(PLUGINS) $(ADAPTORS)
	echo $(PLUGINS)
	echo $(CCFLAGS)

build/libmoberg.so: Makefile | build
	$(CC) -o $@ $(CCFLAGS) -shared -fPIC -I. \
		$(filter %.o,$^) -lxdg-basedir -ldl

build/lib build:
	mkdir -p $@

%:	build/%.o Makefile
	$(CC) $(LDFLAGS) $(LDFLAGS_$(*)) -o $@  $(filter %.o,$^)

build/%.o:	%.c Makefile
	$(CC) $(CCFLAGS) -c -o $@ $<

build/lib/%.o:	%.c Makefile | build/lib
	$(CC) $(CCFLAGS) -c -fPIC -o $@ $<


.PHONY: $(PLUGINS) $(ADAPTORS)
$(ADAPTORS) $(PLUGINS): 
	$(MAKE) -C $@


.PHONY: test
test: all
	$(MAKE) -C test test

clean:
	rm -f build/*.so build/*.mex*
	rm -f *~
	make -C test clean

build/libmoberg.so: build/lib/moberg.o
build/libmoberg.so: build/lib/moberg_config.o
build/libmoberg.so: build/lib/moberg_device.o
build/libmoberg.so: build/lib/moberg_parser.o
build/lib/%.o: %.h
