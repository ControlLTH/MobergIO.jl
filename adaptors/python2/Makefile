MOBERG_VERSION=$(shell git describe --tags)

all: BUILD INSTALL

.PHONY: BUILD 
BUILD:
	python2 ./setup.py build

.PHONY: INSTALL
INSTALL:
	MOBERG_VERSION=$(MOBERG_VERSION) python2 \
		./setup.py install -O1 --prefix=/usr/ --root=./install

clean:
	rm -rf build install
