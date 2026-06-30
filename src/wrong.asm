;file to test assembler error handling
.ORIG x3000

LD R0 #2
LDI R1 ADR

TEN: .FILL xA
STRING: .STRINGZ "Hello"
ADR: .FILL STRING

.END
