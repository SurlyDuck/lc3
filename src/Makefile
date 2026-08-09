.PHONY: all clean run

CFLAGS:= -ggdb -std=c99 -Wall -Wextra -pedantic 
CC:= gcc

all: lc3 lcasm

lc3: lc3.c os.h
	$(CC) $(CFLAGS) -o lc3 lc3.c -lncurses

lcasm: assembler.c tokenizer.c tokenizer.h
	$(CC) $(CFLAGS) -o lcasm assembler.c tokenizer.c -lm

clean:
	rm ./lc3 ./lcasm

