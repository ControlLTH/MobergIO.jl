LIBRARIES=libmoberg4simulink.so
SFUNC=analogin analogout digitalin digitalout encoderin
MATLAB_VERSION=
MEX=MATLAB_VERSION=$(MATLAB_VERSION)  mex
MEX_SUFFIX=$(shell $(MEX) -v -n analogin.c \
	    | sed -e 's/^.*LDEXTENSION.*[.]\(mex.*\)/\1/p;d')

CCFLAGS+=-Wall -Werror -I. -I../.. -g

all:	$(LIBRARIES:%=../../build/%) $(SFUNC:%=../../build/%.$(MEX_SUFFIX))
	echo $(SUFFIX)

../../build/libmoberg4simulink.so: moberg4simulink.c Makefile
	$(CC) -o $@ $(CCFLAGS) -L../../build -shared -fPIC -lmoberg $<

../../build/libmoberg4simulink.so: ../../moberg.h

../../build/%.$(MEX_SUFFIX): %.c Makefile
	$(MEX) CFLAGS="$(CCFLAGS) -fPIC" \
	       -outdir ../../build -L../../build -lmoberg4simulink $<
