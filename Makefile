# Builds the tcc-json kit with TinyCC. See README.md.
TCC ?= tcc
CC ?= cc
CFLAGS = -std=c99 -Wall -Werror -Isrc
CLANG_FLAGS = -std=c99 -Wall -Wextra -Wswitch-enum -Werror -fsyntax-only -Isrc
FRONT = src/front/base.c src/front/lexer.c src/front/parser.c src/front/front.c src/front/eval.c src/front/check.c
TARGET = src/json.c src/read.c
SRC = src/main.c $(FRONT) $(TARGET)
READ_TEST_SRC = test/read-typed.c $(FRONT) $(TARGET)
HEADERS = $(wildcard src/*.h src/front/*.h)

.PHONY: build check check-clang clean

build: build/langc

build/domain.c: domain/domain.lang domain/finstoch.lang domain/auction.lang domain/opengame.lang gen/embed.c
	mkdir -p build
	cat domain/domain.lang domain/finstoch.lang domain/auction.lang domain/opengame.lang > build/domain-all.lang
	$(TCC) -run gen/embed.c build/domain-all.lang build/domain.c

build/langc: $(SRC) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o build/langc $(SRC) build/domain.c

build/read-typed-test: $(READ_TEST_SRC) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o build/read-typed-test $(READ_TEST_SRC) build/domain.c

check-clang: build/domain.c
	$(CC) $(CLANG_FLAGS) $(SRC) test/read-typed.c build/domain.c

check: build/langc build/read-typed-test check-clang
	sh test/gate.sh

clean:
	rm -rf build
