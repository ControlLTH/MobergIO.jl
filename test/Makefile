TEST = test_start_stop test_io test_moberg4simulink
CCFLAGS += -Wall -Werror -I$(shell pwd) -g
LDFLAGS += -L$(shell pwd)/build/ -lmoberg
ENV_TEST = LD_LIBRARY_PATH=../build XDG_CONFIG_HOME=.config XDG_CONFIG_DIRS=.
LDFLAGS_test_moberg4simulink = -lmoberg4simulink
CCFLAGS_test_moberg4simulink = -I../adaptors/matlab -Wall -Werror -I$(shell pwd) -g

all:

.PHONY: test
test: $(TEST:%=run_%)
	echo Tests run

run_%:	build/%
	$(ENV_TEST) valgrind --leak-check=full ./build/$*

.PRECIOUS: build/%

build/%: %.c | build
	$(CC) $(CCFLAGS) $(CCFLAGS_$*) $(LDFLAGS) $(LDFLAGS_$*) -o $@ $<

build:
	mkdir build

clean:
	rm -f vgcore.* *~
	rm -rf build
