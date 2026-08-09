;file to test assembler error handling
.ORIG x3000

HALT
STR R0, R1, #1

.FILL xFE00

.END
