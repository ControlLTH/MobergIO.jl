LIBRARIES=libmoberg.so
CCFLAGS+=-Wall -Werror -I. -g
LDFLAGS+=-Lbuild/ -lmoberg
MODULES:=$(wildcard modules/*)

LDFLAGS_parse_config=-ldl -export-dynamic

all: $(LIBRARIES:%=build/%) $(MODULES) test/test_c parse_config
	echo $(MODULES)

build/libmoberg.so:	moberg.c Makefile | build
	$(CC) -o $@ $(CCFLAGS) -lxdg-basedir -shared -fPIC -I. $<

build:
	mkdir $@

%:	build/%.o Makefile
	$(CC) $(LDFLAGS) $(LDFLAGS_$(*)) -o $@  $(filter %.o,$^)

build/%.o:	%.c Makefile
	$(CC) $(CCFLAGS) -c -o $@ $<


.PHONY: $(MODULES)
$(MODULES): 
	$(MAKE) -C $@


.PHONY: test
test: all
	LD_LIBRARY_PATH=build \
		./parse_config test/*/*.conf
	LD_LIBRARY_PATH=build HOME=$$(pwd)/test \
		valgrind -q --error-exitcode=1 test/test_c

test/test_c:	test/test_c.c Makefile
	$(CC) $(CCFLAGS) $(LDFLAGS) -o $@  $<

clean:
	rm build/*


build/parse_config.o: moberg_config_parser.h
parse_config: build/moberg_driver.o
parse_config: build/parse_config.o
