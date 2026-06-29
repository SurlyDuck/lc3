;file to test assembler error handling

.ORIG x3000
SUM:
	ADD R0 R0 #1
SUB: 
	ADD R0 R0 #-2

BR SUB
BR TEST2
TEST: AND R0 R0 #0
TEST2: AND R2 R0 #0


.FILL SUM
FOUR: .FILL 4

.END
