.SUFFIXES:
DEPFLAGS = -MT $@ -MMD -MP -MF
Depends: ; mkdir -p $@
Bin: ; mkdir -p $@
Documentation: ; mkdir -p $@
%.dep: ;
DEPFILES:=$(wildcard Depends/*.dep)
include $(DEPFILES)

Bin/drspread: drspread_cli.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/drspread.dep -Wall -Wextra -Wpedantic -Wno-fixed-enum-extension -Wno-nullability-extension -g -O0
Bin/drspread.o: drspread.c Makefile | Bin Depends
	$(CC) $< -c -o $@ $(DEPFLAGS) Depends/drspread.o.dep -Wall -Wextra -Wpedantic -Wno-fixed-enum-extension -Wno-nullability-extension -g -O0 -fsanitize=address,nullability,undefined
.PHONY: clean
clean:
	rm -rf Bin
	rm -rf Depends
.DEFAULT_GOAL:=Bin/drspread

WASMCFLAGS=--target=wasm32 --no-standard-libraries -Wl,--export-all -Wl,--no-entry -Wl,--allow-undefined -O3 -ffreestanding -nostdinc -isystem Wasm -iquote .
Bin/drspread.wasm: drspread.c | Bin Depends
	clang $< -c -o $@ $(DEPFLAGS) Depends/drspread.o.dep -Wall -Wextra -Wpedantic -Wno-fixed-enum-extension -Wno-nullability-extension -g -O3 $(WASMCFLAGS)
