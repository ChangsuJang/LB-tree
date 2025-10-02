CUR_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

include $(CUR_DIR)/Makefile.inc

PHONY := all
all: lib benchmark

# ----- CLEAN: 깊은 정리(예전 distclean 수준) -----
PHONY += clean
clean:
	@echo -e "\033[0;32m# Deep clean (formerly distclean)...\033[0m"
	(cd $(PROJ_DIR)/lib && make -s clean || true)
	(cd $(PROJ_DIR)/benchmark && make -s clean || true)
	(cd $(PROJ_DIR)/benchmark && make -s distclean || true)
	rm -f $(BIN_DIR)/zipf/*-rlu
	rm -f $(BIN_DIR)/zipf/*-rlu-ordo
	rm -f $(BIN_DIR)/zipf/*-mvrlu
	rm -f $(BIN_DIR)/zipf/*-mvrlu-ordo
	rm -f $(BIN_DIR)/zipf/*-mvrlu-gclk
	rm -f $(BIN_DIR)/zipf/*-vanilla

# distclean은 clean의 별칭
PHONY += distclean
distclean: clean

PHONY += format
format:
	@echo -e "\033[0;32m# Running clang-format...\033[0m"
	@clang-format -i $(INC_DIR)/*.[ch] $(LIB_DIR)/*.[ch]

PHONY += benchmark
benchmark: lib
	(cd $(PROJ_DIR)/benchmark && make)

PHONY += benchmark-clean
benchmark-clean:
	(cd $(PROJ_DIR)/benchmark && make clean)

PHONY += lib
lib:
	(cd $(PROJ_DIR)/lib && \
	 CONF=ordo make -j$(NJOB) && \
	 CONF=gclk make -j$(NJOB))

PHONY += lib-clean
lib-clean:
	(cd $(PROJ_DIR)/lib && \
	 CONF=ordo make -j$(NJOB) clean && \
	 CONF=gclk make -j$(NJOB) clean)

PHONY += ordo
ordo:
	make -C tools/ordo/
	(cd tools/ordo && sudo ./gen_table.py)

PHONY += help
help:
	@echo '## Generic targets:'
	@echo '  all             - Configure and build all source code including mv-rlu.'
	@echo '  clean           - Remove ALL generated files (deep clean; formerly distclean).'
	@echo '  distclean       - Alias of clean.'
	@echo '  format          - Apply clang-format. Follow LLVM style for C++ code'
	@echo '                    and Linux kernel style for C code'
	@echo ''
	@echo '## Library targets:'
	@echo '  lib             - Build mv-rlu library'
	@echo '  lib-clean       - Clean mv-rlu library'
	@echo '  ordo            - Get ordo value of the server'
	@echo ''
	@echo '## Benchmark targets:'
	@echo '  benchmark       - Build all benchmarks'
	@echo '  benchmark-clean - Clean all benchmarks'

.PHONY: $(PHONY)
