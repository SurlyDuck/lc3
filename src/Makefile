.PHONY: all clean record test

CFLAGS:= -ggdb -std=c99 -Wall -Wextra -pedantic 
CC:= gcc

all: lc3 lcasm test

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

