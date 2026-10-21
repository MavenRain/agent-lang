# Builds the tcc-json kit with TinyCC. See README.md.
TCC ?= tcc
CC ?= cc
CFLAGS = -std=c99 -Wall -Werror -Isrc
CLANG_FLAGS = -std=c99 -Wall -Wextra -Wswitch-enum -Werror -fsyntax-only -Isrc
FRONT = src/front/base.c src/front/lexer.c src/front/parser.c src/front/front.c src/front/eval.c src/front/check.c src/front/big.c src/front/jsontext.c
TARGET = src/json.c
DRIVER = src/runtime/driver.c
SRC = src/main.c src/build.c $(DRIVER) $(FRONT) $(TARGET)
HEADERS = $(wildcard src/*.h src/front/*.h src/runtime/*.h)
# libtcc for langc build (S5 Q7 (a)). TCC_LIB_DIR holds libtcc1.a and
# include/.
TCC_HOME ?= $(HOME)/.local
TCC_FLAGS = -I"$(TCC_HOME)/include" -DTCC_LIB_DIR='"$(TCC_HOME)/lib/tcc"'
# The units of a console program (S5 Q8 (a)), in build/runtime.c.
RUNTIME = $(FRONT) $(TARGET) $(DRIVER) src/runtime/rt_main.c build/domain.c

.PHONY: build check check-clang clean

build: build/langc

build/domain.c: domain/domain.lang gen/embed.c
	mkdir -p build
	$(TCC) -run gen/embed.c domain/domain.lang build/domain.c

# A build product for this machine (the tcc -E text holds the SDK headers).
build/runtime.c: $(RUNTIME) $(HEADERS) gen/embed.c
	mkdir -p build/rt
	for f in $(RUNTIME); do $(TCC) -E $(CFLAGS) -o build/rt/$$(basename $$f .c).i $$f || exit 1; done
	$(TCC) -run gen/embed.c -r $@ $(addprefix build/rt/,$(notdir $(RUNTIME:.c=.i)))

# One object for langc and the domain compilers of test/regressions.js.
build/runtime.o: build/runtime.c
	$(CC) $(CFLAGS) -c -o $@ build/runtime.c

# libtcc.a holds Mach-O objects (clang built it) and the tcc linker
# reads only ELF objects. Thus $(CC) links langc (S5 chunk B, D5).
build/langc: $(SRC) $(HEADERS) build/domain.c build/runtime.o
	$(CC) $(CFLAGS) $(TCC_FLAGS) -o build/langc $(SRC) build/domain.c build/runtime.o "$(TCC_HOME)/lib/libtcc.a"

build/path-regressions: test/path-regressions.c $(FRONT) $(TARGET) $(DRIVER) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o $@ test/path-regressions.c $(FRONT) $(TARGET) $(DRIVER) build/domain.c

build/terminal: test/terminal.c
	$(TCC) $(CFLAGS) -o $@ test/terminal.c

build/terminal-env: test/terminal-env.c test/terminal.c
	$(TCC) $(CFLAGS) -o $@ test/terminal-env.c

check-clang: build/domain.c
	$(CC) $(CLANG_FLAGS) $(TCC_FLAGS) $(SRC) src/runtime/rt_main.c test/path-regressions.c test/terminal.c test/terminal-env.c build/domain.c

check: build/langc build/path-regressions build/terminal build/terminal-env check-clang
	sh test/gate.sh
	CC="$(CC)" TCC="$(TCC)" TCC_HOME="$(TCC_HOME)" node test/regressions.js
	node test/printer-regressions.js
	build/path-regressions
	build/terminal-env
	build/terminal

clean:
	rm -rf build
