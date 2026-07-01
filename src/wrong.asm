;file to test assembler error handling
.ORIG x3000

AND R0 R0 #0
JSRR R0
AND R0 R0 #0
AND R0 R0 #0
AND R0 R0 #0
SUM:
	ADD R0 R0 #1

SUB: 
	ADD R0 R0 #-1
	RET


.END
