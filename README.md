# LC-3 Virtual Machine and Assembler

-This is my implementation of the LC-3 virtual machine and assembler (lcasm) in C. Still unfinished, but the assembler is already somewhat functional. You can try assembling programs in /src/examples to test its current capabilities.

-Linux only for now.

### Assembler usage
```
./lcasm -f file.asm -o output.obj OPTIONS[hl]
h --> help message
l --> little endian output
```

### Virtual machine usage
```
[IN CONSTRUCTION]
```

### Building
```
cd src
gcc -std=c11 -o lcasm assembler.c tokenizer.c
gcc -std=c11 -o lc3 lc3.c
```

or 

```
cd src
make
```

