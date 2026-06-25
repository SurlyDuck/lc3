;file to test assembler error handling

.ORIG x3000

andop AND R0 R0 R1
sumop ADD R0 R1 R2
.STRINGZ "TEST" 
.BLKW 1
test  ADD R0 R1 R2



.END
