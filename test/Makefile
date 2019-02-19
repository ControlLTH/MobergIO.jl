TEST = test_start_stop

ENV_test_start_stop = LD_LIBRARY_PATH=../build HOME=. 
all: 

.PHONY: test
test: $(TEST:%=run_%)
	echo Testing
	exit 1

run_%:	%
	$(ENV_$*) valgrind --leak-check=full ./$*

%:	%.c
	$(CC) $(CCFLAGS) $(LDFLAGS) -o $@ $<
