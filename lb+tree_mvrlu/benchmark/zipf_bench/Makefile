CUR_DIR   := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
include $(CUR_DIR)/../../Makefile.inc

# XXX: need to clean up following
URCUDIR ?= /usr/local

CC := gcc
LD := gcc

CFLAGS += -I$(URCUDIR)/include -I$(INC_DIR)
CFLAGS += -D_REENTRANT
CFLAGS += -Werror
#CFLAGS += --param inline-unit-growth=1000
CFLAGS += -mrtm
#CFLAGS += -DTHREAD_PINNING=1         # test param

ifdef DEBUG
	CFLAGS += -O0 -g3
else
	CFLAGS += -DNDEBUG
	CFLAGS += -O1 -g
endif

IS_RLU = -DIS_RLU
IS_MVRLU = -DIS_MVRLU

IS_LBTREE = -DIS_LBTREE

LDFLAGS += -L$(URCUDIR)/lib
LDFLAGS += -lpthread $(MEMMGR)
LDFLAGS += -lm

BINS = bench-lbtree-mvrlu-gclk bench-lbtree-mvrlu-ordo bench-lbtree-rlu bench-lbtree-rlu-ordo

.PHONY: all clean

all: $(BINS)
	@mkdir -p $(BIN_DIR)/zipf
	@for d in $(BINS); \
	do \
		cp $$d $(BIN_DIR)/zipf/; \
	done


numa-config.h:
	$(TOOLS_DIR)/cpu-topology.py > $(CUR_DIR)/numa-config.h

rand.o: zipf/rand.c
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

zipf.o: zipf/zipf.c
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

rlu.o: rlu.c rlu.h
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

rlu-ordo.o: rlu.c rlu.h
	$(CC) $(CFLAGS) $(DEFINES) -DRLU_ORDO_TIMESTAMPING -c -o $@ $<

lbtree.o:  lb_tree.c lb_tree.h
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

lbtree-mvrlu.o: lb_tree.c lb_tree.h
	$(CC) $(CFLAGS) $(IS_MVRLU) $(IS_LBTREE) $(DEFINES) -c -o $@ $<

bench-lbtree-mvrlu-gclk.o: bench.c numa-config.h
	$(CC) $(CFLAGS) $(IS_MVRLU) $(IS_RLU) $(IS_LBTREE) $(DEFINES) -c -o $@ $<

bench-lbtree-mvrlu-ordo.o: bench.c numa-config.h
	$(CC) $(CFLAGS) $(IS_MVRLU) $(IS_RLU) $(IS_LBTREE) $(DEFINES) -c -o $@ $<

bench-lbtree-rlu.o: bench.c numa-config.h
	$(CC) $(CFLAGS) $(IS_RLU) $(IS_LBTREE) $(DEFINES) -c -o $@ $<

bench-lbtree-rlu-ordo.o: bench.c numa-config.h
	$(CC) $(CFLAGS) $(IS_RLU) $(IS_LBTREE) $(DEFINES) -c -o $@ $<

bench-lbtree-mvrlu-gclk: rand.o zipf.o lbtree-mvrlu.o bench-lbtree-mvrlu-gclk.o $(LIB_DIR)/libmvrlu-gclk.a
	$(LD) -o $@ $^ $(LDFLAGS)

bench-lbtree-mvrlu-ordo: rand.o zipf.o lbtree-mvrlu.o bench-lbtree-mvrlu-ordo.o $(LIB_DIR)/libmvrlu-ordo.a
	$(LD) -o $@ $^ $(LDFLAGS)

bench-lbtree-rlu: rand.o zipf.o rlu.o lbtree.o bench-lbtree-rlu.o
	$(LD) -o $@ $^ $(LDFLAGS)

bench-lbtree-rlu-ordo: rand.o zipf.o rlu-ordo.o lbtree.o bench-lbtree-rlu-ordo.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BINS) *.o
	rm -f $(CUR_DIR)/numa-config.h
	@for d in $(BINS); \
                do ( rm -f $(BIN_DIR)/$$d; \
                ); \
        done