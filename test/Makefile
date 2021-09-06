CTEST = test_start_stop test_io test_moberg4simulink
PYTEST=test_py
JULIATEST=test_jl
CCFLAGS += -Wall -Werror -I$(shell pwd) -g
LDFLAGS += -L$(shell pwd)/build/ -lmoberg
ENV_TEST = LD_LIBRARY_PATH=../build \
           XDG_CONFIG_HOME=.config \
           XDG_CONFIG_DIRS=. \
	   JULIA_LOAD_PATH=../adaptors/julia
LDFLAGS_test_moberg4simulink = -lmoberg4simulink
CCFLAGS_test_moberg4simulink = -I../adaptors/matlab -Wall -Werror -I$(shell pwd) -g
PYTHON2PATH=$(shell realpath ../adaptors/python2/install/usr/lib*/python2*/site-packages)
PYTHON3PATH=$(shell realpath ../adaptors/python3/install/usr/lib*/python3*/site-packages)
all:

.PHONY: test
test: $(CTEST:%=run_c_%) $(PYTEST:%=run_py_%) $(JULIATEST:%=run_jl_%) 
	echo Tests run

.PHONY: run_c_%
run_c_%:build/%
	$(ENV_TEST) valgrind --leak-check=full ./build/$*

.PHONY: run_py_%
run_py_%: %.py
	$(ENV_TEST) PYTHONPATH=$(PYTHON2PATH) python2 $*.py
	$(ENV_TEST) PYTHONPATH=$(PYTHON3PATH) python3 $*.py

.PHONY: run_jl_%
run_jl_%: %.jl
	$(ENV_TEST) julia $*.jl

.PRECIOUS: build/%

build/%: %.c | build
	$(CC) $(CCFLAGS) $(CCFLAGS_$*) -o $@ $< $(LDFLAGS) $(LDFLAGS_$*)

build:
	mkdir build

clean:
	rm -f vgcore.* *~
	rm -rf build
