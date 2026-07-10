;file to test assembler error handling
.ORIG x3000

BR TEST
BRn TEST
BRz TEST
BRp TEST
BRzp TEST
BRnp TEST
BRnz TEST
BRnzp TEST
LDI R0,TEST

TEST: .FILL x1


.END
