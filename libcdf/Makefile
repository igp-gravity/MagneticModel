#
# install CDF library from sources
#

VERSION=36_4
DIRNAME=./cdf$(VERSION)-dist/
FILENAME=./cdf$(VERSION)-dist-cdf.tar.gz
SOURCE_URL=https://spdf.sci.gsfc.nasa.gov/pub/software/cdf/dist/cdf$(VERSION)/linux/cdf$(VERSION)-dist-cdf.tar.gz
MD5CHECKSUM=a0fc52ac46aec74588676ba1327e1ebf
INSTALLDIR=/usr/local/cdf

all: build

clean:
	-rm -fR $(DIRNAME)

install: build test
	-make -C $(DIRNAME) INSTALLDIR=$(INSTALLDIR) install

test: build
	-make -C $(DIRNAME) test

build: $(DIRNAME)
	-make -C $(DIRNAME) OS=linux ENV=gnu CURSES=no all

$(DIRNAME): $(FILENAME) md5check
	-tar -xzf $(FILENAME)

md5check: $(FILENAME)
	-echo $(MD5CHECKSUM) $(FILENAME) | md5sum -c

$(FILENAME):
	-curl $(SOURCE_URL) > $(FILENAME)