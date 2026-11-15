CC      ?= gcc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Werror
SRCS    := $(wildcard src/*.c src/dev/*.c)
OBJS    := $(patsubst src/%.c,build/%.o,$(SRCS))
GSRCS   := $(wildcard guests/*.asm)
GBINS   := $(patsubst guests/%.asm,build/guests/%.bin,$(GSRCS))
BIN     := build/minivmm

.PHONY: all asan guests test clean

all:
ifeq ($(strip $(SRCS)),)
	@echo "minivmm: no sources yet (src/ has no .c files); nothing to build."
else
	@$(MAKE) --no-print-directory $(BIN)
endif

$(BIN): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -Isrc -c -o $@ $<

asan:
ifeq ($(strip $(SRCS)),)
	@echo "minivmm: no sources yet (src/ has no .c files); nothing to build."
else
	@mkdir -p build
	$(CC) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc -o build/minivmm-asan $(SRCS)
endif

guests:
ifeq ($(strip $(GSRCS)),)
	@echo "minivmm: no guest sources yet (guests/ has no .asm files)."
else
	@$(MAKE) --no-print-directory $(GBINS)
endif

build/guests/%.bin: guests/%.asm
	@mkdir -p $(@D)
	nasm -f bin -Iguests/ -o $@ $<

test:
	@if [ -x tests/run.sh ]; then tests/run.sh; else echo "minivmm: tests/run.sh not present yet."; fi

clean:
	rm -rf build
