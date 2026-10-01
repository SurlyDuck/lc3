.PHONY: all clear record test release

CFLAGS:= -ggdb -std=c99 -Wall -Wextra -pedantic 
CC:= gcc

LC3_VERSION ?= 1.0.0
ASM_VERSION ?= 1.0.0
RELEASE_DIR ?= ./build/

all: lc3 lcasm test

release: $(RELEASE_DIR)lc3_$(LC3_VERSION).tar.gz $(RELEASE_DIR)lcasm_$(ASM_VERSION).tar.gz

$(RELEASE_DIR)lc3_$(LC3_VERSION).tar.gz: lc3
	mkdir -p $(RELEASE_DIR)lc3
	cp lc3   $(RELEASE_DIR)lc3
	tar -czf $@ -C $(RELEASE_DIR) lc3
	rm -R ./build/lc3

$(RELEASE_DIR)lcasm_$(ASM_VERSION).tar.gz: lcasm
	mkdir -p $(RELEASE_DIR)lcasm
	cp lcasm $(RELEASE_DIR)lcasm
	tar -czf $@ -C $(RELEASE_DIR) lcasm
	rm -R ./build/lcasm

test:  
	./tests/test.sh ./examples ./tests

record:
	./tests/record.sh ./examples ./tests

lc3: lc3.c os.h
	$(CC) $(CFLAGS) -o lc3 lc3.c -lncurses

lcasm: assembler.c tokenizer.c tokenizer.h
	$(CC) $(CFLAGS) -o lcasm assembler.c tokenizer.c -lm

clear:
	rm ./lc3 ./lcasm
	rm -R ./build

