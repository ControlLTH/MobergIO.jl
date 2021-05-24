MOBERG_VERSION=$(shell git describe --tags)

all: BUILD INSTALL

.PHONY: BUILD 
BUILD:
	python3 ./setup.py build

.PHONY: INSTALL
INSTALL:
	MOBERG_VERSION=$(MOBERG_VERSION) python3 \
		./setup.py install -O1 --prefix=/usr/ --root=./install

clean:
	rm -rf build install
