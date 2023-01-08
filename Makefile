.SUFFIXES:
DEPFLAGS = -MT $@ -MMD -MP -MF
Depends: ; mkdir -p $@
Bin: ; mkdir -p $@
Documentation: ; mkdir -p $@
%.dep: ;
DEPFILES:=$(wildcard Depends/*.dep)
include $(DEPFILES)

Bin/drspread: drspread.c Makefile | Bin Depends
	$(CC) $< -o $@ $(DEPFLAGS) Depends/drspread.dep -Wall -Wextra -Wpedantic -Wno-fixed-enum-extension -Wno-nullability-extension -g -fsanitize=nullability,address,undefined -O0
.PHONY: clean
clean:
	rm -rf Bin
	rm -rf Depends
.DEFAULT_GOAL:=Bin/drspread
