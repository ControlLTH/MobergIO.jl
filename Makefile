LIBRARIES=libmoberg.so
MOBERG_VERSION=$(shell git describe --tags | sed -e 's/^v//;s/-/_/g' )
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

.PHONY: TAR
TAR:
	git archive \
		--prefix moberg-$(MOBERG_VERSION)/ \
		--output moberg-$(MOBERG_VERSION).tar.gz -- HEAD

.PHONY: moberg-$(MOBERG_VERSION).spec
moberg-$(MOBERG_VERSION).spec: moberg.spec.template Makefile
	sed -e 's/__MOBERG_VERSION__/$(MOBERG_VERSION)/' $< > $@

.PHONY: SRPM
SRPM:	moberg-$(MOBERG_VERSION).spec TAR
	rpmbuild --define "_sourcedir $$(pwd)" \
		 -bs $<



.PHONY: test
test: all
	$(MAKE) -C test test

clean:
	rm -f build/*.so
	rm -f build/*.mex*
	rm -f *~
	rm -f moberg-*.spec
	rm -f moberg-*.tar.gz
	make -C test clean

build/libmoberg.so: build/lib/moberg.o
build/libmoberg.so: build/lib/moberg_config.o
build/libmoberg.so: build/lib/moberg_device.o
build/libmoberg.so: build/lib/moberg_parser.o
build/lib/%.o: %.h
