;file to test assembler error handling

.ORIG x3000
SUM:
	ADD R0 R0 #1

.FILL SUM
FOUR: .FILL 4

.END
