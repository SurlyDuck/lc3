;file to test assembler error handling

.ORIG x3000

BEGIN:
	AND R0 R0 x0
	ADD R0 R0 xA
LOOP:
	ADD R0 R0 #-1
	BRP LOOP
BR BEGIN

string: .STRINGZ "TEST" 
array0: .BLKW 1
.END
