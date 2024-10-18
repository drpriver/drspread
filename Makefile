.SUFFIXES:
DEPFLAGS = -MT $@ -MMD -MP -MF
Depends: ; mkdir $@
Bin: ; mkdir $@
FuzzDir: ; mkdir $@
Documentation: ; mkdir $@
TestResults: ; mkdir $@
%.dep: ;
DEPFILES:=$(wildcard Depends/*.dep)
include $(DEPFILES)
WCC?=clang

ifeq ($(OS),Windows_NT)
EXE=.exe
LM=
LTHREADS=
CC=clang
OPEN=start
else
EXE=
UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
LM=
LTHREADS=
CC=clang
OPEN=open
else
CC=gcc
LM=-lm
LTHREADS=-lpthread
OPEN=xdg-open
endif
endif

ifneq (,$(findstring gcc,$(CC)))
SANITIZE=-fsanitize=address,undefined
WFLAGS=-Wall -Wextra -Werror=int-conversion -Werror=incompatible-pointer-types -Werror=return-type -Wno-missing-field-initializers
endif

ifneq (,$(findstring clang,$(CC)))
SANITIZE=-fsanitize=address,undefined,nullability
WFLAGS=-Wall -Wextra -Wpedantic -Wno-fixed-enum-extension -Wno-nullability-extension -Wno-gnu-zero-variadic-macro-arguments -Werror=int-conversion -Werror=incompatible-pointer-types -Wno-language-extension-token -Werror=return-type -Werror=undefined-internal -Wuninitialized -Wconditional-uninitialized
endif
WWFLAGS=-Wall -Wextra -Wpedantic -Wno-fixed-enum-extension -Wno-nullability-extension -Wno-gnu-zero-variadic-macro-arguments -Werror=int-conversion -Werror=incompatible-pointer-types  -Wno-language-extension-token -Werror=return-type -Werror=undefined-internal


Bin/drspread$(EXE): drspread_cli.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/drspread.dep $(WFLAGS) -g -O3  $(LM) $(SANITIZE)
Bin/drspread_bench$(EXE): drspread_cli.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/drspread_bench.dep $(WFLAGS) -g -O3 -DBENCHMARKING=1 $(LM)
Bin/drspread.o: drspread.c Makefile | Bin Depends
	$(CC) $< -c -o $@ $(DEPFLAGS) Depends/drspread.o.dep $(WFLAGS) -g -O3

Bin/drsp$(EXE): drspread_tui.c | Bin
	$(CC) $(WFLAGS) -Wno-sign-compare $< -o $@ $(DEPFLAGS) Depends/d.dep -g $(LM) $(SANITIZE)

.PHONY: drspread_tui
drspread_tui: Bin/drsp$(EXE)




ifneq ($(OS),Windows_NT)
# This doesn't work on windows.
.PHONY: clean
clean:
	rm -rf Bin
	rm -rf Depends
endif

# not using -mreference-types
WASMCFLAGS=--target=wasm32 --no-standard-libraries -Wl,--export-all -Wl,--no-entry -Wl,--allow-undefined -ffreestanding -nostdinc -isystem Wasm -mbulk-memory -mmultivalue -mmutable-globals -mnontrapping-fptoint -msign-ext -Wl,--stack-first
Bin/drspread.wasm: drspread_wasm.c Makefile | Bin Depends
	$(WCC) $< -o $@ $(DEPFLAGS) Depends/drspread.wasm.dep $(WWFLAGS) -iquote. $(WASMCFLAGS) -O3
%.js: %.ts
	$(TSC) $< --noImplicitAny --strict --noUnusedLocals --noImplicitReturns --removeComments --target es2020 --strictFunctionTypes --noEmitOnError

Bin/TestDrSpread_O0$(EXE): TestDrSpread.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpread_O0.c.dep $(WFLAGS) -Wno-unused-function -g $(LM) $(LTHREADS) -O0
Bin/TestDrSpread_O0_leaks$(EXE): TestDrSpread.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpread_O0_leaks.c.dep $(WFLAGS) -Wno-unused-function -g $(LM) $(LTHREADS) -O0 -DRECORD_ALLOCATIONS=1
Bin/TestDrSpread_O0_leaks_san$(EXE): TestDrSpread.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpread_O0_leaks_san.c.dep $(WFLAGS) -Wno-unused-function -g $(SANITIZE) $(LM) $(LTHREADS) -O0 -DRECORD_ALLOCATIONS=1
Bin/TestDrSpread_O1$(EXE): TestDrSpread.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpread_O1.c.dep $(WFLAGS) -Wno-unused-function -g $(LM) $(LTHREADS) -O1
Bin/TestDrSpread_O2$(EXE): TestDrSpread.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpread_O2.c.dep $(WFLAGS) -Wno-unused-function -g $(LM) $(LTHREADS) -O2
Bin/TestDrSpread_O3$(EXE): TestDrSpread.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpread_O3.c.dep $(WFLAGS) -Wno-unused-function -g $(LM) $(LTHREADS) -O3
Bin/TestDrSpread_O0_san$(EXE): TestDrSpread.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpread_O0_san.c.dep $(WFLAGS) -Wno-unused-function -g $(SANITIZE) $(LM) $(LTHREADS) -O0
Bin/TestDrSpread_O1_san$(EXE): TestDrSpread.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpread_O1_san.c.dep $(WFLAGS) -Wno-unused-function -g $(SANITIZE) $(LM) $(LTHREADS) -O1
Bin/TestDrSpread_O2_san$(EXE): TestDrSpread.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpread_O2_san.c.dep $(WFLAGS) -Wno-unused-function -g $(SANITIZE) $(LM) $(LTHREADS) -O2
Bin/TestDrSpread_O3_san$(EXE): TestDrSpread.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpread_O3_san.c.dep $(WFLAGS) -Wno-unused-function -g $(SANITIZE) $(LM) $(LTHREADS) -O3

Bin/TestDrSpread.wasm: TestDrSpreadWasm.c Makefile | Bin Depends
	$(WCC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpreadWasm.c.dep $(WWFLAGS) -Wno-unused-function -iquote . $(WASMCFLAGS) -O3 -g -std=gnu2x
test.html: testspread_glue.js Bin/TestDrSpread.wasm

FUZZCC=clang
Bin/drspread_fuzz$(EXE): drspread_fuzz.c | Bin Depends
	$(FUZZCC) $< -O1 -g $(DEPFLAGS) Depends/drspread_fuzz.dep -fsanitize=fuzzer,address,undefined -o $@ $(LM)
.PHONY: fuzz
fuzz: Bin/drspread_fuzz$(EXE) | FuzzDir
	$< FuzzDir -fork=6 -only_ascii=1 -max_len=8000

TestResults/TestDrSpread%: Bin/TestDrSpread%$(EXE) | TestResults
	$< --tee $@ -s -j
TestResults/TestDrSpread_O0: Bin/TestDrSpread_O0$(EXE) | TestResults
	$< --tee $@
tests: \
    TestResults/TestDrSpread_O0 \
    TestResults/TestDrSpread_O0_leaks \
    TestResults/TestDrSpread_O0_leaks_san \
    TestResults/TestDrSpread_O1 \
    TestResults/TestDrSpread_O2 \
    TestResults/TestDrSpread_O3 \
    TestResults/TestDrSpread_O0_san \
    TestResults/TestDrSpread_O1_san \
    TestResults/TestDrSpread_O2_san \
    TestResults/TestDrSpread_O3_san
.PHONY: test
test: TestResults/TestDrSpread_O0
.PHONY: tests
.PHONY: all
ALL= \
    tests \
    Bin/drspread$(EXE) \
    Bin/drspread.wasm \
    Bin/drspread_bench$(EXE) \
    Bin/drspread.o \
    Bin/TestDrSpread.wasm \
    drspread_glue.js \
    demo.js \
    testspread_glue.js \
    Bin/drsp$(EXE)
# requires meson
# also, rm -rf doesn't work on windows
.PHONY: coverage
coverage:
	rm -rf covdir && meson setup covdir -Db_coverage=true && cd covdir && ninja test && ninja -v coverage-html && cd .. && $(OPEN) covdir/meson-logs/coveragereport/index.html

all: $(ALL)
.DEFAULT_GOAL:=all

include $(wildcard private.mak)
