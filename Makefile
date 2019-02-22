LIBRARIES=libmoberg.so
CCFLAGS+=-Wall -Werror -I$(shell pwd) -g
LDFLAGS+=-L$(shell pwd)/build/ -lmoberg
MODULES:=$(wildcard modules/*)
export CCFLAGS LDFLAGS
LDFLAGS_parse_config=-ldl -export-dynamic

all: $(LIBRARIES:%=build/%) $(MODULES) 
	echo $(MODULES)
	echo $(CCFLAGS)

build/libmoberg.so:	moberg.c Makefile | build
	$(CC) -o $@ $(CCFLAGS) -shared -fPIC -I. $< \
		$(filter %.o,$^) -lxdg-basedir -ldl

build/lib build:
	mkdir -p $@

%:	build/%.o Makefile
	$(CC) $(LDFLAGS) $(LDFLAGS_$(*)) -o $@  $(filter %.o,$^)

build/%.o:	%.c Makefile
	$(CC) $(CCFLAGS) -c -o $@ $<

build/lib/%.o:	%.c Makefile | build/lib
	$(CC) $(CCFLAGS) -c -fPIC -o $@ $<


.PHONY: $(MODULES)
$(MODULES): 
	$(MAKE) -C $@


.PHONY: test
test: all
	$(MAKE) -C test test
#	LD_LIBRARY_PATH=build \
#		valgrind ./parse_config test/*/*.conf
#	LD_LIBRARY_PATH=build HOME=$$(pwd)/test \
#		valgrind -q --error-exitcode=1 test/test_c


clean:
	find build -type f -delete
	rm *~

build/libmoberg.so: build/lib/moberg_config.o
build/libmoberg.so: build/lib/moberg_device.o
build/libmoberg.so: build/lib/moberg_config_parser.o
build/lib/moberg_device.o: moberg_device.h
build/parse_config.o: moberg_config_parser.h
parse_config: build/moberg_driver.o
parse_config: build/parse_config.o
