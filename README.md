# LC-3 Virtual Machine and Assembler

-This is my implementation of the LC-3 virtual machine and assembler (lcasm) in C. Still unfinished, but the assembler is already somewhat functional. You can try assembling programs in /src/examples to test its current capabilities.

-Linux only for now.

### Assembler usage
```
./lcasm file.asm -o output.obj OPTIONS[hl]
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
gcc -std=c11 -o lcasm assembler.c tokenizer.c -lm
gcc -std=c11 -o lc3 lc3.c -lncurses
```

or 

```
cd src
make
```

### Resources

MEINERS, Justin; PENDLETON, Ryan. Write your Own Virtual Machine \
https://www.jmeiners.com/lc3-vm/

PATT, Yale. Introduction to Computing Systems: From Bits and Gates to C and Beyond. 2nd ed.
