TEST = test_start_stop test_io

ENV_TEST = LD_LIBRARY_PATH=../build HOME=.

all: 

.PHONY: test
test: $(TEST:%=run_%)
	echo Tests run

run_%:	build/%
	$(ENV_TEST) valgrind --leak-check=full ./build/$*

.PRECIOUS: build/%

build/%: %.c | build
	$(CC) $(CCFLAGS) $(LDFLAGS) -o $@ $<

build:
	mkdir build

clean:
	rm -f vgcore.* *~
