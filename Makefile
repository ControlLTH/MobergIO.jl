LIBRARIES=libmoberg.so
MOBERG_VERSION=$(shell git describe --tags | sed -e 's/^v//;s/-/./g' )
CCFLAGS+=-Wall -Werror -I$(shell pwd) -O3 -g
LDFLAGS+=-L$(shell pwd)/build/ -lmoberg
PLUGINS:=$(sort $(wildcard plugins/*))
ADAPTORS:=$(sort $(wildcard adaptors/*))
export CCFLAGS LDFLAGS
LDFLAGS_parse_config=-ldl
#-export-dynamic

all: $(LIBRARIES:%=build/%) build/moberg $(PLUGINS) $(ADAPTORS)
	echo $(PLUGINS)
	echo $(CCFLAGS)

build/libmoberg.so: Makefile | build
	$(CC) -o $@ $(CCFLAGS) -shared -fPIC -I. \
		$(filter %.o,$^) -lxdg-basedir -ldl

build/moberg: moberg_tool.c Makefile | build
	$(CC) -o $@ $(CCFLAGS) $< -Lbuild -lmoberg

build/lib build:
	mkdir -p $@

%:	build/%.o Makefile
	$(CC) -o $@  $(filter %.o,$^) $(LDFLAGS) $(LDFLAGS_$(*))

build/%.o:	%.c Makefile
	$(CC) $(CCFLAGS) -c -o $@ $<

build/lib/%.o:	%.c Makefile | build/lib
	$(CC) $(CCFLAGS) -c -fPIC -o $@ $<

.PHONY: $(ADAPTORS)
$(ADAPTORS): Makefile
	$(MAKE) -C $@

.PHONY: $(PLUGINS)
$(PLUGINS): Makefile
	$(MAKE) -C $@
	cp $@/build/libmoberg_*.so build


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
	rpmbuild --define "_sourcedir $$(pwd)" -bs $<



.PHONY: test
test: all
	$(MAKE) -C test test

clean:
	rm -rf build/
	rm -f *~
	rm -f moberg-*.spec
	rm -f moberg-*.tar.gz
	make -C test clean
	for d in $(ADAPTORS) ; do make -C $$d clean ; done
	for d in $(PLUGINS) ; do make -C $$d clean ; done

build/libmoberg.so: build/lib/moberg.o
build/libmoberg.so: build/lib/moberg_config.o
build/libmoberg.so: build/lib/moberg_device.o
build/libmoberg.so: build/lib/moberg_parser.o
build/lib/%.o: %.h
build/lib/%.o: moberg_inline.h
build/lib/moberg.o: moberg_config.h
build/lib/moberg.o: moberg_module.h
build/lib/moberg.o: moberg_parser.h
build/lib/moberg_device.o: moberg.h
build/lib/moberg_device.o: moberg_channel.h
build/lib/moberg_device.o: moberg_config.h
build/lib/moberg_device.o: moberg_device.h
build/lib/moberg_device.o: moberg_inline.h

