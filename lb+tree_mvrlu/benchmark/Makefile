CUR_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

include $(CUR_DIR)/../Makefile.inc

DIRS = $(wildcard */)

all:
	@for d in $(DIRS); \
                do ( echo; \
                        echo '==================================================='; \
                        echo '[*] BUILDING '$$d; \
                        (cd $$d && $(MAKE) -j$(NJOB                    )); \
                ); \
        done

clean:
	@for d in $(DIRS); \
                do ( echo; \
                        echo '==================================================='; \
                        echo '[*] CLEAN '$$d; \
                        (cd $$d && $(MAKE) clean); \
                ); \
        done

distclean: clean

.PHONY: all clean distclean

