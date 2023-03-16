.SUFFIXES:
DEPFLAGS = -MT $@ -MMD -MP -MF
Depends: ; mkdir -p $@
Bin: ; mkdir -p $@
FuzzDir: ; mkdir -p $@
Documentation: ; mkdir -p $@
TestResults: ; mkdir -p $@
%.dep: ;
DEPFILES:=$(wildcard Depends/*.dep)
include $(DEPFILES)
WFLAGS=-Wall -Wextra -Wpedantic -Wno-fixed-enum-extension -Wno-nullability-extension -Wno-gnu-zero-variadic-macro-arguments -Werror=int-conversion -Werror=incompatible-pointer-types -Wno-gnu-alignof-expression
WCC?=clang
SANITIZE=-fsanitize=address,nullability,undefined

Bin/drspread: drspread_cli.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/drspread.dep $(WFLAGS) -g -O3 -lm
Bin/drspread_bench: drspread_cli.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/drspread_bench.dep $(WFLAGS) -g -O3 -DBENCHMARKING=1 -lm
Bin/drspread.o: drspread.c Makefile | Bin Depends
	$(CC) $< -c -o $@ $(DEPFLAGS) Depends/drspread.o.dep $(WFLAGS) -g -O3
.PHONY: clean
clean:
	rm -rf Bin
	rm -rf Depends
.DEFAULT_GOAL:=Bin/drspread

WASMCFLAGS=--target=wasm32 --no-standard-libraries -Wl,--export-all -Wl,--no-entry -Wl,--allow-undefined -ffreestanding -nostdinc -isystem Wasm -mbulk-memory -mreference-types -mmultivalue -mmutable-globals -mnontrapping-fptoint -msign-ext -Wl,--stack-first
Bin/drspread.wasm: drspread_wasm.c Makefile | Bin Depends
	$(WCC) $< -o $@ $(DEPFLAGS) Depends/drspread.wasm.dep $(WFLAGS) -iquote. $(WASMCFLAGS) -O3
%.js: %.ts
	$(TSC) $< --noImplicitAny --strict --noUnusedLocals --noImplicitReturns --removeComments --target es2020 --strictFunctionTypes --noEmitOnError

Bin/TestDrSpread: TestDrSpread.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpread.c.dep $(WFLAGS) -Wno-unused-function -g $(SANITIZE) -lm

# codegen bugs with -O3
Bin/TestDrSpread.wasm: TestDrSpreadWasm.c Makefile | Bin Depends
	$(WCC) $< -o $@ $(DEPFLAGS) Depends/TestDrSpreadWasm.c.dep $(WFLAGS) -Wno-unused-function -iquote . $(WASMCFLAGS) -O3 -g
test.html: testspread_glue.js Bin/TestDrSpread.wasm

Bin/drspread_fuzz: drspread_fuzz.c | Bin Depends
	$(CC) -O1 -g $(DEPFLAGS) Depends/drspread_fuzz.dep -lm -fsanitize=fuzzer,address,undefined
.PHONY: fuzz
fuzz: Bin/drspread_fuzz | FuzzDir
	$< FuzzDir -fork=4 --only_ascii=1

TestResults/TestDrSpread: Bin/TestDrSpread | TestResults
	$< --tee $@
.PHONY: all
ALL=TestResults/TestDrSpread \
    Bin/TestDrSpread \
    Bin/drspread \
    Bin/drspread.wasm \
    Bin/drspread_bench \
    Bin/drspread.o \
    Bin/TestDrSpread.wasm \
    drspread_glue.js \
    demo.js \
    testspread_glue.js \

all: $(ALL)
.DEFAULT_GOAL:=all

include $(wildcard private.mak)
