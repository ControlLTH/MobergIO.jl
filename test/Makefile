CTEST = test_start_stop test_io test_moberg4simulink
PYTEST=test_py
CCFLAGS += -Wall -Werror -I$(shell pwd) -g
LDFLAGS += -L$(shell pwd)/build/ -lmoberg
ENV_TEST = LD_LIBRARY_PATH=../build XDG_CONFIG_HOME=.config XDG_CONFIG_DIRS=.
LDFLAGS_test_moberg4simulink = -lmoberg4simulink
CCFLAGS_test_moberg4simulink = -I../adaptors/matlab -Wall -Werror -I$(shell pwd) -g
PYTHON2PATH=$(shell realpath ../adaptors/python/install/usr/lib*/python2*/site-packages)
PYTHON3PATH=$(shell realpath ../adaptors/python/install/usr/lib*/python3*/site-packages)
all:

.PHONY: test
test: $(PYTEST:%=run_py_%) $(CTEST:%=run_c_%) 
	echo Tests run

.PHONY: run_c_%
run_c_%:build/%
	$(ENV_TEST) valgrind --leak-check=full ./build/$*

.PHONY: run_py_%
run_py_%: %.py
	$(ENV_TEST) PYTHONPATH=$(PYTHON2PATH) python2 $*.py
	$(ENV_TEST) PYTHONPATH=$(PYTHON3PATH) python3 $*.py

.PRECIOUS: build/%

build/%: %.c | build
	$(CC) $(CCFLAGS) $(CCFLAGS_$*) $(LDFLAGS) $(LDFLAGS_$*) -o $@ $<

build:
	mkdir build

clean:
	rm -f vgcore.* *~
	rm -rf build
