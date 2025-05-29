#############################################
## compiler
#############################################
#URCUDIR ?= /usr/local

CC := gcc
LD := gcc

#CFLAGS += -I$(URCUDIR)/include
CFLAGS += -D_REENTRANT
CFLAGS += -Wall -Winline
#CFLAGS += --param inline-unit-growth=1000
CFLAGS += -mrtm

ifdef DEBUG
	CFLAGS += -O0 -g3
else
	CFLAGS += -DNDEBUG
	CFLAGS += -O3
endif

#############################################
## container option
#############################################
IS_PURE = -DIS_PURE
#IS_RCU = -DIS_RCU
IS_RLU = -DIS_RLU

#LDFLAGS += -L$(URCUDIR)/lib
LDFLAGS += -lpthread

BINS = bench-pure bench-rlu

.PHONY: all clean

all: $(BINS)

#############################################
## compiler
#############################################
#new-urcu.o: new-urcu.c new-urcu.h
#	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

rlu.o: rlu.c rlu.h
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

pre_define.o: pre_define.c pre_define.h operation_meta.h
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

lb_tree.o: lb_tree.c lb_tree.h
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

bench-pure.o: bench.c bench.h
	$(CC) $(CFLAGS) $(IS_PURE) $(DEFINES) -c -o $@ $<

bench-rlu.o: bench.c bench.h
	$(CC) $(CFLAGS) $(IS_RLU) $(DEFINES) -c -o $@ $<

bench-pure: rlu.o  pre_define.o lb_tree.o bench-pure.o 
	$(LD) -o $@ $^ $(LDFLAGS)

bench-rlu: rlu.o  pre_define.o lb_tree.o bench-rlu.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BINS) *.o