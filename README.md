# LC-3 Virtual Machine and Assembler

-This is my implementation of the [LC-3](https://en.wikipedia.org/wiki/Little_Computer_3) virtual machine and assembler (lcasm) in C.

-The virtual machine has a simple debugger and disassembler implemented as a TUI. You can use it with -d. Some features of it are not yet finished.

-Linux only for now.

### Assembler usage
```
./lcasm -o output.obj OPTIONS[hl] file.asm 
h --> help message
l --> little endian output
```

### Virtual machine usage
```
./lc3 OPTIONS[-dh] image.obj 
d --> debugger mode
h --> help message
```

### Building
```
cd src
gcc -std=c99 -o lcasm assembler.c tokenizer.c -lm
gcc -std=c99 -o lc3 lc3.c -lncurses
```

or 

```
cd src
make
```

### Debugger
- The debugger is embbeded with virtual machine and can be run from the terminal with -d option:

```
	./lc3 -d image.obj
```

- The text-based user interface can be interacted with following commands:

```
help --> list all commands
quit --> exit debugger  
next --> next instruction  
run --> run program until breakpoint  
show x0000-xFFFF --> show contents in memory  
goto x0000-xFFFF --> go to memory page  
down --> go down the memory page  
up --> go up the memory page  
break x0000-xFFFF --> add breakpoint  
rb x0000-xFFFF --> remove breakpoint  
lb x0000-xFFFF --> list breakpoints  
/char --> add 'char' to the inputer buffer (LIFO)  
clear --> clear input window  
```

### Resources

MEINERS, Justin; PENDLETON, Ryan. Write your Own Virtual Machine \
https://www.jmeiners.com/lc3-vm/

PATT, Yale. Introduction to Computing Systems: From Bits and Gates to C and Beyond. 2nd ed.
