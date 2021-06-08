VERSION=1.0
JARNAME=Moberg
TARFILE=java_se.lth.control.realtime.Moberg-$(VERSION).tar.gz
JNINAME=se_lth_control_realtime_moberg_Moberg

JAVA_HOME=$(shell realpath $$(which javac) | sed 's|/bin/javac||')
JNI_INCLUDE=$(JAVA_HOME)/include/

JNI_INCLUDE=$(JAVA_HOME)/include/
CC=gcc
CC_JNI_FLAGS=-Wall -Werror -shared -fPIC \
	     -I$(JNI_INCLUDE) -I$(JNI_INCLUDE)/linux \
	     -I../../ -Ibuild \
	     -L../../build -lmoberg
JAVADIR=/tmp
INSTALL_DIR=$(JAVADIR)/$(JARNAME)
JAR_DIR=$(INSTALL_DIR)/jre/lib/ext/
SO_DIR=$(INSTALL_DIR)/jre/lib/$(shell /bin/sh ./src/getProperty_os_arch)
SRCFILES=README \
	 src/Makefile \
	 src/getProperty_os_arch \
	 $(JNINAME:%=src/%.c) \
	 $(JAVAFILES:%=src/%)
JAVAFILES=$(shell find src -name '*.java' | sed -e 's:^src/::g')

all: JAVAC build/$(JARNAME).jar $(JNINAME:%=build/lib%.so)

install: install_src install_jar install_so

install_src:
	for f in $(SRCFILES) ; do install -D $$f $(INSTALL_DIR)/$$f ; done

install_jar: build/$(JARNAME).jar
	install -d $(JAR_DIR)
	install build/$(JARNAME).jar $(JAR_DIR)

install_so: $(JNINAME:%=install_%.so)

install_%.so: build/lib%.so
	install -d $(SO_DIR)
	install $< $(SO_DIR)

JAVAC:	$(JAVAFILES:%.java=build/%.class)

build/%.class:	src/%.java
# Recompile all javafiles at once (do JIT compilation once)
	mkdir -p build	
	cd src ; javac -d ../build $(JAVAFILES)

build/$(JARNAME).jar: JAVAC
	jar -cf $@ \
	    -C build se \
	     $(SRCFILES)

build/%.h: $(shell echo $(JNINAME:%=build/%.class) | sed -e 's:_:/:g')
# Too many dependencies, base the ones we need on $* (matches % above)	
	cd src ; javac -h ../$(dir $@) -d ../build \
		       $(shell echo $* | sed -e 's|_|/|g').java 

build/lib%.so: $(JNINAME:%=build/%.h) $(JNINAME:%=src/%.c)
# Too many dependencies, base the ones we need on $* (matches % above)	
	$(CC) -o $@ $(CC_JNI_FLAGS) -I. src/$*.c

clean:
	rm -rf build *~

TAR:
	mkdir -p build
	tar -C .. -cvzf build/$(TARFILE) $(SRCFILES:%=$(shell basename $$(pwd))/%)
