.PHONY: all clean record test release

CFLAGS:= -ggdb -std=c99 -Wall -Wextra -pedantic 
CC:= gcc

LC3_VERSION ?= 1.0.0
ASM_VERSION ?= 1.0.0

all: lc3 lcasm test release

release: lc3_$(LC3_VERSION).tar lcasm_$(ASM_VERSION).tar

lc3_$(LC3_VERSION).tar: lc3
	tar -cf $@ $<

lcasm_$(ASM_VERSION).tar: lcasm
	tar -cf $@ $<

test:  
	./tests/test.sh ./examples ./tests

record:
	./tests/record.sh ./examples ./tests

lc3: lc3.c os.h
	$(CC) $(CFLAGS) -o lc3 lc3.c -lncurses

lcasm: assembler.c tokenizer.c tokenizer.h
	$(CC) $(CFLAGS) -o lcasm assembler.c tokenizer.c -lm

clean:
	rm ./lc3 ./lcasm
	rm *.tar

