;file to test assembler error handling
.ORIG x3000

LD R0 ADDR   ;R0 = base register
LDR R1 R0 #1 ;R1 = array[1]
LDR R2 R0 #2 ;R2 = array[2]
LDR R3 R0 #3 ;...
LDR R4 R0 #4



ADDR: .FILL ARRAY
ARRAY: .BLKW 100


.END
