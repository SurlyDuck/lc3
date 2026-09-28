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
-A debugger is embedded alongside the virtual machine and can be run from the terminal with -d option:

```
./lc3 -d image.obj
```

-The text-based user interface can be interacted with following commands:

```
help or `h`  --> list all commands
quit or `q`  --> exit debugger  
next or `n`  --> next instruction  
run  or `r`  --> run program until breakpoint  
show or `s` x0000-xFFFF --> show contents in memory  
goto or `g` x0000-xFFFF --> go to memory page  
down or `d` --> go down the memory page  
up  or `u` --> go up the memory page  
break or `b` x0000-xFFFF --> add breakpoint  
rb x0000-xFFFF --> remove breakpoint  
lb x0000-xFFFF --> list breakpoints  
/char --> add 'char' to the inputer buffer (LIFO)  
clear or `c` --> clear input window  
```

### Resources

MEINERS, Justin; PENDLETON, Ryan. Write your Own Virtual Machine \
https://www.jmeiners.com/lc3-vm/

PATT, Yale. Introduction to Computing Systems: From Bits and Gates to C and Beyond. 2nd ed.
