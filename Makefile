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
WFLAGS=-Wall -Wextra -Wpedantic -Wno-fixed-enum-extension -Wno-nullability-extension -Wno-gnu-zero-variadic-macro-arguments -Werror=int-conversion -Werror=incompatible-pointer-types -Wno-gnu-alignof-expression -Wno-language-extension-token -Werror=return-type -Werror=undefined-internal
WCC?=clang
SANITIZE=-fsanitize=address,nullability,undefined

ifeq ($(OS),Windows_NT)
EXE=.exe
else
EXE=
endif


Bin/drspread$(EXE): drspread_cli.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/drspread.dep $(WFLAGS) -g -O3  -std=gnu2x
Bin/drspread_bench$(EXE): drspread_cli.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/drspread_bench.dep $(WFLAGS) -g -O3 -DBENCHMARKING=1 -std=gnu2x
Bin/drspread.o: drspread.c Makefile | Bin Depends
	$(CC) $< -c -o $@ $(DEPFLAGS) Depends/drspread.o.dep $(WFLAGS) -g -O3 -std=gnu2x
.PHONY: clean
clean:
	rm -rf Bin
	rm -rf Depends
.DEFAULT_GOAL:=Bin/drspread

WASMCFLAGS=--target=wasm32 --no-standard-libraries -Wl,--export-all -Wl,--no-entry -Wl,--allow-undefined -ffreestanding -nostdinc -isystem Wasm -mbulk-memory -mreference-types -mmultivalue -mmutable-globals -mnontrapping-fptoint -msign-ext -Wl,--stack-first
Bin/drspread.wasm: drspread_wasm.c Makefile | Bin Depends
	$(WCC) $< -o $@ $(DEPFLAGS) Depends/drspread.wasm.dep $(WFLAGS) -iquote. $(WASMCFLAGS) -O3 -std=gnu2x
%.js: %.ts
	$(TSC) $< --noImplicitAny --strict --noUnusedLocals --noImplicitReturns --removeComments --target es2020 --strictFunctionTypes --noEmitOnError

Bin/TestDrSpread$(EXE): TestDrSpread.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpread.c.dep $(WFLAGS) -Wno-unused-function -g $(SANITIZE) -std=gnu2x

# codegen bugs with -O3
Bin/TestDrSpread.wasm: TestDrSpreadWasm.c Makefile | Bin Depends
	$(WCC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpreadWasm.c.dep $(WFLAGS) -Wno-unused-function -iquote . $(WASMCFLAGS) -O3 -g -std=gnu2x
test.html: testspread_glue.js Bin/TestDrSpread.wasm

Bin/drspread_fuzz$(EXE): drspread_fuzz.c | Bin Depends
	$(CC) $< -O1 -g $(DEPFLAGS) Depends/drspread_fuzz.dep -fsanitize=fuzzer,address,undefined -o $@ -std=gnu2x
.PHONY: fuzz
fuzz: Bin/drspread_fuzz$(EXE) | FuzzDir
	$< FuzzDir -fork=6 -only_ascii=1 -max_len=8000

TestResults/TestDrSpread: Bin/TestDrSpread$(EXE) | TestResults
	$< --tee $@
.PHONY: all
ALL=TestResults/TestDrSpread \
    Bin/TestDrSpread$(EXE) \
    Bin/drspread$(EXE) \
    Bin/drspread.wasm \
    Bin/drspread_bench$(EXE) \
    Bin/drspread.o \
    Bin/TestDrSpread.wasm \
    drspread_glue.js \
    demo.js \
    testspread_glue.js \

all: $(ALL)
.DEFAULT_GOAL:=all

include $(wildcard private.mak)
