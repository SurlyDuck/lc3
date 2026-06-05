.PHONY: all clean

CFLAGS:= -std=c11 -Wall -Wextra -pedantic
CC:= gcc

all: lc3 assembler 

lc3: lc3.c
	$(CC) $(CFLAGS) -o lc3 lc3.c

assembler: assembler.c tokenizer.c
	$(CC) $(CFLAGS) -o assembler assembler.c tokenizer.c

clean:
	rm ./lc3 ./assembler

